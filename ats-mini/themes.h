typedef struct {
  char name[12];
  uint16_t bg;
  uint16_t text;
  uint16_t text_muted;
  uint16_t text_warn;

  uint16_t smeter_icon;
  uint16_t smeter_bar;
  uint16_t smeter_bar_plus;

  uint16_t batt_voltage;
  uint16_t batt_border;
  uint16_t batt_full;
  uint16_t batt_low;
  uint16_t batt_charge;
  uint16_t batt_icon;

  uint16_t band_text;

  uint16_t mode_text;
  uint16_t mode_border;

  uint16_t box_bg;
  uint16_t box_border;
  uint16_t box_text;
  uint16_t box_off_bg;
  uint16_t box_off_text;

  uint16_t menu_bg;
  uint16_t menu_border;
  uint16_t menu_hdr;
  uint16_t menu_item;
  uint16_t menu_hl_bg;
  uint16_t menu_hl_text;
  uint16_t menu_param;

  uint16_t freq_text;
  uint16_t funit_text;

  uint16_t rds_text;

  uint16_t scale_text;
  uint16_t scale_pointer;
  uint16_t scale_line;
} ColorTheme;

const ColorTheme theme[] = {

  {
    "Default",
    0x0000, // bg
    0xFFFF, // text
    0xD69A, // text_muted
    0xF800, // text_warn
    0xD69A, // smeter_icon
    0x07E0, // smeter_bar
    0xF800, // smeter_bar_plus
    0xFFFF, // batt_voltage
    0xFFFF, // batt_border
    0x07E0, // batt_full
    0xF800, // batt_low
    0x001F, // batt_charge
    0xFFE0, // batt_icon
    0xD69A, // band_text
    0xD69A, // mode_text
    0xD69A, // mode_border
    0x0000, // box_bg
    0xD69A, // box_border
    0xD69A, // box_text
    0xF800, // box_off_bg
    0xBEDF, // box_off_text
    0x0000, // menu_bg
    0xF800, // menu_border
    0xFFFF, // menu_hdr
    0xBEDF, // menu_item
    0x105B, // menu_hl_bg
    0xBEDF, // menu_hl_text
    0xBEDF, // menu_param
    0xFFFF, // freq_text
    0xD69A, // funit_text
    0xD69A, // rds_text
    0xFFFF, // scale_text
    0xF800, // scale_pointer
    0xC638, // scale_line
  },

};

const uint8_t lastTheme = (sizeof theme / sizeof(ColorTheme)) - 1;
uint8_t themeIdx = 0;
