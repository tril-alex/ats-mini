#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <TFT_eSPI.h>
#include <SI4735.h>

#define APP_ID        67   // EEPROM ID
#define APP_VERSION   200  // EEPROM VERSION

#define USE_REMOTE    1  // Enable serial port control and monitoring
#define TUNE_HOLDOFF  1  // Hold off display updates while tuning
#define BFO_MENU_EN   0  // Add BFO menu option for debugging
#define THEME_EDITOR  0  // Enable setting and printing of theme colors
#define DEBUG1_PRINT  0  // Highest level - Primary information
#define DEBUG2_PRINT  0  //               - Function call results
#define DEBUG3_PRINT  0  //               - Misc
#define DEBUG4_PRINT  0  // Lowest level  - EEPROM

// Modes
#define FM            0
#define LSB           1
#define USB           2
#define AM            3

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
// Global Variables
//

extern SI4735 rx;
extern TFT_eSprite spr;
extern TFT_eSPI tft;

extern bool display_on;
extern bool bfoOn;
extern bool ssbLoaded;
extern bool tuning_flag;
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
extern uint8_t AmTotalSteps;

extern int8_t FmAgcIdx;
extern int8_t AmAgcIdx;
extern int8_t SsbAgcIdx;
extern int8_t AmAvcIdx;
extern int8_t SsbAvcIdx;
extern int8_t AmSoftMuteIdx;
extern int8_t SsbSoftMuteIdx;

extern int8_t agcIdx;
extern int8_t agcNdx;
extern int8_t softMuteMaxAttIdx;
extern uint8_t disableAgc;

extern const int CALMax;

static inline bool isSSB() { return(currentMode>FM && currentMode<AM); }

void loadSSB(uint8_t bandwidth);
const char *getVersion();
void useBand(uint8_t bandIdx);
uint8_t getStrength();
void updateBFO();
void displayOff();
void displayOn();

// Draw.c
void drawLoadingSSB();
void drawCommandStatus(const char *status);
void drawScreen();

// Battery.c
float batteryMonitor();
void drawBattery(int x, int y);

// Station.c
const char *getStationName();
void clearStationName();
void checkRds();
void checkCbChannel();

#if USE_REMOTE
// Remote.c
void remoteTickTime(uint32_t millis);
bool remoteDoCommand(char key);
#endif // USE_REMOTE

#endif // COMMON_H
