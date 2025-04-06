#include "Common.h"
#include "Themes.h"
#include "Storage.h"
#include "Menu.h"

// Display position control
#define menu_offset_x    0    // Menu horizontal offset
#define menu_offset_y   20    // Menu vertical offset
#define menu_delta_x    10    // Menu width delta
#define meter_offset_x   0    // Meter horizontal offset
#define meter_offset_y   0    // Meter vertical offset
#define save_offset_x   87    // EEPROM save icon horizontal offset
#define save_offset_y    0    // EEPROM save icon vertical offset
#define freq_offset_x  250    // Frequency horizontal offset
#define freq_offset_y   65    // Frequency vertical offset
#define funit_offset_x 255    // Frequency Unit horizontal offset
#define funit_offset_y  48    // Frequency Unit vertical offset
#define band_offset_x  150    // Band horizontal offset
#define band_offset_y    3    // Band vertical offset
// #define mode_offset_x   95    // Mode horizontal offset
// #define mode_offset_y  114    // Mode vertical offset
#define vol_offset_x   120    // Volume horizontal offset
#define vol_offset_y   150    // Volume vertical offset
#define rds_offset_x   165    // RDS horizontal offset
#define rds_offset_y    94    // RDS vertical offset
#define batt_offset_x  288    // Battery meter x offset
#define batt_offset_y    0    // Battery meter y offset
#define clock_datum     90    // Clock x offset

//
// Show ABOUT screen
//
static void drawAbout()
{
  // About screen
  spr.setTextDatum(TL_DATUM);
  spr.setTextColor(theme[themeIdx].text_muted, theme[themeIdx].bg);
  spr.drawString("ESP32-SI4732 Receiver", 0, 0, 4);
  spr.setTextColor(theme[themeIdx].text, theme[themeIdx].bg);
  spr.drawString(getVersion(), 2, 33, 2);
  spr.drawString("https://github.com/esp32-si4732/ats-mini", 2, 33 + 16, 2);
  spr.drawString("Authors: PU2CLR (Ricardo Caratti),", 2, 33 + 16 * 3, 2);
  spr.drawString("Volos Projects, Ralph Xavier, Sunnygold,", 2, 33 + 16 * 4, 2);
  spr.drawString("Goshante, G8PTN (Dave), R9UCL (Max Arnold),", 2, 33 + 16 * 5, 2);
  spr.drawString("Marat Fayzullin", 2, 33 + 16 * 6, 2);

#if TUNE_HOLDOFF
  // Update if not tuning
  if(!tuning_flag) spr.pushSprite(0,0);
#else
  // No hold off
  spr.pushSprite(0,0);
#endif
}

//
// Show "Loading SSB" message
//
void drawLoadingSSB()
{
  if(!display_on) return;

  spr.setTextDatum(MC_DATUM);
  spr.fillSmoothRoundRect(80,40,160,40,4,theme[themeIdx].text);
  spr.fillSmoothRoundRect(81,41,158,38,4,theme[themeIdx].menu_bg);
  spr.drawString("Loading SSB",160,62,4);
  spr.pushSprite(0,0);
}

//
// Show command status message
//
void drawCommandStatus(const char *status)
{
  if(!display_on) return;

  spr.drawString(status,38,14,2);
}

//
// Draw band and mode indicators
//
static void drawBandAndMode(const char *band, const char *mode, int x, int y)
{
  spr.setTextDatum(TC_DATUM);
  spr.setTextColor(theme[themeIdx].band_text, theme[themeIdx].bg);
  uint16_t band_width = spr.drawString(band, x, y);

  spr.setTextDatum(TL_DATUM);
  spr.setTextColor(theme[themeIdx].mode_text, theme[themeIdx].bg);
  uint16_t mode_width = spr.drawString(mode, x + band_width / 2 + 12, y + 8, 2);

  spr.drawSmoothRoundRect(x + band_width / 2 + 7, y + 7, 4, 4, mode_width + 8, 17, theme[themeIdx].mode_border, theme[themeIdx].bg);
}

//
// Draw frequency
//
static void drawFrequency(uint32_t freq, int x, int y, int ux, int uy)
{
  spr.setTextDatum(MR_DATUM);
  spr.setTextColor(theme[themeIdx].freq_text, theme[themeIdx].bg);

  if(currentMode==FM)
  {
    // FM frequency
    spr.drawFloat(freq/100.00, 2, x, y, 7);
    spr.setTextDatum(ML_DATUM);
    spr.setTextColor(theme[themeIdx].funit_text, theme[themeIdx].bg);
    spr.drawString("MHz", ux, uy);
  }
  else
  {
    if(isSSB())
    {
      // SSB frequency
      char text[32];
      freq = freq * 1000 + currentBFO;
      sprintf(text, "%3.3u", freq / 1000);
      spr.drawString(text, x, y, 7);
      spr.setTextDatum(ML_DATUM);
      sprintf(text, ".%3.3d", freq % 1000);
      spr.drawString(text, 5+x, 15+y, 4);
    }
    else
    {
      // AM frequency
      spr.drawNumber(freq, x, y, 7);
      spr.setTextDatum(ML_DATUM);
      spr.drawString(".000", 5+x, 15+y, 4);
    }

    // SSB/AM frequencies are measured in kHz
    spr.setTextColor(theme[themeIdx].funit_text, theme[themeIdx].bg);
    spr.drawString("kHz", ux, uy);
  }
}

//
// Draw tuner scale
//
static void drawScale(uint32_t freq)
{
  spr.fillTriangle(156, 122, 160, 132, 164, 122, theme[themeIdx].scale_pointer);
  spr.drawLine(160, 124, 160, 169, theme[themeIdx].scale_pointer);

  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(theme[themeIdx].scale_text, theme[themeIdx].bg);

  // Start drawing frequencies from the left 
  freq = freq/10 - 20;

  // Get band edges
  const Band *band = getCurrentBand();
  uint32_t minFreq = band->minimumFreq/10;
  uint32_t maxFreq = band->maximumFreq/10;

  for(int i=0 ; i<40 ; i++, freq++)
  {
    if(freq>=minFreq && freq<=maxFreq)
    {
      uint16_t lineColor = i==20?
        theme[themeIdx].scale_pointer : theme[themeIdx].scale_line;

      if((freq%10)==0)
      {
        spr.drawLine(i*8, 169, i*8, 150, lineColor);
        spr.drawLine((i*8)+1, 169, (i*8)+1, 150, lineColor);
        if(currentMode==FM)
          spr.drawFloat(freq/10.0, 1, i*8, 140, 2);
        else if(freq>=100)
          spr.drawFloat(freq/100.0, 3, i*8, 140, 2);
        else
          spr.drawNumber(freq*10, i*8, 140, 2);
      }
      else if((freq%5)==0 && (freq%10)!=0)
      {
        spr.drawLine(i*8, 169, i*8, 155, lineColor);
        spr.drawLine((i*8)+1, 169, (i*8)+1, 155, lineColor);
      }
      else
      {
        spr.drawLine(i*8, 169, i*8, 160, lineColor);
      }
    }
  }
}

//
// Draw BFO
//
static void drawBFO(int bfo, int x, int y)
{
  char text[32];

  if(bfo>0)
    sprintf(text, "+%4.4d", bfo);
  else
    sprintf(text, "%4.4d", bfo);

  spr.setTextDatum(ML_DATUM);
  spr.setTextColor(theme[themeIdx].text, theme[themeIdx].bg);
  spr.drawString("BFO:",x,y,4);
  spr.drawString(text,x+70,y,4);
}

//
// Draw S-meter
//
static void drawSMeter(int strength, int x, int y)
{
  spr.drawTriangle(x + 1, y + 1, x + 11, y + 1, x + 6, y + 6, theme[themeIdx].smeter_icon);
  spr.drawLine(x + 6, y + 1, x + 6, y + 14, theme[themeIdx].smeter_icon);

  for(int i=0 ; i<strength ; i++)
  {
    if(i<10)
      spr.fillRect(15+x + (i*4), 2+y, 2, 12, theme[themeIdx].smeter_bar);
    else
      spr.fillRect(15+x + (i*4), 2+y, 2, 12, theme[themeIdx].smeter_bar_plus);
  }
}

//
// Draw RDS station name (also CB channel, etc)
//
static void drawStationName(const char *name, int x, int y)
{
  spr.setTextDatum(TC_DATUM);
  spr.setTextColor(theme[themeIdx].rds_text, theme[themeIdx].bg);
  spr.drawString(name, x, y, 4);
}

//
// Draw screen according to given command
//
void drawScreen(uint16_t cmd)
{
  if(!display_on) return;

  // Clear screen buffer
  spr.fillSprite(theme[themeIdx].bg);

  // Time
  /* spr.setTextColor(theme[themeIdx].text,theme[themeIdx].bg); */
  /* spr.setTextDatum(ML_DATUM); */
  /* spr.drawString(time_disp,clock_datum,12,2); */
  /* spr.setTextColor(theme[themeIdx].text,theme[themeIdx].bg); */

  /* // Screen activity icon */
  /* screen_toggle = !screen_toggle; */
  /* spr.drawCircle(clock_datum+50,11,6,theme[themeIdx].text); */
  /* if (screen_toggle) spr.fillCircle(clock_datum+50,11,5,theme[themeIdx].bg); */
  /* else               spr.fillCircle(clock_datum+50,11,5,TFT_GREEN); */

  // Draw EEPROM write request icon
  drawEepromIndicator(save_offset_x, save_offset_y);

  // About screen is a special case
  if(cmd==CMD_ABOUT)
  {
    drawAbout();
    return;
  }

  // Set font we are going to use
  spr.setFreeFont(&Orbitron_Light_24);

  //
  // Band and mode
  //
  drawBandAndMode(
    getCurrentBand()->bandName,
    bandModeDesc[currentMode],
    band_offset_x, band_offset_y
  );

#if THEME_EDITOR
  spr.setTextDatum(TR_DATUM);
  spr.setTextColor(theme[themeIdx].text_warn, theme[themeIdx].bg);
  spr.drawString("WARN", 319, rds_offset_y, 4);
#endif

  // Draw frequency and units
  drawFrequency(
    currentFrequency,
    freq_offset_x, freq_offset_y,
    funit_offset_x, funit_offset_y
  );

  // Draw left-side menu/info bar
  // @@@ FIXME: Frequency display (above) intersects the side bar!
  drawSideBar(cmd, menu_offset_x, menu_offset_y, menu_delta_x);

  // Draw BFO value
  if(bfoOn) drawBFO(currentBFO, 10, 158);

  // Draw S-meter
  drawSMeter(getStrength(), meter_offset_x, meter_offset_y);

  // Draw FM-specific information
  if(currentMode==FM)
  {
    // Indicate FM pilot detection
    if(rx.getCurrentPilot())
      spr.fillRect(15 + meter_offset_x, 7+meter_offset_y, 4*17, 2, theme[themeIdx].bg);
    // Draw RDS station name
    drawStationName(getStationName(), rds_offset_x, rds_offset_y);
  }
  // Draw CB-specific information
  else if(isCB())
  {
    // Draw CB channel name
    drawStationName(getStationName(), rds_offset_x, rds_offset_y);
  }

  // Draw tuner scale
  drawScale(isSSB()? (currentFrequency + currentBFO/1000) : currentFrequency);

#if TUNE_HOLDOFF
  // Update if not tuning
  if(!tuning_flag)
  {
    drawBattery(batt_offset_x, batt_offset_y);
    spr.pushSprite(0,0);
  }
#else
  // No hold off
  drawBattery(batt_offset_x, batt_offset_y);
  spr.pushSprite(0,0);
#endif
}
