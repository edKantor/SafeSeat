# SafeSeat

Zephyr RTOS application. Runs on top of an existing nRF Connect SDK (NCS) install rather
than vendoring Zephyr into this repo — a "freestanding application" in Zephyr's terms.

`AvEdan/` is our own platform submodule: a [Zephyr module](https://docs.zephyrproject.org/latest/develop/modules.html)
holding custom boards/drivers, pulled in via `ZEPHYR_EXTRA_MODULES` (see `CMakeLists.txt`).
Everything else — HAL, BSP, drivers for STM32/nRF peripherals — comes from Zephyr itself.

## Prerequisites

An installed nRF Connect SDK (NCS) with its toolchain manager bundle, and its environment
sourced before building — normally via the nRF Connect for VS Code extension, or
`nrfutil toolchain-manager launch --ncs-version <version> -- <shell>`, or manually:

```
export TCROOT=<path-to-toolchain-bundle>            # e.g. .../ncs/toolchains/<hash>
export PATH="$TCROOT/bin:$TCROOT/opt/zephyr-sdk/gnu/arm-zephyr-eabi/bin:$PATH"
export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
export ZEPHYR_SDK_INSTALL_DIR="$TCROOT/opt/zephyr-sdk"
export ZEPHYR_BASE=<path-to-ncs>/zephyr
```

## Build

```
west build -b nrf52dk/nrf52832 -d build/nrf52dk .
```

Swap `-b <board>` for any Zephyr-supported board target.

## Known gap: STM32

NCS's own manifest only imports `hal_st` (ST *sensor* chips), not `hal_stm32` (the STM32
*MCU* HAL) — see `nrf/west.yml`'s `name-allowlist`. STM32 boards (e.g. `nucleo_f429zi`)
won't configure against this NCS install as-is; building for STM32 needs either a
separate vanilla-Zephyr west workspace (which does include `hal_stm32`), or manually
adding it as an extra west project on top of NCS. Not yet resolved.
