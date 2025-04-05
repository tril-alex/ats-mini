#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <TFT_eSPI.h>
#include <SI4735.h>

#define USE_REMOTE    1  // Enable serial port control and monitoring
#define TUNE_HOLDOFF  1  // Hold off display updates while tuning
#define BFO_MENU_EN   0  // Add BFO menu option for debugging
#define DEBUG1_PRINT  0  // Highest level - Primary information
#define DEBUG2_PRINT  0  //               - Function call results
#define DEBUG3_PRINT  0  //               - Misc
#define DEBUG4_PRINT  0  // Lowest level  - EEPROM

// Modes
#define FM            0
#define LSB           1
#define USB           2
#define AM            3

// Band Types
#define FM_BAND_TYPE  0
#define MW_BAND_TYPE  1
#define SW_BAND_TYPE  2
#define LW_BAND_TYPE  3

// Bands
#define BAND_VHF      0
#define BAND_MW1      1
#define BAND_MW2      2
#define BAND_MW3      3
#define BAND_80M      4
#define BAND_SW1      5
#define BAND_SW2      6
#define BAND_40M      7
#define BAND_SW3      8
#define BAND_SW4      9
#define BAND_SW5      10
#define BAND_SW6      11
#define BAND_20M      12
#define BAND_SW7      13
#define BAND_SW8      14
#define BAND_15M      15
#define BAND_SW9      16
#define BAND_CB       17
#define BAND_10M      18
#define BAND_ALL      19

// Commands
#define CMD_NONE      0x0000
#define CMD_BAND      0x0100 //-MENU MODE starts here
#define CMD_VOLUME    0x0200 // |
#define CMD_AGC       0x0300 // |
#define CMD_BANDWIDTH 0x0400 // |
#define CMD_STEP      0x0500 // |
#define CMD_MODE      0x0600 // |
#define CMD_MENU      0x0700 // |
#define CMD_SOFTMUTEMAXATT 0x0800
#define CMD_CAL       0x0900 // |
#define CMD_AVC       0x0A00 //-+
#define CMD_SETTINGS  0x0B00 //-SETTINGS MODE starts here
#define CMD_BRT       0x0C00 // |
#define CMD_SLEEP     0x0D00 // |
#define CMD_THEME     0x0E00 //-+
#define CMD_ABOUT     0x0F00

// SI4732/5 PINs
#define PIN_POWER_ON  15            // GPIO15   External LDO regulator enable (1 = Enable)
#define RESET_PIN     16            // GPIO16   SI4732/5 Reset
#define ESP32_I2C_SCL 17            // GPIO17   SI4732/5 Clock
#define ESP32_I2C_SDA 18            // GPIO18   SI4732/5 Data
#define AUDIO_MUTE     3            // GPIO3    Hardware L/R mute, controlled via SI4735 code (1 = Mute)
#define PIN_AMP_EN    10            // GPIO10   Hardware Audio Amplifer enable (1 = Enable)

// Display PINs
#define PIN_LCD_BL    38            // GPIO38   LCD backlight (PWM brightness control)
// All other pins are defined by the TFT_eSPI library

// Rotary Enconder PINs
#define ENCODER_PIN_A  2            // GPIO02
#define ENCODER_PIN_B  1            // GPIO01
#define ENCODER_PUSH_BUTTON 21      // GPIO21

// Compute number of items in an array
#define ITEM_COUNT(array) (sizeof(array) / sizeof((array)[0]))
#define LAST_ITEM(array)  (ITEM_COUNT(array) - 1)

//
// Data Types
//

typedef struct
{
  uint8_t idx;      // SI473X device bandwidth index
  const char *desc; // Bandwidth description
} Bandwidth;

typedef struct
{
  int step;         // Step
  const char *desc; // Step description
} Step;

typedef struct
{
  const char *bandName;   // Band description
  uint8_t bandType;       // Band type (FM, MW, or SW)
  uint8_t bandMode;       // Band mode (FM, AM, LSB, or SSB)
  uint16_t minimumFreq;   // Minimum frequency of the band
  uint16_t maximumFreq;   // maximum frequency of the band
  uint16_t currentFreq;   // Default frequency or current frequency
  int8_t currentStepIdx;  // Index of stepAM[]: defeult frequency step (see stepAM[])
  int8_t bandwidthIdx;    // Index of the table bandwidthFM, bandwidthAM or bandwidthSSB;
} Band;

//
// Global Variables
//

extern SI4735 rx;
extern TFT_eSprite spr;
extern Band band[];
extern const char *bandModeDesc[];

extern bool display_on;
extern bool bfoOn;
extern bool ssbLoaded;
extern bool tuning_flag;
extern bool eeprom_wr_flag;
extern bool muted;

extern uint8_t mute_vol_val;
extern uint16_t currentFrequency;
extern const uint16_t currentBFOStep;
extern int16_t currentBFO;
extern uint8_t currentMode;
extern uint16_t currentCmd;
extern int16_t currentCAL;
extern int8_t currentAVC;
extern uint16_t currentBrt;
extern uint16_t currentSleep;

extern int8_t agcIdx;
extern int8_t agcNdx;
extern int8_t softMuteMaxAttIdx;
extern uint8_t disableAgc;

extern int bandIdx;
extern int8_t bwIdxAM;
extern int8_t bwIdxFM;
extern int8_t bwIdxSSB;
extern const int CALMax;

extern int16_t bandCAL[];

static inline bool isSSB() { return(currentMode>FM && currentMode<AM); }
static inline bool isCB()  { return(bandIdx==BAND_CB); }

// These are menu commands
static inline bool isMenuMode(uint16_t cmd)
{
  return((cmd>=CMD_BAND) && (cmd<CMD_SETTINGS));
}

// These are settings
static inline bool isSettingsMode(uint16_t cmd)
{
  return((cmd>=CMD_SETTINGS) && (cmd<CMD_ABOUT));
}

// These are modal commands
static inline bool isModalMode(uint16_t cmd)
{
  return(isMenuMode(cmd) || isSettingsMode(cmd) || (cmd==CMD_ABOUT));
}

// SSB functions
void loadSSB();

const char *get_fw_ver();
void useBand();
uint8_t getStrength();
void updateBFO();

// Draw.c
void drawLoadingSSB();
void drawCommandStatus(const char *status);
void drawScreen(uint16_t cmd);

// Battery.c
float batteryMonitor();
void drawBattery(int x, int y);

// Menu.c
void drawSideBar(uint16_t cmd, int x, int y, int sx);
bool doSideBar(uint16_t cmd, int dir);
bool clickSideBar(uint16_t cmd);
void clickVolume();
void doSoftMute(int dir);
void doAgc(int dir);
void doAvc(int dir);
void selectBand(uint8_t idx);
int getTotalBands();
const Step *getCurrentStep();
int getSteps(bool fast = false);

// Station.c
const char *getStationName();
void clearStationName();
void checkRds();
void checkCbChannel();

#if USE_REMOTE
// Remote.c
void remotePrintStatus();
bool remoteDoCommand(char key);
#endif // USE_REMOTE

#endif // COMMON_H
