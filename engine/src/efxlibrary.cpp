/*
  Q Light Controller Plus
  efxlibrary.cpp

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include <QMutexLocker>
#include <QSaveFile>
#include <QFileInfo>
#include <QFile>
#include <QDebug>

#include "efxlibrary.h"
#include "qlcconfig.h"
#include "qlcfile.h"

EFXLibrary::EFXLibrary(QObject *parent)
    : QObject(parent)
    , m_userDirSet(false)
    , m_watcher(nullptr)
    , m_debounce(nullptr)
{
}

EFXLibrary::~EFXLibrary()
{
}

EFXLibrary *EFXLibrary::instance()
{
    static EFXLibrary *lib = new EFXLibrary();
    return lib;
}

QDir EFXLibrary::systemDirectory()
{
    return QLCFile::systemDirectory(QString(EFXEFFECTDIR), QString(".json"));
}

QDir EFXLibrary::userDirectory()
{
    return QLCFile::userDirectory(QString(USEREFXEFFECTDIR), QString(EFXEFFECTDIR),
                                  QStringList() << QString("*.json"));
}

void EFXLibrary::setUserDirectory(const QDir& dir)
{
    QMutexLocker locker(&m_mutex);
    m_userDir = dir;
    m_userDirSet = true;
}

int EFXLibrary::reload()
{
    {
        QMutexLocker locker(&m_mutex);
        m_effects.clear();
        m_errors.clear();
    }

    int count = load(systemDirectory(), false);

    QDir user;
    {
        QMutexLocker locker(&m_mutex);
        user = m_userDirSet ? m_userDir : QDir();
    }
    if (!user.exists() || user.path() == QLatin1String("."))
        user = userDirectory();
    count += load(user, true);

    updateWatches();
    emit effectsChanged(); // also when effects were removed
    return count;
}

void EFXLibrary::setWatching(bool watch)
{
    if (watch == isWatching())
        return;

    if (watch)
    {
        m_debounce = new QTimer(this);
        m_debounce->setSingleShot(true);
        m_debounce->setInterval(300);
        connect(m_debounce, &QTimer::timeout, this, &EFXLibrary::slotRescan);
        m_watcher = new QFileSystemWatcher(this);
        connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &EFXLibrary::slotPathChanged);
        connect(m_watcher, &QFileSystemWatcher::fileChanged, this, &EFXLibrary::slotPathChanged);
        updateWatches();
    }
    else
    {
        delete m_watcher;
        delete m_debounce;
        m_watcher = nullptr;
        m_debounce = nullptr;
    }
}

bool EFXLibrary::isWatching() const
{
    return m_watcher != nullptr;
}

void EFXLibrary::updateWatches()
{
    if (m_watcher == nullptr)
        return;

    QStringList paths;
    {
        QMutexLocker locker(&m_mutex);
        for (const Entry& e : m_effects)
            paths << e.path;
    }
    QDir user = m_userDirSet ? m_userDir : QDir();
    if (!user.exists() || user.path() == QLatin1String("."))
        user = userDirectory();
    paths << systemDirectory().absolutePath() << user.absolutePath();

    // Files replaced by a rename (QSaveFile, editors) drop out of the watcher: re-add all
    if (!m_watcher->files().isEmpty())
        m_watcher->removePaths(m_watcher->files());
    if (!m_watcher->directories().isEmpty())
        m_watcher->removePaths(m_watcher->directories());
    for (const QString& p : paths)
        if (QFileInfo::exists(p))
            m_watcher->addPath(p);
}

void EFXLibrary::slotPathChanged()
{
    if (m_debounce)
        m_debounce->start();
}

void EFXLibrary::slotRescan()
{
    reload();
}

int EFXLibrary::load(const QDir& dir, bool user)
{
    if (!dir.exists())
        return 0;

    int added = 0;
    const QFileInfoList files = dir.entryInfoList(QStringList() << "*.json", QDir::Files, QDir::Name);
    {
        QMutexLocker locker(&m_mutex);
        for (const QFileInfo& fi : files)
        {
            QFile f(fi.absoluteFilePath());
            if (!f.open(QIODevice::ReadOnly))
            {
                m_errors << QString("%1: cannot open file").arg(fi.fileName());
                continue;
            }

            QString err;
            EFXEffectPtr fx = EFXEffect::fromJson(f.readAll(), &err);
            if (fx.isNull())
            {
                m_errors << QString("%1: %2").arg(fi.fileName(), err);
                qWarning() << "EFX effect" << fi.fileName() << "rejected:" << err;
                continue;
            }

            Entry e;
            e.fx = fx;
            e.path = fi.absoluteFilePath();
            e.user = user;
            m_effects.insert(fx->id(), e); // later directories (user) override earlier ones
            added++;
        }
    }

    if (added)
        emit effectsChanged();
    return added;
}

EFXEffectPtr EFXLibrary::effect(const QString& id) const
{
    QMutexLocker locker(&m_mutex);
    return m_effects.value(id).fx;
}

QList<EFXEffectPtr> EFXLibrary::effects() const
{
    QMutexLocker locker(&m_mutex);
    QList<EFXEffectPtr> list;
    for (const Entry& e : m_effects)
        list << e.fx;
    return list;
}

bool EFXLibrary::isUserEffect(const QString& id) const
{
    QMutexLocker locker(&m_mutex);
    return m_effects.contains(id) && m_effects.value(id).user;
}

QStringList EFXLibrary::loadErrors() const
{
    QMutexLocker locker(&m_mutex);
    return m_errors;
}

bool EFXLibrary::addUserEffect(const QByteArray& json, QString *error, QString *id)
{
    QString err;
    EFXEffectPtr fx = EFXEffect::fromJson(json, &err);
    if (fx.isNull())
    {
        if (error)
            *error = err;
        return false;
    }

    QDir dir;
    {
        QMutexLocker locker(&m_mutex);
        dir = m_userDirSet ? m_userDir : QDir();
    }
    if (!dir.exists() || dir.path() == QLatin1String("."))
        dir = userDirectory();

    // The id was validated to contain only [A-Za-z0-9._-], so this cannot escape dir
    QString path = dir.absoluteFilePath(fx->id() + ".json");

    QSaveFile out(path);
    if (!out.open(QIODevice::WriteOnly) || out.write(json) != json.size() || !out.commit())
    {
        if (error)
            *error = QString("cannot write %1").arg(path);
        return false;
    }

    {
        QMutexLocker locker(&m_mutex);
        Entry e;
        e.fx = fx;
        e.path = path;
        e.user = true;
        m_effects.insert(fx->id(), e);
    }

    if (id)
        *id = fx->id();
    emit effectsChanged();
    return true;
}

bool EFXLibrary::removeUserEffect(const QString& id, QString *error)
{
    QString path;
    {
        QMutexLocker locker(&m_mutex);
        if (!m_effects.contains(id))
        {
            if (error)
                *error = "no such effect";
            return false;
        }
        if (!m_effects.value(id).user)
        {
            if (error)
                *error = "system effects cannot be removed";
            return false;
        }
        path = m_effects.value(id).path;
    }

    if (!QFile::remove(path))
    {
        if (error)
            *error = QString("cannot remove %1").arg(path);
        return false;
    }

    // A system effect with the same id may have been shadowed, so rescan
    reload();
    return true;
}
