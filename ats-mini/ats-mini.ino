// =================================
// INCLUDE FILES
// =================================

#include <Wire.h>
#include <TFT_eSPI.h>            // https://github.com/Xinyuan-LilyGO/T-Display-S3#quick-start
#include "EEPROM.h"
#include <SI4735.h>
#include "Rotary.h"              // Disabled half-step mode
#include "Common.h"
#include "Menu.h"
#include "Storage.h"
#include "Themes.h"
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

// =================================
// CONSTANTS AND VARIABLES
// =================================

// SI4732/5 patch
const uint16_t size_content = sizeof ssb_patch_content; // see patch_init.h

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

uint8_t currentMode = FM;

uint8_t rssi = 0;
uint8_t snr = 0;
uint8_t volume = DEFAULT_VOLUME;

// Devices class declarations
// G8PTN: Corrected mapping based on rotary library
Rotary encoder = Rotary(ENCODER_PIN_B, ENCODER_PIN_A);


TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);

SI4735 rx;

const char *getVersion()
{
  static char versionString[25] = "\0";

  if(!versionString[0])
    sprintf(versionString, "F/W: v%1.1d.%2.2d %s",
      APP_VERSION / 100,
      APP_VERSION % 100,
      __DATE__
    );

  return(versionString);
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

  // Press and hold Encoder button to force an EEPROM reset
  // Note: EEPROM reset is recommended after firmware updates
  if(digitalRead(ENCODER_PUSH_BUTTON)==LOW) eepromInvalidate();

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

  // If EEPROM contents are ok...
  if(eepromVerify())
  {
    // Load configuration from EEPROM
    eepromLoadConfig(); 
  }
  else
  {
    // Save default configuration to EEPROM
    eepromSaveConfig();
    // Set initial volume after EEPROM reset
    rx.setVolume(volume);
    // Set initial brightness after EEPROM reset
    ledcWrite(PIN_LCD_BL, currentBrt);
  }

  // ** SI4732 STARTUP **
  // Uses values from EEPROM (Last stored or defaults after EEPROM reset)
  useBand(bandIdx);

  // Draw display for the first time
  drawScreen();

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
    rx.setFM(band[bandIdx].minimumFreq, band[bandIdx].maximumFreq, band[bandIdx].currentFreq, getCurrentStep()->step);
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
        band[bandIdx].currentStepIdx >= AmTotalSteps ? 1 : getCurrentStep()->step
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
  drawScreen();
}

void loadSSB(uint8_t bandwidth)
{
  // You can try rx.setI2CFastModeCustom(700000); or greater value
  rx.setI2CFastModeCustom(400000);
  rx.loadPatch(ssb_patch_content, size_content, bandwidth);
  rx.setI2CFastModeCustom(100000);
  ssbLoaded = true;
}

/**
 *  This function is called by the seek function process.
 */
bool checkStopSeeking()
{
  // Returns true if the user rotates the encoder
  return(seekStop); 
}

/**
 *  This function is called by the seek function process.
 */
void showFrequencySeek(uint16_t freq)
{
  currentFrequency = freq;
  drawScreen();
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
    currentCAL = getCurrentBand()->bandCal;
    rx.setSSBBfo((currentBFO + currentCAL) * -1);

#if DEBUG2_PRINT
    // Debug
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

void displayOff()
{
  display_on = false;
  ledcWrite(PIN_LCD_BL, 0);
  tft.writecommand(ST7789_DISPOFF);
  tft.writecommand(ST7789_SLPIN);
  delay(120);
}

void displayOn()
{
  display_on = true;
  tft.writecommand(ST7789_SLPOUT);
  delay(120);
  tft.writecommand(ST7789_DISPON);
  ledcWrite(PIN_LCD_BL, currentBrt);
  drawScreen();
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
    eepromRequestSave();
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
      drawScreen();
    }
    else if(bfoOn)
    {
      bfoOn = false;
      drawScreen();
    }
    else
    {
      // Activate menu
      currentCmd = CMD_MENU;
      drawScreen();
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
      drawScreen();
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
      drawScreen();
    }
    else if(isModalMode(currentCmd))
    {
      disableCommands();
      drawScreen();
    }

    elapsedCommand = millis();
  }

  if((millis() - lastRDSCheck) > RDS_CHECK_TIME)
  {
    if((currentMode == FM) && (snr >= 12)) checkRds();
    lastRDSCheck = millis();
  }

  // Tick EEPROM time, saving changes if the occurred and there has
  // been no activity for a while
  eepromTickTime(millis());

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
    if(!isModalMode(currentCmd)) drawScreen();
  }

#if TUNE_HOLDOFF
  // Check if tuning flag is set
  if(tuning_flag && ((millis() - tuning_timer) > TUNE_HOLDOFF_TIME))
  {
    tuning_flag = false;
    drawScreen();
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
  remoteTickTime(millis());
  // Receive and execute serial command
  if(Serial.available()>0) remoteDoCommand(Serial.read());
#endif

  // Add a small default delay in the main loop
  delay(5);
}
