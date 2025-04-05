// =================================
// INCLUDE FILES
// =================================

#include <Wire.h>
#include <TFT_eSPI.h>            // https://github.com/Xinyuan-LilyGO/T-Display-S3#quick-start
#include "EEPROM.h"
#include <SI4735.h>
#include "Rotary.h"              // Disabled half-step mode
#include "Common.h"
#include "patch_init.h"          // SSB patch for whole SSBRX initialization string

// SI473/5 and UI
#define MIN_ELAPSED_TIME         5  // 300
#define MIN_ELAPSED_RSSI_TIME  200  // RSSI check uses IN_ELAPSED_RSSI_TIME * 6 = 1.2s
#define ELAPSED_COMMAND      10000  // time to turn off the last command controlled by encoder. Time to goes back to the VFO control // G8PTN: Increased time and corrected comment
#define DEFAULT_VOLUME          35  // change it for your favorite sound volume
#define STRENGTH_CHECK_TIME   1500  // Not used
#define RDS_CHECK_TIME         250  // Increased from 90
#define CLICK_TIME              50
#define SHORT_PRESS_TIME       500
#define LONG_PRESS_TIME        2000

#define BACKGROUND_REFRESH_TIME 5000    // Background screen refresh time. Covers the situation where there are no other events causing a refresh
#define TUNE_HOLDOFF_TIME         90    // Timer to hold off display whilst tuning

#define EEPROM_SIZE     512
#define STORE_TIME    10000                  // Time of inactivity to make the current receiver status writable (10s)

// =================================
// CONSTANTS AND VARIABLES
// =================================

// SI4732/5 patch
const uint16_t size_content = sizeof ssb_patch_content; // see patch_init.h


// EEPROM
// ====================================================================================================================================================
// Update F/W version comment as required   F/W VER    Function                                                           Locn (dec)            Bytes
// ====================================================================================================================================================
const uint8_t  app_id  = 67;          //               EEPROM ID.  If EEPROM read value mismatch, reset EEPROM            eeprom_address        1
const uint16_t app_ver = 108;         //     v1.08     EEPROM VER. If EEPROM read value mismatch (older), reset EEPROM    eeprom_ver_address    2
char app_date[] = "2025-03-25";
const int eeprom_address = 0;         //               EEPROM start address
const int eeprom_set_address = 256;   //               EEPROM setting base address
const int eeprom_setp_address = 272;  //               EEPROM setting (per band) base address
const int eeprom_ver_address = 496;   //               EEPROM version base address

long storeTime = millis();
bool itIsTimeToSave = false;

bool bfoOn = false;
bool ssbLoaded = false;
char bfo[18]="0000";
bool muted = false;
int8_t agcIdx = 0;
uint8_t disableAgc = 0;
int8_t agcNdx = 0;
int8_t softMuteMaxAttIdx = 4;

uint8_t seekDirection = 1;
bool seekStop = false;        // G8PTN: Added flag to abort seeking on rotary encoder detection
bool seekModePress = false;   // Seek happened during long press

uint16_t currentCmd = CMD_NONE;
bool fmRDS = false;

int16_t currentBFO = 0;
long elapsedRSSI = millis();
long elapsedButton = millis();

long lastStrengthCheck = millis();
long lastRDSCheck = millis();

long elapsedCommand = millis();
volatile int encoderCount = 0;
uint16_t currentFrequency;

const uint16_t currentBFOStep = 10;

// G8PTN: Main additional variables
// BFO and Calibration limits (BFOMax + CALMax <= 16000)
const int BFOMax = 14000;               // Maximum range for currentBFO = +/- BFOMax
const int CALMax =  2000;               // Maximum range for currentCAL = +/- CALMax

// AGC/ATTN index per mode (FM/AM/SSB)
int8_t FmAgcIdx = 0;                    // Default FM  AGGON  : Range = 0 to 37, 0 = AGCON, 1 - 27 = ATTN 0 to 26
int8_t AmAgcIdx = 0;                    // Default AM  AGCON  : Range = 0 to 37, 0 = AGCON, 1 - 37 = ATTN 0 to 36
int8_t SsbAgcIdx = 0;                   // Default SSB AGCON  : Range = 0 to 1,  0 = AGCON,      1 = ATTN 0

// AVC index per mode (AM/SSB)
int8_t AmAvcIdx = 48;                   // Default AM  = 48 (as per AN332), range = 12 to 90 in steps of 2
int8_t SsbAvcIdx = 48;                  // Default SSB = 48, range = 12 to 90 in steps of 2

// SoftMute index per mode (AM/SSB)
int8_t AmSoftMuteIdx = 4;               // Default AM  = 4, range = 0 to 32
int8_t SsbSoftMuteIdx = 4;              // Default SSB = 4, range = 0 to 32

// Button checking
unsigned long pb1_time = 0;             // Push button timer
unsigned long pb1_edge_time = 0;        // Push button edge time
unsigned long pb1_pressed_time = 0;     // Push button pressed time
unsigned long pb1_short_pressed_time = 0; // Push button short pressed time
unsigned long pb1_long_pressed_time = 0;// Push button long pressed time
unsigned long pb1_released_time = 0;    // Push button released time
int pb1_current = HIGH;                 // Push button current state
int pb1_stable = HIGH;                  // Push button stable state
int pb1_last = HIGH;                    // Push button last state (after debounce)
bool pb1_pressed = false;               // Push button pressed
bool pb1_short_pressed = false;         // Push button short pressed
bool pb1_long_pressed = false;          // Push button long pressed
bool pb1_released = false;              // Push button released
bool pb1_short_released = false;        // Push button short released
bool pb1_long_released = false;         // Push button long released

bool display_on = true;                 // Display state

// Status bar icon flags
bool screen_toggle = false;             // Toggle when drawsprite is called
bool eeprom_wr_flag = false;            // Flag indicating EEPROM write request

// Firmware controlled mute
uint8_t mute_vol_val = 0;               // Volume level when mute is applied

// Menu options
int16_t currentCAL = 0;                 // Calibration offset, +/- 1000Hz in steps of 10Hz
uint16_t currentBrt = 128;              // Display brightness, range = 32 to 255 in steps of 32
int8_t currentAVC = 48;                 // Selected AVC, range = 12 to 90 in steps of 2
uint16_t currentSleep = 30;             // Display sleep timeout, range = 0 to 255 in steps of 5
long elapsedSleep = millis();           // Display sleep timer

// Background screen refresh
uint32_t background_timer = millis();   // Background screen refresh timer.
uint32_t tuning_timer = millis();       // Tuning hold off timer.
bool tuning_flag = false;               // Flag to indicate tuning

// Time
uint32_t clock_timer = 0;
uint8_t time_seconds = 0;
uint8_t time_minutes = 0;
uint8_t time_hours = 0;
char time_disp [16];

// Remote serial
#if USE_REMOTE
uint32_t g_remote_timer = millis();
uint8_t g_remote_seqnum = 0;
bool g_remote_log = false;
#endif


// Tables

#include "themes.h"

uint8_t currentMode = FM;

//int tabStep[] = {1, 5, 10, 50, 100, 500, 1000};
//const int lastStep = (sizeof tabStep / sizeof(int)) - 1;

// Calibration (per band). Size needs to be the same as band[]
// Defaults
int16_t bandCAL[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

uint8_t rssi = 0;
uint8_t snr = 0;
uint8_t volume = DEFAULT_VOLUME;

// Devices class declarations
Rotary encoder = Rotary(ENCODER_PIN_B, ENCODER_PIN_A);      // G8PTN: Corrected mapping based on rotary library


TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);

SI4735 rx;

const char *get_fw_ver()
{
  static char fw_ver[25] = "\0";

  if(!fw_ver[0])
  {
    uint16_t ver_major = (app_ver / 100);
    uint16_t ver_minor = (app_ver % 100);
    sprintf(fw_ver, "F/W: v%1.1d.%2.2d %s", ver_major, ver_minor, app_date);
  }

  return(fw_ver);
}

void setup()
{
  // Enable Serial. G8PTN: Added
  Serial.begin(115200);

  // Audio Amplifier Enable. G8PTN: Added
  // Initally disable the audio amplifier until the SI4732 has been setup
  pinMode(PIN_AMP_EN, OUTPUT);
  digitalWrite(PIN_AMP_EN, LOW);

  // SI4732 VDD Enable
  pinMode(PIN_POWER_ON, OUTPUT);
  digitalWrite(PIN_POWER_ON, HIGH);

  // Encoder pins. Enable internal pull-ups
  pinMode(ENCODER_PUSH_BUTTON, INPUT_PULLUP);
  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);

  // The line below may be necessary to setup I2C pins on ESP32
  Wire.begin(ESP32_I2C_SDA, ESP32_I2C_SCL);

  // TFT display setup
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(theme[themeIdx].bg);
  spr.createSprite(320,170);
  spr.setTextDatum(MC_DATUM);
  spr.setSwapBytes(true);
  spr.setFreeFont(&Orbitron_Light_24);
  spr.setTextColor(theme[themeIdx].text, theme[themeIdx].bg);

  // TFT display brightness control (PWM)
  // Note: At brightness levels below 100%, switching from the PWM may cause power spikes and/or RFI
  ledcAttach(PIN_LCD_BL, 16000, 8);  // Pin assignment, 16kHz, 8-bit
  ledcWrite(PIN_LCD_BL, 255);        // Default value 255 = 100%)

  // EEPROM
  // Note: Use EEPROM.begin(EEPROM_SIZE) before use and EEPROM.begin.end after use to free up memory and avoid memory leaks
  EEPROM.begin(EEPROM_SIZE);

  // Press and hold Encoder button to force an EEPROM reset
  // Indirectly forces the reset by setting app_id = 0 (Detected in the subsequent check for app_id and app_ver)
  // Note: EEPROM reset is recommended after firmware updates
  if (digitalRead(ENCODER_PUSH_BUTTON) == LOW) {

    tft.setTextSize(2);
    tft.setTextColor(theme[themeIdx].text, theme[themeIdx].bg);
    tft.println(get_fw_ver());
    tft.println();
    EEPROM.write(eeprom_address, 0);
    EEPROM.commit();
    tft.setTextColor(theme[themeIdx].text_warn, theme[themeIdx].bg);
    tft.print("EEPROM Resetting");
    delay(2000);
  }

  EEPROM.end();

  // G8PTN: Moved this to later, to avoid interrupt action
  /*
  // ICACHE_RAM_ATTR void rotaryEncoder(); see rotaryEncoder implementation below.
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), rotaryEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), rotaryEncoder, CHANGE);
  */

  // Check for SI4732 connected on I2C interface
  // If the SI4732 is not detected, then halt with no further processing
  rx.setI2CFastModeCustom(100000);

  int16_t si4735Addr = rx.getDeviceI2CAddress(RESET_PIN); // Looks for the I2C bus address and set it.  Returns 0 if error

  if ( si4735Addr == 0 ) {
    tft.setTextSize(2);
    tft.setTextColor(theme[themeIdx].text_warn, theme[themeIdx].bg);
    tft.println("Si4735 not detected");
    while (1);
  }

  rx.setup(RESET_PIN, MW_BAND_TYPE);
  // Comment the line above and uncomment the three lines below if you are using external ref clock (active crystal or signal generator)
  // rx.setRefClock(32768);
  // rx.setRefClockPrescaler(1);   // will work with 32768
  // rx.setup(RESET_PIN, 0, MW_BAND_TYPE, SI473X_ANALOG_AUDIO, XOSCEN_RCLK);

  // Attached pin to allows SI4732 library to mute audio as required to minimise loud clicks
  rx.setAudioMuteMcuPin(AUDIO_MUTE);

  clearStationName();

  delay(300);

  // Audio Amplifier Enable. G8PTN: Added
  // After the SI4732 has been setup, enable the audio amplifier
  digitalWrite(PIN_AMP_EN, HIGH);


  // Checking the EEPROM content
  // Checks app_id (which covers manual reset) and app_ver which allows for automatic reset
  // The app_ver is equivalent to a F/W version.

  // Debug
  // Read all EEPROM locations
  #if DEBUG4_PRINT
  EEPROM.begin(EEPROM_SIZE);
  Serial.println("**** EEPROM READ: Pre Check");
  for (int i = 0; i <= (EEPROM_SIZE - 1); i++){
    Serial.print(EEPROM.read(i));
    delay(10);
    Serial.print("\t");
    if (i%16 == 15) Serial.println();
  }
  Serial.println("****");
  EEPROM.end();
  #endif

  // Perform check against app_id and app_ver
  uint8_t  id_read;
  uint16_t ver_read;

  EEPROM.begin(EEPROM_SIZE);
  id_read = EEPROM.read(eeprom_address);
  ver_read  = EEPROM.read(eeprom_ver_address) << 8;
  ver_read |= EEPROM.read(eeprom_ver_address + 1);
  EEPROM.end();

  if (id_read == app_id) {
    readAllReceiverInformation();                        // Load EEPROM values
  }
  else {
    saveAllReceiverInformation();                        // Set EEPROM to defaults
    rx.setVolume(volume);                                // Set initial volume after EEPROM reset
    ledcWrite(PIN_LCD_BL, currentBrt);                   // Set initial brightness after EEPROM reset
  }

  // Debug
  // Read all EEPROM locations
  #if DEBUG4_PRINT
  EEPROM.begin(EEPROM_SIZE);
  Serial.println("**** START READ: Post check actions");
  for (int i = 0; i <= (EEPROM_SIZE - 1); i++){
    Serial.print(EEPROM.read(i));
    delay(10);
    Serial.print("\t");
    if (i%16 == 15) Serial.println();
  }
  Serial.println("****");
  EEPROM.end();
  #endif

  // ** SI4732 STARTUP **
  // Uses values from EEPROM (Last stored or defaults after EEPROM reset)
  useBand();

  drawScreen(currentCmd);

  // Interrupt actions for Rotary encoder
  // Note: Moved to end of setup to avoid inital interrupt actions
  // ICACHE_RAM_ATTR void rotaryEncoder(); see rotaryEncoder implementation below.
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), rotaryEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), rotaryEncoder, CHANGE);
}


/**
 * Prints a given content on display
 */
void print(uint8_t col, uint8_t lin, const GFXfont *font, uint8_t textSize, const char *msg) {
  tft.setCursor(col,lin);
  tft.setTextSize(textSize);
  tft.setTextColor(theme[themeIdx].text_warn, theme[themeIdx].bg);
  tft.println(msg);
}

void printParam(const char *msg) {
  tft.fillScreen(theme[themeIdx].bg);
  print(0,10,NULL,2, msg);
}

/*
   writes the conrrent receiver information into the eeprom.
   The EEPROM.update avoid write the same data in the same memory position. It will save unnecessary recording.
*/
void saveAllReceiverInformation()
{
  eeprom_wr_flag = true;
  int addr_offset;
  int16_t currentBFOs = (currentBFO % 1000);            // G8PTN: For SSB ensures BFO value is valid wrt band[bandIdx].currentFreq = currentFrequency;

  EEPROM.begin(EEPROM_SIZE);

  EEPROM.write(eeprom_address, app_id);                 // Stores the app id;
  EEPROM.write(eeprom_address + 1, rx.getVolume());     // Stores the current Volume
  EEPROM.write(eeprom_address + 2, bandIdx);            // Stores the current band
  EEPROM.write(eeprom_address + 3, fmRDS);              // G8PTN: Not used
  EEPROM.write(eeprom_address + 4, currentMode);        // Stores the current Mode (FM / AM / LSB / USB). Now per mode, leave for compatibility
  EEPROM.write(eeprom_address + 5, currentBFOs >> 8);   // G8PTN: Stores the current BFO % 1000 (HIGH byte)
  EEPROM.write(eeprom_address + 6, currentBFOs & 0XFF); // G8PTN: Stores the current BFO % 1000 (LOW byte)
  EEPROM.commit();

  addr_offset = 7;

  // G8PTN: Commented out the assignment
  // - The line appears to be required to ensure the band[bandIdx].currentFreq = currentFrequency
  // - Updated main code to ensure that this should occur as required with frequency, band or mode changes
  // - The EEPROM reset code now calls saveAllReceiverInformation(), which is the correct action, this line
  //   must be disabled otherwise band[bandIdx].currentFreq = 0 (where bandIdx = 0; by default) on EEPROM reset
  //band[bandIdx].currentFreq = currentFrequency;

  for (int i = 0; i <= getTotalBands(); i++)
  {
    EEPROM.write(addr_offset++, (band[i].currentFreq >> 8));   // Stores the current Frequency HIGH byte for the band
    EEPROM.write(addr_offset++, (band[i].currentFreq & 0xFF)); // Stores the current Frequency LOW byte for the band
    EEPROM.write(addr_offset++, band[i].currentStepIdx);       // Stores current step of the band
    EEPROM.write(addr_offset++, band[i].bandwidthIdx);         // table index (direct position) of bandwidth
    EEPROM.commit();
  }

  // G8PTN: Added
  addr_offset = eeprom_set_address;
  EEPROM.write(addr_offset++, currentBrt >> 8);         // Stores the current Brightness value (HIGH byte)
  EEPROM.write(addr_offset++, currentBrt & 0XFF);       // Stores the current Brightness value (LOW byte)
  EEPROM.write(addr_offset++, FmAgcIdx);                // Stores the current FM AGC/ATTN index value
  EEPROM.write(addr_offset++, AmAgcIdx);                // Stores the current AM AGC/ATTN index value
  EEPROM.write(addr_offset++, SsbAgcIdx);               // Stores the current SSB AGC/ATTN index value
  EEPROM.write(addr_offset++, AmAvcIdx);                // Stores the current AM AVC index value
  EEPROM.write(addr_offset++, SsbAvcIdx);               // Stores the current SSB AVC index value
  EEPROM.write(addr_offset++, AmSoftMuteIdx);           // Stores the current AM SoftMute index value
  EEPROM.write(addr_offset++, SsbSoftMuteIdx);          // Stores the current SSB SoftMute index value
  EEPROM.write(addr_offset++, currentSleep >> 8);       // Stores the current Sleep value (HIGH byte)
  EEPROM.write(addr_offset++, currentSleep & 0XFF);     // Stores the current Sleep value (LOW byte)
  EEPROM.write(addr_offset++, themeIdx);                // Stores the current Theme index value
  EEPROM.commit();

  addr_offset = eeprom_setp_address;
  for (int i = 0; i <= getTotalBands(); i++)
  {
    EEPROM.write(addr_offset++, (bandCAL[i] >> 8));     // Stores the current Calibration value (HIGH byte) for the band
    EEPROM.write(addr_offset++, (bandCAL[i] & 0XFF));   // Stores the current Calibration value (LOW byte) for the band
    EEPROM.write(addr_offset++, band[i].bandMode);      // Stores the current Mode value for the band
    EEPROM.commit();
  }

  addr_offset = eeprom_ver_address;
  EEPROM.write(addr_offset++, app_ver >> 8);            // Stores app_ver (HIGH byte)
  EEPROM.write(addr_offset++, app_ver & 0XFF);          // Stores app_ver (LOW byte)
  EEPROM.commit();

  EEPROM.end();
}

/**
 * reads the last receiver status from eeprom.
 */
void readAllReceiverInformation()
{
  uint8_t volume;
  int addr_offset;
  EEPROM.begin(EEPROM_SIZE);

  volume = EEPROM.read(eeprom_address + 1); // Gets the stored volume;
  bandIdx = EEPROM.read(eeprom_address + 2);
  fmRDS = EEPROM.read(eeprom_address + 3);                // G8PTN: Not used
  currentMode = EEPROM.read(eeprom_address + 4);          // G8PTM: Reads stored Mode. Now per mode, leave for compatibility
  currentBFO = EEPROM.read(eeprom_address + 5) << 8;      // G8PTN: Reads stored BFO value (HIGH byte)
  currentBFO |= EEPROM.read(eeprom_address + 6);          // G8PTN: Reads stored BFO value (HIGH byte)

  addr_offset = 7;
  for (int i = 0; i <= getTotalBands(); i++)
  {
    band[i].currentFreq = EEPROM.read(addr_offset++) << 8;
    band[i].currentFreq |= EEPROM.read(addr_offset++);
    band[i].currentStepIdx = EEPROM.read(addr_offset++);
    band[i].bandwidthIdx = EEPROM.read(addr_offset++);
  }

  // G8PTN: Added
  addr_offset = eeprom_set_address;
  currentBrt      = EEPROM.read(addr_offset++) << 8;      // Reads stored Brightness value (HIGH byte)
  currentBrt     |= EEPROM.read(addr_offset++);           // Reads stored Brightness value (LOW byte)
  FmAgcIdx        = EEPROM.read(addr_offset++);           // Reads stored FM AGC/ATTN index value
  AmAgcIdx        = EEPROM.read(addr_offset++);           // Reads stored AM AGC/ATTN index value
  SsbAgcIdx       = EEPROM.read(addr_offset++);           // Reads stored SSB AGC/ATTN index value
  AmAvcIdx        = EEPROM.read(addr_offset++);           // Reads stored AM AVC index value
  SsbAvcIdx       = EEPROM.read(addr_offset++);           // Reads stored SSB AVC index value
  AmSoftMuteIdx   = EEPROM.read(addr_offset++);           // Reads stored AM SoftMute index value
  SsbSoftMuteIdx  = EEPROM.read(addr_offset++);           // Reads stored SSB SoftMute index value
  currentSleep    = EEPROM.read(addr_offset++) << 8;      // Reads stored Sleep value (HIGH byte)
  currentSleep   |= EEPROM.read(addr_offset++);           // Reads stored Sleep value (LOW byte)
  themeIdx        = EEPROM.read(addr_offset++);           // Reads stored Theme index value

  addr_offset = eeprom_setp_address;
  for (int i = 0; i <= getTotalBands(); i++)
  {
    bandCAL[i]  = EEPROM.read(addr_offset++) << 8;      // Reads stored Calibration value (HIGH byte) per band
    bandCAL[i] |= EEPROM.read(addr_offset++);           // Reads stored Calibration value (LOW byte) per band
    band[i].bandMode = EEPROM.read(addr_offset++);      // Reads stored Mode value per band
  }

  EEPROM.end();

  // G8PTN: Added
  ledcWrite(PIN_LCD_BL, currentBrt);

  selectBand(bandIdx);

  delay(50);
  rx.setVolume(volume);
}

// To store any change into the EEPROM, we need at least STORE_TIME
// milliseconds of inactivity.
void resetEepromDelay()
{
  storeTime = millis();
  itIsTimeToSave = true;
}

// When no command is selected, the encoder controls the frequency
void disableCommands()
{
  currentCmd = CMD_NONE;
  bfoOn = false;
}

/**
 * Reads encoder via interrupt
 * Use Rotary.h and  Rotary.cpp implementation to process encoder via interrupt
 * if you do not add ICACHE_RAM_ATTR declaration, the system will reboot during attachInterrupt call.
 * With ICACHE_RAM_ATTR macro you put the function on the RAM.
 */
ICACHE_RAM_ATTR void rotaryEncoder()
{ // rotary encoder events
  uint8_t encoderStatus = encoder.process();
  if (encoderStatus) {
    encoderCount = (encoderStatus == DIR_CW) ? 1 : -1;
    seekStop = true;  // G8PTN: Added flag
  }
}

/**
 * Switch the radio to current band
 */
void useBand(uint8_t bandIdx)
{
  // Set mode, frequency, step, bandwidth
  selectBand(bandIdx);

  if(band[bandIdx].bandType==FM_BAND_TYPE)
  {
    // rx.setTuneFrequencyAntennaCapacitor(0);
    rx.setFM(band[bandIdx].minimumFreq, band[bandIdx].maximumFreq, band[bandIdx].currentFreq, tabFmStep[band[bandIdx].currentStepIdx]);
    rx.setSeekFmLimits(band[bandIdx].minimumFreq, band[bandIdx].maximumFreq);
    rx.setFMDeEmphasis(1);
    rx.RdsInit();
    rx.setRdsConfig(1, 2, 2, 2, 2);
    rx.setGpioCtl(1,0,0);   // G8PTN: Enable GPIO1 as output
    rx.setGpio(0,0,0);      // G8PTN: Set GPIO1 = 0
  } 
  else
  {
    // Set the tuning capacitor for SW or MW/LW
    // rx.setTuneFrequencyAntennaCapacitor((band[bandIdx].bandType == MW_BAND_TYPE || band[bandIdx].bandType == LW_BAND_TYPE) ? 0 : 1);

    if(ssbLoaded)
    {
      // Configure SI4732 for SSB (SI4732 step not used, set to 0)
      rx.setSSB(
        band[bandIdx].minimumFreq,
        band[bandIdx].maximumFreq,
        band[bandIdx].currentFreq,
        0, currentMode
      );
      // G8PTN: Always enabled
      rx.setSSBAutomaticVolumeControl(1);
      // G8PTN: Commented out
      //rx.setSsbSoftMuteMaxAttenuation(softMuteMaxAttIdx);
    }
    else
    {
      // Setting step to 1kHz
      rx.setAM(
        band[bandIdx].minimumFreq,
        band[bandIdx].maximumFreq,
        band[bandIdx].currentFreq,
        band[bandIdx].currentStepIdx >= AmTotalSteps ? 1 : amStep[band[bandIdx].currentStepIdx].step
      );
    }

    // G8PTN: Enable GPIO1 as output
    rx.setGpioCtl(1,0,0);
    // G8PTN: Set GPIO1 = 1
    rx.setGpio(1,0,0);
    // Consider the range all defined current band
    rx.setSeekAmLimits(band[bandIdx].minimumFreq, band[bandIdx].maximumFreq);
    // Max 10kHz for spacing
    rx.setSeekAmSpacing(5);
  }

  // G8PTN: Added
  // Call doSoftMute(0), 0 = No incr/decr action (eqivalent to getSoftMute)
  // This gets softMuteMaxAttIdx based on mode (AM, SSB)
  doSoftMute(0);

  // Call doAgc(0), 0 = No incr/decr action (eqivalent to getAgc)
  // This gets disableAgc and agcNdx values based on mode (FM, AM , SSB)
  doAgc(0);

  // Call doAvc(0), 0 = No incr/decr action (eqivalent to getAvc)
  // This gets currentAVC values based on mode (AM, SSB)
  doAvc(0);

  delay(100);

#if DEBUG2_PRINT
  // Debug
  Serial.print("Info: useBand() >>> currentStepIdx = ");
  Serial.print(currentStepIdx);
  Serial.print(", idxAmStep = ");
  Serial.print(idxAmStep);
  Serial.print(", band[bandIdx].currentStepIdx = ");
  Serial.print(band[bandIdx].currentStepIdx);
  Serial.print(", currentMode = ");
  Serial.println(currentMode);
#endif

  rssi = 0;
  snr  = 0;
 
  clearStationName();
  drawScreen(currentCmd);
}

void loadSSB()
{
  rx.setI2CFastModeCustom(400000); // You can try rx.setI2CFastModeCustom(700000); or greater value
  rx.loadPatch(ssb_patch_content, size_content, bandwidthSSB[bwIdxSSB].idx);
  rx.setI2CFastModeCustom(100000);
  ssbLoaded = true;
}

void correctCutoffFilter()
{
  // If audio bandwidth selected is about 2 kHz or below, it is recommended to set Sideband Cutoff Filter to 0.
  if (bandwidthSSB[bwIdxSSB].idx == 0 || bandwidthSSB[bwIdxSSB].idx == 4 || bandwidthSSB[bwIdxSSB].idx == 5)
    rx.setSSBSidebandCutoffFilter(0);
  else
    rx.setSSBSidebandCutoffFilter(1);
}

/**
 *  This function is called by the seek function process.  G8PTN: Added
 */
bool checkStopSeeking() {
  // Checks the seekStop flag
  return seekStop;  // returns true if the user rotates the encoder
}

/**
 *  This function is called by the seek function process.
 */
void showFrequencySeek(uint16_t freq)
{
  currentFrequency = freq;
  drawScreen(currentCmd);
}

/**
 *  Find a station. The direction is based on the last encoder move clockwise or counterclockwise
 */
void doSeek()
{
  if (isSSB()) return; // It does not work for SSB mode

  rx.seekStationProgress(showFrequencySeek, checkStopSeeking, seekDirection);   // G8PTN: Added checkStopSeeking
  currentFrequency = rx.getFrequency();

}

uint8_t getStrength() {
#if THEME_EDITOR
  return 17;
#endif
  if (currentMode != FM) {
    //dBuV to S point conversion HF
    if ((rssi <=  1)) return  1;                  // S0
    if ((rssi >  1) and (rssi <=  2)) return  2;  // S1         // G8PTN: Corrected table
    if ((rssi >  2) and (rssi <=  3)) return  3;  // S2
    if ((rssi >  3) and (rssi <=  4)) return  4;  // S3
    if ((rssi >  4) and (rssi <= 10)) return  5;  // S4
    if ((rssi > 10) and (rssi <= 16)) return  6;  // S5
    if ((rssi > 16) and (rssi <= 22)) return  7;  // S6
    if ((rssi > 22) and (rssi <= 28)) return  8;  // S7
    if ((rssi > 28) and (rssi <= 34)) return  9;  // S8
    if ((rssi > 34) and (rssi <= 44)) return 10;  // S9
    if ((rssi > 44) and (rssi <= 54)) return 11;  // S9 +10
    if ((rssi > 54) and (rssi <= 64)) return 12;  // S9 +20
    if ((rssi > 64) and (rssi <= 74)) return 13;  // S9 +30
    if ((rssi > 74) and (rssi <= 84)) return 14;  // S9 +40
    if ((rssi > 84) and (rssi <= 94)) return 15;  // S9 +50
    if  (rssi > 94)                   return 16;  // S9 +60
    if  (rssi > 95)                   return 17;  //>S9 +60
  }
  else
  {
    //dBuV to S point conversion FM
    if  ((rssi <=  1)) return  1;                 // G8PTN: Corrected table
    if ((rssi >  1) and (rssi <=  2)) return  7;  // S6
    if ((rssi >  2) and (rssi <=  8)) return  8;  // S7
    if ((rssi >  8) and (rssi <= 14)) return  9;  // S8
    if ((rssi > 14) and (rssi <= 24)) return 10;  // S9
    if ((rssi > 24) and (rssi <= 34)) return 11;  // S9 +10
    if ((rssi > 34) and (rssi <= 44)) return 12;  // S9 +20
    if ((rssi > 44) and (rssi <= 54)) return 13;  // S9 +30
    if ((rssi > 54) and (rssi <= 64)) return 14;  // S9 +40
    if ((rssi > 64) and (rssi <= 74)) return 15;  // S9 +50
    if  (rssi > 74)                   return 16;  // S9 +60
    if  (rssi > 76)                   return 17;  //>S9 +60
    // newStereoPilot=si4735.getCurrentPilot();
  }
  return 1;
}

/***************************************************************************************
** Description:   In SSB mode tuning uses VFO and BFO
**                - Algorithm from ATS-20_EX Goshante firmware
***************************************************************************************/
// Tuning algorithm
void doFrequencyTuneSSB(bool fast = false) {
    int step = encoderCount == 1 ? getSteps(fast) : getSteps(fast) * -1;
    int newBFO = currentBFO + step;
    int redundant = 0;

    if (newBFO > BFOMax) {
        redundant = (newBFO / BFOMax) * BFOMax;
        currentFrequency += redundant / 1000;
        newBFO -= redundant;
    } else if (newBFO < -BFOMax) {
        redundant = ((abs(newBFO) / BFOMax) * BFOMax);
        currentFrequency -= redundant / 1000;
        newBFO += redundant;
    }

    currentBFO = newBFO;
    updateBFO();

    if (redundant != 0) {
        clampSSBBand();                                   // G8PTN: Added
        rx.setFrequency(currentFrequency);
        //agcSetFunc(); //Re-apply to remove noize        // G8PTN: Commented out
        currentFrequency = rx.getFrequency();
    }

    band[bandIdx].currentFreq = currentFrequency + (currentBFO / 1000);     // Update band table currentFreq

    if (clampSSBBand()) {
      // Debug
      #if DEBUG1_PRINT
      Serial.println("Info: clampSSBBand() >>> SSB Band Clamp !");
      #endif
    }
}

// Clamp SSB tuning to band limits
bool clampSSBBand()
{
    uint16_t freq = currentFrequency + (currentBFO / 1000);

    // Special case to cover SSB frequency negative!
    bool SsbFreqNeg = false;
    if (currentFrequency & 0x8000)
      SsbFreqNeg = true;

    // Priority to minimum check to cover SSB frequency negative
    bool upd = false;
    if (freq < band[bandIdx].minimumFreq || SsbFreqNeg)
    {
        currentFrequency = band[bandIdx].maximumFreq;
        upd = true;
    }
    else if (freq > band[bandIdx].maximumFreq)
    {
        currentFrequency = band[bandIdx].minimumFreq;
        upd = true;
    }

    if (upd)
    {
        band[bandIdx].currentFreq = currentFrequency;    // Update band table currentFreq
        rx.setFrequency(currentFrequency);
        currentBFO = 0;
        updateBFO();
        return true;
    }

    return false;
}


void updateBFO()
{
    // To move frequency forward, need to move the BFO backwards, so multiply by -1
    currentCAL = bandCAL[bandIdx];    // Select from table
    rx.setSSBBfo((currentBFO + currentCAL) * -1);

    // Debug
    #if DEBUG2_PRINT
    Serial.print("Info: updateBFO() >>> ");
    Serial.print("currentBFO = ");
    Serial.print(currentBFO);
    Serial.print(", currentCAL = ");
    Serial.print(currentCAL);
    Serial.print(", rx.setSSBbfo() = ");
    Serial.println((currentBFO + currentCAL) * -1);
    #endif
}

void buttonCheck() {
  // G8PTN: Added
  // Push button detection
  // Only execute every 10 ms
  if ((millis() - pb1_time) > 10) {
    pb1_time = millis();
    pb1_current = digitalRead(ENCODER_PUSH_BUTTON);        // Read pin value
    if (pb1_last != pb1_current) {                         // Start debounce timer
      pb1_edge_time = millis();
      pb1_last = pb1_current;
    }

    if ((millis() - pb1_edge_time) > CLICK_TIME) {         // Debounced
      if (pb1_stable == HIGH && pb1_last == LOW) {         // button is pressed
        // Debug
        #if DEBUG2_PRINT
        Serial.println("Info: button_check() >>> Button Pressed");
        #endif
        pb1_pressed_time = pb1_edge_time;
        pb1_short_pressed_time = pb1_long_pressed_time = 0;
        pb1_stable = pb1_last;
        pb1_pressed = true;                                // Set flags
        pb1_short_pressed = false;
        pb1_long_pressed = false;
        pb1_released = false;
        pb1_short_released = false;
        pb1_long_released = false;
      } else if (pb1_stable == LOW && pb1_last == LOW) {   // button is still pressed
        long pb1_press_duration = millis() - pb1_pressed_time;
        if (pb1_press_duration > SHORT_PRESS_TIME && (pb1_short_pressed_time - pb1_pressed_time) != SHORT_PRESS_TIME) {
          pb1_short_pressed = true;
          pb1_short_pressed_time = pb1_pressed_time + SHORT_PRESS_TIME;
          #if DEBUG2_PRINT
          Serial.println("Info: button_check() >>> Short Press triggered");
          #endif
        }
        if (pb1_press_duration > LONG_PRESS_TIME && (pb1_long_pressed_time - pb1_pressed_time) != LONG_PRESS_TIME) {
          pb1_short_pressed = false;
          pb1_long_pressed = true;
          pb1_long_pressed_time = pb1_pressed_time + LONG_PRESS_TIME;
          #if DEBUG2_PRINT
          Serial.println("Info: button_check() >>> Long Press triggered");
          #endif
        }
      } else if (pb1_stable == LOW && pb1_last == HIGH) {  // button is released
        // Debug
        #if DEBUG2_PRINT
        Serial.println("Info: button_check() >>> Button Released");
        #endif
        pb1_released_time = pb1_edge_time;
        pb1_stable = pb1_last;
        pb1_released = true;
        pb1_pressed = pb1_short_pressed = pb1_long_pressed = false;
        long pb1_press_duration = pb1_released_time - pb1_pressed_time;
        if (pb1_press_duration > LONG_PRESS_TIME) {
          pb1_short_released = false;
          pb1_long_released = true;
          #if DEBUG2_PRINT
          Serial.println("Info: button_check() >>> Long Release triggered");
          #endif
        } else if (pb1_press_duration > SHORT_PRESS_TIME) {
          pb1_short_released = true;
          pb1_long_released = false;
          #if DEBUG2_PRINT
          Serial.println("Info: button_check() >>> Short Release triggered");
          #endif
        }
      }
    }
  }
}

void clock_time()
{
  if ((micros() - clock_timer) >= 1000000) {
    clock_timer = micros();
    time_seconds++;
    if (time_seconds >= 60) {
      time_seconds = 0;
      time_minutes ++;

      if (time_minutes >= 60) {
        time_minutes = 0;
        time_hours++;

        if (time_hours >= 24) {
          time_hours = 0;
        }
      }
    }

    // Format for display HH:MM (24 hour format)
    sprintf(time_disp, "%2.2d:%2.2d", time_hours, time_minutes);
  }
}

void toggleRemoteLog() {
  g_remote_log = !g_remote_log;
}

void displayOff() {
  display_on = false;
  ledcWrite(PIN_LCD_BL, 0);
  tft.writecommand(ST7789_DISPOFF);
  tft.writecommand(ST7789_SLPIN);
  delay(120);
}

void displayOn() {
  display_on = true;
  tft.writecommand(ST7789_SLPOUT);
  delay(120);
  tft.writecommand(ST7789_DISPON);
  ledcWrite(PIN_LCD_BL, currentBrt);
  drawScreen(currentCmd);
}

void captureScreen() {
  uint16_t width = spr.width(), height = spr.height();
  char sb[9];
  Serial.println("");
  // 14 bytes of BMP header
  Serial.print("424d"); // BM
  sprintf(sb, "%08x", (unsigned int)htonl(14 + 40 + 12 + width * height * 2)); // Image size
  Serial.print(sb);
  Serial.print("00000000");
  sprintf(sb, "%08x", (unsigned int)htonl(14 + 40 + 12)); // Offset to image data
  Serial.print(sb);

  //Image header
  Serial.print("28000000"); // Header size
  sprintf(sb, "%08x", (unsigned int)htonl(width));
  Serial.print(sb);
  sprintf(sb, "%08x", (unsigned int)htonl(height));
  Serial.print(sb);
  Serial.print("01001000"); // 1 plane, 16 bpp
  Serial.print("00000000"); // Compression
  Serial.print("00000000"); // Compressed image size
  Serial.print("00000000"); // X res
  Serial.print("00000000"); // Y res
  Serial.print("00000000"); // Color map
  Serial.print("00000000"); // Colors
  Serial.print("00f80000"); // Red mask
  Serial.print("e0070000"); // Green mask
  Serial.println("1f000000"); // Blue mask

  // Image data
  for (int y=height-1; y>=0; y--) {
    for (int x=0; x<width; x++) {
      sprintf(sb, "%04x", htons(spr.readPixel(x, y)));
      Serial.print(sb);
    }
    Serial.println("");
  }
}

#if THEME_EDITOR
char readSerialWithEcho() {
  char key;
  while (Serial.available() == 0) {};
  key = Serial.read();
  Serial.print(key);
  return key;
}

uint8_t char2nibble(char key) {
  if (key < '0') return 0;
  if (key <= '9') return key - '0';
  if (key < 'A') return 0;
  if (key <= 'F') return key - 'A' + 10;
  if (key < 'a') return 0;
  if (key <= 'f') return key - 'a' + 10;
}

void setColorTheme() {
  Serial.print("Enter a string of hex colors (x0001x0002...): ");
  int i = 0;
  char key;
  while(true) {
    if (i >= (sizeof(ColorTheme) - offsetof(ColorTheme, bg))) {
      Serial.println(" Ok");
      break;
    }
    key = readSerialWithEcho();
    if (key != 'x') {
      Serial.println(" Err");
      break;
    }

    key = readSerialWithEcho();
    ((char *) &theme[themeIdx])[offsetof(ColorTheme, bg) + i + 1] = char2nibble(key) * 16;
    key = readSerialWithEcho();
    ((char *) &theme[themeIdx])[offsetof(ColorTheme, bg) + i + 1] |= char2nibble(key);

    key = readSerialWithEcho();
    ((char *) &theme[themeIdx])[offsetof(ColorTheme, bg) + i] = char2nibble(key) * 16;
    key = readSerialWithEcho();
    ((char *) &theme[themeIdx])[offsetof(ColorTheme, bg) + i] |= char2nibble(key);

    i += sizeof(uint16_t);
  }
  drawSprite();
}


void getColorTheme() {
  char sb[6];
  Serial.print("Color theme ");
  Serial.print(theme[themeIdx].name);
  Serial.print(": ");
  for (int i=0; i<(sizeof(ColorTheme) - offsetof(ColorTheme, bg)); i += sizeof(uint16_t)) {
    sprintf(sb, "x%02X%02X", ((char *) &theme[themeIdx])[offsetof(ColorTheme, bg) + i + 1], ((char *) &theme[themeIdx])[offsetof(ColorTheme, bg) + i]);
    Serial.print(sb);
  }
  Serial.println();
}
#endif

//
// Handle encoder PRESS + ROTATE
//
void doPressAndRotate(int8_t dir)
{
  if(isSSB())
  {
#if TUNE_HOLDOFF
    // Tuning timer to hold off (FM/AM) display updates
    tuning_flag = true;
    tuning_timer = millis();
#if DEBUG3_PRINT
    Serial.print("Info: TUNE_HOLDOFF SSB (Set) >>> ");
    Serial.print("tuning_flag = ");
    Serial.print(tuning_flag);
    Serial.print(", millis = ");
    Serial.println(millis());
#endif
#endif
    doFrequencyTuneSSB(true);
  }
  else
  {
    seekDirection = dir>0? 1 : 0;
    // G8PTN: Flag is set by rotary encoder and cleared on seek entry
    seekStop = false;
    doSeek();
    // G8PTN: Added to ensure update of currentFreq in table for AM/FM
    band[bandIdx].currentFreq = currentFrequency;
  }
}

//
// Handle encoder ROTATE
//
void doRotate(int8_t dir)
{
  // G8PTN: The manual BFO adjusment is not required with the
  // doFrequencyTuneSSB() method, but leave for debug
  if(bfoOn && isSSB())
  {
    currentBFO = (encoderCount == 1) ? (currentBFO + currentBFOStep) : (currentBFO - currentBFOStep);
    // G8PTN: Clamp range to +/- BFOMax (as per doFrequencyTuneSSB)
    if (currentBFO >  BFOMax) currentBFO =  BFOMax;
    if (currentBFO < -BFOMax) currentBFO = -BFOMax;
    band[bandIdx].currentFreq = currentFrequency + (currentBFO / 1000);     // G8PTN; Calculate frequency value to store in EEPROM
    updateBFO();
  }

  //
  // Side bar menus / settings
  //
  else if(doSideBar(currentCmd, encoderCount))
  {
    // Do nothing, everything is done
  }

  //
  // SSB tuning
  //
  else if(isSSB())
  {
#if TUNE_HOLDOFF
    // Tuning timer to hold off (SSB) display updates
    tuning_flag = true;
    tuning_timer = millis();
#if DEBUG3_PRINT
    Serial.print("Info: TUNE_HOLDOFF SSB (Set) >>> ");
    Serial.print("tuning_flag = ");
    Serial.print(tuning_flag);
    Serial.print(", millis = ");
    Serial.println(millis());
#endif
#endif
    doFrequencyTuneSSB();
#if DEBUG1_PRINT
    Serial.print("Info: SSB >>> ");
    Serial.print("currentFrequency = ");
    Serial.print(currentFrequency);
    Serial.print(", currentBFO = ");
    Serial.print(currentBFO);
    Serial.print(", rx.setSSBbfo() = ");
    Serial.println((currentBFO + currentCAL) * -1);
#endif
  }

  //
  // Normal tuning
  //
  else
  {
#if TUNE_HOLDOFF
    // Tuning timer to hold off (FM/AM) display updates
    tuning_flag = true;
    tuning_timer = millis();
#if DEBUG3_PRINT
    Serial.print("Info: TUNE_HOLDOFF FM/AM (Set) >>> ");
    Serial.print("tuning_flag = ");
    Serial.print(tuning_flag);
    Serial.print(", millis = ");
    Serial.println(millis());
#endif
#endif

    // G8PTN: Used in place of rx.frequencyUp() and rx.frequencyDown()
    uint16_t step = getCurrentStep()->step;
    uint16_t stepAdjust = currentFrequency % step;
    step = !stepAdjust? step : dir>0? step - stepAdjust : stepAdjust;
    currentFrequency += step * dir;
    
    // Check band limits
    uint16_t bMin = band[bandIdx].minimumFreq;
    uint16_t bMax = band[bandIdx].maximumFreq;
    currentFrequency =
      (currentMode==AM) && (currentFrequency&0x8000)? bMax // Negative in AM
    : currentFrequency < bMin? bMax                        // Lower bound
    : currentFrequency > bMax? bMin                        // Upper bound
    : currentFrequency;
   
    // Set new frequency
    rx.setFrequency(currentFrequency);
   
    // Clear FM RDS information
    if(currentMode==FM) clearStationName();
   
    // Check current CB channel
    if(isCB()) checkCbChannel();
   
    // G8PTN: Added to ensure update of currentFreq in table for AM/FM
    band[bandIdx].currentFreq = currentFrequency = rx.getFrequency();
     
#if DEBUG1_PRINT
    // Debug
    Serial.print("Info: AM/FM >>> currentFrequency = ");
    Serial.print(currentFrequency);
    Serial.print(", currentBFO = ");
    Serial.println(currentBFO);                              // Print to check the currentBFO value
    //Serial.print(", rx.setSSBbfo() = ");                   // rx.setSSBbfo() will not have been written
    //Serial.println((currentBFO + currentCAL) * -1);        // rx.setSSBbfo() will not have been written
#endif
  }
}

void loop()
{
  // Block encoder rotation when display is off
  if(encoderCount && !display_on) encoderCount = 0;

  // If encoder has been rotated...
  if(encoderCount)
  {
    // If encoder has been rotated AND pressed...
    if(pb1_pressed && !isModalMode(currentCmd))
    {
      doPressAndRotate(encoderCount);
      seekModePress = true;
    }
    else
    {
      doRotate(encoderCount);
    }

    // Clear encoder rotation
    encoderCount = 0;
    resetEepromDelay();
    elapsedSleep = elapsedCommand = millis();
  }

  // Encoder released after LONG PRESS: TOGGLE DISPLAY
  else if(pb1_long_pressed && !seekModePress)
  {
    pb1_long_pressed = pb1_short_pressed = pb1_pressed = false;

    if(display_on) displayOff(); else displayOn();
    elapsedSleep = millis();
  }

  // Encoder released after SHORT PRESS: CHANGE VOLUME
  else if(pb1_short_released && display_on && !seekModePress)
  {
    pb1_released = pb1_short_released = pb1_long_released = false;

    if(muted)
    {
      rx.setVolume(mute_vol_val);
      muted = false;
    }

    clickVolume();

    // Wait a little more for the button release
    delay(MIN_ELAPSED_TIME);
    elapsedSleep = elapsedCommand = millis();
  }

  // ???: SELECT MENU ITEM
  else if(pb1_released && !pb1_long_released && !seekModePress)
  {
    pb1_released = pb1_short_released = pb1_long_released = false;

    if(!display_on)
    {
      if(currentSleep) displayOn();
    }
    else if(clickSideBar(currentCmd))
    {
      // Do nothing, command handled
    }
    else if(isModalMode(currentCmd))
    {
      disableCommands();
      drawCommandStatus("VFO ");
      drawScreen(currentCmd);
    }
    else if(bfoOn)
    {
      bfoOn = false;
      drawScreen(currentCmd);
    }
    else
    {
      // Activate menu
      currentCmd = CMD_MENU;
      drawScreen(currentCmd);
    }

    // Wait a little more for the button release
    delay(MIN_ELAPSED_TIME);
    elapsedSleep = elapsedCommand = millis();
  }

  // Display sleep timeout
  if(currentSleep && display_on && ((millis() - elapsedSleep) > currentSleep * 1000))
    displayOff();

  // Show RSSI status only if this condition has changed
  if((millis() - elapsedRSSI) > MIN_ELAPSED_RSSI_TIME * 6)
  {
#if DEBUG3_PRINT
    Serial.println("Info: loop() >>> Checking signal information");
#endif

    rx.getCurrentReceivedSignalQuality();
    snr = rx.getCurrentSNR();
    int aux = rx.getCurrentRSSI();

#if DEBUG3_PRINT
    Serial.print("Info: loop() >>> RSSI = ");
    Serial.println(rssi);
#endif

    // G8PTN: Based on 1.2s update, always allow S-Meter
    if(rssi!=aux)
    {
#if DEBUG3_PRINT
      Serial.println("Info: loop() >>> RSI diff detected");
#endif
      rssi = aux;
      showRSSI();
    }

    elapsedRSSI = millis();
  }

  // Disable commands control
  if((millis() - elapsedCommand) > ELAPSED_COMMAND)
  {
    if(isSSB())
    {
      bfoOn = false;
      // showBFO();
      disableCommands();
      drawScreen(currentCmd);
    }
    else if(isModalMode(currentCmd))
    {
      disableCommands();
      drawScreen(currentCmd);
    }

    elapsedCommand = millis();
  }

  if((millis() - lastRDSCheck) > RDS_CHECK_TIME)
  {
    if((currentMode == FM) && (snr >= 12)) checkRds();
    lastRDSCheck = millis();
  }

  // Save the current frequency only if it has changed
  if(itIsTimeToSave && ((millis() - storeTime) > STORE_TIME))
  {
    saveAllReceiverInformation();
    storeTime = millis();
    itIsTimeToSave = false;
  }

  // Check for button activity
  buttonCheck();
  if(!pb1_pressed && seekModePress)
  {
    seekModePress = false;
    pb1_released = pb1_short_released = pb1_long_released = false;
  }

  // Periodically refresh the main screen
  // This covers the case where there is nothing else triggering a refresh
  if((millis() - background_timer) > BACKGROUND_REFRESH_TIME)
  {
    background_timer = millis();
    if(!isModalMode(currentCmd)) drawScreen(currentCmd);
  }

#if TUNE_HOLDOFF
  // Check if tuning flag is set
  if(tuning_flag && ((millis() - tuning_timer) > TUNE_HOLDOFF_TIME))
  {
    tuning_flag = false;
    showFrequency();
#if DEBUG3_PRINT
    Serial.print("Info: TUNE_HOLDOFF (Reset) >>> ");
    Serial.print("tuning_flag = ");
    Serial.print(tuning_flag);
    Serial.print(", millis = ");
    Serial.println(millis());
#endif
  }
#endif

  // Run clock
  clock_time();

#if USE_REMOTE
  // Periodically print status to serial
  if(millis() - g_remote_timer >= 500 && g_remote_log)
  {
    // Mark time and increment diagnostic sequence number
    g_remote_timer = millis();
    g_remote_seqnum++;
    // Show status
    remotePrintStatus();
  }

  // Receive and execute serial command
  if(Serial.available()>0) remoteDoCommand(Serial.read());
#endif

  // Add a small default delay in the main loop
  delay(5);
}
