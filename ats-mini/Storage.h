#ifndef STORAGE_H
#define STORAGE_H

void eepromTickTime();
void eepromRequestSave();
void eepromInvalidate();
bool eepromVerify();
void eepromSaveConfig();
void eepromLoadConfig();

void drawEepromIndicator(int x, int y);

#endif // STORAGE_H
