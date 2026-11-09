# EFX 2.0 roadmap / status

- [x] Phase 0  Baseline, design (EFX-JSON-DESIGN.md), legacy golden values
- [x] Phase 1  Expression compiler, effect definition, library, 10 presets, parity tests
- [x] Phase 2  Rebuild the `EFX` core on EFXEffect (replace Algorithm enum, freq/phase fields,
               XML save/load with Effect/Params/Embedded, fix external callers)
- [x] Phase 3  Keyframe / Bezier paths and per-channel keyframe tracks
- [x] Phase 4  Arbitrary channels (dimmer, colour, gobo, raw) in EFXFixture
- [ ] Phase 5  QML editor rewrite
- [ ] Phase 6  Library hot reload (QFileSystemWatcher)
- [ ] Phase 7  Built-in HTTP API server for uploading effects from an external generator
               (user request, project independent library)
- [ ] Phase 8  Visual effect authoring in the QML editor (user request):
               - draw own paths / waypoints, e.g. points on the stage floor that the heads
                 sweep through (floor point -> pan/tilt per fixture)
               - edit effects imported through the API with a proper UI (no JSON editing)
               - save the result as a library effect (usable in every project, exportable)
               Depends on phase 3 (paths/keyframes) and phase 5 (editor). Floor-point aiming
               needs fixture positions (3D/monitor) and pan/tilt ranges: open question.

## Phase 7 requirements (from the user)
- API server runs inside QLC+
- external generator uploads effect JSON via the API
- uploaded effects are stored in the user library => available in every project
- implemented on top of EFXLibrary::addUserEffect()/removeUserEffect() (already validating,
  atomic, path-traversal safe)

## Behaviours to keep or consciously drop (EFX core rewrite)
propagation modes, relative mode, per-fixture direction/start offset/mode, 16-bit fine
channels, head-level fixtures, fade-in scaling, dimmerControl, adjustAttribute attributes
(Width/Height/Rotation/XOffset/YOffset/StartOffset) driven by VC sliders.
