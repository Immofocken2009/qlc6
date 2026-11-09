# CLAUDE.md – EFX 2.0 rewrite of QLC+ (qlc5)

Briefing for any session continuing this work. Language with the user: German.
Rule from the user: implementation and feature questions go to the user (ask before deciding).

## Goal

Rewrite the EFX editor/engine of QLC+ (4.14.4 master, Qt6, CMake) so that the config basis of an
EFX is a **JSON effect file**. The user wants many more effects than stock QLC+ and full freedom.

Decisions made by the user:
- Effect model "everything": formulas, keyframes, Bezier paths, arbitrary channels (dimmer, colour, gobo ...)
- Editor: QML UI (`qmlui`) only. The old Widgets UI in `ui/` is **not** maintained and no longer compiles
  against the new EFX.
- No legacy: old XML EFX (without `<Effect>`) are not converted (they load as "circle" with a log warning).
- External effect library: folder of .json files (system dir + user dir). Projects reference effects by id
  and embed a fallback copy.
- Own expression parser, not JavaScript. Pan/Tilt transform (Width/Height/Rotation/Offset/Fade-In) stays engine-side.
- Phase 7: API server inside QLC+ so an external generator can upload effect JSONs (project independent).
- Phase 8: visual authoring in the editor (own paths, floor points the heads sweep through, editing
  API-imported effects with a nice UI, not JSON editing).
- At the very end the user wants a **prompt for the external generator** (describing the JSON schema and the
  upload API) – not yet written.

## Status (see EFX-ROADMAP.md)

Done and committed: phases 0-7.
- Ph1 `engine/src/efxexpression.*`, `efxeffect.*`, `efxlibrary.*`, presets in `resources/efx/*.json` (13)
- Ph2 `EFX` core on EFXEffect (`engine/src/efx.*`), XML `<Effect Id Version><Params/><Embedded/></Effect>`
- Ph3 `engine/src/efxpath.*` (Bezier/polyline paths, `EFXPath::setPoints/toJson` ready for the visual editor)
- Ph4 `engine/src/efxfixture.*` channel bindings (dimmer, colours, gobo, raw channels, 16 bit)
- Ph5 `qmlui/qml/fixturesfunctions/EFXEditor.qml`: effect picker + dynamic param controls from `effectParams`
- Ph6 `EFXLibrary::setWatching(true)` hot reload (enabled in `qmlui/app.cpp` initDoc)
- Ph7 `webaccess/src/webaccessbase.cpp`: `POST /api/v1/effects`, `PUT /api/v1/effects/<id>` (upload only,
  no auth of its own, 2 MB limit); `qhttpserver` now waits for the body on PUT too

**Open: Phase 8** (visual authoring). Open question for the user: fixture positions and pan/tilt ranges for
aiming heads at floor points (3D/monitor positions?). Ask before designing. Ideas: path editor canvas with
Bezier handles in EFXPreview/EFXEditor, "save as library effect" via `EFXLibrary::addUserEffect`, edit params
of imported effects, duplicate system effects into user dir.

Then: write the generator prompt, run the full qmlui build, call the advisor before declaring done.

## Docs
- `EFX-JSON-DESIGN.md`: JSON schema 1 (params, channels expr/keys, reverse, path), expression language, upload API
- `EFX-ROADMAP.md`: phases and behaviours to keep

## Known limitations
- Fade-in scaling only affects pan/tilt, not dimmer/colour. Gobo/zoom etc. targets are 8 bit only.
- Full `qmlui` build was never run (1 CPU core, takes hours). Touched qmlui files were compiled
  individually; QML only checked with `/usr/lib/qt6/bin/qmllint`.
- Seven unrelated engine tests fail in this environment (inputoutputmap, inputpatch, outputpatch,
  qlcfixturedefcache, qlci18n, qlcinputprofile, rgbscript: no IO plugins, root user, no translations).

## Build and test
Build dir `/opt/qlc5/build` (cmake `-Dqmlui=ON`, Qt 6.4.2 via apt). Build only what you need, e.g.
`cd build && make -j1 efx_test`. Run a test from its source dir:

    cd engine/test/efx && QT_QPA_PLATFORM=offscreen LD_LIBRARY_PATH=/opt/qlc5/build/engine/src \
        /opt/qlc5/build/engine/test/efx/efx_test

Relevant tests: `efxeffect` (24), `efx` (34, 1 skip), `efxfixture` (25), `efx_golden`.
Compiler uses -Werror (unused variables fail the build).

## Gotchas
- `emit` is a macro in Qt. `Universe::preGMValues()` are signed chars (cast to uchar).
- `EFXFixture::setHead` auto-chooses the mode; universes keep old faders between tests.
- `efx_test` needs a `QCoreApplication` (custom `main`) because of the file system watcher.
- Commit messages end with `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>`.
