This is a custom bootloader for the SJ2 board. You may compile and load this bootloader, and then that will provide you the functionality to be able to re-write application by placing your compiled application on an SD card.

## How it works

### Build Application with Flash Offset

The bootloader assumes that it lives at the first 64K of flash memory. Hence, when you build your application, you need to build it with the correct address that makes this assumption correct. It is very easy to build your application such that it will start at 64K rather than the default of 0K. Simply modify `layout_lpc4078.ld`.

```
/* Use application_flash_offset=64k if using SD card bootloader; see lpc40xx_bootloader/README.md */
application_flash_offset = 64k;

MEMORY
{
  flash_512k (rx) : ORIGIN = 0x00000000 + application_flash_offset, LENGTH = 512k - application_flash_offset
  ram_64k (rwx)   : ORIGIN = 0x10000000, LENGTH = 64K   /* Main RAM       */
  ram_32k (rwx)   : ORIGIN = 0x20000000, LENGTH = 32K   /* Peripheral RAM */
}
```

### SD Card Interface

The bootloader can copy compiled binary file from the SD card to the flash memory starting at 64K offset.

* Place a file `lpc40xx_application.bin` to the SD card
  * SD Card needs to be formatted with FAT16 or FAT32
  * The binary file needs to be compiled with offset of `64k`
* Bootloader will boot, and copy the file from SD card to the on-board flash memory
* Bootloader will rename `lpc40xx_application.bin` to `lpc40xx_application.bin.flash`
  * This will prevent programming of the same file in a repetitive loop

## Boot messages

### New Application

When a new application is placed on the SD card, you should see messages that resemble the following output:
```
-----------------
BOOTLOADER
SD card: OK
-----------------
INFO: Located new FW file: lpc40xx_application.bin
Preparing and erasing sectors...
  Prepare sectors: OK (0)
  Erased sectors : OK (0)

  Opened lpc40xx_application.bin

  Read 4096 bytes,  Write 4096 bytes to 0x00010000: OK (0)
  Compare 4096 bytes at 0x00010000: OK (0)

  Read 4096 bytes,  Write 4096 bytes to 0x00011000: OK (0)
  Compare 4096 bytes at 0x00011000: OK (0)

  Read 4096 bytes,  Write 4096 bytes to 0x00012000: OK (0)
  Compare 4096 bytes at 0x00012000: OK (0)

  Read 4096 bytes,  Write 4096 bytes to 0x00013000: OK (0)
  Compare 4096 bytes at 0x00013000: OK (0)

  Read 4096 bytes,  Write 4096 bytes to 0x00014000: OK (0)
  Compare 4096 bytes at 0x00014000: OK (0)

  Read 4096 bytes,  Write 4096 bytes to 0x00015000: OK (0)
  Compare 4096 bytes at 0x00015000: OK (0)

  Read 4096 bytes,  Write 4096 bytes to 0x00016000: OK (0)
  Compare 4096 bytes at 0x00016000: OK (0)

  Read 4096 bytes,  Write 4096 bytes to 0x00017000: OK (0)
  Compare 4096 bytes at 0x00017000: OK (0)

  Read 4096 bytes,  Write 4096 bytes to 0x00018000: OK (0)
  Compare 4096 bytes at 0x00018000: OK (0)

  Read 4096 bytes,  Write 4096 bytes to 0x00019000: OK (0)
  Compare 4096 bytes at 0x00019000: OK (0)

  Read 4096 bytes,  Write 4096 bytes to 0x0001A000: OK (0)
  Compare 4096 bytes at 0x0001A000: OK (0)

  Read 3248 bytes,  Write 4096 bytes to 0x0001B000: OK (0)
  Compare 4096 bytes at 0x0001B000: OK (0)
SUCCESS: Renamed lpc40xx_application.bin to lpc40xx_application.bin.flashed


-----------------------------
Attemping to boot application
  Booting...
```

### Normal boot

When a new application is not present on the SD card, the bootloader should quickly boot your application and you should observe output like the following:

```
-----------------
BOOTLOADER
SD card: OK
-----------------
INFO: lpc40xx_application.bin not detected on SD card

-----------------------------
Attemping to boot application
  Booting...
```
