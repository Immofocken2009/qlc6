# QLC+ Simulation Integration

## Overview

QLC+ can connect to a running **dmx_semulator** instance to calculate pan/tilt keyframes
from XY floor-position paths and import the results as reusable EFX 2.0 effects.

The integration is entirely **optional** — QLC+ remains fully functional without a
simulation server.

---

## Architecture

```
QLC+ EFX Editor
   │
   │  POST /api/v1/fx/calculate
   ▼
dmx_semulator (Flask)
   │  pan/tilt keyframes (16-bit DMX)
   ▼
SimToEfxConverter::convert()
   │  EFX 2.0 JSON per fixture
   ▼
EFXLibrary::addUserEffect()   ← hot-reload fires automatically (Phase 6)
```

---

## C++ Classes

### `SimulationClient` (`qmlui/simulationclient.h`)

Async HTTP client using `QNetworkAccessManager`.  
Settings are persisted in `QSettings` under group `"SimulationAPI"`.  
The bearer token is stored obfuscated (XOR 0xAA + base64) — not cryptographic,
just prevents casual plaintext exposure.

**Key methods:**

| Method | Description |
|---|---|
| `testConnection()` | GET `/api/v1/fx/health` — no auth required |
| `fetchScenarios()` | GET `/api/v1/fx/scenarios` |
| `fetchScenarioSpots(id)` | GET `/api/v1/fx/scenarios/<id>/spots` |
| `calculatePanTilt(QJsonObject)` | POST `/api/v1/fx/calculate` |
| `save()` / `load()` | Persist/restore URL + token via QSettings |

All results are delivered via Qt signals:
- `connectionTested(bool ok, QString message, QJsonObject serverInfo)`
- `scenariosFetched(QJsonArray)`
- `spotsFetched(int scenarioId, QJsonObject)`
- `panTiltCalculated(QJsonObject)`
- `requestFailed(QString operation, QString error)`

---

### `SimToEfxConverter` (`qmlui/simtoefxconverter.h`)

Static converter: simulation API response → EFX 2.0 JSON files.

**DMX to EFX range mapping:**
```
v = (dmx16 / 65535.0) * 2.0 - 1.0
```
This maps 0 → -1, 32767 → ~0, 65535 → +1.

**One effect per fixture** — allows independent assignment in QLC+.

**Effect id format:** `"sim-" + sanitize(qlc_fixture_id)`  
where `sanitize()` lowercases and replaces non-alphanumeric chars with dashes.

Keyframes with `"unreachable": true` are **skipped** in the output.

---

## EFX Editor UI

A **"Simulation API"** section is appended at the bottom of the EFX Editor right panel:

1. **URL** input — simulation server base URL (e.g. `http://192.168.1.100:5000`)
2. **Token** input — bearer token (password-masked)
3. **Test Connection** — fires `GET /api/v1/fx/health`, shows result in status line
4. **Save Settings** — persists URL + token to QSettings
5. **Scenario selector** + **Load Scenarios** button
6. **Status line** — shows last result or error in red

After picking a scenario the EFX editor calls `simFetchSpots()` automatically.
To trigger the full import, call `simCalculateAndImport(request)` from QML with
the assembled request map (hall_id, fixtures, movements, coord_system).

---

## Request Format

```js
// QVariantMap passed to simCalculateAndImport()
{
  "api_version": "1",
  "hall_id": 1,                        // scenario_id
  "fixtures": [
    { "spot_id": 10, "qlc_fixture_id": "my-moving-head-1" }
  ],
  "movements": [
    { "t": 0.0, "x": 0.0, "y": 0.0 },
    { "t": 1.0, "x": 1.0, "y": 1.0 }
  ],
  "coord_system": "normalized"          // or "absolute" (metres)
}
```

---

## Import Result

After `simCalculateAndImport()` completes, the signal `simImportDone(imported, failed)`
is emitted.  Successfully imported effects appear immediately in the EFX algorithm picker
(Phase 6 file watcher triggers a hot-reload of the user effects directory).

---

## Build Notes

New files registered in `qmlui/CMakeLists.txt`:
- `simulationclient.cpp simulationclient.h`
- `simtoefxconverter.cpp simtoefxconverter.h`

Qt modules required: `Qt6::Network` (already linked).

---

## Security

- The bearer token check in dmx_semulator is enforced only on `/api/v1/fx/calculate`
  and the other `/api/v1/fx/*` endpoints.
- Leave the token empty in `config/db.ini` for LAN-only deployments.
- Rotate the token by updating `db.ini` and restarting Flask — no DB migration needed.
