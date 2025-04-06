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

  //
  // Band and mode
  //
  spr.setFreeFont(&Orbitron_Light_24);
  spr.setTextDatum(TC_DATUM);
  spr.setTextColor(theme[themeIdx].band_text, theme[themeIdx].bg);
  uint16_t band_width = spr.drawString(getCurrentBand()->bandName, band_offset_x, band_offset_y);
  spr.setTextDatum(TL_DATUM);
  spr.setTextColor(theme[themeIdx].mode_text, theme[themeIdx].bg);
  uint16_t mode_width = spr.drawString(bandModeDesc[currentMode], band_offset_x + band_width / 2 + 12, band_offset_y + 8, 2);
  spr.drawSmoothRoundRect(band_offset_x + band_width / 2 + 7, band_offset_y + 7, 4, 4, mode_width + 8, 17, theme[themeIdx].mode_border, theme[themeIdx].bg);

#if THEME_EDITOR
  spr.setTextDatum(TR_DATUM);
  spr.setTextColor(theme[themeIdx].text_warn, theme[themeIdx].bg);
  spr.drawString("WARN", 319, rds_offset_y, 4);
#endif

  //
  // Frequency
  //
  spr.setTextDatum(MR_DATUM);
  spr.setTextColor(theme[themeIdx].freq_text, theme[themeIdx].bg);
  if(currentMode==FM)
  {
    // FM frequency
    spr.drawFloat(currentFrequency/100.00, 2, freq_offset_x, freq_offset_y, 7);
    spr.setTextDatum(ML_DATUM);
    spr.setTextColor(theme[themeIdx].funit_text, theme[themeIdx].bg);
    spr.drawString("MHz", funit_offset_x, funit_offset_y);
  }
  else
  {
    if(isSSB())
    {
      // SSB frequency
      uint32_t freq  = (uint32_t(currentFrequency) * 1000) + currentBFO;
      uint16_t khz   = freq / 1000;
      uint16_t tail  = (freq % 1000);
      char skhz [32];
      char stail [32];
      sprintf(skhz, "%3.3u", khz);
      sprintf(stail, ".%3.3d", tail);
      spr.drawString(skhz, freq_offset_x, freq_offset_y, 7);
      spr.setTextDatum(ML_DATUM);
      spr.drawString(stail, 5+freq_offset_x, 15+freq_offset_y, 4);
    }
    else
    {
      // AM frequency
      spr.drawNumber(currentFrequency, freq_offset_x, freq_offset_y, 7);
      spr.setTextDatum(ML_DATUM);
      spr.drawString(".000", 5+freq_offset_x, 15+freq_offset_y, 4);
    }

    // SSB/AM frequencies are measured in kHz
    spr.setTextColor(theme[themeIdx].funit_text, theme[themeIdx].bg);
    spr.drawString("kHz", funit_offset_x, funit_offset_y);
  }

  // Draw left-side menu/info bar
  // @@@ FIXME: Frequency display (above) intersects the side bar!
  drawSideBar(cmd, menu_offset_x, menu_offset_y, menu_delta_x);

  // BFO
  if(bfoOn)
  {
    char text[32];
    if(currentBFO>0)
      sprintf(text, "+%4.4d", currentBFO);
    else
      sprintf(text, "%4.4d", currentBFO);

    spr.setTextDatum(ML_DATUM);
    spr.setTextColor(theme[themeIdx].text, theme[themeIdx].bg);
    spr.drawString("BFO:",10,158,4);
    spr.drawString(text,80,158,4);
  }

  // S-Meter
  spr.drawTriangle(meter_offset_x + 1, meter_offset_y + 1, meter_offset_x + 11, meter_offset_y + 1, meter_offset_x + 6, meter_offset_y + 6, theme[themeIdx].smeter_icon);
  spr.drawLine(meter_offset_x + 6, meter_offset_y + 1, meter_offset_x + 6, meter_offset_y + 14, theme[themeIdx].smeter_icon);
  for(int i=0 ; i<getStrength() ; i++)
  {
    if(i<10)
      spr.fillRect(15+meter_offset_x + (i*4), 2+meter_offset_y, 2, 12, theme[themeIdx].smeter_bar);
    else
      spr.fillRect(15+meter_offset_x + (i*4), 2+meter_offset_y, 2, 12, theme[themeIdx].smeter_bar_plus);
  }

  // RDS info
  if(currentMode==FM)
  {
    if(rx.getCurrentPilot())
      spr.fillRect(15 + meter_offset_x, 7+meter_offset_y, 4*17, 2, theme[themeIdx].bg);

    spr.setTextDatum(TC_DATUM);
    spr.setTextColor(theme[themeIdx].rds_text, theme[themeIdx].bg);

#if THEME_EDITOR
    spr.drawString("*STATION*", rds_offset_x, rds_offset_y, 4);
#else
    spr.drawString(getStationName(), rds_offset_x, rds_offset_y, 4);
#endif
  }

  // CB info
  if(isCB())
  {
    spr.setTextDatum(TC_DATUM);
    spr.setTextColor(theme[themeIdx].rds_text, theme[themeIdx].bg);
    spr.drawString(getStationName(), rds_offset_x, rds_offset_y, 4);
  }

  //
  // Tuner scale
  //
  spr.fillTriangle(156, 122, 160, 132, 164, 122, theme[themeIdx].scale_pointer);
  spr.drawLine(160, 124, 160, 169, theme[themeIdx].scale_pointer);
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(theme[themeIdx].scale_text, theme[themeIdx].bg);

  int freq = isSSB()? (currentFrequency + currentBFO/1000) : currentFrequency;
  freq = freq/10 - 20;

  for(int i=0 ; i<40 ; i++, freq++)
  {
    uint16_t lineColor = i==20?
      theme[themeIdx].scale_pointer : theme[themeIdx].scale_line;

    const Band *band = getCurrentBand();
    if(!(freq<band->minimumFreq/10.00 || freq>band->maximumFreq/10.00))
    {
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
