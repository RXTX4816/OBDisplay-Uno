# Testability and Future Refactors

This document records potential future improvements for making more of the UNO
application testable in the native (host) environment, without changing
observable behavior on the Arduino.

## Current State (branch `refactor`)

- Native tests (`pio test -e native`) only build `src/obd/Model/*` via
  `build_src_filter = +<obd/Model/*>` to avoid Arduino core / AVR headers.
- The model layer (e.g. `OBDSignals`, `DTCStore`) has Unity tests under
  `test/test_obd_signals_more.cpp`.

## Future Refactors for Better Testability

These are **not** implemented yet, but are good candidates for a later pass:

1. **LCD abstraction for DisplayManager**
   - Introduce a very small `ILcd` interface (or similar) that exposes only the
     subset of `LiquidCrystal` methods actually used (`setCursor`, `print`,
     `clear`, `begin`).
   - Change `DisplayManager` to depend on this interface instead of the concrete
     `LiquidCrystal` type in its header.
   - Keep the actual `LiquidCrystal` member and wiring in the `.cpp` file for
     AVR builds.
   - For native tests, provide a trivial mock implementation that captures
     writes into an in-memory buffer so that menu layout and formatting can be
     asserted without hardware.

2. **Serial abstraction / helpers for KWP1281Session**
   - Extract the pure parsing/mapping logic from `KWP1281Session` (the part that
     takes raw KWP blocks and updates `OBDSignals` / `DTCStore`) into helper
     functions that operate on byte arrays and model references only.
   - Leave `NewSoftwareSerial` and timing/`delay()` usage in a thin wrapper
     around those helpers.
   - Native tests can then construct synthetic KWP response blocks and feed
     them into the helpers to validate mapping logic, while the UNO build still
     uses the full session implementation.

3. **Input abstraction (optional)**
   - If needed later, separate the analog read and button threshold logic:
     - Keep `ButtonInput` responsible for interpreting raw values into high-
       level actions (`up`, `down`, `select`), but hide `analogRead` behind a
       tiny function pointer or interface.
   - Native tests could then inject predefined analog values to validate menu
     navigation behavior without accessing hardware.

These changes are intentionally deferred to avoid large, behavior-affecting
refactors during the current restructuring. They are documented here for
future maintainers or tooling that may want to improve test coverage.
