# ATS Mini

This firmware is for use on the SI4732 (ESP32-S3) Mini/Pocket Receiver

Based on the following sources:

* Ralph Xavier:      https://github.com/ralphxavier/SI4735
* PU2CLR, Ricardo:   https://github.com/pu2clr/SI4735
* Goshante:          https://github.com/goshante/ats20_ats_ex

G8PTN, Dave (2025)

## Development notes

Install [Arduino CLI](https://arduino.github.io/arduino-cli/1.2/installation/)

```shell
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
```

Install the toolchain and libraries

```shell
arduino-cli core install esp32:esp32
arduino-cli lib install "PU2CLR SI4735" TFT_eSPI
arduino-cli board details -b esp32:esp32:esp32s3
```

Compile and flash the firmware

``` shell
arduino-cli compile -b esp32:esp32:esp32s3 --board-options CDCOnBoot=cdc,FlashSize=8M,PSRAM=disabled,CPUFreq=80,USBMode=hwcdc,FlashMode=qio,PartitionScheme=default_8MB,DebugLevel=none --clean -e -p COM_PORT -u ats-mini
```


## Pinout

The pinout table is shown below.

The relevant colums are ESP32-WROOM-1 "Pin Name" and "ATS-Mini Sketch Pin Definitions"

| ESP32-WROOM-1<br>Pin # | ESP32-WROOM-1<br>Pin Name | ATS-MINI Sketch<br>Pin Definitions | TFT_eSPI<br>Pin Definition | xtronic.org<br>Schematic | Comments<br>Info         |
|------------------------|---------------------------|------------------------------------|----------------------------|--------------------------|--------------------------|
| 1                      | GND                       |                                    |                            | GND                      |                          |
| 2                      | 3V3                       |                                    |                            | VCC_33                   |                          |
| 3                      | EN                        |                                    |                            | EN                       | RST Button               |
| 4                      | IO4                       | VBAT_MON                           |                            | BAT_ADC                  | Battery monitor          |
| 5                      | IO5                       |                                    | TFT_RST                    | LCD_RES                  |                          |
| 6                      | IO6                       |                                    | TFT_CS                     | LCD_CS                   |                          |
| 7                      | IO7                       |                                    | TFT_DC                     | LCD_DC                   |                          |
| 8                      | IO15                      | PIN_POWER_ON                       |                            | RADIO_EN                 | 1= Radio LDO Enable      |
| 9                      | IO16                      | RESET_PIN                          |                            | RST                      | SI4732 Reset             |
| 10                     | IO17                      | ESP32_I2C_SCL                      |                            | I2C_SCL                  | SI4732 Clock             |
| 11                     | IO18                      | ESP32_I2C_SDA                      |                            | I2C_SDA                  | SI4732 Data              |
| 12                     | IO8                       |                                    | TFT_WR                     | LCD_WR                   |                          |
| 13                     | IO19                      |                                    |                            | USB_DM                   | USB_D- (CDC Port)        |
| 14                     | IO20                      |                                    |                            | USB_DP                   | USB_D+ (CDC Port)        |
| 15                     | IO3                       | AUDIO_MUTE                         |                            | MUTE                     | 1 = Mute L/R audio       |
| 16                     | IO46                      |                                    | TFT_D5                     | LCD_DS                   |                          |
| 17                     | IO9                       |                                    | TFT_RD                     | LCD_RD                   |                          |
| 18                     | IO10                      | PIN_AMP_EN                         |                            | AMP_EN                   | 1 = Audio Amp Enable     |
| 19                     | IO11                      |                                    |                            | NC                       | Spare                    |
| 20                     | IO12                      |                                    |                            | NC                       | Spare                    |
| 21                     | IO13                      |                                    |                            | NC                       | Spare                    |
| 22                     | IO14                      |                                    |                            | NC                       | Spare                    |
| 23                     | IO21                      | ENCODER_PUSH_BUTTON                |                            | SW                       | Rotary encoder SW signal |
| 24                     | IO47                      |                                    | TFT_D6                     | LCD_D6                   |                          |
| 25                     | IO48                      |                                    | TFT_D7                     | LCD_D7                   |                          |
| 26                     | IO45                      |                                    | TFT_D4                     | LCD_D4                   |                          |
| 27                     | IO0                       |                                    |                            | GPIO0                    | BOOT button              |
| 28                     | IO35                      |                                    |                            | NC                       | Used for OSPI PSRAM      |
| 29                     | IO36                      |                                    |                            | NC                       | Used for OSPI PSRAM      |
| 30                     | IO37                      |                                    |                            | NC                       | Used for OSPI PSRAM      |
| 31                     | IO38                      | PIN_LCD_BL                         | TFT_BL                     | LCD_BL                   | Backlight control        |
| 32                     | IO39                      |                                    | TFT_D0                     | LCD_D0                   |                          |
| 33                     | IO40                      |                                    | TFT_D1                     | LCD_D1                   |                          |
| 34                     | IO41                      |                                    | TFT_D2                     | LCD_D2                   |                          |
| 35                     | IO42                      |                                    | TFT_D3                     | LCD_D2                   |                          |
| 36                     | RXD0                      |                                    |                            | NC                       | GPIO44                   |
| 37                     | TXD0                      |                                    |                            | NC                       | GPIO43                   |
| 38                     | IO2                       | ENCODER_PIN_A                      |                            | A                        | Rotary encoder A signal  |
| 39                     | IO1                       | ENCODER_PIN_B                      |                            | B                        | Rotary encoder B signal  |
| 40                     | GND                       |                                    |                            | GND                      |                          |
| 41                     | EPAD                      |                                    |                            | GND                      |                          |
-----------------------------------------------------------------------------------------------------------------
