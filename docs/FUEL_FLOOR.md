# Fuel below the tank sender's floor (0x17)

Issue #83. User-facing summary: [Screen Reference → Fuel below the sender's floor](wiki/Screen-Reference.md#fuel-below-the-senders-floor-0x17).

## The problem

The Golf 4 fuel sender is a float-and-resistor design. It is non-linear and least
accurate at the bottom of the tank, the cluster damps the signal, and it has a lower
measurement limit: the `0x17` cluster reports **3 L** and keeps reporting 3 L until
the tank is dry. So "3 L" means "at or below the lowest measurable level". When the
reading first shows 3 L, the real amount is somewhere between 3.0 and ~3.9 L.

Before this change, `0x17` computed everything from the settled sender level:

```
burned      = start − settled
L/100km     = burned / tripDistance
kmRemaining = settled / (L/100km)
```

At the floor, `settled` stops at 3 L. `burned` stops growing, `tripDistance` keeps
growing, so L/100km falls and **the range estimate rises while the tank runs dry**.
That was the real bug, beyond "it shows 3 L".

The `0x01` trip computer is not affected: it integrates fuel burned from RPM × load,
counts down from the start level saved in EEPROM, and never reads the sender.

## The estimate

Once the settled level reads `FUEL_FLOOR_L` (3 L), the sender is ignored for fuel
and range:

```
leftX8      = FUEL_FLOOR_L × 8 − dist100 / (12500 / per100)
kmRemaining = leftX8 × 125 / per100                 (the formula used above the floor)
```

| Symbol | Unit | Source |
|---|---|---|
| `dist100` | km × 100 | the speed-integrated trip distance since the floor |
| `per100` | L/100 km × 10 | `fuelPer100km` frozen at the floor |
| `leftX8` | L × 8 | estimate, 0.125 L steps, shown with one decimal |

`12500 / per100` is the distance in km × 100 per 1/8 L. Splitting the formula into
two 16-bit divisions avoids 32-bit math (see the memory budget below). The truncation
error is under 1 % for every realistic consumption.

### Decisions

- **Reference point: the first time the settled level reads 3 L, assume 3.0 L.**
  The true amount is 3.0–3.9 L. Taking the lowest value makes the estimate err low
  near empty, so hidden extra fuel is reserve, never a surprise.
  The settled level (held through slosh for `FUEL_SETTLE_DWELL_CYCLES` = 10 s) is
  used, not the raw reading, so a corner at 6 L cannot start the estimate.
- **Distance from `tripDistance100`, not the odometer.** The odometer moves in 10 km
  steps; at 7 L/100 km that is 0.7 L per step. The trip distance is integrated from
  speed every 50 ms in 0.01 km steps.
- **Consumption frozen at the floor.** The live value is exactly what goes wrong at
  the floor. Frozen is the trip-measured value (burned / distance, more than 10 km).
  If the trip has none (shorter, or power-on with the tank already at the floor),
  `FUEL_FLOOR_DEFAULT_PER100` = 8.0 L/100 km is frozen instead. Not the RPM/speed
  estimate: it is a momentary value without the 30 L/100 km cap, and pulling away
  at 3000 rpm and 18 km/h gives 38 L/100 km, which would empty the estimate within
  8 km and stay saved for the rest of the tank.
- **Seeding after a failed first read.** If the seed poll at connect fails, the
  smoothed level starts at 0. It now takes the first reading as is; climbing up
  from 0 would pass the floor, and the settled level would lock onto ~1 L and start
  the estimate on a full tank.
- **End on refuel only.** The estimate ends when the settled level reads
  `FUEL_FLOOR_EXIT_L` (5 L) or more, i.e. 2 L above the floor for 10 s. That leaves
  room for the float to bob up to 4 L without losing the count.
- **No separate "active" flag.** `dist100 == 0` means no floor; entering sets it to 1,
  a 10 m bias. One field less to keep, save and check.
- **Shown as an estimate.** `~1.8L` / `~24K`: the `~` is prepended to the already
  formatted text by `Display::prefixLast()`, so no second number formatter was needed.
  L/100 km shows the frozen value. The bar gauge and the fuel warnings keep using
  the sender (they fire at 8 L / 4 L, above the floor anyway).

## Persistence

`tripDistance100` restarts at every power-on, so the floor state goes to EEPROM
(bytes 3–8, [layout](wiki/Development.md#eeprom-layout)):

| Field | Bytes |
|---|---|
| `dist100` | 2 |
| `per100` | 2 |
| `odo`: low 16 bits of the odometer at the save | 2 |

- **When.** `OBDDisplay::saveFuelFloor_()` runs after every `0x17` compute and writes
  when `(dist100 + 127) >> 7` changes: on entering the floor, about every 1.28 km,
  and on leaving it (key 0). A power-off loses at most 1.28 km. A tank's worth of
  floor driving is a few dozen saves, and `EEPROM.update()` only writes bytes that
  changed. Against ~100,000 write cycles per cell, wear does not matter.
- **Format.** The record is the first 6 bytes of `Model::FuelFloor`, copied byte by
  byte and **bit-inverted**: erased cells (`0xFF`) read back as 0 = no floor, so no
  magic byte or validity check is needed.
- **Load.** At each `0x17` connect, after the first poll (so the odometer is known)
  and before the first compute. A reconnect within a session keeps the live state,
  which is newer than the last save. The first compute after connect always saves,
  which stores the odometer of this session.

### Drives with the display unplugged

These are not in the trip distance. At connect, `OBDSignals::resumeFuelFloor()`
compares the saved odometer with the current one:

| Odometer difference | Meaning | Action |
|---|---|---|
| 0 or 10 km | nothing, or one tick during the last km before power-off | none |
| > 10 km | the car was driven with the display unplugged or on `0x01` | add the difference |

The difference is accurate to ±10 km. It is clamped to 300 km, which is more than
3 L lasts at any consumption, so the math stays in 16 bits. The low 16 bits of the
odometer are enough because the difference is taken modulo 65536. Odometer 0 means
"not read yet" and skips the check.

If the car was refuelled meanwhile, the first compute sees a settled level ≥ 5 L and
ends the floor, whatever the loaded record says.

The record has no checksum. A power-off in the middle of a save can leave a torn
record; `resumeFuelFloor()` drops a `per100` above the 30 L/100 km cap (the default
is used instead), which also rules out a division by zero in the estimate. A torn
`dist100` only makes the estimate wrong until the next refuel.

A reconnect after a dropped K-Line connection keeps integrating the distance over
the outage at the last speed (a few seconds), which roughly matches the real drive.

## Memory budget

The debug image had 290 bytes of flash left before this change, the release image 654.

| Part | Flash (bytes) |
|---|---|
| Floor model in `OBDSignals::compute()`: trip accumulation, enter/exit, freeze, estimate | ~210 |
| Odometer catch-up (`resumeFuelFloor`) | ~90 |
| EEPROM save/load | ~40 |
| Screens (`~` prefix, fuel and range rows) | ~150 |
| `OBDSignals::reset()` with `memset` instead of `= Struct{}` | −342 |
| Robustness from review: default consumption, EMA seeding, record checks | ~50 |
| **Net** (measured; the parts are approximate, LTO shares code between them) | **+208** |

| Build | Flash before → after | Free after |
|---|---|---|
| `uno` | 31602 → 31810 | 446 |
| `uno_debug` | 31966 → 32174 | 82 |

Static RAM grows by 8 bytes (1010 → 1018): the 7-byte `FuelFloor` and the 1-byte save key.

What did not work, for the next feature that needs room:

- **32-bit math** (`(uint32_t)dist100 * per100 / 12500`) cost ~100 bytes more than
  two 16-bit divisions, even though `__mulsi3` and `__udivmodsi4` were already linked.
- **Drawing at an x other than 0.** `printBigScaled10()` and `printBigWithLabel()` are
  compiled as `x = 0` constprop clones. One call with `x = 12` brings back the general
  versions, ~150 bytes.
- **`eeprom_update_block()` / `eeprom_read_block()`** pull in ~100 bytes of avr-libc;
  a loop over the already-linked `EEPROM.read()` / `update()` is smaller.
- **`fuelFloor` as the first member** of `OBDSignals` for short offsets: +62, because
  every other member moves instead.
- **Assigning `Struct{}`** emits a full constructor copy per struct; that is where the
  342 bytes for this feature came from.

## Tests

- `test/test_model`: countdown by distance, frozen consumption, end on refuel only,
  the default consumption without a measured one (and no pull-away spike), seeding
  after a failed first read, resume after power-off, odometer catch-up (including
  the 16-bit wrap and "odometer not read"), a garbage `per100` from EEPROM, and
  `reset()` against a fresh object.
- `test/test_display_screens` (simavr): `~1.8L` / `~24K` on both `0x17` dashboards and
  `---` without a consumption.

## Still open

- Whether 3.0 L is exactly where every Golf 4 sender stops. Check on a low-tank drive
  as described in the Screen Reference, and adjust `FUEL_FLOOR_L`.
