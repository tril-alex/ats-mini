#include "Common.h"
#include "Themes.h"
#include "Menu.h"

#if USE_REMOTE

// @@@ FIXME: These should not be force-exported!!!
extern volatile int encoderCount;
extern bool pb1_released;

static uint32_t remoteTimer = millis();
static uint8_t remoteSeqnum = 0;
static bool remoteLogOn = false;

//
// Capture current screen image to the remote
//
void remoteCaptureScreen()
{
  uint16_t width  = spr.width();
  uint16_t height = spr.height();
  char sb[9];

  // 14 bytes of BMP header
  Serial.println("");
  Serial.print("424d"); // BM
  // Image size
  sprintf(sb, "%08x", (unsigned int)htonl(14 + 40 + 12 + width * height * 2));
  Serial.print(sb);
  Serial.print("00000000");
  // Offset to image data
  sprintf(sb, "%08x", (unsigned int)htonl(14 + 40 + 12));
  Serial.print(sb);

  // Image header
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
  for(int y=height-1 ; y>=0 ; y--)
  {
    for(int x=0 ; x<width ; x++)
    {
      sprintf(sb, "%04x", htons(spr.readPixel(x, y)));
      Serial.print(sb);
    }

    Serial.println("");
  }
}

//
// Print current status to the remote
//
void remotePrintStatus()
{
  // Prepare information ready to be sent
  float remoteVoltage = batteryMonitor();
  int remoteVolume = rx.getVolume();

  // S-Meter conditional on compile option
  rx.getCurrentReceivedSignalQuality();
  uint8_t remoteRssi = rx.getCurrentRSSI();

  // Use rx.getFrequency to force read of capacitor value from SI4732/5
  rx.getFrequency();
  uint16_t tuningCapacitor = rx.getAntennaTuningCapacitor();

  // Remote serial
  Serial.print(APP_VERSION);                  // Firmware version
  Serial.print(",");

  Serial.print(currentFrequency);             // Frequency (KHz)
  Serial.print(",");
  Serial.print(currentBFO);                   // Frequency (Hz x 1000)
  Serial.print(",");

  Serial.print(getCurrentBand()->bandName);   // Band
  Serial.print(",");
  Serial.print(currentMode);                  // Mode
  Serial.print(",");
  Serial.print(getCurrentStep()->step);       // Step (FM/AM/SSB)
  Serial.print(",");
  Serial.print(bwIdxFM);                      // Bandwidth (FM)
  Serial.print(",");
  Serial.print(bwIdxAM);                      // Bandwidth (AM)
  Serial.print(",");
  Serial.print(bwIdxSSB);                     // Bandwidth (SSB)
  Serial.print(",");
  Serial.print(agcIdx);                       // AGC/ATTN (FM/AM/SSB)
  Serial.print(",");

  Serial.print(remoteVolume);                 // Volume
  Serial.print(",");
  Serial.print(remoteRssi);                   // RSSI
  Serial.print(",");
  Serial.print(tuningCapacitor);              // Tuning cappacitor
  Serial.print(",");
  Serial.print(remoteVoltage);                // V_BAT/2 (ADC average value)
  Serial.print(",");
  Serial.println(remoteSeqnum);               // Sequence number
}

//
// Tick remote time, periodically printing status
//
void remoteTickTime(uint32_t millis)
{
  if(millis - remoteTimer >= 500 && remoteLogOn)
  {
    // Mark time and increment diagnostic sequence number
    remoteTimer = millis;
    remoteSeqnum++;
    // Show status
    remotePrintStatus();
  }
}

//
// Recognize and execute given remote command
//
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
      doBand(1);
      break;
    case 'b': // Band Down
      doBand(-1);
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
      displayOn(false);
      break;
    case 'o':
      displayOn(true);
      break;
    case 'C':
      remoteCaptureScreen();
      break;
    case 't':
      remoteLogOn = !remoteLogOn;
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