/*
  Q Light Controller Plus
  efxlibrary.h

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

#ifndef EFXLIBRARY_H
#define EFXLIBRARY_H

#include <QStringList>
#include <QObject>
#include <QMutex>
#include <QMap>
#include <QDir>
#include <QFileSystemWatcher>
#include <QTimer>

#include "efxeffect.h"

/** @addtogroup engine Engine
 * @{
 */

/**
 * The application wide library of JSON effects. It is independent of any
 * project: effects live in the system directory (shipped presets, read-only)
 * and the user directory (uploaded/edited effects). A user effect with the
 * same id overrides a system effect.
 *
 * All methods are thread-safe.
 */
class EFXLibrary : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(EFXLibrary)

public:
    EFXLibrary(QObject *parent = nullptr);
    ~EFXLibrary();

    /** The process wide library used by the engine and the UI. */
    static EFXLibrary *instance();

    static QDir systemDirectory();
    static QDir userDirectory();

    /** Directory used to store uploaded effects; defaults to userDirectory(). */
    void setUserDirectory(const QDir& dir);

    /** Clear the library and load system then user effects. @return number of effects */
    int reload();

    /** Load all *.json files of a directory. @return number of effects added */
    int load(const QDir& dir, bool user);

    EFXEffectPtr effect(const QString& id) const;
    QList<EFXEffectPtr> effects() const;
    bool isUserEffect(const QString& id) const;

    /** Problems found while loading ("file: message"). */
    QStringList loadErrors() const;

    /**
     * Validate @a json and store it as a user effect (<id>.json, written atomically).
     * An existing user effect with the same id is replaced.
     */
    bool addUserEffect(const QByteArray& json, QString *error = nullptr, QString *id = nullptr);

    /** Delete a user effect. System effects cannot be removed. */
    bool removeUserEffect(const QString& id, QString *error = nullptr);

    /** Reload automatically when files in the system or user directory change. */
    void setWatching(bool watch);
    bool isWatching() const;

signals:
    void effectsChanged();

private slots:
    void slotPathChanged();
    void slotRescan();

private:
    struct Entry
    {
        EFXEffectPtr fx;
        QString path;
        bool user;
    };

    mutable QMutex m_mutex;
    QMap<QString, Entry> m_effects;
    QStringList m_errors;
    QDir m_userDir;
    bool m_userDirSet;

    void updateWatches();

    QFileSystemWatcher *m_watcher;
    QTimer *m_debounce;
};

/** @} */

#endif
