# C++ Build Flags Guide for Arduino/AVR

## Currently Enabled Flags in platformio.ini

### Warning Flags
- **`-Wall`**: Enables all common warnings about questionable constructions
- **`-Wextra`**: Enables additional warnings not covered by `-Wall`
- **`-Wpedantic`**: Issues warnings for constructs that violate ISO C++
- **`-Wunused`**: Warns about unused variables, functions, parameters
- **`-Wshadow`**: Warns when local variables shadow other variables (caught several issues!)
- **`-Wformat=2`**: Checks printf/scanf format string correctness
- **`-Wno-unused-parameter`**: Disables warnings for unused parameters (common in Arduino callbacks)

### Optimization/Memory Flags
- **`-fno-exceptions`**: Disables C++ exceptions (saves ~2KB on AVR)
- **`-fno-threadsafe-statics`**: Disables thread-safe static initialization (saves memory, safe for single-threaded AVR)

## Additional Recommended Flags (Currently Commented Out)

Add these when you want smaller code size:

```ini
-Os                    ; Optimize for size (instead of speed)
-flto                  ; Link-time optimization (whole program optimization)
-ffunction-sections    ; Each function gets its own section
-fdata-sections        ; Each data item gets its own section
-Wl,--gc-sections      ; Linker removes unused sections
```

## Other Useful Flags to Consider

### More Strict Warnings
```ini
-Wconversion           ; Warn on implicit type conversions that may change value
-Wsign-conversion      ; Warn on implicit conversions between signed/unsigned
-Wcast-qual            ; Warn when casting removes const/volatile
-Wcast-align           ; Warn on casts that increase alignment requirements
-Wold-style-cast       ; Warn about C-style casts in C++ (use static_cast, etc.)
-Wdouble-promotion     ; Warn when float is implicitly promoted to double
-Wnull-dereference     ; Warn about dereferencing null pointers (GCC 6+)
-Wlogical-op           ; Warn about suspicious logical operations
```

### Performance/Optimization
```ini
-finline-limit=n       ; Control how aggressively functions are inlined
-fno-rtti              ; Disable run-time type information (saves memory if not needed)
-mcall-prologues       ; Reduce code size by using call prologues (AVR specific)
```

### Debugging (Disable for Release)
```ini
-g                     ; Include debug symbols
-Og                    ; Optimize for debugging experience
```

## What NOT to Use on Arduino

- **`-Werror`**: Treats warnings as errors (too strict for Arduino framework code)
- **`-fstack-protector`**: Not supported/useful on AVR
- **`-fsanitize=*`**: Address/memory sanitizers don't work on AVR

## Current Build Results

- **Flash Usage**: 518 bytes / 32,256 bytes (1.6%) 
- **RAM Usage**: 11 bytes / 2,048 bytes (0.5%)

Very minimal footprint with just the Controller boilerplate!

## Tips

1. The Arduino framework itself may generate warnings - that's normal
2. The `-fno-threadsafe-statics` warning for C files is harmless (flag is C++ only)
3. Binary constants (0b001) warnings are from Arduino core, safe to ignore
4. When refactoring, fix warnings incrementally rather than all at once

## Next Steps for Your Refactor

1. Move code from `obdisplay.cpp.old` to new modular structure
2. Use proper C++ features: classes, namespaces, const correctness
3. Fix shadow warnings by using better parameter names
4. Replace C-style casts with `static_cast<>`
5. Use `nullptr` instead of `NULL` or `0` for pointers
6. Consider using `enum class` instead of `#define` constants
