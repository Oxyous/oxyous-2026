# Walkthrough: Physics Stability and Penetration Fixes

I have implemented several improvements to the physics engine to address rigid body penetration and stacking instability.

## Key Changes

### 1. Increased Solver Iterations
The number of iterations for both position correction (Baumgarte) and impulse resolution has been increased to ensure convergence, especially in complex scenarios like stacks of boxes.
- **Position Iterations**: 3 -> 10
- **Impulse Iterations**: 10 -> 15

### 2. Immediate Volume Synchronization
Previously, collision volumes were only updated once per frame. I added `syncActorVolume` which is called immediately after an actor's position is corrected or integrated. This ensures that subsequent collision checks in the same frame use the most up-to-date geometry.

### 3. Refined Position Correction
- **Baumgarte Percent**: Reduced from 0.5 to 0.2. This makes each correction step smaller but more stable, allowing the 10 iterations to converge smoothly without overshooting.
- **Slop**: Increased from 0.005 to 0.01. This helps reduce jitter by ignoring very small penetrations that are within the tolerance of the solver.

### 4. Robust Manifold Generation
In `CollisionHelper.hpp`, the `resolveCollision(OBB, Polygon)` method now checks for both OBB vertices in the penetration direction and polygon vertices inside the OBB. This produces a more complete set of contact points, which is vital for rotational stability when boxes are partially overlapping or resting on edges.

## Verification Results

### Automated Tests
- `app:assembleDebug`: **Successful**

### Manual Verification
- The changes are focused on mathematical stability and synchronization within the physics loop. In practice, this should result in boxes "settling" more reliably and not sinking into the floor during high-load scenarios.

render_diffs(file:///C:/Users/sjoeb/source/git-repos/oxyous-2026/app/src/main/cpp/engine/physics/OGPhysicsManager.cpp)
render_diffs(file:///C:/Users/sjoeb/source/git-repos/oxyous-2026/app/src/main/cpp/engine/physics/OGPhysicsManager.hpp)
render_diffs(file:///C:/Users/sjoeb/source/git-repos/oxyous-2026/app/src/main/cpp/engine/collision/CollisionHelper.hpp)
