# EFX 2.0 – JSON-basiertes Effektmodell (Entwurf zur Freigabe)

Status: ENTWURF, nichts davon ist implementiert. Bitte prüfen und kommentieren.

## Architektur-Entscheidungen

- Die Klasse `EFX` (Function-Typ-ID bleibt) behält Fixtures, Propagation,
  Relative-Modus, Fade-In, `adjustAttribute` usw. Ersetzt werden nur
  `Algorithm`-Enum, Frequenz/Phase-Felder und die XML-Algorithmus-Logik.
- Ein Effekt ist eine JSON-Datei in einer Bibliothek (Systemordner + User-Ordner,
  analog zu `rgbscripts`). Hot-Reload beim Speichern.
- Ausdrücke werden **einmal beim Laden** zu einem Bytecode/AST kompiliert und
  sind danach thread-sicher auswertbar (MasterTimer + UI-Vorschau gleichzeitig).
  Kein QJSEngine, keine externe Abhängigkeit.
- Kein Legacy: alte XML-EFX (`Algorithm`, `Frequency`, `Phase`) werden nicht
  mehr geladen. Die 10 bisherigen Formen kommen als JSON-Presets mit.

## Variablen und Funktionen in Ausdrücken

Variablen: `t` (0..1 Zyklusposition), `theta` (= 2π·t), `i` (Fixture-Index),
`n` (Fixture-Anzahl), `h` (Head-Index), alle Parameter beim Namen, `pi`.

Funktionen: `sin cos tan asin acos atan atan2 abs sqrt pow exp log floor ceil
round sign min max clamp mix mod smoothstep tri saw square pulse noise`.

## Ausgabeziele (`channels[].target`)

| target                                   | Bereich | Bedeutung                              |
|------------------------------------------|---------|----------------------------------------|
| `pan`, `tilt`                            | -1..1   | wird mit Width/Height/Rotation/Offset transformiert |
| `dimmer`, `red`, `green`, `blue`, `white`, `amber`, `uv`, `shutter`, `zoom`, `focus`, `gobo`, `prism`, `iris`, ... | 0..1 | semantisch, über QLCChannel-Gruppe/Preset des Fixtures aufgelöst |
| `{"channel": 12}`                        | 0..1    | roher Kanal-Offset im Fixture          |

16-Bit-Feinkanäle werden automatisch mitbedient.

## Beispiel 1 – Kreis als Formel (`circle.json`)

```json
{
  "schema": 1,
  "id": "circle",
  "name": "Circle",
  "params": [
    { "id": "turns", "label": "Turns per cycle", "type": "float",
      "min": 0, "max": 16, "default": 1, "step": 0.25 }
  ],
  "channels": [
    { "target": "pan",  "expr": "cos(theta * turns + pi / 2)" },
    { "target": "tilt", "expr": "cos(theta * turns)" }
  ]
}
```

## Beispiel 2 – Bezier-/Keyframe-Pfad (`heart.json`)

```json
{
  "schema": 1,
  "id": "heart",
  "name": "Heart",
  "path": {
    "type": "bezier",
    "closed": true,
    "easing": "linear",
    "points": [
      { "x": 0.0,  "y": -1.0, "out": [ 0.4, -0.6], "in": [-0.4, -0.6] },
      { "x": 1.0,  "y":  0.3, "out": [ 1.0,  0.9], "in": [ 1.0, -0.2] },
      { "x": 0.0,  "y":  0.6, "out": [-0.2,  1.0], "in": [ 0.2,  1.0] },
      { "x": -1.0, "y":  0.3, "out": [-1.0, -0.2], "in": [-1.0,  0.9] }
    ]
  }
}
```

`path` liefert pan/tilt. Zusätzlich sind Keyframe-Spuren pro Kanal möglich:
`{ "target": "dimmer", "keys": [ {"t":0,"v":1}, {"t":0.5,"v":0,"ease":"smooth"}, {"t":1,"v":1} ] }`

## Beispiel 3 – Dimmer + Farbe pro Fixture (`rainbow-chase.json`)

```json
{
  "schema": 1,
  "id": "rainbow-chase",
  "name": "Rainbow Chase",
  "params": [
    { "id": "spread", "label": "Spread", "type": "float",
      "min": 0, "max": 1, "default": 0.25 }
  ],
  "channels": [
    { "target": "pan",    "expr": "sin(theta)" },
    { "target": "tilt",   "expr": "0.3 * sin(2 * theta)" },
    { "target": "dimmer", "expr": "pulse(t + i * spread, 0.5)" },
    { "target": "red",    "expr": "0.5 + 0.5 * sin(theta + i * spread * 2 * pi)" },
    { "target": "green",  "expr": "0.5 + 0.5 * sin(theta + i * spread * 2 * pi + 2.094)" },
    { "target": "blue",   "expr": "0.5 + 0.5 * sin(theta + i * spread * 2 * pi + 4.188)" }
  ]
}
```

## Projekt-Referenz (.qxw)

```xml
<Function Type="EFX" ...>
  <Effect Id="rainbow-chase" Version="1">
    <Params>{"spread":0.4}</Params>
    <Embedded><![CDATA[ { ...vollständiges JSON... } ]]></Embedded>
  </Effect>
  ... Fixtures, Propagation usw. wie bisher ...
</Function>
```

- Beim Laden gilt: Bibliothek zuerst, bei fehlender Datei die eingebettete Kopie.
- Fehlt beides oder ist ein Ausdruck fehlerhaft: Effekt wird als „defekt"
  markiert, Fixtures bleiben unverändert, es gibt eine Warnung im Log.
- Parameter-Overrides liegen nur im Projekt, die Bibliotheksdatei bleibt
  unverändert.
