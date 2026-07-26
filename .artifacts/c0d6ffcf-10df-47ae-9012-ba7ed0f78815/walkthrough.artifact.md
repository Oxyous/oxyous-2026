# Walkthrough - Unique Skeletal Animations

I have implemented a bone slot allocation system and updated the NPC logic to ensure that every skeletal mesh has its own unique animation state.

## Changes Made

### Core Engine & Data Structures
- Added `MAX_BONE_SLOTS` and `MAX_BONES_PER_SLOT` to `DataStructures.hpp` to centralize bone buffer management.
- Implemented `GPUResources::registerBoneBlock()` to allow components to request a unique starting index in the global bone buffer.
- Refactored `GPUResources.cpp` to use these new constants, ensuring the bone storage buffer is sized correctly and offsets are calculated consistently.

### Components
- Updated `OGSkeletalMeshComponent::initialize()` to automatically register a unique bone block upon creation. This ensures that different actors (Player, NPCs, etc.) no longer overwrite each other's animation data.

### NPC Logic (OGCharacter)
- Implemented full animation support in `OGCharacter`.
- NPCs now load "idle" and "walk" animations and blend between them based on their current movement speed.
- NPCs correctly calculate and upload their skinning matrices to their assigned GPU bone slot during every frame update.

## Verification Results

### Automated Tests
- Successfully built the native C++ code using Gradle:
  ```bash
  ./gradlew :app:buildCMakeDebug[arm64-v8a]
  ```

### Manual Verification (Expected Behavior)
- When multiple actors with skeletal meshes are present in the scene, they will now animate independently based on their own logic and assigned bone slots.
