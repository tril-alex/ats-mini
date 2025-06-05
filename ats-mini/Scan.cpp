#include "Common.h"
#include "Utils.h"
#include "Menu.h"

#define SCAN_TIME   500
#define SCAN_POINTS 50
#define SCAN_RSSI_THRESHOLD 20

typedef struct
{
  uint8_t rssi;
  uint8_t snr;
} ScanPoint;

ScanPoint scanData[SCAN_POINTS];

uint32_t scanTime = millis();
uint16_t scanStartFreq = 0;
uint16_t scanStep    = 0;
uint16_t scanCount   = 0;
uint8_t  scanMinRSSI = 255;
uint8_t  scanMaxRSSI = 0;
uint8_t  scanMinSNR  = 255;
uint8_t  scanMaxSNR  = 0;
uint8_t  scanStatus  = 0;

static inline uint8_t min(uint8_t a, uint8_t b) { return(a<b? a:b); }
static inline uint8_t max(uint8_t a, uint8_t b) { return(a>b? a:b); }

float scanGetRSSI(uint16_t freq)
{
  // Input frequency must be in range of existing data
  if((scanStatus!=2) || (freq<scanStartFreq) || (freq>=scanStartFreq+scanStep*scanCount))
    return(0.0);

  uint8_t result = scanData[(freq - scanStartFreq) / scanStep].rssi;
  return((result - scanMinRSSI) / (float)(scanMaxRSSI - scanMinRSSI + 1));
}

float scanGetSNR(uint16_t freq)
{
  // Input frequency must be in range of existing data
  if((scanStatus!=2) || (freq<scanStartFreq) || (freq>=scanStartFreq+scanStep*scanCount))
    return(0.0);

  uint8_t result = scanData[(freq - scanStartFreq) / scanStep].snr;
  return((result - scanMinSNR) / (float)(scanMaxSNR - scanMinSNR + 1));
}

bool scanOn(uint8_t x)
{
  if((x==1) && (scanStatus!=1))
  {
    scanStep    = getCurrentStep()->step;
    scanCount   = 0;
    scanMinRSSI = 255;
    scanMaxRSSI = 0;
    scanMinSNR  = 255;
    scanMaxSNR  = 0;
    scanStatus  = 1;
    scanTime    = millis();

    scanStartFreq = currentFrequency - scanStep * (SCAN_POINTS / 2);

    if(!isFreqInBand(getCurrentBand(), scanStartFreq))
      scanStartFreq = getCurrentBand()->minimumFreq;

    memset(scanData, 0, sizeof(scanData));
  }
  else if((x==0) && (scanStatus==1))
  {
    rx.setFrequency(currentFrequency);
    scanStatus = 0;
  }

  // Return current scanning status
  return(scanStatus==1);
}

void scanTickTime()
{
  // Scan must be on
  if((scanStatus!=1) || (scanCount>=SCAN_POINTS) || (millis()-scanTime<SCAN_TIME))
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
    scanStatus = 2;
  else
    rx.setFrequency(freq);

  // Save last scan time
  scanTime = millis();
}

#if 0
void drawScanScreen()
{
  char freq_str[16];
  spr.fillSprite(TH.bg);

  int graphX0 = 10, graphY0 = 130, graphH = 120, graphW = 300;
  int freqLabelY = graphY0 + 12;
  int valueLabelX = 235;

  float rssiK = (scanMaxRSSI > scanMinRSSI) ? (float)graphH / (scanMaxRSSI - scanMinRSSI) : 1.0;
  float snrK  = (scanMaxSNR > scanMinSNR)   ? (float)graphH / (scanMaxSNR - scanMinSNR)   : 1.0;
  int stepX = (scanCount > 1) ? graphW / (scanCount - 1) : graphW;

  for(int i = 1; i < scanCount; ++i)
  {
    int x1 = graphX0 + (i-1)*stepX;
    int x2 = graphX0 + i*stepX;
    int y1r = graphY0 - (scanData[i-1].rssi - scanMinRSSI) * rssiK;
    int y2r = graphY0 - (scanData[i].rssi - scanMinRSSI) * rssiK;
    int y1s = graphY0 - (scanData[i-1].snr - scanMinSNR) * snrK;
    int y2s = graphY0 - (scanData[i].snr - scanMinSNR) * snrK;
    spr.drawLine(x1, y1r, x2, y2r, TFT_BLUE);
    spr.drawLine(x1, y1s, x2, y2s, TFT_GREEN);
  }

  if(scanStatus == 2)
  {
    int x = graphX0 + scanSelectedIdx * stepX;
    spr.fillTriangle(x-6, 2, x+6, 2, x, 12, TFT_RED);
    // line
    int y_graph = graphY0 - (scanData[scanSelectedIdx].rssi - scanMinRSSI) * rssiK;
    spr.drawLine(x, 2+1, x, y_graph, TFT_RED);
  }

  // RSSI
  //spr.setTextDatum(TR_DATUM);
  //spr.setTextColor(TFT_BLUE, TH.bg);
  //spr.drawString(String(scanMinRSSI), valueLabelX, graphY0, 2);
  //spr.drawString(String(scanMaxRSSI), valueLabelX, graphY0-graphH, 2);

  // print RSSI & SNR
  spr.setTextDatum(TL_DATUM);
  spr.setTextColor(TFT_BLUE, TH.bg);
  spr.drawString("RSSI", 256, 10, 2);
  spr.setTextColor(TFT_GREEN, TH.bg);
  spr.drawString("SNR", 256, 28, 2);

  // print freq
  spr.setTextColor(TFT_WHITE);
  if(currentMode == FM)
    sprintf(freq_str, "%3d.%02d", scanData[0].freq / 100, scanData[0].freq % 100);
  else
    sprintf(freq_str, "%2d.%03d", scanData[0].freq / 1000, scanData[0].freq % 1000);
  spr.setTextDatum(TL_DATUM);
  spr.drawString(freq_str, graphX0, freqLabelY, 2);
  
  if(currentMode == FM)
    sprintf(freq_str, "%3d.%02d", scanData[scanCount-1].freq / 100, scanData[scanCount-1].freq % 100);
  else
    sprintf(freq_str, "%2d.%03d", scanData[scanCount-1].freq / 1000, scanData[scanCount-1].freq % 1000);
  spr.setTextDatum(TR_DATUM);
  spr.drawString(freq_str, graphX0 + graphW, freqLabelY, 2);
  
  if(currentMode == FM)
    sprintf(freq_str, "%3d.%02d", scanData[scanSelectedIdx].freq / 100, scanData[scanSelectedIdx].freq % 100);
  else
    sprintf(freq_str, "%2d.%03d", scanData[scanSelectedIdx].freq / 1000, scanData[scanSelectedIdx].freq % 1000);
  spr.setTextDatum(TC_DATUM);
  spr.drawString(freq_str, graphX0 + graphW/2, freqLabelY, 4);
  spr.pushSprite(0, 0);
}
#endif
