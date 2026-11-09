# EFX 2.0 roadmap / status

- [x] Phase 0  Baseline, design (EFX-JSON-DESIGN.md), legacy golden values
- [x] Phase 1  Expression compiler, effect definition, library, 10 presets, parity tests
- [ ] Phase 2  Rebuild the `EFX` core on EFXEffect (replace Algorithm enum, freq/phase fields,
               XML save/load with Effect/Params/Embedded, fix external callers)
- [ ] Phase 3  Keyframe / Bezier paths and per-channel keyframe tracks
- [ ] Phase 4  Arbitrary channels (dimmer, colour, gobo, raw) in EFXFixture
- [ ] Phase 5  QML editor rewrite
- [ ] Phase 6  Library hot reload (QFileSystemWatcher)
- [ ] Phase 7  Built-in HTTP API server for uploading effects from an external generator
               (user request, project independent library)

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
