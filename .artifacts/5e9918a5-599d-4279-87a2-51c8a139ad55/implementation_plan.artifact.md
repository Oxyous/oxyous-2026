# Fix Rigid Body Penetration and Improve Stacking Stability

The goal is to prevent physics objects (specifically OBBs/boxes) from penetrating static geometry and each other when stacked. The current issues identified are low solver iterations, lack of volume synchronization during the physics step, and stale manifold data during iterations.

## User Review Required

> [!IMPORTANT]
> The physics simulation runs in a separate thread. Synchronizing collision volumes during the physics step is crucial for stability but may have slight performance implications.

## Proposed Changes

### Physics Engine Core

#### [MODIFY] [OGPhysicsManager.cpp](file:///C:/Users/sjoeb/source/git-repos/oxyous-2026/app/src/main/cpp/engine/physics/OGPhysicsManager.cpp)
- Increase position correction (Baumgarte) iterations from 3 to 10.
- Increase velocity/impulse iterations from 10 to 15.
- Update `updatePositionManifold` to synchronize the actor's collision volume immediately after moving it.
- Update `step` to synchronize all actor volumes after the final velocity integration.
- Reduce Baumgarte `percent` factor from 0.5 to 0.2 to allow smoother convergence over more iterations.
- Increase `slop` slightly to 0.01f to reduce jitter in resting stacks.

#### [MODIFY] [OGPhysicsManager.hpp](file:///C:/Users/sjoeb/source/git-repos/oxyous-2026/app/src/main/cpp/engine/physics/OGPhysicsManager.hpp)
- Add a helper method `syncActorVolume(OGEntity* actor)` to handle volume updates safely.

### Collision Resolution

#### [MODIFY] [CollisionHelper.hpp](file:///C:/Users/sjoeb/source/git-repos/oxyous-2026/app/src/main/cpp/engine/collision/CollisionHelper.hpp)
- Refine OBB-Polygon contact point generation to ensure more robust manifold data.
- (Optional) Investigate adding a small collision margin/padding to `resolveCollision` methods.

## Verification Plan

### Automated Tests
- Run existing physics tests (if any).
- Create a test scenario with a stack of boxes and verify they don't penetrate significantly over time.

### Manual Verification
- Deploy the app and observe the box stacks in the scene.
- Verify that boxes no longer sink into the floor or each other.
- Check for jitter or instability in the stacks.
