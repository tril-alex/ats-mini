#ifndef UTILS_H
#define UTILS_H

#include "Common.h"

// SSB patch functions
void loadSSB(uint8_t bandwidth, bool draw = true);
void unloadSSB();

// Get firmware version
const char *getVersion();

// Convert RSSI to signal strength
int getStrength(int rssi);

// Set, reset, toggle, or query switches
bool sleepOn(int x = 2);
bool muteOn(int x = 2);

// Wall clock functions
const char *clockGet();
bool clockGetHM(uint8_t *hours, uint8_t *minutes);
bool clockSet(uint8_t hours, uint8_t minutes, uint8_t seconds = 0);
void clockReset();
bool clockTickTime();
void clockRefreshTime();

// Check if given memory entry belongs to a band
bool isMemoryInBand(const Band *band, const Memory *memory);

#endif // UTILS_H
