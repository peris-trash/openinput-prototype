# openinput-cm3

Open source mouse firmware.

### Supported Devices
- Blue pill
- Steelseries Rival 310

### Instructions

```
make
```

The binaries will be located at `bin/(version).openinput.hex` and `bin/(version).openinput.bin`.

### Dependencies

This project requires cmsis to be installed system-wide. I will only be able
to provide techincal support for Archlinux.

  - Compiler: [arm-none-eabi-gcc](https://www.archlinux.org/packages/community/x86_64/arm-none-eabi-gcc/)
  - Flash (optional): [stlink](https://www.archlinux.org/packages/community/x86_64/stlink/)
  - JLinkComander (optional): []()
  - armmem: [armmem]()
  - cmsis: []()

