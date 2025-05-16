#ifndef STORAGE_H
#define STORAGE_H

void eepromTickTime();
void eepromRequestSave();
void eepromInvalidate();
bool eepromVerify();
void eepromSaveConfig();
void eepromLoadConfig();

void drawEepromIndicator(int x, int y);

bool diskInit();

#endif // STORAGE_H
