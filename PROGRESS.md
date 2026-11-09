# qlc6 — Projektstand

Letzter Stand: 2026-11-09

## Was ist qlc6?

Ein paralleler Build von QLC+ als `qlcplus6`-Paket, das gleichzeitig mit `qlcplus` und `qlcplus5` installiert sein kann (kein Konflikt). Ziel: Qt6/QML-UI als separater Binary `qlcplus6`, eigene Bibliothek `libqlcplusengine6.so`, eigenes Installations-Unterverzeichnis `qlcplus6`.

## Build-Status

**Letzter Build:** Gestartet auf 192.168.10.27 (Ubuntu 24.04, Qt 6.4.2 aus apt)
- Build-Log: `/root/qlc6-build.log` auf 192.168.10.27
- Starten: `ssh root@192.168.10.27 "nohup bash /root/qlc6/build-deb.sh > /root/qlc6-build.log 2>&1 &"`
- Status prüfen: `ssh root@192.168.10.27 "tail -5 /root/qlc6-build.log"`
- .deb nach Fertigstellung kopieren: `scp "root@192.168.10.27:/root/qlcplus6_6.0.0*.deb" /home/technik/Downloads/`

## Fertige Änderungen (Phasen 0–7, Packaging)

### Parallelinstallation (qlcplus6)

| Datei | Änderung |
|---|---|
| `variables.cmake` | `qlc6`-Block: `APPBINARY="qlcplus6"`, `PLUGINSUBDIR="qlcplus6"`, `APPVERSION="6.0.0"`, `qmlui=ON` |
| `engine/src/CMakeLists.txt` | Library-Output für qlc6: `libqlcplusengine6.so` |
| `qmlui/CMakeLists.txt` | `module_name` = `${APPBINARY}` statt hartcodiertem `qlcplus5` |
| `webaccess/res/CMakeLists.txt` | Web-Dateien bei qlc6: `-v5` → `-v6` umbenennen |
| `platforms/linux/CMakeLists.txt` | Desktop-Datei + AppStream für qlcplus6 installieren |
| `platforms/linux/qlcplus6.desktop` | **Neu**: Desktop-Entry mit `Exec=qlcplus6` |
| `platforms/linux/org.qlcplus.QLCPlus6.appdata.xml` | **Neu**: AppStream-Metadaten |

### Debian-Packaging

| Datei | Änderung |
|---|---|
| `debian/rules` | qlc6-Flavour: `-Dqlc6=on -Dqmlui=on` (beide Flags nötig!) |
| `debian/control` | `qlcplus6`-Paket ohne Konflikt zu `qlcplus`/`qlcplus5` |
| `debian/qlcplus6.install` | **Neu**: Liste aller qlcplus6-Dateien für dpkg |
| `debian/changelog-v6` | **Neu**: Changelog für Version 6.0.0 |
| `build-deb.sh` | **Neu**: One-Shot-Build-Skript mit Dependency-Install |
| `create-deb.sh` | Angepasst: v6-Flavour unterstützt |

### EFX 2.0 JSON-System (Phasen 0–7)

Vorhanden, Teil der Upstream-Entwicklung. Details in `EFX-JSON-DESIGN.md`.

- Phase 0–6: JSON-Schema, Serialisierung, Hot-Reload, Bibliothek
- Phase 7: `POST /api/v1/effects` im Webaccess-Server (`webaccess/src/webaccessbase.cpp`)

## Kritische cmake-Besonderheit

**Problem:** `CMakeLists.txt` Zeile 43 prüft `if(qmlui)` **vor** `include(./variables.cmake)` (Zeile 61).
Mit nur `-Dqlc6=on` ist `qmlui` beim Check noch ungesetzt → `find_package(Qt6 ... WebSockets)` wird übersprungen → Build-Fehler.

**Fix:** Immer beide Flags zusammen übergeben: `-Dqlc6=on -Dqmlui=on`

## Offene Punkte

| Punkt | Status |
|---|---|
| `qmlui/EFXPathEditor.qml` (Phase 8a) | Syntax geprüft, **nicht kompiliert** — vor Integration testen |
| Phase 8b: Floor-Points via Simulations-API | Offen (hängt von Phase 8a und SimulationClient ab) |
| .deb in `/home/technik/Downloads/` kopieren | Nach Build-Abschluss via scp |
| Integration dmx_semulator ↔ qlc6 | Nächste Entwicklungsphase (SimulationClient) |

## Build-Abhängigkeiten (Ubuntu 24.04)

```
debhelper fakeroot dpkg-dev cmake g++ make pkg-config
qt6-base-dev qt6-declarative-dev qt6-multimedia-dev
qt6-serialport-dev qt6-websockets-dev qt6-3d-dev
qt6-tools-dev qt6-tools-dev-tools qt6-svg-dev libxkbcommon-dev
libfftw3-dev libftdi1-dev libsndfile1-dev
libasound2-dev libudev-dev libusb-1.0-0-dev
shared-mime-info libxml2-utils
```
