# WeAct STM32H750 Board Support

This directory contains support for the WeAct STM32H750 development board.

## Board Information

- **MCU**: STM32H750VBT6 or STM32H750IBK6  
- **Flash**: 128KB internal
- **RAM**: 1MB total (512KB + 128KB + 128KB + 64KB + 32KB + 64KB + 4KB)
- **External SPI Flash**: 8MB W25Q64 on SPI1 (PA5/PB4/PA7, CS: PD6)
- **External QSPI Flash**: 8MB W25Q64 on QSPI (PB2/6, PD11/12/13, PE2)
- **Display**: 0.96" ST7735 TFT LCD (128x160) on SPI4 (PE12/14, CS: PE11, DC: PE13)
- **Storage**: MicroSD card on SDMMC1 (PC8/9/10/11/12, PD2, Detect: PD4)
- **Camera**: 8-bit DVP interface with I2C control (PWDN: PA7)
- **USB**: USB-C connector (PA11/12)
- **LEDs**: Blue LED on PE3
- **Buttons**: User button on PC13

## Hardware Features

### External Flash Storage
- **SPI Flash**: 8MB W25Q64 on SPI1 for general storage
- **QSPI Flash**: 8MB W25Q64 on QSPI for high-speed applications
- **File Systems**: SmartFS, LittleFS, or NXFFS support

### Display and Graphics
- **0.96" ST7735 LCD**: 80x160 RGB565 display with NX graphics
- **Framebuffer**: Direct pixel access support
- **Graphics demos**: Included framebuffer and NX examples

### Camera Interface
- **DVP Port**: 8-bit digital camera interface (DCMI)
- **I2C Control**: Camera configuration via I2C1
- **Supported sensors**: OV7670, OV2640, OV7725, OV5640-AF

### Storage Options
- **MicroSD card**: FAT32 file system support
- **External flash**: Multiple file system options
- **Internal flash**: Program storage (128KB)

## Configurations

The board supports the following configurations:

- **nsh**: Basic NuttShell configuration
- **sdcard**: NSH with SD card support  
- **st7735**: NSH with ST7735 LCD display and flash storage
- **full**: Complete configuration with all peripherals enabled
- **usbnsh**: NSH with USB CDC/ACM console

All configurations are optimized to fit within the 128KB flash constraint.

## Pin Assignment

### Serial Console (USART1)
- **TX**: PA9
- **RX**: PA10  
- **Baud**: 115200

### SPI Flash (SPI1)
- **CLK**: PA5
- **MISO**: PB4 (alternate pin to avoid DVP conflict)
- **MOSI**: PA7 (shared with DVP_PWDN via SB1)
- **CS**: PD6 (configured via SB3)

### QSPI Flash
- **CLK**: PB2
- **CS**: PB6
- **IO0**: PD11
- **IO1**: PD12
- **IO2**: PE2  
- **IO3**: PD13

### ST7735 LCD (SPI4)
- **CLK**: PE12
- **MOSI**: PE14
- **DC**: PE13
- **CS**: PE11
- **LED**: PE10

### SD Card (SDMMC1)
- **CLK**: PC12
- **CMD**: PD2
- **D0**: PC8
- **D1**: PC9
- **D2**: PC10
- **D3**: PC11
- **Detect**: PD4 (configured via SB2)

### Camera Interface (DCMI)
- **PIXCLK**: PA6
- **HSYNC**: PH8  
- **VSYNC**: PB7
- **PWDN**: PA7 (configured via SB1)
- **D0-D7**: PH9-PH12, PH14, PD3, PE5-PE6

### Camera I2C (I2C1)
- **SCL**: PB8
- **SDA**: PB9

### User Interface
- **LED**: PE3 (Blue, active low)

## Solder Bridge Configuration

The board includes configurable solder bridges for flexible pin assignment:

- **SB1**: DVP_PWDN -> PA7 (conflicts with SPI1_MOSI)
- **SB2**: MicroSD_SW -> PD4 (card detect)
- **SB3**: SPI_Flash_CS -> PD6 (SPI flash chip select)
- **SB4**: DVP_AF2V8 -> AF2V8 (camera power)
- **SB5**: DVP_AFGND -> GND (camera ground)
- **SB6**: VBUS -> 5V (USB power)

### Hardware Notes
- PA6 is shared between SPI1_MISO and DVP_PIXCLK
- PA7 is shared between SPI1_MOSI and DVP_PWDN  
- Configure solder bridges based on intended use case
- SPI1 uses alternate MISO pin (PB4) to resolve conflicts
- **Button**: PC13 (active low)

### USB
- **DM**: PA11
- **DP**: PA12

## Building

```bash
cd nuttx

# Basic configuration
./tools/configure.sh weact-stm32h750:nsh

# With ST7735 LCD and flash storage
./tools/configure.sh weact-stm32h750:st7735

# Full configuration with all peripherals
./tools/configure.sh weact-stm32h750:full

make -j4
```

## Flashing

### Method 1: Specify address explicitly

```bash
openocd -f interface/stlink.cfg -f target/stm32h7x.cfg -c 'init' -c 'program nuttx.bin 0x08000000 verify reset' -c 'shutdown'
```

### Method 2: Use ELF file (recommended)

```bash
openocd -f interface/stlink.cfg -f target/stm32h7x.cfg -c 'init' -c 'program nuttx verify reset' -c 'shutdown'
```

## Serial Console

The board uses USART1 for the console:

- **TX**: PA9
- **RX**: PA10  
- **Baud**: 115200

## LED

- **User LED**: PC13 (active low)
