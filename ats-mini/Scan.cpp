#include "Common.h"
#include "Utils.h"
#include "Menu.h"

#define SCAN_TIME   100
#define SCAN_POINTS 200

#define SCAN_OFF    0
#define SCAN_RUN    1
#define SCAN_DONE   2

static struct
{
  uint8_t rssi;
  uint8_t snr;
} scanData[SCAN_POINTS];

static uint32_t scanTime = millis();
static uint8_t  scanStatus = SCAN_OFF;

static uint16_t scanStartFreq;
static uint16_t scanStep;
static uint16_t scanCount;
static uint8_t  scanMinRSSI;
static uint8_t  scanMaxRSSI;
static uint8_t  scanMinSNR;
static uint8_t  scanMaxSNR;

static inline uint8_t min(uint8_t a, uint8_t b) { return(a<b? a:b); }
static inline uint8_t max(uint8_t a, uint8_t b) { return(a>b? a:b); }

float scanGetRSSI(uint16_t freq)
{
  // Input frequency must be in range of existing data
  if((scanStatus!=SCAN_DONE) || (freq<scanStartFreq) || (freq>=scanStartFreq+scanStep*scanCount))
    return(0.0);

  uint8_t result = scanData[(freq - scanStartFreq) / scanStep].rssi;
  return((result - scanMinRSSI) / (float)(scanMaxRSSI - scanMinRSSI + 1));
}

float scanGetSNR(uint16_t freq)
{
  // Input frequency must be in range of existing data
  if((scanStatus!=SCAN_DONE) || (freq<scanStartFreq) || (freq>=scanStartFreq+scanStep*scanCount))
    return(0.0);

  uint8_t result = scanData[(freq - scanStartFreq) / scanStep].snr;
  return((result - scanMinSNR) / (float)(scanMaxSNR - scanMinSNR + 1));
}

bool scanOn(uint8_t x)
{
  if((x==1) && (scanStatus!=SCAN_RUN))
  {
    scanStep    = getCurrentStep()->step;
    scanCount   = 0;
    scanMinRSSI = 255;
    scanMaxRSSI = 0;
    scanMinSNR  = 255;
    scanMaxSNR  = 0;
    scanStatus  = SCAN_RUN;
    scanTime    = millis();

    scanStartFreq = currentFrequency - scanStep * (SCAN_POINTS / 2);

    if(!isFreqInBand(getCurrentBand(), scanStartFreq))
      scanStartFreq = getCurrentBand()->minimumFreq;

    memset(scanData, 0, sizeof(scanData));
  }
  else if((x==0) && (scanStatus==SCAN_RUN))
  {
    rx.setFrequency(currentFrequency);
    scanStatus = scanCount? SCAN_DONE : SCAN_OFF;
  }

  // Return current scanning status
  return(scanStatus==SCAN_RUN);
}

void scanTickTime()
{
  // Scan must be on
  if((scanStatus!=SCAN_RUN) || (scanCount>=SCAN_POINTS) || (millis()-scanTime<SCAN_TIME))
    return;

  // This is our current frequency to scan
  uint16_t freq = scanStartFreq + scanStep * scanCount;

  // If frequency not yet set, set it and wait until next call to measure
  if(rx.getFrequency() != freq)
  {
    rx.setFrequency(freq);
    scanTime = millis();
    return;
  }

  // Measure RSSI/SNR values
  rx.getCurrentReceivedSignalQuality();
  scanData[scanCount].rssi = rx.getCurrentRSSI();
  scanData[scanCount].snr  = rx.getCurrentSNR();

  // Measure range of values
  scanMinRSSI = min(scanData[scanCount].rssi, scanMinRSSI);
  scanMaxRSSI = max(scanData[scanCount].rssi, scanMaxRSSI);
  scanMinSNR  = min(scanData[scanCount].snr, scanMinSNR);
  scanMaxSNR  = max(scanData[scanCount].snr, scanMaxSNR);

  // Next frequency to scan
  freq += scanStep;

  // Set next frequency to scan or expire scan
  if((++scanCount >= SCAN_POINTS) || !isFreqInBand(getCurrentBand(), freq))
    scanStatus = SCAN_DONE;
  else
    rx.setFrequency(freq);

  // Save last scan time
  scanTime = millis();
}

//
// Run entire scan once
//
void scanRun()
{
  drawScanningBand();
  for(scanOn(true) ; scanOn() ; delay(SCAN_TIME)) scanTickTime();
}
