# Implementation Plan - Fix `registerActor` build error

The build is failing because `ActorFactory::createNPC()` returns a `std::unique_ptr<OGEntity>`, but it is being passed to `registerActor<OGCharacter>`, which expects a `std::unique_ptr<OGCharacter>`. Even though `OGCharacter` inherits from `OGEntity`, `std::unique_ptr<Base>` cannot be implicitly converted to `std::unique_ptr<Derived>`.

## Proposed Changes

### Component: Engine Actors

#### [MODIFY] [ActorFactory.hpp](file:///C:/Users/sjoeb/source/git-repos/oxyous-2026/app/src/main/cpp/engine/actors/ActorFactory.hpp)

- Include `OGCharacter.hpp` to ensure the type is fully defined.
- Change the return type of `createNPC` from `std::unique_ptr<OGEntity>` to `std::unique_ptr<OGCharacter>`. This matches the behavior of `createPlayerActor` and allows `registerActor<OGCharacter>` to work correctly.

## Verification Plan

### Automated Tests
- Run the CMake build to ensure the error is resolved.
```bash
./gradlew :app:buildCMakeDebug[arm64-v8a]
```
(I will use `gradle_build` tool for this)

### Manual Verification
- None required as this is a compile-time fix.
