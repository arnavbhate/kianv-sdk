# KianV SDK

An SDK for bare-metal C and C++ programs on the KianV RV32IMA SoC, modelled on the Raspberry Pi Pico SDK: the same CMake structure (`kianv_sdk_init()`, INTERFACE libraries linked per program) and, where the hardware allows, the same function names.

Short example programs for each feature, and a self-test, are in [kianv-sdk-examples](https://github.com/arnavbhate/kianv-sdk-examples).

## Getting the tools

Building programs needs CMake (3.13 or newer), make, and a `riscv32-unknown-elf` GCC toolchain for RV32IMA with newlib. The SDK is tested with GCC 16 built from [riscv-gnu-toolchain](https://github.com/riscv-collab/riscv-gnu-toolchain) as described below. Toolchains whose programs are named differently, such as prebuilt ones (`riscv64-unknown-elf-`, `riscv-none-elf-`), can be selected with `KIANV_TOOLCHAIN_PREFIX` if they include RV32IMA libraries for the `ilp32` ABI; they are not tested.

### Toolchain on Linux

Install the build dependencies (from riscv-gnu-toolchain's README):

```sh
# Ubuntu / Debian
sudo apt-get install autoconf automake autotools-dev curl python3 python3-pip python3-tomli libmpc-dev libmpfr-dev libgmp-dev gawk build-essential bison flex texinfo gperf libtool patchutils bc zlib1g-dev libexpat-dev meson ninja-build git cmake libglib2.0-dev expect device-tree-compiler libslirp-dev libzstd-dev libncurses-dev
# Fedora / RHEL
sudo yum install autoconf automake python3 libmpc-devel mpfr-devel gmp-devel gawk bison flex texinfo patchutils gcc gcc-c++ zlib-devel expat-devel libslirp-devel ncurses-devel meson ninja-build cmake dtc
# Arch
sudo pacman -Syu curl python3 libmpc mpfr gmp base-devel texinfo gperf patchutils bc zlib expat libslirp ncurses meson ninja cmake
```

Then build and install the toolchain into `/opt/riscv32`:

```sh
git clone https://github.com/riscv-collab/riscv-gnu-toolchain
cd riscv-gnu-toolchain
sudo mkdir -p /opt/riscv32 && sudo chown $USER /opt/riscv32
./configure --prefix=/opt/riscv32 --with-arch=rv32ima_zicsr --with-abi=ilp32 --enable-strip
make CFLAGS_FOR_TARGET_EXTRA=-mstrict-align CXXFLAGS_FOR_TARGET_EXTRA=-mstrict-align
```

`make` downloads about 200 MiB of sources, needs about 8 GiB of disk space and takes up to an hour or more; it installs the toolchain as it goes. Point the SDK at it with `-DKIANV_TOOLCHAIN_PATH=/opt/riscv32` (see [Using the SDK](#using-the-sdk)), or add `/opt/riscv32/bin` to `PATH`.

`-mstrict-align` makes newlib's string functions (`memcmp`, ...) avoid misaligned loads and stores. The KianV core traps on those, and the SDK then performs them in software (see [Misaligned accesses](#misaligned-accesses)), which works but is slow, so a toolchain built this way is faster. Toolchains built without it also work.

### Toolchain on macOS

Install the dependencies with [Homebrew](https://brew.sh):

```sh
brew install python3 gawk gnu-sed make gmp mpfr libmpc isl zlib expat texinfo flock libslirp ncurses meson ninja cmake bison m4 wget
```

Then follow the Linux build steps, using `gmake` instead of `make`. On Apple Silicon, riscv-gnu-toolchain's README asks for the repository to be cloned on a case-sensitive volume, for `macos.zsh` in it to be sourced before `./configure`, and for the open-file limit to be raised with `ulimit -n 65536`; its `macos-build.md` lists common build errors.

### Toolchain on Windows

Use WSL 2 with Ubuntu (`wsl --install -d Ubuntu` in an administrator PowerShell), and follow the Linux steps inside it. Build in the Linux file system (e.g. `~/riscv-gnu-toolchain`), not under `/mnt/c`: the Windows drives are much slower and not case-sensitive. CMake and the SDK are then also used inside WSL.

### Toolchain on BSD

Not tested: riscv-gnu-toolchain should build with GNU make (`gmake`) and the equivalents of the Linux dependencies from the ports collection.

### CMake

| System               | Command                                      |
| -------------------- | -------------------------------------------- |
| Ubuntu / Debian, WSL | `sudo apt-get install cmake build-essential` |
| Fedora / RHEL        | `sudo yum install cmake make`                |
| Arch                 | `sudo pacman -S cmake make`                  |
| macOS                | `brew install cmake`                         |
| FreeBSD              | `pkg install cmake gmake`                    |
| any (with Python)    | `pip install cmake`                          |

Check the version with `cmake --version`; installers are also on [cmake.org](https://cmake.org/download/).

## Using the SDK

Get the SDK, for example into `~/.kianv-sdk`, where the [examples](https://github.com/arnavbhate/kianv-sdk-examples) look for it by default:

```sh
git clone https://github.com/arnavbhate/kianv-sdk ~/.kianv-sdk
```

A project's `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.13...3.27)

# Where the SDK is: set with -DKIANV_SDK_PATH=..., otherwise ~/.kianv-sdk
if (NOT DEFINED KIANV_SDK_PATH)
    set(KIANV_SDK_PATH "$ENV{HOME}/.kianv-sdk")
endif()

# Before project(): sets up the cross toolchain
include("${KIANV_SDK_PATH}/kianv_sdk_init.cmake")

project(my_project C ASM) # Add CXX if you want C++ support

# After project(): adds the SDK libraries
kianv_sdk_init()

add_executable(app main.c)
target_link_libraries(app kianv_stdlib hardware_gpio)
```

`kianv_stdlib` brings in the runtime, C library support, UART, stdio and time functions. Other libraries (`hardware_gpio`, `hardware_spi`) are linked when needed. The libraries can also be linked one by one instead of `kianv_stdlib`; see [What each library needs](#what-each-library-needs).

Build from the project folder:

```sh
cmake -B build -DKIANV_TOOLCHAIN_PATH=/opt/riscv32
cmake --build build -j$(nproc)
```

The first command configures the build in `build/`. It takes the [CMake options](#cmake-options) (`-D...`), such as the toolchain's location (without `KIANV_TOOLCHAIN_PATH`, the toolchain is looked for in `PATH`) and `KIANV_SDK_PATH` if the SDK is not in `~/.kianv-sdk`; they are remembered, so later builds only need the second command. `-j$(nproc)` builds on all cores (on macOS, use `-j$(sysctl -n hw.ncpu)`); without it, CMake builds one file at a time.

Building `app` writes, next to `app.elf`:

| File      | Contents                                           |
| --------- | -------------------------------------------------- |
| `app.bin` | the raw image to write to flash at offset 0x100000 |
| `app.map` | the linker map                                     |
| `app.asm` | section headers and disassembly                    |

### CMake options

Set as CMake variables on the first `cmake` command line, e.g. `-DKIANV_NUM_UARTS=3`. Environment variables are not used.

| Option                        | Default                                           | Meaning                                                         |
| ----------------------------- | ------------------------------------------------- | --------------------------------------------------------------- |
| `KIANV_TOOLCHAIN_PATH`        | search `PATH`                                     | install prefix of the toolchain (the folder containing `bin/`)  |
| `KIANV_TOOLCHAIN_PREFIX`      | `riscv32-unknown-elf-`                            | prefix of the toolchain's programs (`gcc`, `objcopy`, ...)      |
| `KIANV_ARCH`                  | `rv32ima_zicntr_zicsr_zifencei_zaamo_zalrsc_sstc` | `-march` value; older GCCs need `rv32ima_zicntr_zicsr_zifencei` |
| `KIANV_CLIB`                  | detected                                          | `newlib` or `picolibc`                                          |
| `KIANV_NUM_UARTS`             | `2`                                               | the SoC's `NUM_UARTS` (1 to 5)                                  |
| `KIANV_NUM_GPIOS`             | `10`                                              | the SoC's GPIO pin count (0 to 32)                              |
| `KIANV_CXX_ENABLE_EXCEPTIONS` | `0`                                               | C++ exceptions                                                  |
| `KIANV_CXX_ENABLE_RTTI`       | `0`                                               | C++ RTTI                                                        |
| `KIANV_CXX_ENABLE_CXA_ATEXIT` | `0`                                               | `__cxa_atexit`                                                  |

The SDK's compile and link flags (`-march`, `-mabi`, ...) are kept when `CMAKE_C_FLAGS` etc. are set on the command line.

### Compile-time options

Set with `target_compile_definitions()`.

| Define                                   | Default                                       | Meaning                                      |
| ---------------------------------------- | --------------------------------------------- | -------------------------------------------- |
| `KIANV_DEFAULT_UART`                     | `0`                                           | UART used by stdio                           |
| `KIANV_DEFAULT_UART_BAUD_RATE`           | `115200`                                      | baud rate set by `stdio_init_all()`          |
| `KIANV_STDIO_DEFAULT_CRLF`               | `1`                                           | stdio sends `\n` as `\r\n`                   |
| `KIANV_DEFAULT_SPI`                      | `0`                                           | `spi_default`                                |
| `KIANV_PANIC_HANDLER`                    | see [Panics and crashes](#panics-and-crashes) | function `panic()` calls                     |
| `KIANV_STACK_SIZE`                       | `0x4000`                                      | stack space reserved (checked by the linker) |
| `KIANV_HEAP_SIZE`                        | `0x800`                                       | minimum heap (checked by the linker)         |
| `KIANV_EMULATE_MISALIGNED`               | `1`                                           | emulate misaligned loads and stores          |
| `KIANV_TIME_MAX_ALARMS`                  | `16`                                          | number of alarm slots                        |
| `PARAM_ASSERTIONS_ENABLED_HARDWARE_GPIO` | `0`                                           | check GPIO numbers (panics)                  |
| `PARAM_ASSERTIONS_ENABLED_HARDWARE_SPI`  | `0`                                           | check SPI instances (panics)                 |

## Memory map

| Address    | Size   | Used for                                                                            |
| ---------- | ------ | ----------------------------------------------------------------------------------- |
| 0x20000000 | 16 MiB | SPI flash; programs at 0x20100000                                                   |
| 0x80000000 | 28 MiB | heap (`malloc`)                                                                     |
| 0x81C00000 | 4 MiB  | program image, `.bss`, stack (from the top, 0x82000000, down)                       |
| 0x10000000 |        | UARTs (0x100 apart), `DIV_REG`, `CPU_FREQ_REG`, `SDRAM_SIZE_REG`, GPIO (0x10000700) |
| 0x10500000 |        | SPI0 to SPI2 (0x100 apart)                                                          |
| 0x02000000 |        | CLINT                                                                               |
| 0x11100000 |        | reboot register                                                                     |

### Accessing hardware registers directly

The SDK's functions follow these rules; code that accesses the registers itself has to as well:

- Each register responds only at its exact address and access width as listed in `platform.h` (e.g. the UART line status register is a byte at 0x10000005; a word read of 0x10000004 returns 0). Unmapped addresses in 0x10000000-0x12000000 read as 0 and ignore writes, without a fault.
- `DIV_REG` and the GPIO registers ignore byte enables: always access them with 32-bit loads and stores.
- `DIV_REG` holds the UART divider (bits 15:0, shared by all UARTs) and the SPI0 divider (bits 31:16, readable only through `DIV_REG`, not at 0x10000012): change one half with a read-modify-write, with interrupts disabled.
- Writing to a read-only register can hang the CPU: the UART line status register, and anything in flash (0x20000000-0x20FFFFFF) never answer a write.
- Writing to addresses 0-0xFF overwrites the FSBL in block RAM, and the board then no longer boots until the FPGA is reprogrammed; a write through a null pointer does this.
- Before writing an SPI data register, wait until the SPI is not busy (`spi_is_busy()`): writing during a transfer locks the SPI up.

## Libraries

### What each library needs

Linking a library also links the libraries in its "Links" column, and theirs in turn.

| Library                | Provides                                                                                                                                                                              | Links                                             |
| ---------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------- |
| `kianv_platform`       | register definitions, interrupt control, `panic()`, crash reports, misaligned-access emulation, C++ static initialisation guards, `kianv_reboot()`                                    |                                                   |
| `kianv_crt0`           | startup code and trap handler                                                                                                                                                         | `kianv_platform`                                  |
| `kianv_standard_link`  | linker script                                                                                                                                                                         |                                                   |
| `hardware_clock`       | `clock_get_hz()` (header only)                                                                                                                                                        | `kianv_platform`                                  |
| `hardware_uart`        | UART functions (header only)                                                                                                                                                          | `hardware_clock`                                  |
| `hardware_gpio`        | GPIO functions                                                                                                                                                                        | `kianv_platform`                                  |
| `hardware_spi`         | SPI functions                                                                                                                                                                         | `hardware_clock`                                  |
| `hardware_timer`       | `time_us_64()`, busy waits, the hardware alarm and timer interrupt                                                                                                                    | `kianv_platform`                                  |
| `kianv_time`           | `sleep_ms()`, timeouts, alarms, repeating timers                                                                                                                                      | `hardware_timer`                                  |
| `kianv_stdio`          | `stdio_init_all()`, `stdio_puts()`, `stdio_getchar()`, ..., the printing panic handler; with `kianv_clib_interface`, also `printf()` and the rest of the C library's input and output | `hardware_uart`, `kianv_time`                     |
| `kianv_atomic`         | atomics, including 64-bit ones                                                                                                                                                        | `kianv_platform`                                  |
| `kianv_cxx_options`    | C++ compile options (exceptions, RTTI)                                                                                                                                                |                                                   |
| `kianv_clib_interface` | the C library (newlib or picolibc) and, in projects with C++, the C++ standard library; their system calls (`_write`, `_read`, `_sbrk`, ...)                                          | `kianv_time`, `kianv_atomic`, `kianv_cxx_options` |
| `kianv_stdlib`         | the common set below                                                                                                                                                                  | see below                                         |

Every program needs `kianv_crt0` (which links `kianv_platform`) and `kianv_standard_link`. Without them, a program still builds, but it has no startup code or correct memory layout and does not run.

`kianv_stdlib` links `kianv_platform`, `kianv_crt0`, `kianv_standard_link`, `kianv_clib_interface`, `kianv_stdio`, `kianv_time`, `kianv_atomic`, `kianv_cxx_options` and `hardware_uart`, and through them `hardware_timer` and `hardware_clock`. It does not include `hardware_gpio` or `hardware_spi`, which are linked next to it when needed:

```cmake
target_link_libraries(app kianv_stdlib hardware_gpio hardware_spi)
```

The C library and the C++ standard library are linked only into programs that link `kianv_clib_interface`, directly or through `kianv_stdlib`. Programs without it are built without them: C programs can use the compiler's freestanding headers (`<stdint.h>`, `<stdbool.h>`, `<stdarg.h>`, ...) and libgcc, which is always linked, and C++ programs the language itself without its standard library. Linking libraries one by one instead of `kianv_stdlib`:

```cmake
# Bare minimum: register access only
target_link_libraries(app kianv_crt0 kianv_standard_link)

# Raw UART, GPIO and timing, without the C library
target_link_libraries(app kianv_crt0 kianv_standard_link hardware_uart hardware_gpio kianv_time)

# stdio and the printing panic handler, without the C library (no printf())
target_link_libraries(app kianv_crt0 kianv_standard_link kianv_stdio)

# The C library without stdio: printf() output is discarded and panic() prints nothing
target_link_libraries(app kianv_crt0 kianv_standard_link kianv_clib_interface)
```

### kianv_platform

Register addresses and counts (`platform.h`), interrupt control (`sync.h`: `save_and_disable_interrupts()`, `restore_interrupts()`, ...), CSR access (`csr.h`), `panic()` and `kianv_reboot()`. Always linked.

There is no general interrupt library like the RPi Pico SDK's `hardware_irq` (handler registration, a PLIC driver), because the SoC's interrupts are of almost no use: only UART0 is connected to the PLIC, and its interrupt cannot be enabled on its own, as it is active whenever the transmitter is busy, so a program would be interrupted for the whole of every transmission (see [Interrupts](#interrupts) under the hardware issues). GPIO, SPI and the other UARTs can only be polled. The one useful interrupt, the timer's, is handled by `hardware_timer` and `kianv_time`.

### Panics and crashes

`panic(fmt, ...)` takes a printf-style message, calls the panic handler with interrupts disabled, then halts. The handler is chosen when the program is built:

- `KIANV_PANIC_HANDLER`, if the program defines it: a `void handler(const char *fmt, va_list args)`, e.g. `target_compile_definitions(app PRIVATE KIANV_PANIC_HANDLER=my_handler)`
- otherwise, if `kianv_stdio` is linked, `kianv_stdio_panic_handler()`, which prints `*** PANIC *** <message>` to the default UART (setting it up if needed). It has its own small formatter (`%c %s %d %i %u %x %X %p %%`, the `0` and `-` flags, a width and `l`), so it works without a C library and in interrupt handlers.
- otherwise a handler that does nothing

Unhandled exceptions and interrupts are reported through `panic()`, with the cause, `pc`, `mtval`, `ra` and `sp`:

```
*** PANIC *** illegal instruction (mcause 2) at pc 0x81c0017c, mtval 0x00000000, ra 0x81c0017c, sp 0x81fffff0
```

Failed `assert()`s and `abort()` also panic. Returning from `main()` or calling `exit()` halts quietly.

### Misaligned accesses

The core traps on misaligned loads and stores (a word access to an address that is not a multiple of 4, or a halfword access to an odd address). The trap handler emulates `lh`, `lhu`, `lw`, `sh` and `sw` by accessing memory a byte at a time and then continues after the instruction, so programs behave as if the hardware supported them, only much more slowly. Code that would otherwise crash this way includes newlib's string functions (`memcmp`, ...) in a toolchain built without `-mstrict-align`, and with them C++'s `<iostream>`, as well as accesses through casted or packed-struct pointers.

Misaligned AMOs, `lr.w` and `sc.w` are not emulated, as they would no longer be atomic, and neither are loads into `sp`; these are reported as crashes. Building with `KIANV_EMULATE_MISALIGNED=0` turns the emulation off, so that every misaligned access is reported, which helps to find them.

### hardware_clock

`clock_get_hz()`: the system clock, read from the SoC.

### hardware_uart

`uart0` to `uart4` (as many as `KIANV_NUM_UARTS`), `uart_init()`, `uart_set_baudrate()`, `uart_get_baudrate()`, `uart_putc()`, `uart_puts()`, `uart_getc()`, `uart_write_blocking()`, `uart_read_blocking()`, `uart_is_writable()`, `uart_is_readable()` and `uart_tx_wait_blocking()`.

Differences from the Pico SDK: all UARTs share one baud rate divider, so setting the baud rate of one sets it for all; the format is fixed at 8N1; there are no FIFOs to configure, no flow control and no UART interrupts.

### hardware_gpio

`gpio_init()`, `gpio_set_dir()`, `gpio_put()`, `gpio_get()` and their `_masked`/`_all` variants, `gpio_set_mask()`, `gpio_clr_mask()`, `gpio_xor_mask()`, `gpio_get_out_level()`, `gpio_is_dir_out()`.

Differences from the Pico SDK: every pin is a plain input or output. There are no pin functions (the UART and SPI pins are dedicated), no pull-ups or pull-downs (set in the FPGA constraints file) and no GPIO interrupts. The hardware has no atomic set/clear registers, so functions changing only some pins disable interrupts while they read-modify-write. A pin set as an output reads back the level it drives.

### hardware_spi

`spi0` to `spi2`, `spi_init()`, `spi_set_baudrate()`, `spi_get_baudrate()`, `spi_set_format()`, `spi_write_read_blocking()`, `spi_write_blocking()`, `spi_read_blocking()`, `spi_is_busy()`, and the KianV-specific `spi_cs_select()`, `spi_cs_deselect()` and `spi_cs_is_selected()`.

| Instance | Used for (ULX3S)    | Clock at 60 MHz            | Mode |
| -------- | ------------------- | -------------------------- | ---- |
| `spi0`   | SD card             | up to 30 MHz, programmable | 0    |
| `spi1`   | network header      | 7.5 MHz, fixed             | 0    |
| `spi2`   | OLED, transmit only | 10 MHz, fixed              | 3    |

Differences from the Pico SDK: transfers are always 8 bits, MSB first, in the instance's fixed mode (`spi_set_format()` panics if asked for anything else); chip select is driven by the SPI block, so use `spi_cs_select()`/`spi_cs_deselect()` around a transaction; there is no FIFO, DMA, slave mode or 16-bit transfer.

### hardware_timer

The CLINT timer: `time_us_64()`, `time_us_32()`, `busy_wait_us()`, `busy_wait_ms()`, `busy_wait_until()`, and the single hardware alarm (`hardware_alarm_*`) that `kianv_time` uses. The timer counts microseconds.

### kianv_time

`sleep_ms()`, `sleep_us()`, `sleep_until()`, alarms (`add_alarm_in_ms()`, `add_alarm_at()`, `cancel_alarm()`, ...) and repeating timers (`add_repeating_timer_ms()`, `cancel_repeating_timer()`), with the Pico SDK's semantics. Alarm callbacks run in the timer interrupt. The core treats `wfi` as a no-op, so sleeping polls the timer.

### kianv_stdio

`stdio_init_all()`, `stdio_flush()`, `stdio_getchar()`, `stdio_getchar_timeout_us()`, `stdio_set_translate_crlf()`; `printf()` and friends go to the default UART.

`kianv_stdlib` includes `kianv_stdio`, but the C library interface does not need it: a program linking the individual libraries without `kianv_stdio` can still use the C library, but `printf()` output is discarded, input reads fail and `panic()` prints nothing (see [Panics and crashes](#panics-and-crashes)).

### kianv_atomic

C11 atomics: 32-bit atomics use the A extension's instructions directly, and the compiler builds 8 and 16-bit ones from 32-bit `lr.w`/`sc.w` loops; 64-bit atomics are implemented by disabling interrupts.

The core's LR/SC reservation does not record the address and is not cleared by traps (see [Known hardware issues](#known-hardware-issues)). If an interrupt handler changes a variable with an ordinary store while the interrupted code is in an `lr.w`/`sc.w` loop on it, the `sc.w` still succeeds and the handler's write is lost. Update variables shared with interrupt handlers with atomic operations in the handler too, or disable interrupts around the update.

### kianv_clib_interface, kianv_cxx_options

Links the C library (newlib or picolibc) and, in projects with C++, the C++ standard library, and provides their system calls (`_write`, `_read`, `_sbrk`, `_gettimeofday`, ...) and `new`/`delete`.

C++ programs (add `CXX` to `project()`) can use classes, templates, lambdas, global constructors and, when they link `kianv_clib_interface` (or `kianv_stdlib`), the standard library's containers, algorithms, strings and smart pointers (see the `cpp_demo` example). Exceptions and RTTI are off by default (`KIANV_CXX_ENABLE_EXCEPTIONS`, `KIANV_CXX_ENABLE_RTTI`). `<iostream>` works, but with a toolchain built without `-mstrict-align` it starts up slowly, because newlib then makes many misaligned accesses (see [Misaligned accesses](#misaligned-accesses)).

### kianv_crt0, kianv_standard_link

The startup code (copies the image, clears `.bss`, sets up the trap handler, runs constructors through `runtime_init()` and calls `main()`) and the linker script.

The trap handler saves the caller-saved registers and calls an ordinary C function with `mcause` and a pointer to them: `isr_riscv_machine_timer()` for the timer interrupt, `isr_riscv_machine_exception()` for exceptions, and `__unhandled_user_irq()` for anything else. `hardware_timer` provides the timer handler; the others default to crash reports and, being weak, can be replaced. Misaligned loads and stores go to `kianv_emulate_misaligned()` first, and only reach `isr_riscv_machine_exception()` if they cannot be emulated.

## Known hardware issues

Issues found in the SoC's RTL (`asic_fpga/`) that programmers may run into. The SDK works around those that affect it; every workaround in the SDK's code is marked with a comment starting `RTL-WORKAROUND`, naming the RTL file concerned, so `grep -rn RTL-WORKAROUND src` lists them.

### Memory map and bus

- The FSBL in block RAM (256 bytes at address 0) is writable (`bram_if.v`): a write through a null pointer corrupts it, and every later reset or reboot fails until the FPGA is reprogrammed. Null-pointer reads return FSBL code instead of faulting.
- A store to flash (0x20000000-0x20FFFFFF) hangs the CPU: it passes the access-fault check (`soc.v`), but the flash interface only answers reads (`spi_nor_if.v`). It should raise a store access fault.
- `is_reboot` in `soc.v` is `is_reboot_addr || is_reboot_data` where `&&` was meant, and the access-fault check uses it: any data access whose write data is `0x7777` skips the check, and an unmapped address then hangs the CPU.
- Unmapped addresses in 0x10000000-0x12000000 read as 0 and ignore writes instead of faulting, which hides bugs. The range check also includes 0x12000000 itself.
- Registers decode only their exact byte address: a word read of 0x10000004 does not see the UART line status register (0x10000005), and the SPI0 half of `DIV_REG` cannot be read at 0x10000012.
- `DIV_REG` and the GPIO registers ignore byte enables (`soc.v`, `gpio.v`): a byte or halfword store changes the whole register.

### CPU core

- The custom `mtimecmp`/`mtimecmph` CSRs (0x7C0/0x7C1) are stored but never compared with the time: only the CLINT's `mtimecmp` raises the machine timer interrupt (`csr_unit.v`). The SDK uses the CLINT.
- The LR/SC reservation is a single bit without the address, cleared only by `sc.w` (`datapath_unit.v`): `sc.w` to a different address than the `lr.w` succeeds, and a trap between them does not clear it, so a handler's ordinary store to the variable is overwritten by the interrupted `sc.w`.
- In vectored mode, `mtvec` also offsets exceptions (`base + 4 * cause`, `csr_unit.v`); the specification sends them to `base`. The SDK uses direct mode.
- For an illegal instruction, `mtval` holds only its 7-bit opcode (`main_fsm.v`), not the whole instruction.
- Misaligned loads and stores trap (the specification allows this) instead of being performed. GCC's default tuning assumes they work, only slowly, so newlib's string functions use them. The SDK emulates them in its trap handler (see [Misaligned accesses](#misaligned-accesses)).
- `wfi` is a no-op, so waiting for an interrupt cannot save power.
- `mstatus` stores any value written, including reserved MPP values that `mret` then switches to; writing `mip` replaces the hardware-driven bits for a cycle; `mepc` does not clear its low bits; `mcountinhibit` is stored but has no effect (`csr_unit.v`).

### Interrupts

- Only UART0 can interrupt: every other PLIC source is tied to 0 (`soc.v`), so UART1-UART4, GPIO, SPI and audio can only be polled.
- UART0's interrupt is "receive FIFO not empty" or "transmitter busy", with no enable bits (`soc.v`), so it is active for the whole of every transmission.
- The PLIC has no priority or threshold registers, and a claim does not clear the pending bit until completion.

### Timer

- The CLINT's `mtimecmp` resets to 0, so the timer interrupt is pending from reset: enabling it before writing `mtimecmp` interrupts immediately. The SDK sets `mtimecmp` first.
- `time` ticks at `SYSTEM_CLK / 1000000` (integer division), so it drifts if the clock is not a whole number of MHz.
- The CLINT's `div` input is unused (`clint.v`).

### UART

- Characters received back-to-back are garbled: after the stop bit, the receiver waits a further bit time before looking for the next start bit (`rx_uart.v`), and misses a start bit that follows immediately. Typed input works; pasted text or continuous data may not.
- A write to the line status register hangs the CPU: nothing answers it (`uart_if.v`).
- No errors are reported: receive FIFO overflow drops bytes, framing errors are discarded, and the line status register has no error bits.
- All UARTs share one baud rate divider, in the same register as the SPI0 divider.
- Each bit takes `div + 1` clocks, not `div` as `tx_uart.v` documents (in both `rx_uart.v` and `tx_uart.v`, the bit state takes a clock on top of the `div`-clock wait), so the baud rate is `SYSTEM_CLK / (div + 1)`. The SDK writes the divider accordingly; with `div = SYSTEM_CLK / baud`, the rate is slow by about 1/`div` (0.2% at 115200 baud and 60 MHz, but 5% at `div` = 20).

### SPI

- Writing the data register while a transfer is in progress locks the SPI up: the write completes in the same cycle as the transfer and is lost, the busy bit stays set, and the next write hangs the CPU (`spi.v`). The SDK waits until the SPI is idle before every write.
- Reading the data register during a transfer returns the previous byte.
- `KIANV_SPI_CTRL1_FREQ` and `KIANV_SPI_CTRL2_FREQ` in `defines_soc.vh` are used as dividers, and the clock is half the named frequency (13 MHz gives 7.5 MHz, 20 MHz gives 10 MHz at 60 MHz).
- In `spi.v`, the comments on the clock edges are swapped (MISO is sampled on the rising edge and MOSI changes on the falling edge); there is also a redundant `!resetn` check in the reset branch's `else`, and the state register duplicates `in_xfer`.

### GPIO

- There are no atomic set, clear or toggle registers, so changing some pins needs a read-modify-write with interrupts disabled.

### Caches

- `fence.i` resets the whole cache block, so it also flushes the data cache. This is harmless, as the data cache is write-through, but costs speed.

### Documentation

- `Kian_RISCV_Documentation.pdf` swaps the SPI register offsets (data at +0x00, control at +0x04); the hardware has control at +0x00 and data at +0x04.
