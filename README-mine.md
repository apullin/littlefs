# Test System Refactoring

## Why

The upstream littlefs test system used a custom Python codegen pipeline
(`scripts/test.py`) that parsed TOML test definitions, generated C code
with parameter permutations, and compiled/ran them via a custom C test
runner (`runners/test_runner.c`). This worked but had several issues:

- **Opaque test definitions** - Tests were defined as inline C snippets
  inside TOML files, making them hard to read, navigate, and debug.
  No IDE support (jump-to-definition, autocomplete, breakpoints).

- **Custom infrastructure burden** - ~4,500 lines of Python codegen and
  C runner code to maintain, with no ecosystem support. Test failures
  produced raw C output rather than structured results.

- **No embedded target testing** - Tests only ran on the host via the
  emulated block device. No path to validate on real hardware or RTOS
  environments like Zephyr.

- **Parameterization was implicit** - TOML `define` arrays generated
  cross-product permutations at codegen time. Hard to see which
  parameter combinations actually ran or add new dimensions.

## What changed

Replaced the entire Python/TOML/custom-runner pipeline with two standard
test frameworks:

### GoogleTest (host, C++)

`gtest/` - 18 test files, ~780 tests across 5 flash geometries.

- Standard C++ test framework with `TEST_P` parameterization
- Shared fixture (`LfsTestFixture`) manages emubd lifecycle
- Two executables:
  - `lfs_tests` - public API tests
  - `lfs_internal_tests` - tests needing access to static functions
    (evil, powerloss, compat) via a C wrapper that `#include`s `lfs.c`
- Wear leveling benchmark with per-block heatmap visualization

Build and run:
```bash
cd build && cmake .. && make -j && ctest
```

### Zephyr ztest (embedded, C)

`ztest/` - 10 test files, 54 tests on Cortex-M3 QEMU.

- Runs on `mps2/an385` in QEMU or on real hardware
- Uses Zephyr flash simulator (or your own SPI driver)
- Tests cover the public littlefs API: format/mount, files, dirs,
  paths, seek, move, truncate, alloc, attrs, entries
- Tests that need emubd features (power loss, bad blocks, exhaustion)
  stay in gtest only

Build and run:
```bash
cd ~/zephyrproject
west build -b mps2/an385 /path/to/littlefs/ztest --pristine
west build -t run
```

### What was removed

- `scripts/test.py` (1,487 lines)
- `runners/test_runner.c` (2,818 lines) + `runners/test_runner.h`
- All `tests/*.toml` files

Benchmark infrastructure (`scripts/bench.py`, `benches/`) was kept as-is.

## Test coverage summary

| Suite | gtest | ztest | Notes |
|-------|-------|-------|-------|
| Block device | 25 | 5 | Geometry permutations in gtest |
| Superblocks | 30 | 6 | |
| Files | 105 | 6 | Large parameterized matrix in gtest |
| Dirs | 35 | 7 | |
| Paths | 65 | 13 | |
| Seek | 15 | 3 | |
| Move | 20 | 4 | |
| Truncate | 25 | 3 | |
| Alloc | 50 | 2 | |
| Attrs | 20 | 4 | |
| Entries | 20 | 4 | |
| Interspersed | 25 | - | |
| Orphans | 10 | - | |
| Relocations | 10 | - | |
| Bad blocks | 25 | - | Needs emubd |
| Exhaustion | 25 | - | Needs emubd |
| Shrink | 25 | - | |
| Wear leveling | 12 | - | Benchmark with heatmap |
| Evil | 10 | - | Needs internal API |
| Power loss | 10 | - | Needs internal API |
| Compat | 15 | - | Needs multi-version linking |
| **Total** | **~780** | **57** | |

## Porting your own hardware

To run ztest on real hardware with your own flash driver:

1. Update `lfs_test_bd_zephyr.c` to use your SPI/QSPI device instead
   of `DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller))`
2. Update the geometry in `lfs_test_geometries[]` to match your flash
3. Add a board overlay for your target
4. `west build -b your_board ztest --pristine && west build -t run`
