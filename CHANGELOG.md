## 1.02 (2025-03-11)


### Added

- * Display timeout has been added (Settings > Sleep), a value of zero disables the timeout
  * Turning the display off/on by long pressing the encoder (2 seconds). This reduces QRM from the display
  * Added "O" and "o" commands (display off and on) to the serial port protocol 
- AM/FM station search by scrolling with the encoder pressed (the search can also be stopped by rotating the encoder) 
- Pressing the encoder for more than 0.5 seconds triggers volume adjustment (this compensates the Volume menu option no longer being highlighted by default) 


### Changed

- * The menu has been reorganized, and the Spare items have been removed
  * A Settings menu has been added, and the display brightness adjustment option has been moved there
  * The position in the main menu and settings menu is remembered (until power is turned off)
  * The startup splash screen has been moved to Settings > About (but the reset of settings by turning on the receiver with the encoder pressed still works)
  * The Seek Up/Down menu items have been removed 
- Changed the charging indicator (lightning icon instead of the EXT text) 


### Fixed

- Fixed the duplication of the MW2 band name 

## 1.01 (2025-03-11)


### Added

- Added "MODE" configuration per band (FM, AM, LSB, USB) 


### Changed

- Improved tuning speed by delaying the display updates 

## 1.00 (2025-03-11)


### Added

- Added "Brightness" menu option

  * This controls the PWM from 32 to 255 (full on) in steps of steps of 32
  * When the brightness is set lower than 255, PSU or RFI noise may be present 
- Added "Calibration" menu option

  This allows the SI4732 reference clock offset to be compensated per band 
- Added Automatic Volume Control (AVC) menu option. This allows the maximum audio gain to be adjusted. 
- Added GPIO1 (Output) control (0=FM, 1 = AM/SSB) 
- Added a REMOTE serial interface for debug control and monitoring 
- User interface modified:

  * Removed the frequency scale
  * Set "Volume" as the default adjustment parameter
  * Modifed the S-Meter size and added labels
  * All actions now use a single press of the rotary encoder button, with a 10s timeout
  * Added status bar with indicators for Display and EEPROM write activity
  * Added unit labels for "Step" and "BW"
  * Added SSB tuning step options 10Hz, 25Hz, 50Hz, 0.1k and 0.5k
  * Added background refresh of main screen 
- VFO/BFO tuning mechanism added based on Goshante ATS_EX firmware

  * This provides "chuff" free tuning over a 28kHz span (+/- 14kHz)
  * Compile option "BFO_MENU_EN" for debug purposes, manual BFO is not required 


### Changed

- Modified FM steps options (50k, 100k, 200k, 1M) 
- Modified the audio mute behaviour

  * Previously the rx.setAudioMute() appeared to unmute when changing band
  * The "Mute" option now toggles the volume level between 0 and previous value 
- Modified the battery monitoring function

  * Uses set voltages for 25%, 50% and 75% with a configurable hysteresis voltage
  * Added voltage reading to status bar 
- Settings for AGC/ATTN, SoftMute and AVC stored in EEPROM per mode

  AGC/ATTN (FM, AM, SSB), SoftMute (AM, SSB), AVC (AM, SSB) 


### Fixed

- Fix compilation errors related to ledc* calls

  https://docs.espressif.com/projects/arduino-esp32/en/latest/migration_guides/2.x_to_3.0.html#ledc
