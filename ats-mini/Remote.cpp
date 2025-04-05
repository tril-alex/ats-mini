#include "Common.h"

#if USE_REMOTE

void remotePrintStatus()
{
  // Prepare information ready to be sent
  int remote_volume  = rx.getVolume();

  // S-Meter conditional on compile option
  rx.getCurrentReceivedSignalQuality();
  uint8_t remote_rssi = rx.getCurrentRSSI();

  // Use rx.getFrequency to force read of capacitor value from SI4732/5
  rx.getFrequency();
  uint16_t tuning_capacitor = rx.getAntennaTuningCapacitor();

  // Remote serial
  Serial.print(app_ver);                      // Firmware version
  Serial.print(",");

  Serial.print(currentFrequency);             // Frequency (KHz)
  Serial.print(",");
  Serial.print(currentBFO);                   // Frequency (Hz x 1000)
  Serial.print(",");

  Serial.print(band[bandIdx].bandName);      // Band
  Serial.print(",");
  Serial.print(currentMode);                  // Mode
  Serial.print(",");
  Serial.print(currentStepIdx);               // Step (FM/AM/SSB)
  Serial.print(",");
  Serial.print(bwIdxFM);                      // Bandwidth (FM)
  Serial.print(",");
  Serial.print(bwIdxAM);                      // Bandwidth (AM)
  Serial.print(",");
  Serial.print(bwIdxSSB);                     // Bandwidth (SSB)
  Serial.print(",");
  Serial.print(agcIdx);                       // AGC/ATTN (FM/AM/SSB)
  Serial.print(",");

  Serial.print(remote_volume);                // Volume
  Serial.print(",");
  Serial.print(remote_rssi);                  // RSSI
  Serial.print(",");
  Serial.print(tuning_capacitor);             // Tuning cappacitor
  Serial.print(",");
  Serial.print(adc_read_avr);                 // V_BAT/2 (ADC average value)
  Serial.print(",");
  Serial.println(g_remote_seqnum);            // Sequence number
}

bool remoteDoCommand(char key)
{
  switch(key)
  {
    case 'U': // Encoder Up
      encoderCount++;
      break;
    case 'D': // Encoder Down
      encoderCount--;
      break;
    case 'P': // Encoder Push Button
      pb1_released = true;
      break;
    case 'B': // Band Up
      setBand(1);
      break;
    case 'b': // Band Down
      setBand(-1);
      break;
    case 'M': // Mode Up
      doMode(1);
      break;
    case 'm': // Mode Down
      doMode(-1);
      break;
    case 'S': // Step Up
      doStep(1);
      break;
    case 's': // Step Down
      doStep(-1);
      break;
    case 'W': // Bandwidth Up
      doBandwidth(1);
      break;
    case 'w': // Bandwidth Down
      doBandwidth(-1);
      break;
    case 'A': // AGC/ATTN Up
      doAgc(1);
      break;
    case 'a': // AGC/ATTN Down
      doAgc(-1);
      break;
    case 'V': // Volume Up
      doVolume(1);
      break;
    case 'v': // Volume Down
      doVolume(-1);
      break;
    case 'L': // Backlight Up
      doBrt(1);
      break;
    case 'l': // Backlight Down
      doBrt(-1);
      break;
    case 'O':
      displayOff();
      break;
    case 'o':
      displayOn();
      break;
    case 'C':
      captureScreen();
      break;
    case 't':
      toggleRemoteLog();
      break;

#if THEME_EDITOR
    case '!':
      setColorTheme();
      break;
    case '@':
      getColorTheme();
      break;
#endif

    default:
      // Command not recognized
      return(false);
  }

  // Command recognized
  return(true);
}

#endif // USE_REMOTE