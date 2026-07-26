# Tasks: Fix Rigid Body Penetration and Stacking Stability

- [ ] Modify `OGPhysicsManager.hpp` to add helper methods for volume sync and position correction.
- [ ] Implement `syncActorVolume` and updated `step` logic in `OGPhysicsManager.cpp`.
- [ ] Refine position correction loop in `OGPhysicsManager.cpp` (Baumgarte improvements).
- [ ] Verify build and basic physics behavior.
