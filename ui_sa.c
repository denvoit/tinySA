/* Copyright (c) 2020, Erik Kaashoek erik@kaashoek.com
 * All rights reserved.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * The software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */


#define FORM_ICON_WIDTH      16
#define FORM_ICON_HEIGHT     16

static const uint8_t left_icons [] =
{
#define I_EMPTY   (0*16)
  _BMP16(0b0000000000000000),
  _BMP16(0b0000000000000000),
  _BMP16(0b0000000000000000),
  _BMP16(0b0000000000000001),
  _BMP16(0b0000000000000001),
  _BMP16(0b0000000000000001),
  _BMP16(0b0000000000000000),
  _BMP16(0b0000000000000000),
  _BMP16(0b0000000000000000),
  _BMP16(0b0000000000000000),
  _BMP16(0b0000000000000001),
  _BMP16(0b0000000000000001),
  _BMP16(0b0000000000000001),
  _BMP16(0b0000000000000000),
  _BMP16(0b0000000000000000),
  _BMP16(0b0000000000000000),

#define I_HIGH_INPUT (1*16)
                              // +----------------+
  _BMP16(0b0000000000000000), // |                |
  _BMP16(0b0000000000000000), // |                |
  _BMP16(0b0000000001100000), // |         **     |
  _BMP16(0b0000000000111001), // |          ***  *|
  _BMP16(0b0000111111111111), // |   *************|
  _BMP16(0b0000000000111001), // |          ***  *|
  _BMP16(0b0000000001100000), // |         **     |
  _BMP16(0b0000000000000000), // |                |
  _BMP16(0b0000000000000000), // |                |
  _BMP16(0b0000000000000000), // |                |
  _BMP16(0b0000000000000001), // |                |
  _BMP16(0b0000000000000001), // |                |
  _BMP16(0b0000000000000001), // |                |
  _BMP16(0b0000000000000000), // |                |
  _BMP16(0b0000000000000000), // |                |
  _BMP16(0b0000000000000000), // |                |
                              // +----------------+
#define I_LOW_INPUT (2*16)
                              // +----------------+
  _BMP16(0b0000000000000000), // |                |
  _BMP16(0b0000000000000000), // |                |
  _BMP16(0b0000000000000000), // |                |
  _BMP16(0b0000000000000001), // |                |
  _BMP16(0b0000000000000001), // |                |
  _BMP16(0b0000000000000001), // |                |
  _BMP16(0b0000000000000000), // |                |
  _BMP16(0b0000000000000000), // |                |
  _BMP16(0b0000000000000000), // |                |
  _BMP16(0b0000000001100000), // |         **     |
  _BMP16(0b0000000000111001), // |         ****  *|
  _BMP16(0b0000111111111111), // |   *************|
  _BMP16(0b0000000000111001), // |         ****  *|
  _BMP16(0b0000000001100000), // |         **     |
  _BMP16(0b0000000000000000), // |                |
  _BMP16(0b0000000000000000), // |                |
                              // +----------------+
#define I_LOW_OUTPUT (3*16)
  _BMP16(0b0000000000000000),
  _BMP16(0b0000000000000000),
  _BMP16(0b0000000000000000),
  _BMP16(0b0000000000000001),
  _BMP16(0b0000000000000001),
  _BMP16(0b0000000000000001),
  _BMP16(0b0000000000000000),
  _BMP16(0b0000000000000000),
  _BMP16(0b0000000000000000),
  _BMP16(0b0000000110000000),
  _BMP16(0b0000011100000001),
  _BMP16(0b0000111111111111),
  _BMP16(0b0000011100000001),
  _BMP16(0b0000000110000000),
  _BMP16(0b0000000000000000),
  _BMP16(0b0000000000000000),

#define I_HIGH_OUTPUT (4*16)
  _BMP16(0b0000000000000000),
  _BMP16(0b0000000000000000),
  _BMP16(0b0000000110000000),
  _BMP16(0b0000011100000001),
  _BMP16(0b0000111111111111),
  _BMP16(0b0000011100000001),
  _BMP16(0b0000000110000000),
  _BMP16(0b0000000000000000),
  _BMP16(0b0000000000000000),
  _BMP16(0b0000000000000000),
  _BMP16(0b0000000000000001),
  _BMP16(0b0000000000000001),
  _BMP16(0b0000000000000001),
  _BMP16(0b0000000000000000),
  _BMP16(0b0000000000000000),
  _BMP16(0b0000000000000000),

#define I_CONNECT (5*16)
  _BMP16(0b0000000000000000),
  _BMP16(0b0000000000000000),
  _BMP16(0b0000000000110000),
  _BMP16(0b0000000000111101),
  _BMP16(0b0000001111111111),
  _BMP16(0b0000010000111101),
  _BMP16(0b0000100000110000),
  _BMP16(0b0001000000000000),
  _BMP16(0b0001000000000000),
  _BMP16(0b0000100000110000),
  _BMP16(0b0000010000111101),
  _BMP16(0b0000001111111111),
  _BMP16(0b0000000000111101),
  _BMP16(0b0000000000110000),
  _BMP16(0b0000000000000000),
  _BMP16(0b0000000000000000),
};

const uint8_t right_icons [] =
{
#define I_SA    0
                              // +----------------+
  _BMP16(0b0000000000000000), // |                |
  _BMP16(0b0111111111111111), // | ***************|
  _BMP16(0b0100000000000001), // | *             *|
  _BMP16(0b1100000000000001), // |**             *|
  _BMP16(0b1100000000000001), // |**  *          *|
  _BMP16(0b1100000000000001), // |**  *          *|
  _BMP16(0b0100100000000001), // | *  *   *      *|
  _BMP16(0b0100100000000001), // | *  *   *      *|
  _BMP16(0b0100101010001001), // | *  *   *   *  *|
  _BMP16(0b0100101010101001), // | *  * * *   *  *|
  _BMP16(0b1100101010101001), // |**  * * * * *  *|
  _BMP16(0b1101111111111101), // |**  * * * * *  *|
  _BMP16(0b1100000000000001), // |** *********** *|
  _BMP16(0b0100000000000001), // | *             *|
  _BMP16(0b0111111111111111), // | ***************|
  _BMP16(0b0000000000000000), // |                |
                              // +----------------+

#define I_GEN   1
                              // +----------------+
  _BMP16(0b0000000000000000), // |                |
  _BMP16(0b0111111111111111), // | ***************|
  _BMP16(0b0100000000000001), // | *             *|
  _BMP16(0b1100000000000001), // |**             *|
  _BMP16(0b1100111110001101), // |**  *****   ** *|
  _BMP16(0b1100100010001001), // |**  *   *   *  *|
  _BMP16(0b0100100010001001), // | *  *   *   *  *|
  _BMP16(0b0100100010001001), // | *  *   *   *  *|
  _BMP16(0b0100100010001001), // | *  *   *   *  *|
  _BMP16(0b0100100010001001), // | *  *   *   *  *|
  _BMP16(0b1100100010001001), // |**  *   *   *  *|
  _BMP16(0b1101100011111001), // |** **   *****  *|
  _BMP16(0b1100000000000001), // |**             *|
  _BMP16(0b0100000000000001), // | *             *|
  _BMP16(0b0111111111111111), // | ***************|
  _BMP16(0b0000000000000000), // |                |
                              // +----------------+
#define I_CONFIG 2
  _BMP16(0b0000000000000000),
  _BMP16(0b0111111111111111),
  _BMP16(0b0100000000000001),
  _BMP16(0b1100000010000001),
  _BMP16(0b1100001111000001),
  _BMP16(0b1100011110001001),
  _BMP16(0b0100011100011101),
  _BMP16(0b0100011110111001),
  _BMP16(0b0100001111111001),
  _BMP16(0b0100011111110001),
  _BMP16(0b1100111110000001),
  _BMP16(0b1101111100000001),
  _BMP16(0b1100111000000001),
  _BMP16(0b0100000000000001),
  _BMP16(0b0111111111111111),
  _BMP16(0b0000000000000000),

#define I_SINUS 3
  _BMP16(0b0000000000000000),
  _BMP16(0b0111111111111111),  // 1
  _BMP16(0b0100000000000001),  // 2
  _BMP16(0b1100000000000001),  // 3
  _BMP16(0b1100000000110001),  // 4
  _BMP16(0b1100000001001001),  // 5
  _BMP16(0b0100000010000101),  // 6
  _BMP16(0b0101000010000101),  // 7
  _BMP16(0b0101000010000101),  // 8
  _BMP16(0b0101000010000001),  // 9
  _BMP16(0b1100100100000001),  //10
  _BMP16(0b1100011000000001),  //11
  _BMP16(0b1100000000000001),  //12
  _BMP16(0b0100000000000001),  //13
  _BMP16(0b0111111111111111),  //14
  _BMP16(0b0000000000000000),
};



#define KP_X(x) (48*(x) + 2 + (LCD_WIDTH-BUTTON_WIDTH-192))
#define KP_Y(y) (48*(y) + 2)


#define KP_PERIOD 10
#define KP_MINUS 11
#define KP_X1 12
#define KP_K 13
#define KP_M 14
#define KP_G 15
#define KP_BS 16
#define KP_INF 17
#define KP_DB 18
#define KP_PLUSMINUS 19
#define KP_KEYPAD 20
#define KP_m 21
#define KP_u 22
#define KP_n 23
#define KP_p 24

#define KP_0    31
#define KP_1    32
#define KP_2    33
#define KP_5    34
#define KP_10   35
#define KP_20   36
#define KP_50   37
#define KP_100  38
#define KP_200  39
#define KP_500  40


typedef struct {
  uint8_t x:4;
  uint8_t y:4;
  int8_t  c;
} keypads_t;

static const keypads_t *keypads;

// 7 8 9 G
// 4 5 6 M
// 1 2 3 k
// 0 . < x

static const keypads_t keypads_freq[] = {
  { 1, 3, KP_PERIOD },
  { 0, 3, 0 },
  { 0, 2, 1 },
  { 1, 2, 2 },
  { 2, 2, 3 },
  { 0, 1, 4 },
  { 1, 1, 5 },
  { 2, 1, 6 },
  { 0, 0, 7 },
  { 1, 0, 8 },
  { 2, 0, 9 },
  { 3, 0, KP_G },
  { 3, 1, KP_M },
  { 3, 2, KP_K },
  { 3, 3, KP_X1 },
  { 2, 3, KP_BS },
  { 0, 0, -1 }
};

// 7 8 9
// 4 5 6
// 1 2 3
// 0 . < x

static const keypads_t keypads_positive[] = {
  { 1, 3, KP_PERIOD },
  { 0, 3, 0 },
  { 0, 2, 1 },
  { 1, 2, 2 },
  { 2, 2, 3 },
  { 0, 1, 4 },
  { 1, 1, 5 },
  { 2, 1, 6 },
  { 0, 0, 7 },
  { 1, 0, 8 },
  { 2, 0, 9 },
  { 3, 3, KP_X1 },
  { 2, 3, KP_BS },
  { 0, 0, -1 }
};

// 100 200 500 n
// 10  20  50  u
// 1   2   5   m
// 0   .   <   x

static const keypads_t keypads_pos_unit[] = {
  { 1, 3, KP_PERIOD },
  { 0, 3, 0 },
  { 0, 2, 1 },
  { 1, 2, 2 },
  { 2, 2, 5 },
  { 0, 1, KP_10 },
  { 1, 1, KP_20 },
  { 2, 1, KP_50 },
  { 0, 0, KP_100 },
  { 1, 0, KP_200 },
  { 2, 0, KP_500 },
  { 3, 0, KP_n },
  { 3, 1, KP_u },
  { 3, 2, KP_m },
  { 3, 3, KP_X1 },
  { 2, 3, KP_BS },
  { 0, 0, -1 }
};

// 7 8 9 m
// 4 5 6 u
// 1 2 3 -
// 0 . < x

static const keypads_t keypads_plusmin_unit[] = {
  { 1, 3, KP_PERIOD },
  { 0, 3, 0 },
  { 0, 2, 1 },
  { 1, 2, 2 },
  { 2, 2, 3 },
  { 0, 1, 4 },
  { 1, 1, 5 },
  { 2, 1, 6 },
  { 0, 0, 7 },
  { 1, 0, 8 },
  { 2, 0, 9 },
  { 3, 0, KP_u},
  { 3, 1, KP_m},
  { 3, 2, KP_MINUS },
  { 3, 3, KP_X1 },
  { 2, 3, KP_BS },
  { 0, 0, -1 }
};
// 7 8 9
// 4 5 6
// 1 2 3 -
// 0 . < x

static const keypads_t keypads_plusmin[] = {
  { 1, 3, KP_PERIOD },
  { 0, 3, 0 },
  { 0, 2, 1 },
  { 1, 2, 2 },
  { 2, 2, 3 },
  { 0, 1, 4 },
  { 1, 1, 5 },
  { 2, 1, 6 },
  { 0, 0, 7 },
  { 1, 0, 8 },
  { 2, 0, 9 },
  { 3, 0, KP_u},
  { 3, 1, KP_m},
  { 3, 2, KP_MINUS },
  { 3, 3, KP_X1 },
  { 2, 3, KP_BS },
  { 0, 0, -1 }
};

// 7 8 9
// 4 5 6
// 1 2 3 m
// 0 . < x
static const keypads_t keypads_time[] = {
  { 1, 3, KP_PERIOD },
  { 0, 3, 0 },
  { 0, 2, 1 },
  { 1, 2, 2 },
  { 2, 2, 3 },
  { 0, 1, 4 },
  { 1, 1, 5 },
  { 2, 1, 6 },
  { 0, 0, 7 },
  { 1, 0, 8 },
  { 2, 0, 9 },
//  { 3, 0, KP_n},
//  { 3, 1, KP_u},
  { 3, 2, KP_m },
  { 3, 3, KP_X1 },
  { 2, 3, KP_BS },
  { 0, 0, -1 }
};

enum {
  KM_START, KM_STOP, KM_CENTER, KM_SPAN, KM_CW, // These must be first to share common help text
  //#5
  KM_REFLEVEL, KM_SCALE, KM_ATTENUATION, KM_ACTUALPOWER, KM_IF,
  // #10
  KM_SAMPLETIME, KM_LOWOUTLEVEL, KM_DECAY, KM_NOISE,
#ifdef TINYSA4
  KM_30MHZ, 
#else
  KM_10MHZ, 
#endif
  // #15
  KM_REPEAT, KM_OFFSET, KM_TRIGGER, KM_LEVELSWEEP, KM_SWEEP_TIME,
  // #20
  KM_OFFSET_DELAY, KM_FAST_SPEEDUP, KM_GRIDLINES, KM_MARKER, KM_MODULATION,
  // #25
  KM_HIGHOUTLEVEL,
#ifdef TINYSA4
  KM_COR_AM,  KM_COR_WFM, KM_COR_NFM,
  KM_IF2,
  // #30
  KM_R,KM_MOD,KM_CP,
#endif
  KM_ATTACK,
#ifdef TINYSA4
  KM_LPF,
#endif
  // #35
  KM_NONE // always at enum end
};

static const struct {
  const keypads_t *keypad_type;
  char * name;
} keypads_mode_tbl[KM_NONE] = {
  {keypads_freq        , "START"}, // start
  {keypads_freq        , "STOP"}, // stop
  {keypads_freq        , "CENTER"}, // center
  {keypads_freq        , "SPAN"}, // span
  {keypads_freq        , "FREQ"}, // cw freq
  {keypads_plusmin_unit, "REF\nLEVEL"}, // reflevel #5
  {keypads_pos_unit    , "SCALE"}, // scale
  {keypads_positive    , "ATTENUATE"}, // attenuation
  {keypads_plusmin_unit, "ACTUAL\nPOWER"}, // actual power
  {keypads_freq        , "IF"}, // IF
  {keypads_positive    , "SAMPLE\nDELAY"}, // sample delay #10
  {keypads_plusmin     , "LEVEL"},    // KM_LOWOUTLEVEL
  {keypads_positive    , "DECAY"},    // KM_DECAY
  {keypads_positive    , "NOISE\nLEVEL"},    // KM_NOISE
  {keypads_freq        , "FREQ"},    // KM_30MHz | KM_10MHz
  {keypads_positive    , "SAMPLE\nREPEAT"},    // KM_REPEA #15
  {keypads_plusmin     , "OFFSET"},    // KM_OFFSET
  {keypads_plusmin_unit, "TRIGGER\nLEVEL"},    // KM_TRIGGER
  {keypads_plusmin     , "LEVEL\nSWEEP"},    // KM_LEVELSWEEP
  {keypads_time        , "SWEEP\nSECONDS"},    // KM_SWEEP_TIME
  {keypads_positive    , "OFFSET\nDELAY"}, // KM_OFFSET_DELAY #20
  {keypads_positive    , "FAST\nSPEEDUP"}, // KM_FAST_SPEEDUP
  {keypads_positive    , "MINIMUM\nGRIDLINES"}, // KM_GRIDLINES
  {keypads_freq        , "MARKER\nFREQ"}, // KM_MARKER
  {keypads_freq        , "MODULATION\nFREQ"}, // KM_MODULATION
  {keypads_plusmin     , "LEVEL"},    // KM_HIGHOUTLEVEL #25
#ifdef TINYSA4
  {keypads_plusmin     , "COR\nAM"},    // KM_COR_AM
  {keypads_plusmin     , "COR\nWFM"},    // KM_COR_WFM
  {keypads_plusmin     , "COR\nNFM"},    // KM_COR_NFM
  {keypads_freq        , "IF2"}, // KM_IF2
  {keypads_positive    , "R"}, // KM_R    #30
  {keypads_positive    , "MODULO"}, // KM_MOD
  {keypads_positive    , "CP"}, // KM_CP
#endif
  {keypads_positive    , "ATTACK"},    // KM_ATTACK
#ifdef TINYSA4
  {keypads_freq        , "ULTRA\nSTART"}, // KM_LPF
#endif
};
#if 0 // Not used

enum { SL_GENLOW_FREQ, SL_GENHIGH_FREQ, SL_GENLOW_LEVEL, SL_GENHIGH_LEVEL };

ui_slider_t ui_sliders [] =
{
 { KM_CENTER,       true, 0, 1000000,   0,          350000000,  M_GENLOW},
 { KM_CENTER,       true, 0, 1000000,   240000000,  960000000,  M_GENHIGH},
 { KM_LOWOUTLEVEL,  false,0, 1,         -76,        -6,         M_GENLOW},
 { KM_HIGHOUTLEVEL, false,0, 1,         -38,        +6,         M_GENHIGH},
};
#endif


// ===[MENU CALLBACKS]=========================================================
static const menuitem_t  menu_lowoutputmode[];
static const menuitem_t  menu_highoutputmode[];
static const menuitem_t  menu_modulation[];
static const menuitem_t  menu_top[];
static const menuitem_t  menu_reffer[];
static const menuitem_t  menu_modulation[];
//static const menuitem_t  menu_drive_wide[];
#ifdef TINYSA4
static const menuitem_t  menu_settings3[];
#endif
static const menuitem_t  menu_sweep[];

static UI_FUNCTION_ADV_CALLBACK(menu_sweep_acb)
{
  (void)data;
  (void)item;
  if (b){
    if (setting.level_sweep != 0 || get_sweep_frequency(ST_SPAN) != 0)  {
      plot_printf(uistat.text, sizeof uistat.text, "SW:%3.2fMHz %+ddB %.3Fs",
                  get_sweep_frequency(ST_SPAN) / 1000000.0,
                  (int)setting.level_sweep,
                  setting.sweep_time_us/(float)ONE_SECOND_TIME);
      b->param_1.text = uistat.text;
    }
    else
      b->param_1.text = "SWEEP: OFF";
    return;
  }
  menu_push_submenu(menu_sweep);
}


static UI_FUNCTION_ADV_CALLBACK(menu_mode_acb)
{
  (void)data;
  if (b){
    if (item == setting.mode)  {
      b->param_1.text = "Return";
      b->bg = LCD_MENU_ACTIVE_COLOR;
      b->border = BUTTON_BORDER_FALLING | MENU_BUTTON_BORDER;
    }
    else
      b->param_1.text = "Switch";
    return;
  }
  set_mode(item);
//  draw_cal_status();
  switch (item) {
  case 0:
//    if (setting.mode != M_LOW)
//      set_mode(M_LOW);
    menu_move_back(true);
    break;
  case 1:
//    if (setting.mode != M_HIGH)
//      set_mode(M_HIGH);
    menu_move_back(true);
    break;
  case 2:
    menu_push_submenu(menu_lowoutputmode);
    break;
  case 3:
    menu_push_submenu(menu_highoutputmode);
    break;
  }
  redraw_request |= REDRAW_CAL_STATUS;
}

static UI_FUNCTION_ADV_CALLBACK(menu_load_preset_acb)
{
  (void)item;
  if(b){
    b->param_1.u = data;
    return;
  }
  if (caldata_recall(data) == -1) {
    if (data == 0)
      reset_settings(setting.mode);  // Restore factory defaults
  }
  menu_move_back(true);
}

static UI_FUNCTION_ADV_CALLBACK(menu_store_preset_acb)
{
  (void)item;
  if(b){
    b->param_1.u = data;
    return;
  }
  if (data == 100) {
    reset_settings(M_LOW);  // Restore all defaults in Low mode
    set_refer_output(-1);
 //   setting.mode = -1;
    data = 0;
  }
  caldata_save(data);
  menu_move_back(true);
}


extern bool dirty;
UI_FUNCTION_CALLBACK(menu_autosettings_cb)
{
  (void)item;
  (void)data;
  reset_settings(setting.mode);

  active_marker = 0;
  for (int i = 1; i<MARKER_COUNT; i++ ) {
    markers[i].enabled = M_DISABLED;
  }
  markers[0].enabled = M_ENABLED;
  markers[0].mtype = M_REFERENCE | M_TRACKING;

  //  set_refer_output(1);

  //  SetPowerLevel(100); // Reset
  set_clear_storage();
  dirty = true;
  //  menu_move_back(true);   // stay in input menu
  ui_mode_normal();
//  draw_cal_status();
}

#ifdef __CALIBRATE__
static UI_FUNCTION_CALLBACK(menu_calibrate_cb)
{
  (void)data;
  switch (item) {
  case 1:
    sweep_mode = SWEEP_CALIBRATE;
    menu_move_back(true);
    break;
  case 2:
    reset_calibration();
    break;
  }
}
#endif

static UI_FUNCTION_ADV_CALLBACK(menu_scanning_speed_acb)
{
  (void)item;
  if(b){
    b->icon = data == setting.step_delay_mode ? BUTTON_ICON_GROUP_CHECKED : BUTTON_ICON_GROUP;
    return;
  }
  set_step_delay(data);
//    menu_move_back(false);
  ui_mode_normal();
}

#define CONFIG_MENUITEM_TOUCH_CAL   0
#define CONFIG_MENUITEM_TOUCH_TEST  1
#define CONFIG_MENUITEM_SELFTEST    2
#define CONFIG_MENUITEM_VERSION     3
static UI_FUNCTION_CALLBACK(menu_config_cb)
{
  (void)item;
  switch (data) {
  case CONFIG_MENUITEM_TOUCH_CAL:
    touch_cal_exec();
    break;
  case CONFIG_MENUITEM_TOUCH_TEST:
    touch_draw_test();
    break;
  case CONFIG_MENUITEM_SELFTEST:
    sweep_mode = 0;         // Suspend sweep to save time
    menu_move_back(true);
    setting.test = 0;
    setting.test_argument = 0;
    sweep_mode = SWEEP_SELFTEST;
    return;
  case CONFIG_MENUITEM_VERSION:
    show_version();
    break;
  }
  ui_mode_normal();
  redraw_frame();
  request_to_redraw_grid();
}

static UI_FUNCTION_CALLBACK(menu_dfu_cb)
{
  (void)data;
  (void)item;
  enter_dfu();
}


// const int menu_modulation_value[]={MO_NONE,MO_AM, MO_NFM, MO_WFM, MO_EXTERNAL};
const char *menu_modulation_text[]={"None", "AM", "NFM", "WFM", "External"};

static UI_FUNCTION_ADV_CALLBACK(menu_modulation_acb)
{
  (void)item;
  if (b){
    b->icon = data == setting.modulation ? BUTTON_ICON_GROUP_CHECKED : BUTTON_ICON_GROUP;
    return;
  }
//Serial.println(item);
  if (data) {
    set_sweep_frequency(ST_SPAN, 0);      // No other scanning allowed when modulation is on!!!!!
    set_level_sweep(0);
  }
  set_modulation(data);
  menu_move_back(false);
//  ui_mode_normal();   // Stay in menu mode
//  draw_cal_status();
}

static UI_FUNCTION_ADV_CALLBACK(menu_smodulation_acb){
  (void)item;
  (void)data;
  if(b){
    if (setting.modulation == MO_NONE || setting.modulation == MO_EXTERNAL)
      b->param_1.text = menu_modulation_text[setting.modulation];
    else {
      plot_printf(uistat.text, sizeof uistat.text, "%5.3fkHz %s", setting.modulation_frequency / 1000.0, menu_modulation_text[setting.modulation]);
      b->param_1.text = uistat.text;
    }
    return;
  }
  menu_push_submenu(menu_modulation);
}

//                               0      1       2       3      4      5      6      7
const char *menu_reffer_text[]={"OFF","30MHz","15MHz","10MHz","4MHz","3MHz","2MHz","1MHz"};
static UI_FUNCTION_ADV_CALLBACK(menu_reffer_acb)
{
  (void)item;
  if (b){
    b->icon = setting.refer == ((int)data-1) ? BUTTON_ICON_GROUP_CHECKED : BUTTON_ICON_GROUP;
    b->param_1.text = menu_reffer_text[data];
    return;
  }
//Serial.println(item);
  set_refer_output((int)data - 1);
  menu_move_back(false);
//  ui_mode_normal();   // Stay in menu mode
//  draw_cal_status();
}

static UI_FUNCTION_ADV_CALLBACK(menu_sreffer_acb){
  (void)item;
  (void)data;
  if(b){
    b->param_1.text = menu_reffer_text[setting.refer+1];
    return;
  }
  menu_push_submenu(menu_reffer);
}

#ifdef TINYSA3
static UI_FUNCTION_ADV_CALLBACK(menu_lo_drive_acb)
{
  (void)item;
  if(b){
    b->param_1.i = drive_dBm[data];
    b->icon = data == setting.lo_drive ? BUTTON_ICON_GROUP_CHECKED : BUTTON_ICON_GROUP;
    return;
  }
//Serial.println(item);
  set_lo_drive(data);
  menu_move_back(false);
//  ui_mode_normal();
//  draw_cal_status();
}
#else
static UI_FUNCTION_ADV_CALLBACK(menu_mixer_drive_acb)
{
  (void)item;
  if(b){
    b->param_1.i = data;
    b->icon = data == setting.lo_drive ? BUTTON_ICON_GROUP_CHECKED : BUTTON_ICON_GROUP;
    return;
  }
//Serial.println(item);
  set_lo_drive(data);
  menu_move_back(false);
//  ui_mode_normal();
//  draw_cal_status();
}
#endif

#if 0
static UI_FUNCTION_ADV_CALLBACK(menu_sdrive_acb){
  (void)item;
  (void)data;
  if(b){
#ifdef TINYSA4
    b->param_1.i = setting.lo_drive;
#else
    b->param_1.i = drive_dBm[setting.lo_drive] + (setting.mode==M_GENHIGH ? setting.offset : 0);
#endif
    return;
  }
  menu_push_submenu(menu_drive_wide);
}
#endif

#ifdef __SPUR__
static UI_FUNCTION_ADV_CALLBACK(menu_spur_acb)
{
  (void)data;
  (void)item;
  if (b){
    if (setting.mode == M_LOW) {
      b->param_1.text = "SPUR\nREMOVAL";
      b->icon = AUTO_ICON(setting.spur_removal);
    } else {
      b->param_1.text = "MIRROR\nMASKING";
#ifdef TINYSA4
      b->icon = AUTO_ICON(setting.mirror_masking ? 1 : 0);  // mirror_masking does not yet have an auto mode so this is never an auto icon
#else
      b->icon = setting.mirror_masking == 0 ? BUTTON_ICON_NOCHECK : BUTTON_ICON_CHECK;
#endif
    }
    return;
  }
  if (setting.mode == M_LOW) {
    toggle_spur();
  } else
    toggle_mirror_masking();
  //  menu_move_back(false);
  ui_mode_normal();
}
#endif

#ifdef TINYSA4
static UI_FUNCTION_ADV_CALLBACK(menu_extra_lna_acb)
{
  (void)data;
  (void)item;
  if (b){
    b->icon = setting.extra_lna == 0 ? BUTTON_ICON_NOCHECK : BUTTON_ICON_CHECK;
    return;
  }
  toggle_extra_lna();
  //  menu_move_back(false);
  ui_mode_normal();
}

static UI_FUNCTION_ADV_CALLBACK(menu_adf_out_acb)
{
  (void)data;
  (void)item;
  if (b){
    b->icon = config.high_out_adf4350 == 0 ? BUTTON_ICON_NOCHECK : BUTTON_ICON_CHECK;
    return;
  }
  toggle_high_out_adf4350();
  //  menu_move_back(false);
  ui_mode_normal();
}


static UI_FUNCTION_ADV_CALLBACK(menu_debug_freq_acb)
{
  (void)data;
  (void)item;
  if (b){
    b->icon = debug_frequencies == 0 ? BUTTON_ICON_NOCHECK : BUTTON_ICON_CHECK;
    return;
  }
  debug_frequencies = ! debug_frequencies;
  //  menu_move_back();
  ui_mode_normal();
}
#endif

static UI_FUNCTION_ADV_CALLBACK(menu_measure_acb)
{
  (void)item;
  if (b){
    b->icon = data == setting.measurement ? BUTTON_ICON_GROUP_CHECKED : BUTTON_ICON_GROUP;
    return;
  }
  menu_move_back(false);
#ifdef __MEASURE__
  switch(data) {
    case M_OFF:                                     // Off
//      reset_settings(setting.mode);
      set_measurement(M_OFF);
      break;
    case M_IMD:                                     // IMD
      reset_settings(setting.mode);
      for (int i = 0; i< MARKERS_MAX; i++) {
        markers[i].enabled = M_ENABLED;
        markers[i].mtype = M_DELTA | M_TRACKING;
      }
      markers[0].mtype = M_REFERENCE | M_TRACKING;
      kp_help_text = "Frequency of fundamental";
      ui_mode_keypad(KM_CENTER);
      set_sweep_frequency(ST_START, 0);
      set_sweep_frequency(ST_STOP, uistat.value*5);
      set_measurement(M_IMD);
      break;
    case M_OIP3:                                     // OIP3
      reset_settings(setting.mode);
      for (int i = 0; i< MARKERS_MAX; i++) {
        markers[i].enabled = M_ENABLED;
        markers[i].mtype = M_DELTA;
      }
      markers[0].mtype = M_REFERENCE | M_TRACKING;
      markers[1].mtype = M_TRACKING;
      kp_help_text = "Frequency of left signal";
      ui_mode_keypad(KM_CENTER);
      int left =  uistat.value;
      kp_help_text = "Right signal";
      ui_mode_keypad(KM_CENTER);
      int right =  uistat.value;
      set_sweep_frequency(ST_CENTER, (left+right)/2);
      set_sweep_frequency(ST_SPAN, (right - left)*5);
      set_measurement(M_OIP3);
      break;
    case M_PHASE_NOISE:                             // Phase noise
      reset_settings(setting.mode);
      for (int i = 0; i< MARKERS_MAX; i++) {
        markers[i].enabled = M_DISABLED;
        markers[i].mtype = M_NORMAL;
      }
      markers[0].enabled = M_ENABLED;
      markers[0].mtype = M_REFERENCE | M_TRACKING;
      markers[1].enabled = M_ENABLED;
      markers[1].mtype = M_DELTA | M_NOISE;
      kp_help_text = "Frequency of signal";
      ui_mode_keypad(KM_CENTER);
      kp_help_text = "Frequency offset";
      ui_mode_keypad(KM_SPAN);
      set_sweep_frequency(ST_SPAN, uistat.value*4);
      set_measurement(M_PHASE_NOISE);
      set_average(4);

      break;
    case M_STOP_BAND:                             // STop band measurement
      reset_settings(setting.mode);
      markers[1].enabled = M_ENABLED;
      markers[1].mtype = M_DELTA;
      markers[2].enabled = M_ENABLED;
      markers[2].mtype = M_DELTA;
      kp_help_text = "Frequency of signal";
      ui_mode_keypad(KM_CENTER);
      kp_help_text = "Width of signal";
      ui_mode_keypad(KM_SPAN);
      set_sweep_frequency(ST_SPAN, uistat.value*4);
      set_measurement(M_STOP_BAND);
//      SetAverage(4);

      break;
    case M_PASS_BAND:                             // pass band measurement
//      reset_settings(setting.mode);
      markers[0].enabled = M_ENABLED;
      markers[0].mtype = M_REFERENCE | M_TRACKING;
      markers[1].enabled = M_ENABLED;
      markers[1].mtype = M_DELTA;
      markers[2].enabled = M_ENABLED;
      markers[2].mtype = M_DELTA;
//      kp_help_text = "Frequency of signal";
//      ui_mode_keypad(KM_CENTER);
//      kp_help_text = "Width of signal";
//      ui_mode_keypad(KM_SPAN);
//      set_sweep_frequency(ST_SPAN, uistat.value*2);
      set_measurement(M_PASS_BAND);
//      SetAverage(4);

      break;
#ifdef __LINEARITY__
    case M_LINEARITY:
      set_measurement(M_LINEARITY);
      break;
#endif
    case M_AM:                                     // AM
      reset_settings(setting.mode);
      for (int i = 0; i< 3; i++) {
        markers[i].enabled = M_ENABLED;
        markers[i].mtype = M_DELTA;// | M_TRACKING;
      }
      freq_t center, span;
      markers[0].mtype = M_REFERENCE;// | M_TRACKING;
      kp_help_text = "Frequency of signal";
      ui_mode_keypad(KM_CENTER);
      center = uistat.value;
      kp_help_text = "Modulation frequency: 3 .. 10kHz";
      ui_mode_keypad(KM_SPAN);
//      if (uistat.value < 3000)
//        break;
      span = uistat.value;
      set_sweep_frequency(ST_SPAN, span * 10);
//      update_frequencies();                     // To ensure markers are positioned right!!!!!!
      set_measurement(M_AM);
      set_marker_frequency(0, center);
      set_marker_frequency(1, center-span);
      set_marker_frequency(2, center+span);
      set_average(4);
      break;
    case M_FM:                                     // FM
      reset_settings(setting.mode);
      for (int i = 0; i< 3; i++) {
        markers[i].enabled = M_ENABLED;
        markers[i].mtype = M_DELTA;
      }
      markers[0].mtype = M_REFERENCE;
      kp_help_text = "Frequency of signal";
      ui_mode_keypad(KM_CENTER);
      set_marker_frequency(0, uistat.value);
      kp_help_text = "Modulation frequency: 1 .. 2.5kHz";
      ui_mode_keypad(KM_SPAN);
      if (uistat.value < 1000 || uistat.value > 2500)
        break;
      set_RBW(uistat.value/100);
      // actual_rbw_x10
      kp_help_text = "Frequency deviation: 3 .. 500kHz";
      ui_mode_keypad(KM_SPAN);
      if (uistat.value < 12000)
        uistat.value = 12000;   // minimum span
      set_sweep_frequency(ST_SPAN, uistat.value*4);
      set_measurement(M_FM);
      break;
    case M_THD:
      set_measurement(M_THD);
      break;
  }
#endif
//  selection = -1;
  ui_mode_normal();
//  draw_cal_status();
}

static UI_FUNCTION_ADV_CALLBACK(menu_atten_acb)
{
  (void)item;
  (void)data;
  if(b){
    b->icon = setting.auto_attenuation ? BUTTON_ICON_GROUP_CHECKED : BUTTON_ICON_GROUP;
    return;
  }
  set_auto_attenuation();
  menu_move_back(true);
}

static UI_FUNCTION_ADV_CALLBACK(menu_atten_high_acb)
{
  (void)item;
  if(b){
    b->icon = (setting.atten_step*30 == data) ? BUTTON_ICON_GROUP_CHECKED : BUTTON_ICON_GROUP;
    return;
  }
  setting.auto_attenuation = false;
  set_attenuation(data);
  menu_move_back(true);
}

static UI_FUNCTION_ADV_CALLBACK(menu_reflevel_acb)
{
  (void)item;
  (void)data;
  if(b){
    b->icon = setting.auto_reflevel ? BUTTON_ICON_GROUP_CHECKED : BUTTON_ICON_GROUP;
    return;
  }
  set_auto_reflevel(true);
  menu_move_back(true);
}

static UI_FUNCTION_ADV_CALLBACK(menu_storage_acb)
{
  (void)item;
  if(b){
    if (data == 0 && setting.show_stored)
      b->icon = BUTTON_ICON_CHECK;
    if (setting.subtract_stored){
      if (data == 2 && setting.show_stored)
        b->icon = BUTTON_ICON_CHECK;
      if (data == 3 && !setting.show_stored)
        b->icon = BUTTON_ICON_CHECK;
    }
    return;
  }
  switch(data) {
    case 0:
      set_storage();
      break;
    case 1:
      set_clear_storage();
      break;
    case 2:
      set_subtract_storage();
      break;
    case 3:
      toggle_normalize();
      if (setting.subtract_stored) {
        kp_help_text = "Ref level";
        ui_mode_keypad(KM_REFLEVEL);
//        setting.normalize_level = uistat.value;
      } else
        set_auto_reflevel(true);
      break;
  }
  ui_mode_normal();
//  draw_cal_status();
}

static UI_FUNCTION_ADV_CALLBACK(menu_waterfall_acb){
  (void)item;
  (void)data;
  if (b){
    b->icon = get_waterfall() ? BUTTON_ICON_CHECK : BUTTON_ICON_NOCHECK;
    return;
  }
  toggle_waterfall();
  ui_mode_normal();
}

static UI_FUNCTION_ADV_CALLBACK(menu_average_acb)
{
  (void)item;
  if (b){
    b->icon = setting.average == data ? BUTTON_ICON_GROUP_CHECKED : BUTTON_ICON_GROUP;
    return;
  }
  set_average(data);
  menu_move_back(true);
}

extern const menuitem_t menu_marker_modify[];
static UI_FUNCTION_ADV_CALLBACK(menu_marker_select_acb)
{
  (void)item;
  if(b){
    b->icon = markers[data-1].enabled ? BUTTON_ICON_CHECK : BUTTON_ICON_NOCHECK;
    b->param_1.i = data;
    return;
  }
  markers[data-1].enabled = true;
//  interpolate_maximum(data-1);        // possibly not a maximum
  markers[data-1].frequency = frequencies[markers[data-1].index];
  active_marker_select(data-1);
  menu_push_submenu(menu_marker_modify);
  redraw_marker(active_marker);
}

static UI_FUNCTION_ADV_CALLBACK(menu_marker_modify_acb)
{
  (void)item;
  if (active_marker == MARKER_INVALID) return;
  if(b){
    if (markers[active_marker].enabled == M_ENABLED) {
      b->icon = BUTTON_ICON_NOCHECK;
      if (data & markers[active_marker].mtype)
        b->icon = BUTTON_ICON_CHECK;
      else if (data==markers[active_marker].mtype)    // This catches the M_NORMAL case
        b->icon = BUTTON_ICON_CHECK;
    }
    return;
  }
  if (markers[active_marker].enabled == M_ENABLED)
  {
    if (data == M_NORMAL) {
      markers[active_marker].mtype = M_NORMAL;
    } else if (data == M_REFERENCE) {
      for (int i = 0; i<MARKER_COUNT; i++ ) {
        if (markers[i].mtype & M_REFERENCE)
          markers[i].mtype &= ~M_REFERENCE;
      }
      markers[active_marker].mtype |= M_REFERENCE;
      markers[active_marker].mtype &= ~M_DELTA;
    } else {
      if (data == M_DELTA && (markers[active_marker].mtype & M_REFERENCE))
        markers[active_marker].mtype &= ~M_REFERENCE;
      if (markers[active_marker].mtype & data)
        markers[active_marker].mtype &= ~data;
      else
        markers[active_marker].mtype |= data;
    }
  }
  markmap_all_markers();
//  redraw_marker(active_marker, TRUE);
//  menu_move_back(false);
}

static UI_FUNCTION_CALLBACK(menu_marker_delete_cb)
{
  (void)item;
  (void)data;
  if (active_marker>=0){
    markers[active_marker].enabled = false;
    markmap_all_markers();
    menu_move_back(false);
  }
}

#ifdef TINYSA4
static const uint16_t rbwsel_x10[]={0,3,10,30,100,300,1000,3000,6000};
static const char* rbwsel_text[]={"auto","300","1k","3k","10k","30k","100k","300k","600k"};
#else
static const uint16_t rbwsel_x10[]={0,30,100,300,1000,3000,6000};
#endif

static UI_FUNCTION_ADV_CALLBACK(menu_rbw_acb)
{
  (void)item;
  if (b){
#ifdef TINYSA4
  b->param_1.text = rbwsel_text[data];
#else
    b->param_1.u = rbwsel_x10[data]/10;
#endif
  b->icon = setting.rbw_x10 == rbwsel_x10[data] ? BUTTON_ICON_GROUP_CHECKED : BUTTON_ICON_GROUP;
    return;
  }
  set_RBW(rbwsel_x10[data]);
  menu_move_back(true);
}

static UI_FUNCTION_ADV_CALLBACK(menu_unit_acb)
{
  (void)item;
  if (b){
    b->icon = data == setting.unit ? BUTTON_ICON_GROUP_CHECKED : BUTTON_ICON_GROUP;
    return;
  }
  set_unit(data);
  menu_move_back(true);
}

#if 0
enum {
  S_20,S_10,S_5,S_2,S_1,S_P5,S_P2,S_P1,S_P05,S_P02,S_P01
};
static const float menu_scale_per_value[11]={20,10,5,2,1,0.5,0.2,0.1,0.05,0.02,0.01};

static UI_FUNCTION_ADV_CALLBACK(menu_scale_per_acb)
{
  (void)item;
  if(b){
    return;
  }
  set_scale(menu_scale_per_value[data]);
  menu_move_back(true);
}
#endif

const char *mode_text[] = {"PRE","POST","MID"};

static UI_FUNCTION_ADV_CALLBACK(menu_trigger_acb)
{
  (void)item;
  if(b){
    if (data == T_MODE) {
      b->param_1.text = mode_text[setting.trigger_mode - T_PRE];
    } else if (data == T_UP || data == T_DOWN)
      b->icon = setting.trigger_direction == data ? BUTTON_ICON_GROUP_CHECKED : BUTTON_ICON_GROUP;
    else
      b->icon = setting.trigger == data ? BUTTON_ICON_GROUP_CHECKED : BUTTON_ICON_GROUP;
    return;
  }
  if (data == T_MODE) {
    setting.trigger_mode += 1;
    if (setting.trigger_mode > T_MID)
      setting.trigger_mode = T_PRE;
    set_trigger(setting.trigger_mode);
  } else if (data != T_DONE) {
    set_trigger(data);
//  menu_move_back(false);
    ui_mode_normal();
  }
  completed = true;
}

#if 0
static void choose_active_trace(void)
{
  int i;
  if (trace[uistat.current_trace].enabled)
    // do nothing
    return;
  for (i = 0; i < TRACE_COUNT ; i++)
    if (trace[i].enabled) {
      uistat.current_trace = i;
      return;
    }
}
#endif
static void choose_active_marker(void)
{
  int i;
  for (i = 0; i < MARKER_COUNT; i++)
    if (markers[i].enabled == M_ENABLED) {
      active_marker = i;
      return;
    }
  active_marker = MARKER_INVALID;
}

#ifdef __HARMONIC__
static UI_FUNCTION_ADV_CALLBACK(menu_harmonic_acb)
{
  (void)item;
  if(b){
    b->icon = setting.harmonic == data ? BUTTON_ICON_CHECK : BUTTON_ICON_NOCHECK;
    return;
  }
  set_harmonic(data);
}
#endif


static UI_FUNCTION_ADV_CALLBACK(menu_settings_agc_acb){
  (void)item;
  (void)data;
  if(b){
#if 0
    if (S_IS_AUTO(setting.agc))
      b->icon = BUTTON_ICON_CHECK_AUTO;
    else
      b->icon = setting.agc ? BUTTON_ICON_CHECK : BUTTON_ICON_NOCHECK;
#else
    b->icon = AUTO_ICON(setting.agc);
#endif
    return;
  }
  toggle_AGC();
}

static UI_FUNCTION_ADV_CALLBACK(menu_settings_lna_acb){
  (void)item;
  (void)data;
  if(b){
    if (S_IS_AUTO(setting.lna))
      b->icon = BUTTON_ICON_CHECK_AUTO;
    else
      b->icon = setting.lna ? BUTTON_ICON_CHECK : BUTTON_ICON_NOCHECK;
    return;
  }
  toggle_LNA();
}

static UI_FUNCTION_ADV_CALLBACK(menu_settings_bpf_acb){
  (void)item;
  (void)data;
  if(b){
    b->icon = setting.tracking ? BUTTON_ICON_CHECK : BUTTON_ICON_NOCHECK;
    return;
  }
  toggle_tracking();
}

#ifdef __HAM_BAND__
static UI_FUNCTION_ADV_CALLBACK(menu_settings_ham_bands){
  (void)item;
  (void)data;
  if(b){
    b->icon = config.hambands ? BUTTON_ICON_CHECK : BUTTON_ICON_NOCHECK;
    return;
  }
  toggle_hambands();
}
#endif

static UI_FUNCTION_ADV_CALLBACK(menu_settings_below_if_acb){
  (void)item;
  (void)data;
  if(b){
    if (S_IS_AUTO(setting.below_IF))
      b->icon = BUTTON_ICON_CHECK_AUTO;
    else
      b->icon = setting.below_IF ? BUTTON_ICON_CHECK : BUTTON_ICON_NOCHECK;
    return;
  }
  toggle_below_IF();
}

#ifdef TINYSA4
static UI_FUNCTION_ADV_CALLBACK(menu_settings_ultra_acb){
  (void)item;
  (void)data;
  if(b){
    if (S_IS_AUTO(setting.ultra))
      b->icon = BUTTON_ICON_CHECK_AUTO;
    else
      b->icon = setting.ultra ? BUTTON_ICON_CHECK : BUTTON_ICON_NOCHECK;
    return;
  }
  toggle_ultra();
}
#endif

static UI_FUNCTION_ADV_CALLBACK(menu_lo_output_acb){
  (void)item;
  (void)data;
  if(b){
    b->icon = setting.tracking_output ? BUTTON_ICON_CHECK : BUTTON_ICON_NOCHECK;
    return;
  }
  toggle_tracking_output();
}

static UI_FUNCTION_ADV_CALLBACK(menu_pause_acb)
{
  (void) data;
  (void) item;
  if (b){
    b->icon = is_paused() ? BUTTON_ICON_CHECK : BUTTON_ICON_NOCHECK;
    return;
  }
  toggle_sweep();
//  menu_move_back(true);
//  draw_cal_status();
}

#ifdef __REMOTE_DESKTOP__
static UI_FUNCTION_ADV_CALLBACK(menu_send_display_acb)
{
  (void) data;
  (void) item;
  if (b){
    b->icon = auto_capture ? BUTTON_ICON_CHECK : BUTTON_ICON_NOCHECK;
    return;
  }
  auto_capture = ! auto_capture;
  // Update all screen to CPU
  if (auto_capture)
    redraw_request|=REDRAW_AREA|REDRAW_BATTERY|REDRAW_FREQUENCY|REDRAW_CAL_STATUS;
}
#endif

static UI_FUNCTION_ADV_CALLBACK(menu_outputmode_acb)
{
  (void) data;
  (void) item;
  if(b){
    b->param_1.text = setting.mute ? "OFF" : "ON";
    return;
  }
  toggle_mute();
}

#ifdef TINYSA4
static const uint16_t points_setting[] = {51, 101, 201, 290, 450};
#else
static const uint16_t points_setting[] = {51, 101, 145, 290};
#endif
static UI_FUNCTION_ADV_CALLBACK(menu_points_acb){
  (void)item;
  if(b){
    b->icon = points_setting[data] == sweep_points ? BUTTON_ICON_GROUP_CHECKED : BUTTON_ICON_GROUP;
    b->param_1.i = points_setting[data];
    return;
  }
  set_sweep_points(points_setting[data]);
}

#ifdef __USE_SERIAL_CONSOLE__
static UI_FUNCTION_ADV_CALLBACK(menu_serial_speed_acb)
{
  (void)item;
  if (b){
    b->icon = config._serial_speed == data ? BUTTON_ICON_GROUP_CHECKED : BUTTON_ICON_GROUP;
    b->param_1.u = USART_GET_SPEED(data);
    return;
  }
  config._serial_speed = data;
  shell_update_speed();
}

static UI_FUNCTION_ADV_CALLBACK(menu_connection_acb)
{
  (void)item;
  if (b){
    b->icon = (config._mode&_MODE_CONNECTION_MASK) == data ? BUTTON_ICON_GROUP_CHECKED : BUTTON_ICON_GROUP;
    return;
  }
  config._mode&=~_MODE_CONNECTION_MASK;
  config._mode|=data;
  shell_reset_console();
}
#endif
// ===[MENU DEFINITION]=========================================================
#if 0
static const menuitem_t menu_store_preset_high[8] =
{
  { MT_ADV_CALLBACK, 0,  "STORE\nSTARTUP",   menu_store_preset_acb},
  { MT_ADV_CALLBACK, 5,  "STORE %d",         menu_store_preset_acb},
  { MT_ADV_CALLBACK, 6,  "STORE %d",         menu_store_preset_acb},
  { MT_ADV_CALLBACK, 7,  "STORE %d",         menu_store_preset_acb},
  { MT_ADV_CALLBACK, 8,  "STORE %d",         menu_store_preset_acb},
  { MT_ADV_CALLBACK, 100,"FACTORY\nDEFAULTS",menu_store_preset_acb},
  { MT_CANCEL,   255, S_LARROW" BACK", NULL },
  { MT_NONE,     0,     NULL,            NULL } // sentinel
};

static const menuitem_t menu_load_preset_high[] =
{
  { MT_ADV_CALLBACK, 0, "LOAD\nSTARTUP",menu_load_preset_acb},
  { MT_ADV_CALLBACK, 5, "LOAD %d",      menu_load_preset_acb},
  { MT_ADV_CALLBACK, 6, "LOAD %d",      menu_load_preset_acb},
  { MT_ADV_CALLBACK, 7, "LOAD %d",      menu_load_preset_acb},
  { MT_ADV_CALLBACK, 8, "LOAD %s",      menu_load_preset_acb},
  { MT_SUBMENU,  0,     "STORE"  ,      menu_store_preset_high},
  { MT_CANCEL,   255, S_LARROW" BACK", NULL },
  { MT_NONE,     0,     NULL,            NULL } // sentinel
};
#endif

static const menuitem_t menu_store_preset[] =
{
  { MT_ADV_CALLBACK, 0,  "STORE AS\nSTARTUP",menu_store_preset_acb},
  { MT_ADV_CALLBACK, 1,  "STORE %d",         menu_store_preset_acb},
  { MT_ADV_CALLBACK, 2,  "STORE %d",         menu_store_preset_acb},
  { MT_ADV_CALLBACK, 3,  "STORE %d",         menu_store_preset_acb},
  { MT_ADV_CALLBACK, 4,  "STORE %d",         menu_store_preset_acb},
  { MT_ADV_CALLBACK, 100,"FACTORY\nDEFAULTS",menu_store_preset_acb},
  { MT_CANCEL,   255, S_LARROW" BACK", NULL },
  { MT_NONE,     0,     NULL,            NULL } // sentinel
};

static const menuitem_t menu_load_preset[] =
{
  { MT_ADV_CALLBACK, 0, "LOAD\nSTARTUP",menu_load_preset_acb},
  { MT_ADV_CALLBACK, 1, "LOAD %d"  ,    menu_load_preset_acb},
  { MT_ADV_CALLBACK, 2, "LOAD %d"  ,    menu_load_preset_acb},
  { MT_ADV_CALLBACK, 3, "LOAD %d"  ,    menu_load_preset_acb},
  { MT_ADV_CALLBACK, 4, "LOAD %d"  ,    menu_load_preset_acb},
  { MT_SUBMENU,  0,     "STORE"  ,       menu_store_preset},
  { MT_CANCEL,   255, S_LARROW" BACK", NULL },
  { MT_NONE,     0,     NULL,            NULL } // sentinel
};
#ifdef TINYSA4
static const menuitem_t menu_mixer_drive[] = {
  { MT_ADV_CALLBACK, 3, "%+ddBm",   menu_mixer_drive_acb},
  { MT_ADV_CALLBACK, 2, "%+ddBm",   menu_mixer_drive_acb},
  { MT_ADV_CALLBACK, 1, "%+ddBm",   menu_mixer_drive_acb},
  { MT_ADV_CALLBACK, 0, "%+ddBm",   menu_mixer_drive_acb},
  { MT_CANCEL,   255, S_LARROW" BACK", NULL },
  { MT_NONE,     0, NULL, NULL } // sentinel
};
#else
static const menuitem_t menu_lo_drive[] = {
  { MT_ADV_CALLBACK, 15, "%+ddBm",   menu_lo_drive_acb},
  { MT_ADV_CALLBACK, 14, "%+ddBm",   menu_lo_drive_acb},
  { MT_ADV_CALLBACK, 13, "%+ddBm",   menu_lo_drive_acb},
  { MT_ADV_CALLBACK, 12, "%+ddBm",   menu_lo_drive_acb},
  { MT_CANCEL,   255, S_LARROW" BACK", NULL },
  { MT_NONE,     0, NULL, NULL } // sentinel
};
#endif

static const menuitem_t  menu_modulation[] = {
  { MT_FORM | MT_TITLE,    0,  "MODULATION",NULL},
  { MT_FORM | MT_ADV_CALLBACK, MO_NONE,              "None",      menu_modulation_acb},
  { MT_FORM | MT_ADV_CALLBACK | MT_LOW, MO_AM,  "AM",   menu_modulation_acb},
  { MT_FORM | MT_ADV_CALLBACK, MO_NFM,               "Narrow FM", menu_modulation_acb},
  { MT_FORM | MT_ADV_CALLBACK, MO_WFM,               "Wide FM",   menu_modulation_acb},
  { MT_FORM | MT_ADV_CALLBACK | MT_LOW, MO_EXTERNAL, "External",  menu_modulation_acb},
  { MT_FORM | MT_KEYPAD,   KM_MODULATION,           "FREQ: %s",         "50Hz..6kHz"},
  { MT_FORM | MT_CANCEL,   0,                 S_LARROW" BACK",NULL },
  { MT_FORM | MT_NONE, 0, NULL, NULL } // sentinel
};

static const menuitem_t  menu_sweep[] = {
  { MT_FORM | MT_KEYPAD,   KM_SPAN,             "SPAN: %s",         "0..350MHz"},
  { MT_FORM | MT_KEYPAD | MT_LOW, KM_LEVELSWEEP,"LEVEL CHANGE: %s", "-70..70"},
  { MT_FORM | MT_KEYPAD,   KM_SWEEP_TIME,       "SWEEP TIME: %s",   "0..600 seconds"},
  { MT_FORM | MT_CANCEL,   0,                 S_LARROW" BACK",NULL },
  { MT_FORM | MT_NONE, 0, NULL, NULL } // sentinel
};

char low_level_help_text[12] = "-76..-6";
char center_text[18] = "FREQ: %s";

static const menuitem_t  menu_lowoutputmode[] = {
  { MT_FORM | MT_ADV_CALLBACK, 0,               "LOW OUTPUT            %s", menu_outputmode_acb},
//  { MT_FORM | MT_ADV_CALLBACK,  0,              "MOD: %s",   menu_smodulation_acb},
#ifdef TINYSA4
  { MT_FORM | MT_SUBMENU,  255, S_RARROW" Settings", menu_settings3},
#endif
  { MT_FORM | MT_KEYPAD,   KM_CENTER,           center_text,         "10kHz..350MHz"},
  { MT_FORM | MT_KEYPAD,   KM_LOWOUTLEVEL,      "LEVEL: %s",        low_level_help_text},
  { MT_FORM | MT_ADV_CALLBACK,  0,              "MOD: %s",   menu_smodulation_acb},
  { MT_FORM | MT_ADV_CALLBACK,  0,              "%s",      menu_sweep_acb},
//  { MT_FORM | MT_KEYPAD,   KM_SPAN,             "SPAN: %s",         "0..350MHz"},
//  { MT_FORM | MT_KEYPAD | MT_LOW, KM_LEVELSWEEP,"LEVEL CHANGE: %s", "-70..70"},
//  { MT_FORM | MT_KEYPAD,   KM_SWEEP_TIME,       "SWEEP TIME: %s",   "0..600 seconds"},
  { MT_FORM | MT_KEYPAD,  KM_OFFSET,            "EXTERNAL AMP: %s",   "-100..+100"},
  { MT_FORM | MT_CANCEL,   0,                   "MODE",             NULL },
  { MT_FORM | MT_NONE, 0, NULL, NULL } // sentinel
};

static const menuitem_t  menu_highoutputmode[] = {
  { MT_FORM | MT_ADV_CALLBACK,  0,      "HIGH OUTPUT           %s", menu_outputmode_acb},
#ifdef TINYSA4
  { MT_FORM | MT_SUBMENU,  255, S_RARROW" Settings", menu_settings3},
#endif
  { MT_FORM | MT_KEYPAD,    KM_CENTER,  center_text,         "240MHz..960MHz"},
//  { MT_FORM | MT_ADV_CALLBACK,   0,     "LEVEL: %+ddBm",    menu_sdrive_acb},
  { MT_FORM | MT_KEYPAD,   KM_HIGHOUTLEVEL,      "LEVEL: %s",        low_level_help_text /* "-76..-6" */},
  { MT_FORM | MT_ADV_CALLBACK,   0,     "MOD: %s",   menu_smodulation_acb},
#ifdef TINYSA4
  { MT_FORM | MT_ADV_CALLBACK,  0,              "%s",      menu_sweep_acb},
#else
  { MT_FORM | MT_KEYPAD,    KM_SPAN,    "SPAN: %s",         NULL},
  { MT_FORM | MT_KEYPAD,  KM_SWEEP_TIME,"SWEEP TIME: %s",   "0..600 seconds"},
#endif
  { MT_FORM | MT_KEYPAD,  KM_OFFSET,            "EXTERNAL AMP: %s",          "-100..+100"},
  { MT_FORM | MT_CANCEL,    0,          "MODE",             NULL },
  { MT_FORM | MT_NONE, 0, NULL, NULL } // sentinel
};

static const menuitem_t  menu_average[] = {
  { MT_ADV_CALLBACK, 0, "OFF",          menu_average_acb},
  { MT_ADV_CALLBACK, 1, "MIN\nHOLD",    menu_average_acb},
  { MT_ADV_CALLBACK, 2, "MAX\nHOLD",    menu_average_acb},
  { MT_ADV_CALLBACK, 3, "MAX\nDECAY",   menu_average_acb},
  { MT_ADV_CALLBACK, 4, "AVER 4",       menu_average_acb},
  { MT_ADV_CALLBACK, 5, "AVER 16",      menu_average_acb},
#ifdef __QUASI_PEAK__
  { MT_ADV_CALLBACK, 6, "QUASI\nPEAK",  menu_average_acb},
#endif
  { MT_CANCEL,   0, S_LARROW" BACK", NULL },
  { MT_NONE, 0, NULL, NULL } // sentinel
};

static const menuitem_t menu_rbw[] = {
  { MT_ADV_CALLBACK, 0, "  AUTO",   menu_rbw_acb},
#ifdef TINYSA4
  { MT_ADV_CALLBACK, 1, "%sHz",   menu_rbw_acb},
  { MT_ADV_CALLBACK, 2, "%sHz",   menu_rbw_acb},
  { MT_ADV_CALLBACK, 3, "%sHz",   menu_rbw_acb},
  { MT_ADV_CALLBACK, 4, "%sHz",   menu_rbw_acb},
  { MT_ADV_CALLBACK, 5, "%sHz",   menu_rbw_acb},
  { MT_ADV_CALLBACK, 6, "%sHz",   menu_rbw_acb},
  { MT_ADV_CALLBACK, 7, "%sHz",   menu_rbw_acb},
  { MT_ADV_CALLBACK, 8, "%sHz",   menu_rbw_acb},
#else
  { MT_ADV_CALLBACK, 1, "%4dkHz",   menu_rbw_acb},
  { MT_ADV_CALLBACK, 2, "%4dkHz",   menu_rbw_acb},
  { MT_ADV_CALLBACK, 3, "%4dkHz",   menu_rbw_acb},
  { MT_ADV_CALLBACK, 4, "%4dkHz",   menu_rbw_acb},
  { MT_ADV_CALLBACK, 5, "%4dkHz",   menu_rbw_acb},
  { MT_ADV_CALLBACK, 6, "%4dkHz",   menu_rbw_acb},
#endif  
  { MT_CANCEL,  0, S_LARROW" BACK", NULL },
  { MT_NONE,      0, NULL, NULL } // sentinel
};

#if 0
static const menuitem_t menu_scale_per2[] = {
  { MT_ADV_CALLBACK, 6, "0.1 /",   menu_scale_per_acb},
  { MT_ADV_CALLBACK, 7, "0.2 /",   menu_scale_per_acb},
  { MT_ADV_CALLBACK, 8, "0.05/",   menu_scale_per_acb},
  { MT_ADV_CALLBACK, 9, "0.02/",   menu_scale_per_acb},
  { MT_ADV_CALLBACK,10, "0.01/",   menu_scale_per_acb},
//{ MT_ADV_CALLBACK,11, "0.005/",  menu_scale_per_acb},
//{ MT_SUBMENU,  0, S_RARROW" MORE",   menu_scale_per2},
  { MT_CANCEL,   0, S_LARROW" BACK", NULL },
  { MT_NONE,     0, NULL, NULL } // sentinel
};

static const menuitem_t menu_scale_per[] = {
  { MT_ADV_CALLBACK, 0, " 20/",   menu_scale_per_acb},
  { MT_ADV_CALLBACK, 1, " 10/",   menu_scale_per_acb},
  { MT_ADV_CALLBACK, 2, "  5/",   menu_scale_per_acb},
  { MT_ADV_CALLBACK, 3, "  2/",   menu_scale_per_acb},
  { M_ADVT_CALLBACK, 4, "  1/",   menu_scale_per_acb},
  { MT_ADV_CALLBACK, 5, "0.5/",   menu_scale_per_acb},
  { MT_SUBMENU,  0, S_RARROW" MORE",  menu_scale_per2},
  { MT_CANCEL,   0, S_LARROW" BACK", NULL },
  { MT_NONE,     0, NULL, NULL } // sentinel
};
#endif

#if 0
static const menuitem_t menu_reffer2[] = {
  { MT_FORM | MT_ADV_CALLBACK, 5, "%s",  menu_reffer_acb},
  { MT_FORM | MT_ADV_CALLBACK, 6, "%s",  menu_reffer_acb},
  { MT_FORM | MT_ADV_CALLBACK, 7, "%s",  menu_reffer_acb},
  { MT_FORM | MT_CANCEL,   0, S_LARROW" BACK", NULL },
  { MT_FORM | MT_NONE,     0, NULL, NULL } // sentinel
};
#endif

static const menuitem_t menu_reffer[] = {
  { MT_FORM | MT_ADV_CALLBACK,  0, "%s", menu_reffer_acb},
  { MT_FORM | MT_ADV_CALLBACK,  1, "%s", menu_reffer_acb},
  { MT_FORM | MT_ADV_CALLBACK,  2, "%s", menu_reffer_acb},
  { MT_FORM | MT_ADV_CALLBACK,  3, "%s", menu_reffer_acb},
  { MT_FORM | MT_ADV_CALLBACK,  4, "%s", menu_reffer_acb},
  { MT_FORM | MT_ADV_CALLBACK,  6, "%s", menu_reffer_acb},
  { MT_FORM | MT_ADV_CALLBACK,  7, "%s", menu_reffer_acb},
//{ MT_FORM | MT_SUBMENU,  0, S_RARROW" MORE", menu_reffer2},
  { MT_FORM | MT_CANCEL,   0, S_LARROW" BACK", NULL },
  { MT_FORM | MT_NONE,     0, NULL, NULL } // sentinel
};

static const menuitem_t menu_atten[] = {
  { MT_ADV_CALLBACK | MT_LOW, 0,           "AUTO",    menu_atten_acb},
  { MT_KEYPAD | MT_LOW,   KM_ATTENUATION,  "MANUAL",  "0 - 30dB"},
  { MT_ADV_CALLBACK | MT_HIGH,0,           "0dB",     menu_atten_high_acb},
  { MT_ADV_CALLBACK | MT_HIGH,30,          "22.5 - 40dB",    menu_atten_high_acb},
  { MT_CANCEL, 0,                   S_LARROW" BACK", NULL },
  { MT_FORM | MT_NONE,   0, NULL, NULL } // sentinel
};

static const menuitem_t menu_reflevel[] = {
  { MT_ADV_CALLBACK,0,        "AUTO",    menu_reflevel_acb},
  { MT_KEYPAD,  KM_REFLEVEL,  "MANUAL",  NULL},
  { MT_CANCEL, 0,      S_LARROW" BACK", NULL },
  { MT_NONE,   0, NULL, NULL } // sentinel
};

const menuitem_t menu_marker_search[] = {
  { MT_CALLBACK, 4, "PEAK\n SEARCH",          menu_marker_search_cb },
  { MT_CALLBACK, 0, "MIN\n" S_LARROW" LEFT",  menu_marker_search_cb },
  { MT_CALLBACK, 1, "MIN\n" S_RARROW" RIGHT", menu_marker_search_cb },
  { MT_CALLBACK, 2, "MAX\n" S_LARROW" LEFT",  menu_marker_search_cb },
  { MT_CALLBACK, 3, "MAX\n" S_RARROW" RIGHT", menu_marker_search_cb },
  { MT_KEYPAD,  KM_MARKER,          "ENTER\nFREQUENCY",         NULL},
  { MT_ADV_CALLBACK, 0,            "TRACKING",menu_marker_tracking_acb },
  { MT_CANCEL, 0,           S_LARROW" BACK", NULL },
  { MT_NONE, 0, NULL, NULL } // sentinel
};

const menuitem_t menu_marker_modify[] = {
  { MT_ADV_CALLBACK, M_REFERENCE,   "REFER",    menu_marker_modify_acb},
  { MT_ADV_CALLBACK, M_DELTA,       "DELTA",    menu_marker_modify_acb},
  { MT_ADV_CALLBACK, M_NOISE,       "NOISE",    menu_marker_modify_acb},
  { MT_ADV_CALLBACK, M_TRACKING,    "TRACKING", menu_marker_tracking_acb},
  { MT_ADV_CALLBACK, M_NORMAL,      "NORMAL",   menu_marker_modify_acb},
  { MT_SUBMENU,  0,                 "SEARCH",   menu_marker_search},
  { MT_CALLBACK, M_DELETE,          "DELETE",   menu_marker_delete_cb},
  { MT_CANCEL,   0,          S_LARROW" BACK", NULL },
  { MT_NONE,     0, NULL, NULL } // sentinel
};

#if 0
const menuitem_t menu_marker_sel[] = {
  { MT_CALLBACK, 1, "MARKER %d", menu_marker_sel_cb },
  { MT_CALLBACK, 2, "MARKER %d", menu_marker_sel_cb },
  { MT_CALLBACK, 3, "MARKER %d", menu_marker_sel_cb },
  { MT_CALLBACK, 4, "MARKER %d", menu_marker_sel_cb },
//  { MT_CALLBACK, 0, "ALL OFF", menu_marker_sel_cb },
  { MT_CALLBACK, 0, "DELTA", menu_marker_sel_cb },
  { MT_CALLBACK, 0, "NOISE", menu_marker_sel_cb },
  { MT_CALLBACK, 0, "TRACKING", menu_marker_sel_cb },
  { MT_CANCEL, 0, S_LARROW" BACK", NULL },
  { MT_NONE, 0, NULL, NULL } // sentinel
};
#endif

const menuitem_t menu_marker_select[] = {
  { MT_ADV_CALLBACK, 1, "MARKER %d", menu_marker_select_acb },
  { MT_ADV_CALLBACK, 2, "MARKER %d", menu_marker_select_acb },
  { MT_ADV_CALLBACK, 3, "MARKER %d", menu_marker_select_acb },
  { MT_ADV_CALLBACK, 4, "MARKER %d", menu_marker_select_acb },
  { MT_CANCEL, 0, S_LARROW" BACK", NULL },
  { MT_NONE, 0, NULL, NULL } // sentinel
};


const menuitem_t menu_marker_ops[] = {
  { MT_CALLBACK, ST_START,  S_RARROW" START",    menu_marker_op_cb },
  { MT_CALLBACK, ST_STOP,   S_RARROW" STOP",     menu_marker_op_cb },
  { MT_CALLBACK, ST_CENTER, S_RARROW" CENTER",   menu_marker_op_cb },
  { MT_CALLBACK, ST_SPAN,   S_RARROW" SPAN",     menu_marker_op_cb },
  { MT_CALLBACK, 4,         S_RARROW" REF LEVEL",menu_marker_op_cb },
  { MT_CANCEL, 0,           S_LARROW" BACK",   NULL },
  { MT_NONE, 0, NULL, NULL } // sentinel
};

static const menuitem_t menu_marker[] = {
//  { MT_SUBMENU,  0, "SELECT\nMARKER",     menu_marker_sel},
  { MT_SUBMENU,  0, "MODIFY\nMARKERS",    menu_marker_select},
  { MT_SUBMENU,  0, "MARKER\nOPS", menu_marker_ops},
  { MT_SUBMENU,  0, "SEARCH\nMARKER",     menu_marker_search},
  { MT_CANCEL,   0, S_LARROW" BACK", NULL },
  { MT_NONE,     0, NULL, NULL } // sentinel
};

static const menuitem_t menu_dfu[] = {
  { MT_FORM | MT_CALLBACK, 0, "ENTER DFU",      menu_dfu_cb},
  { MT_FORM | MT_CANCEL,   0, S_LARROW" BACK",  NULL },
  { MT_FORM | MT_NONE,     0, NULL, NULL } // sentinel
};

#ifdef __HARMONIC__
static const menuitem_t menu_harmonic[] =
{
 { MT_ADV_CALLBACK,  0,     "OFF",              menu_harmonic_acb},
  { MT_ADV_CALLBACK, 2,     "2",              menu_harmonic_acb},
  { MT_ADV_CALLBACK, 3,     "3",              menu_harmonic_acb},
  { MT_ADV_CALLBACK, 4,     "4",              menu_harmonic_acb},
  { MT_ADV_CALLBACK, 5,     "5",              menu_harmonic_acb},
  { MT_CANCEL,   0, S_LARROW" BACK", NULL },
  { MT_NONE,     0, NULL, NULL } // sentinel
};
#endif


static const menuitem_t menu_scanning_speed[] =
{
// { MT_ADV_CALLBACK,     SD_NORMAL,  "NORMAL",           menu_scanning_speed_acb},    // order must match definition of enum
// { MT_ADV_CALLBACK,     SD_PRECISE, PRECISE",           menu_scanning_speed_acb},
// { MT_ADV_CALLBACK | MT_LOW,SD_FAST,  "FAST",           menu_scanning_speed_acb},
// { MT_KEYPAD   | MT_LOW,KM_FAST_SPEEDUP,    "FAST\nSPEEDUP",   "2..20"},
 { MT_KEYPAD, KM_SAMPLETIME,        "SAMPLE\nDELAY",   "250..10000, 0=auto"},              // This must be item 4 to match highlighting
 { MT_KEYPAD, KM_OFFSET_DELAY,      "OFFSET\nDELAY",   "250..10000, 0=auto"},              // This must be item 5 to match highlighting
 { MT_CANCEL,   0,                  "\032 BACK", NULL },
 { MT_NONE,     0, NULL, NULL } // sentinel
};

static const menuitem_t menu_sweep_points[] = {
  { MT_ADV_CALLBACK, 0, "%3d point", menu_points_acb },
  { MT_ADV_CALLBACK, 1, "%3d point", menu_points_acb },
  { MT_ADV_CALLBACK, 2, "%3d point", menu_points_acb },
  { MT_ADV_CALLBACK, 3, "%3d point", menu_points_acb },
#ifdef TINYSA4  
  { MT_ADV_CALLBACK, 4, "%3d point", menu_points_acb },
#endif
  { MT_CANCEL, 0, S_LARROW" BACK", NULL },
  { MT_NONE, 0, NULL, NULL } // sentinel
};

static const menuitem_t menu_sweep_speed[] =
{
 { MT_ADV_CALLBACK,     SD_NORMAL,     "NORMAL",          menu_scanning_speed_acb},    // order must match definition of enum
 { MT_ADV_CALLBACK,     SD_PRECISE,    "PRECISE",         menu_scanning_speed_acb},
#ifdef TINYSA4
 { MT_ADV_CALLBACK,     SD_FAST,        "FAST",            menu_scanning_speed_acb},
#else
 { MT_ADV_CALLBACK | MT_LOW,SD_FAST,   "FAST",            menu_scanning_speed_acb},
#endif
 { MT_KEYPAD,           KM_SWEEP_TIME, "SWEEP\nTIME",     "0..600s, 0=disable"},       // This must be item 3 to match highlighting
 { MT_SUBMENU,          0,             "SWEEP\nPOINTS",   menu_sweep_points},
#ifdef TINYSA4
 { MT_KEYPAD,           KM_FAST_SPEEDUP,"FAST\nSPEEDUP",  "2..20, 0=disable"},
#else
 { MT_KEYPAD   | MT_LOW,KM_FAST_SPEEDUP,"FAST\nSPEEDUP",  "2..20, 0=disable"},
#endif
 { MT_CANCEL,   0,             S_LARROW" BACK", NULL },
 { MT_NONE,     0, NULL, NULL } // sentinel
};

#ifdef TINYSA4
static const menuitem_t menu_settings4[];
#endif

static const menuitem_t menu_settings3[] =
{
#ifdef TINYSA4
//  { MT_KEYPAD,   KM_GRIDLINES,  "MINIMUM\nGRIDLINES", "Enter minimum horizontal grid divisions"},
 { MT_ADV_CALLBACK,     0,     "ADF OUT",          menu_adf_out_acb},
  { MT_KEYPAD,   KM_LPF,        "ULTRA\nSTART",   "Enter ULTRA mode start freq"},
//  { MT_KEYPAD | MT_LOW, KM_IF2,  "IF2 FREQ",           "Set to zero for no IF2"},
  { MT_KEYPAD,  KM_R,  "R",           "Set R"},
  { MT_KEYPAD,  KM_MOD,  "MODULO",           "Set MODULO"},
  { MT_KEYPAD,  KM_CP,  "CP",           "Set CP"},
  { MT_ADV_CALLBACK | MT_LOW, 0,    "ULTRA\nMODE",      menu_settings_ultra_acb},
#ifdef __HAM_BAND__
  { MT_ADV_CALLBACK, 0,         "HAM\nBANDS",         menu_settings_ham_bands},
#endif
  { MT_SUBMENU,  0,             S_RARROW" MORE",     menu_settings4},
#else
  { MT_KEYPAD,   KM_10MHZ,      "CORRECT\nFREQUENCY", "Enter actual l0MHz frequency"},
  { MT_KEYPAD,   KM_GRIDLINES,  "MINIMUM\nGRIDLINES", "Enter minimum horizontal grid divisions"},
#ifdef __HAM_BAND__
  { MT_ADV_CALLBACK, 0,         "HAM\nBANDS",         menu_settings_ham_bands},
#endif
#endif		// TINYSA4
  { MT_CANCEL,   0, S_LARROW" BACK", NULL },
  { MT_NONE,     0, NULL, NULL } // sentinel
};
#ifdef TINYSA4
static const menuitem_t menu_settings4[] =
{
 { MT_ADV_CALLBACK,     0,     "DEBUG\nFREQ",          menu_debug_freq_acb},
#if 1                                                                           // only used during development
  { MT_KEYPAD,   KM_COR_AM,     "COR\nAM", "Enter AM modulation correction"},
  { MT_KEYPAD,   KM_COR_WFM,     "COR\nWFM", "Enter WFM modulation correction"},
  { MT_KEYPAD,   KM_COR_NFM,     "COR\nNFM", "Enter NFM modulation correction"},
#endif
#ifdef __HARMONIC__
  { MT_SUBMENU,0,               "HARMONIC",         menu_harmonic},
#endif
  { MT_SUBMENU,  0,             S_RARROW" MORE",     menu_settings3},
 { MT_CANCEL,   0, S_LARROW" BACK", NULL },
  { MT_NONE,     0, NULL, NULL } // sentinel
};
#endif

static const menuitem_t menu_settings2[] =
{
  { MT_ADV_CALLBACK, 0,             "AGC",           menu_settings_agc_acb},
  { MT_ADV_CALLBACK, 0,             "LNA",           menu_settings_lna_acb},
  { MT_ADV_CALLBACK | MT_LOW, 0,    "BPF",           menu_settings_bpf_acb},
  { MT_ADV_CALLBACK | MT_LOW, 0,    "BELOW IF",      menu_settings_below_if_acb},
  { MT_KEYPAD,   KM_DECAY,      "DECAY",   "0..1000000ms or sweeps"},
#ifdef __QUASI_PEAK__
  { MT_KEYPAD,   KM_ATTACK,      "ATTACK",   "0..100000ms"},
#else
  { MT_KEYPAD,   KM_NOISE,      "NOISE\nLEVEL",   "2..20 dB"},
#endif
#ifdef TINYSA4
  { MT_KEYPAD,   KM_30MHZ,      "ACTUAL\n30MHz", "Enter actual l0MHz frequency"},
#endif
  { MT_SUBMENU,  0,             S_RARROW" MORE",     menu_settings3},
  { MT_CANCEL,   0, S_LARROW" BACK", NULL },
  { MT_NONE,     0, NULL, NULL } // sentinel
};

static const menuitem_t menu_settings[] =
{
  { MT_ADV_CALLBACK | MT_LOW, 0,"LO OUTPUT", menu_lo_output_acb},
  { MT_KEYPAD, KM_ACTUALPOWER,  "ACTUAL\nPOWER",  NULL},
  { MT_KEYPAD | MT_LOW, KM_IF,  "IF FREQ",           "0=auto IF"},
  { MT_SUBMENU,0,               "SCAN SPEED",        menu_scanning_speed},
  { MT_KEYPAD, KM_REPEAT,       "SAMPLE\nREPEAT",    "1..100"},
#ifdef TINYSA4
  { MT_SUBMENU | MT_LOW,0,      "MIXER\nDRIVE",      menu_mixer_drive},
#else
  { MT_SUBMENU | MT_LOW,0,      "MIXER\nDRIVE",      menu_lo_drive},
#endif
  { MT_SUBMENU,  0,             S_RARROW" MORE",     menu_settings2},
  { MT_CANCEL,   0,             S_LARROW" BACK", NULL },
  { MT_NONE,     0, NULL, NULL } // sentinel
};

static const menuitem_t menu_measure2[] = {
  { MT_ADV_CALLBACK,            M_AM,           "AM",           menu_measure_acb},
  { MT_ADV_CALLBACK,            M_FM,           "FM",           menu_measure_acb},
  { MT_ADV_CALLBACK,            M_THD,          "THD",           menu_measure_acb},
#ifdef __LINEARITY__
  { MT_ADV_CALLBACK | MT_LOW,   M_LINEARITY,  "LINEAR",         menu_measure_acb},
#endif
  { MT_CANCEL, 0,               S_LARROW" BACK", NULL },
  { MT_NONE,   0, NULL, NULL } // sentinel
};

static const menuitem_t menu_measure[] = {
  { MT_ADV_CALLBACK,            M_OFF,        "OFF",            menu_measure_acb},
  { MT_ADV_CALLBACK,            M_IMD,        "HARMONIC",       menu_measure_acb},
  { MT_ADV_CALLBACK,            M_OIP3,       "OIP3",           menu_measure_acb},
  { MT_ADV_CALLBACK,            M_PHASE_NOISE,"PHASE\nNOISE",   menu_measure_acb},
  { MT_ADV_CALLBACK,            M_STOP_BAND,  "SNR",            menu_measure_acb},
  { MT_ADV_CALLBACK,            M_PASS_BAND,  "-3dB\nWIDTH",     menu_measure_acb},
  { MT_SUBMENU,  0,             S_RARROW" MORE",                menu_measure2},
  { MT_CANCEL, 0,               S_LARROW" BACK", NULL },
  { MT_NONE,   0, NULL, NULL } // sentinel
};

#ifdef __CALIBRATE__
static const menuitem_t menu_calibrate[] =
{
  { MT_FORM | MT_TITLE,      0, "Connect HIGH and LOW",  NULL},
  { MT_FORM | MT_CALLBACK,   0, "CALIBRATE",                 menu_calibrate_cb},
  { MT_FORM | MT_CALLBACK,   0, "RESET CALBRATION",          menu_calibrate_cb},
  { MT_FORM | MT_CANCEL,     0, S_LARROW" BACK",             NULL },
  { MT_FORM | MT_NONE,     0, NULL, NULL } // sentinel
};
#endif

#ifdef __USE_SERIAL_CONSOLE__
//19200, 38400, 57600, 74800, 115200, 230400, 460800, 921600, 1843200, 3686400
#if 0
const menuitem_t menu_serial_speed2[] = {
  { MT_ADV_CALLBACK, USART_SPEED_SETTING( 460800), "%u", menu_serial_speed_acb },
  { MT_ADV_CALLBACK, USART_SPEED_SETTING( 921600), "%u", menu_serial_speed_acb },
  { MT_ADV_CALLBACK, USART_SPEED_SETTING(1843200), "%u", menu_serial_speed_acb },
  { MT_ADV_CALLBACK, USART_SPEED_SETTING(3686400), "%u", menu_serial_speed_acb },
  { MT_CANCEL, 0, S_LARROW" BACK", NULL },
  { MT_NONE, 0, NULL, NULL } // sentinel
};
#endif

const menuitem_t menu_serial_speed[] = {
  { MT_ADV_CALLBACK, USART_SPEED_SETTING( 19200), "%u", menu_serial_speed_acb },
  { MT_ADV_CALLBACK, USART_SPEED_SETTING( 38400), "%u", menu_serial_speed_acb },
  { MT_ADV_CALLBACK, USART_SPEED_SETTING( 57600), "%u", menu_serial_speed_acb },
//  { MT_ADV_CALLBACK, USART_SPEED_SETTING( 76800), "%u", menu_serial_speed_acb },
  { MT_ADV_CALLBACK, USART_SPEED_SETTING(115200), "%u", menu_serial_speed_acb },
  { MT_ADV_CALLBACK, USART_SPEED_SETTING(230400), "%u", menu_serial_speed_acb },
//  { MT_SUBMENU, 0, S_RARROW" MORE", menu_serial_speed2 },
  { MT_ADV_CALLBACK, USART_SPEED_SETTING( 460800), "%u", menu_serial_speed_acb },
  { MT_ADV_CALLBACK, USART_SPEED_SETTING( 921600), "%u", menu_serial_speed_acb },
  { MT_CANCEL, 0, S_LARROW" BACK", NULL },
  { MT_NONE, 0, NULL, NULL } // sentinel
};

const menuitem_t menu_connection[] = {
  { MT_ADV_CALLBACK, _MODE_USB,    "USB",    menu_connection_acb },
  { MT_ADV_CALLBACK, _MODE_SERIAL, "SERIAL", menu_connection_acb },
  { MT_SUBMENU,  0, "SERIAL\nSPEED", menu_serial_speed },
  { MT_CANCEL, 0, S_LARROW" BACK", NULL },
  { MT_NONE, 0, NULL, NULL } // sentinel
};
#endif

const menuitem_t menu_touch[] = {
  { MT_CALLBACK, CONFIG_MENUITEM_TOUCH_CAL,  "TOUCH CAL",  menu_config_cb},
  { MT_CALLBACK, CONFIG_MENUITEM_TOUCH_TEST, "TOUCH TEST", menu_config_cb},
  { MT_CANCEL, 0, S_LARROW" BACK", NULL },
  { MT_NONE, 0, NULL, NULL } // sentinel
};

static const menuitem_t menu_config[] = {
  { MT_SUBMENU,  0,                        "TOUCH",     menu_touch},
  { MT_CALLBACK, CONFIG_MENUITEM_SELFTEST, "SELF TEST", menu_config_cb},
#ifdef __CALIBRATE__
  { MT_SUBMENU,  0,                        "LEVEL CAL", menu_calibrate},
#endif
  { MT_CALLBACK, CONFIG_MENUITEM_VERSION,  "VERSION",   menu_config_cb},
#ifdef __USE_SERIAL_CONSOLE__
  { MT_SUBMENU,  0, "CONNECTION", menu_connection},
#endif
  { MT_SUBMENU,  0, "EXPERT\nCONFIG", menu_settings},
  { MT_SUBMENU,  0, S_RARROW" DFU",  menu_dfu},
  { MT_CANCEL,   0, S_LARROW" BACK", NULL },
  { MT_NONE,     0, NULL, NULL } // sentinel
};

static const menuitem_t menu_storage[] = {
  { MT_ADV_CALLBACK,0,          "STORE\nTRACE",    menu_storage_acb},
  { MT_ADV_CALLBACK,1,          "CLEAR\nSTORED",   menu_storage_acb},
  { MT_ADV_CALLBACK,2,          "SUBTRACT\nSTORED",menu_storage_acb},
  { MT_ADV_CALLBACK,3,          "NORMALIZE",       menu_storage_acb},
  { MT_CANCEL, 0,           S_LARROW" BACK", NULL },
  { MT_NONE,   0, NULL, NULL } // sentinel
};

static const menuitem_t menu_display[] = {
  { MT_ADV_CALLBACK,0,          "PAUSE\nSWEEP",    menu_pause_acb},
  { MT_SUBMENU,     0,          "CALC",            menu_average},
  { MT_SUBMENU,     0,          "STORAGE",         menu_storage},
//  { MT_ADV_CALLBACK,0,          "STORE\nTRACE",    menu_storage_acb},
//  { MT_ADV_CALLBACK,1,          "CLEAR\nSTORED",   menu_storage_acb},
//  { MT_ADV_CALLBACK,2,          "SUBTRACT\nSTORED",menu_storage_acb},
  { MT_ADV_CALLBACK,3,          "NORMALIZE",       menu_storage_acb},
  { MT_ADV_CALLBACK,4,          "WATER\nFALL",     menu_waterfall_acb},
  { MT_SUBMENU, 0,              "SWEEP\nSETTINGS", menu_sweep_speed},
#ifdef __REMOTE_DESKTOP__
  { MT_ADV_CALLBACK,0,          "SEND\nDISPLAY",    menu_send_display_acb},
#endif
//  { MT_KEYPAD,  KM_SWEEP_TIME,  "SWEEP\nTIME",    NULL},

  { MT_CANCEL, 0,           S_LARROW" BACK", NULL },
  { MT_NONE,   0, NULL, NULL } // sentinel
};

static const menuitem_t menu_unit[] =
{
  { MT_ADV_CALLBACK,U_DBM,   "dBm",             menu_unit_acb},
  { MT_ADV_CALLBACK,U_DBMV,  "dBmV",            menu_unit_acb},
  { MT_ADV_CALLBACK,U_DBUV,  "dB"S_MICRO"V",    menu_unit_acb},
  { MT_ADV_CALLBACK,U_VOLT,  "Volt",            menu_unit_acb},
//{ MT_ADV_CALLBACK,U_UVOLT, S_MICRO"Volt",     menu_unit_acb},
  { MT_ADV_CALLBACK,U_WATT,  "Watt",            menu_unit_acb},
//{ MT_ADV_CALLBACK,U_UWATT, S_MICRO"Watt",     menu_unit_acb},
  { MT_CANCEL, 0,            S_LARROW" BACK", NULL },
  { MT_NONE,   0, NULL, NULL } // sentinel
};

static const menuitem_t menu_trigger[] = {
  { MT_ADV_CALLBACK, T_AUTO,     "AUTO",           menu_trigger_acb},
  { MT_ADV_CALLBACK, T_NORMAL,   "NORMAL",         menu_trigger_acb},
  { MT_ADV_CALLBACK, T_SINGLE,   "SINGLE",         menu_trigger_acb},
//  { MT_ADV_CALLBACK, T_DONE,     "READY",          menu_trigger_acb},
  { MT_KEYPAD,       KM_TRIGGER, "TRIGGER\nLEVEL", NULL},
  { MT_ADV_CALLBACK, T_UP,       "UP\nEDGE",       menu_trigger_acb},
  { MT_ADV_CALLBACK, T_DOWN,     "DOWN\nEDGE",     menu_trigger_acb},
  { MT_ADV_CALLBACK, T_MODE,     "%s\nTRIGGER",     menu_trigger_acb},
  { MT_CANCEL, 0,                S_LARROW" BACK",      NULL },
  { MT_NONE,   0, NULL, NULL } // sentinel
};

static const menuitem_t menu_level[] = {
  { MT_SUBMENU, 0,              "REF LEVEL",    menu_reflevel},
//{ MT_SUBMENU, 0,              "SCALE/DIV",    menu_scale_per},
  { MT_KEYPAD,  KM_SCALE,       "SCALE/DIV",    NULL},
  { MT_SUBMENU, 0,              "ATTENUATE",    menu_atten},
//  { MT_SUBMENU,0,             "CALC",         menu_average},
  { MT_SUBMENU, 0,              "UNIT",         menu_unit},
  { MT_KEYPAD,  KM_OFFSET,      "EXTERNAL\nAMP",NULL},
#ifdef TINYSA4
  { MT_ADV_CALLBACK | MT_LOW ,0,"LNA",          menu_extra_lna_acb},
 #endif
  { MT_SUBMENU,  0,             "TRIGGER",      menu_trigger},
  { MT_CANCEL, 0,               S_LARROW" BACK",NULL },
  { MT_NONE,   0, NULL, NULL } // sentinel
};

static const menuitem_t menu_stimulus[] = {
  { MT_KEYPAD,  KM_START,       "START",       NULL},
  { MT_KEYPAD,  KM_STOP,        "STOP",        NULL},
  { MT_KEYPAD,  KM_CENTER,      "CENTER",      NULL},
  { MT_KEYPAD,  KM_SPAN,        "SPAN",        NULL},
  { MT_KEYPAD,  KM_CW,          "ZERO SPAN",   NULL},
  { MT_SUBMENU,0,               "RBW",         menu_rbw},
#ifdef __SPUR__
  { MT_ADV_CALLBACK,0,          "%s",          menu_spur_acb},
#endif
  { MT_CANCEL,  0,              S_LARROW" BACK", NULL },
  { MT_NONE,    0, NULL, NULL } // sentinel
};

static const menuitem_t menu_mode[] = {
//  { MT_FORM | MT_TITLE,                 0,                      "tinySA MODE",           NULL},
  { MT_FORM | MT_ADV_CALLBACK | MT_ICON,    I_LOW_INPUT+I_SA,       "%s to LOW in",      menu_mode_acb},
  { MT_FORM | MT_ADV_CALLBACK | MT_ICON,    I_HIGH_INPUT+I_SA,      "%s to HIGH in",     menu_mode_acb},
  { MT_FORM | MT_ADV_CALLBACK | MT_ICON,    I_LOW_OUTPUT+I_SINUS,   "%s to LOW out",     menu_mode_acb},
  { MT_FORM | MT_ADV_CALLBACK | MT_ICON,    I_HIGH_OUTPUT+I_GEN,    "%s to HIGH out",    menu_mode_acb},
  { MT_FORM | MT_ADV_CALLBACK | MT_ICON,    I_CONNECT+I_GEN,        "Cal. output: %s",   menu_sreffer_acb},
//  { MT_SUBMENU,  0, "EXPERT\nCONFIG", menu_settings3},

//  { MT_FORM | MT_CANCEL,   0, S_RARROW" BACK", NULL },
  { MT_FORM | MT_NONE,     0, NULL, NULL } // sentinel
};

static const menuitem_t menu_top[] = {
  { MT_SUBMENU,  0, "PRESET",       menu_load_preset},
  { MT_SUBMENU,  0, "FREQUENCY",    menu_stimulus},
  { MT_SUBMENU,  0, "LEVEL",        menu_level},
  { MT_SUBMENU,  0, "DISPLAY",      menu_display},
  { MT_SUBMENU,  0, "MARKER",       menu_marker},
  { MT_SUBMENU,  0, "MEASURE",      menu_measure},
  { MT_SUBMENU,  0, "CONFIG",       menu_config},
  { MT_SUBMENU,  0, "MODE",         menu_mode},
  { MT_NONE,     0, NULL, NULL } // sentinel,
 // MENUITEM_CLOSE,
};

// ===[MENU DEFINITION END]======================================================

#define ACTIVE_COLOR RGBHEX(0x007FFF)

static bool menu_is_form(const menuitem_t *menu)
{
#if 1
  // Not good set only one item as form and others as normal
  return menu[0].type & MT_FORM;
#else
  int i;
  for (i = 0; MT_MASK(menu[i].type) != MT_NONE; i++)
    if (menu[i].type & MT_FORM)
      return (true);
  return(false);
#endif
}

static void menu_item_modify_attribute(
    const menuitem_t *menu, int item, ui_button_t *button)
{
  if (menu == menu_settings) {
    if (item == 2)
      button->icon = setting.auto_IF ? BUTTON_ICON_CHECK_AUTO : BUTTON_ICON_CHECK_MANUAL;
  } else if (menu == menu_scanning_speed) {
    if (item == 0)
      button->icon = setting.step_delay > 0 ? BUTTON_ICON_CHECK_MANUAL : BUTTON_ICON_CHECK_AUTO;
    else if (item == 1)
      button->icon =setting.offset_delay > 0 ? BUTTON_ICON_CHECK_MANUAL : BUTTON_ICON_CHECK_AUTO;
  } else if (menu == menu_sweep_speed) {
    if (item == 3)
      button->icon = setting.sweep_time_us != 0 ? BUTTON_ICON_CHECK_MANUAL : BUTTON_ICON_CHECK_AUTO;
    else if (item == 5)
      button->icon = setting.fast_speedup != 0 ? BUTTON_ICON_CHECK_MANUAL : BUTTON_ICON_CHECK_AUTO;
  } else if (menu == menu_reflevel) {
    if (item == 1)
      button->icon = setting.auto_reflevel ? BUTTON_ICON_GROUP: BUTTON_ICON_GROUP_CHECKED;
  } else if (menu == menu_atten) {
    if (item == 1)
      button->icon = setting.auto_attenuation ? BUTTON_ICON_GROUP: BUTTON_ICON_GROUP_CHECKED;
  }
}

static void fetch_numeric_target(void)
{
  switch (keypad_mode) {
  case KM_START:
    uistat.value = get_sweep_frequency(ST_START);
    plot_printf(uistat.text, sizeof uistat.text, "%3.3fMHz", uistat.value / 1000000.0);
    break;
  case KM_STOP:
    uistat.value = get_sweep_frequency(ST_STOP);
    plot_printf(uistat.text, sizeof uistat.text, "%3.3fMHz", uistat.value / 1000000.0);
    break;
  case KM_CENTER:
    uistat.value = get_sweep_frequency(ST_CENTER);
    plot_printf(uistat.text, sizeof uistat.text, "%3.4fMHz", uistat.value / 1000000.0);
    break;
  case KM_SPAN:
    uistat.value = get_sweep_frequency(ST_SPAN);
    plot_printf(uistat.text, sizeof uistat.text, "%3.3fMHz", uistat.value / 1000000.0);
    break;
  case KM_CW:
    uistat.value = get_sweep_frequency(ST_CW);
    plot_printf(uistat.text, sizeof uistat.text, "%3.3fMHz", uistat.value / 1000000.0);
    break;
  case KM_SCALE:
    uistat.value = setting.scale;
    plot_printf(uistat.text, sizeof uistat.text, "%f/", uistat.value);
    break;
  case KM_REFLEVEL:
    uistat.value = setting.reflevel;
    plot_printf(uistat.text, sizeof uistat.text, "%f", uistat.value);
    break;
  case KM_ATTENUATION:
    uistat.value = get_attenuation();
    plot_printf(uistat.text, sizeof uistat.text, "%ddB", ((int32_t)uistat.value));
     break;
  case KM_ACTUALPOWER:
    uistat.value = get_level_offset();
    plot_printf(uistat.text, sizeof uistat.text, "%ddB", ((int32_t)uistat.value));
    break;
  case KM_IF:
    uistat.value = setting.frequency_IF;
    plot_printf(uistat.text, sizeof uistat.text, "%3.3fMHz", uistat.value / 1000000.0);
    break;
#ifdef TINYSA4
  case KM_IF2:
    uistat.value = config.frequency_IF2;
    plot_printf(uistat.text, sizeof uistat.text, "%3.3fMHz", uistat.value / 1000000.0);
    break;
  case KM_R:
    uistat.value = SI4463_R;
    plot_printf(uistat.text, sizeof uistat.text, "%3d", uistat.value);
    break;
  case KM_MOD:
    uistat.value = ADF4350_modulo;
    plot_printf(uistat.text, sizeof uistat.text, "%4d", uistat.value);
    break;
#endif
  case KM_SAMPLETIME:
    uistat.value = setting.step_delay;
    plot_printf(uistat.text, sizeof uistat.text, "%3dus", ((int32_t)uistat.value));
    break;
  case KM_REPEAT:
    uistat.value = setting.repeat;
    plot_printf(uistat.text, sizeof uistat.text, "%2d", ((int32_t)uistat.value));
    break;
  case KM_LOWOUTLEVEL:
    uistat.value = get_level();           // compensation for dB offset during low output mode
    float end_level =  ((int32_t)uistat.value)+setting.level_sweep;
    if (end_level < level_min)
      end_level = level_min;
    if (end_level > level_max)
      end_level = level_max;
    uistat.value += setting.offset;
    end_level += setting.offset;
    if (setting.level_sweep != 0)
      plot_printf(uistat.text, sizeof uistat.text, "%.1f to %.1fdBm", uistat.value, end_level);
    else
      plot_printf(uistat.text, sizeof uistat.text, "%.1fdBm", uistat.value);
    break;
  case KM_HIGHOUTLEVEL:
    uistat.value = get_level();           // compensation for dB offset during low output mode
    uistat.value += setting.offset;
    plot_printf(uistat.text, sizeof uistat.text, "%.1fdBm", uistat.value);
    break;
  case KM_DECAY:
    uistat.value = setting.decay;
    plot_printf(uistat.text, sizeof uistat.text, "%5d", ((int32_t)uistat.value));
    break;
#ifdef __QUASI_PEAK__
  case KM_ATTACK:
    uistat.value = setting.attack;
    plot_printf(uistat.text, sizeof uistat.text, "%5d", ((int32_t)uistat.value));
    break;
#endif
#ifdef TINYSA4
  case KM_LPF:
    uistat.value = config.ultra_threshold;
    plot_printf(uistat.text, sizeof uistat.text, "%3.6fMHz", uistat.value / 1000000.0);
    break;
#endif
  case KM_NOISE:
    uistat.value = setting.noise;
    plot_printf(uistat.text, sizeof uistat.text, "%3d", ((int32_t)uistat.value));
    break;
#ifdef TINYSA4
  case KM_30MHZ:
    uistat.value = config.setting_frequency_30mhz;
    plot_printf(uistat.text, sizeof uistat.text, "%3.6fMHz", uistat.value / 1000000.0);
    break;
#else
  case KM_10MHZ:
    uistat.value = config.setting_frequency_10mhz;
    plot_printf(uistat.text, sizeof uistat.text, "%3.6fMHz", uistat.value / 1000000.0);
    break;
#endif
  case KM_OFFSET:
    uistat.value = setting.offset;
    plot_printf(uistat.text, sizeof uistat.text, "%.1fdB", uistat.value);
    break;
  case KM_LEVELSWEEP:
    uistat.value = setting.level_sweep;
    plot_printf(uistat.text, sizeof uistat.text, "%.1fdB", uistat.value);
    break;
  case KM_SWEEP_TIME:
//    if (setting.sweep_time_us < calc_min_sweep_time_us())
//      uistat.value = calc_min_sweep_time_us();
//    else
      uistat.value = setting.sweep_time_us;
    uistat.value /= (float)ONE_SECOND_TIME;
    plot_printf(uistat.text, sizeof uistat.text, "%.3Fs", uistat.value);
    break;
  case KM_TRIGGER:
    uistat.value = setting.trigger_level;
    plot_printf(uistat.text, sizeof uistat.text, "%.1fdB", uistat.value);
    break;
  case KM_MARKER:
    if (active_marker >=0) {
      uistat.value = markers[active_marker].frequency;
      plot_printf(uistat.text, sizeof uistat.text, "%3.3fMHz", uistat.value / 1000000.0);
    }
    break;
  case KM_MODULATION:
    if (active_marker >=0) {
      uistat.value = setting.modulation_frequency;
      plot_printf(uistat.text, sizeof uistat.text, "%7.0fHz", uistat.value);
    }
    break;
  }
  
  {
    freq_t x = uistat.value;
    int n = 0;
    for (; x >= 10 && n < 9; n++)
      x /= 10;
    uistat.digit = n;
  }
//  uistat.previous_value = uistat.value;
}

static void
set_numeric_value(void)
{
  switch (keypad_mode) {
  case KM_START:
    set_sweep_frequency(ST_START, (freq_t)uistat.value);
    break;
  case KM_STOP:
    set_sweep_frequency(ST_STOP, (freq_t)uistat.value);
    break;
  case KM_CENTER:
    set_sweep_frequency(ST_CENTER, (freq_t)uistat.value);
    break;
  case KM_SPAN:
    setting.modulation = MO_NONE;
    set_sweep_frequency(ST_SPAN, (freq_t)uistat.value);
    break;
  case KM_CW:
    set_sweep_frequency(ST_CW, (freq_t)uistat.value);
    break;
  case KM_SCALE:
    user_set_scale(uistat.value);
    break;
  case KM_REFLEVEL:
    user_set_reflevel(uistat.value);
    break;
  case KM_ATTENUATION:
    setting.auto_attenuation = false;
    set_attenuation(uistat.value);
    break;
  case KM_ACTUALPOWER:
    set_actual_power(uistat.value);
    config_save();
    break;
  case KM_IF:
    setting.auto_IF = false;
    set_IF(uistat.value);
//    config_save();
    break;
#ifdef TINYSA4
  case KM_IF2:
    set_IF2(uistat.value);
//    config_save();
    break;
  case KM_R:
    set_R(uistat.value);
//    config_save();
    break;
  case KM_MOD:
    set_modulo(uistat.value);
    break;
  case KM_CP:
    ADF4351_CP((int)uistat.value);
//    config_save();
    break;
#endif
  case KM_SAMPLETIME:
    set_step_delay(uistat.value);
    break;
  case KM_OFFSET_DELAY:
    set_offset_delay(uistat.value);
    break;
  case KM_FAST_SPEEDUP:
    set_fast_speedup(uistat.value);
    break;
  case KM_REPEAT:
    set_repeat(uistat.value);
    break;
  case KM_LOWOUTLEVEL:
    set_level(uistat.value - setting.offset);
    break;
  case KM_HIGHOUTLEVEL:
    set_level(uistat.value - setting.offset);
    break;
  case KM_DECAY:
    set_decay(uistat.value);
    break;
#ifdef __QUASI_PEAK__
  case KM_ATTACK:
    set_attack(uistat.value);
    break;
#endif
#ifdef TINYSA4
  case KM_LPF:
    config.ultra_threshold = uistat.value;
    config_save();
    break;
#endif
  case KM_NOISE:
    set_noise(uistat.value);
    break;
#ifdef TINYSA4
  case KM_30MHZ:
    set_30mhz(uistat.value);
    break;
#else
  case KM_10MHZ:
    set_10mhz(uistat.value);
    break;
#endif
  case KM_OFFSET:
    set_offset(uistat.value);
    break;
  case KM_LEVELSWEEP:
    setting.modulation = MO_NONE;
    set_level_sweep(uistat.value);
    break;
  case KM_SWEEP_TIME:
    set_sweep_time_us(uistat.value*ONE_SECOND_TIME);
    update_grid();
    break;
  case KM_TRIGGER:
    if (setting.trigger == T_AUTO )
      set_trigger(T_NORMAL);
    set_trigger_level(to_dBm(uistat.value));
    completed = true;

    break;
  case KM_GRIDLINES:
    set_gridlines(uistat.value);
    break;
  case KM_MARKER:
    set_marker_frequency(active_marker, (freq_t)uistat.value);
    break;
  case KM_MODULATION:
    set_modulation_frequency((int)uistat.value);
    break;
#ifdef TINYSA4
  case KM_COR_AM:
    config.cor_am = -(int)uistat.value;
    config_save();
    dirty = true;
    break;
  case KM_COR_WFM:
    config.cor_wfm = -(int)uistat.value;
    config_save();
    dirty = true;
    break;
  case KM_COR_NFM:
    config.cor_nfm = -(int)uistat.value;
    config_save();
    dirty = true;
    break;
#endif
  }
}

void
menu_move_top(void)
{
  while (menu_current_level > 0)
    menu_move_back(false);
}


// -------------------------- CAL STATUS ---------------------------------------------
const char * const averageText[] = { "OFF", "MIN", "MAX", "MAXD", " A 4", "A 16","QUASI"};
const char * const dBText[] = { "1dB/", "2dB/", "5dB/", "10dB/", "20dB/"};
const int refMHz[] = { 30, 15, 10, 4, 3, 2, 1 };

float my_round(float v)
{
  float m = 1;
  int sign = 1;
  if (v < 0) {
    sign = -1;
    v = -v;
  }
  while (v < 100) {
    v = v * 10;
    m = m / 10;
  }
  while (v > 1000) {
    v = v / 10;
    m = m * 10;
  }
  v = (int)(v+0.5);
  v = v * m;
  if (sign == -1) {
    v = -v;
  }
  return v;
}

const char * const unit_string[] = { "dBm", "dBmV", "dB"S_MICRO"V", "V", "W", "dBc", "dBc", "dBc", "Vc", "Wc" }; // unit + 5 is delta unit

static const float scale_value[]={50000, 20000, 10000, 5000, 2000, 1000, 500, 200, 100, 50, 20,10,5,2,1,0.5,0.2,0.1,0.05,0.02,0.01,0.005,0.002, 0.001,0.0005,0.0002, 0.0001};
static const char * const scale_vtext[]= {"50000", "20000", "10000", "5000", "2000", "1000", "500", "200", "100", "50", "20","10","5","2","1","0.5","0.2","0.1","0.05","0.02","0.01", "0.005","0.002","0.001", "0.0005","0.0002","0.0001"};

// Quick menu
#define MAX_QUICK_MENU  20
#define MAX_ITEM_SPACE   2
static uint16_t    quick_menu_y[MAX_QUICK_MENU];
static menuitem_t  *quick_menu[MAX_QUICK_MENU];
static uint8_t max_quick_menu = 0;
static uint8_t item_space = 0; //

int invoke_quick_menu(int y)
{
  int i;
  for (i = 0; i < max_quick_menu;i++) {
    if (y < quick_menu_y[i]) {
      if ((uint32_t)quick_menu[i] < KM_NONE) {
        ui_mode_keypad((int)quick_menu[i]);
      } else {
        selection = -1;
        menu_current_level = 0;
        menu_push_submenu(quick_menu[i]);
      }
      return TRUE;
    }
  }
  return FALSE;
}
#define YSTEP   8

int add_quick_menu(char *buf, int x, int y, menuitem_t *menu)
{
  ili9341_drawstring(buf, x, y);
  y += YSTEP*item_space/2 + YSTEP;
  if (max_quick_menu<MAX_QUICK_MENU-1) {
    quick_menu_y[max_quick_menu] = y;
    quick_menu[max_quick_menu++] = menu;
  }
  return y;
}

void draw_cal_status(void)
{
#define BLEN    7
  char buf[BLEN+1];
  buf[6]=0;
  int x = 0;
  int y = OFFSETY;
  unsigned int color;
  const bool rounding = !UNIT_IS_LINEAR(setting.unit);
  const char * const unit = unit_string[setting.unit];
redraw_cal_status:
  buf[6]=0;
  x = 0;
  y = OFFSETY;
  ili9341_set_background(LCD_BG_COLOR);
  ili9341_fill(0, 0, OFFSETX, LCD_HEIGHT);
  max_quick_menu = 0;
  if (MODE_OUTPUT(setting.mode)) {     // No cal status during output
    return;
  }

    //  if (current_menu_is_form() && !in_selftest)
//    return;

  ili9341_set_background(LCD_BG_COLOR);

  float yMax = setting.reflevel;
  // Ref level
  if (rounding)
    plot_printf(buf, BLEN, "%+4d", (int)yMax);
  else
    plot_printf(buf, BLEN, "%+4.3F", (yMax/setting.unit_scale));

  if (level_is_calibrated())
    color = setting.auto_reflevel ? LCD_FG_COLOR : LCD_BRIGHT_COLOR_GREEN;
  else
    color = LCD_BRIGHT_COLOR_RED;
  ili9341_set_foreground(color);
  y = add_quick_menu(buf, x, y, (menuitem_t *)menu_reflevel);

  // Unit
#if 0
  color = LCD_FG_COLOR;
  ili9341_set_foreground(color);
  if (setting.auto_reflevel){
    y += YSTEP + YSTEP/2 ;
    ili9341_drawstring("AUTO", x, y);
  }
#endif
  plot_printf(buf, BLEN, "%s%s",unit_scale_text[setting.unit_scale_index], unit);
  y = add_quick_menu(buf, x, y, (menuitem_t *)menu_unit);

  // Scale
  color = LCD_FG_COLOR;
  ili9341_set_foreground(color);
#if 1
  unsigned int i = 0;
  while (i < sizeof(scale_value)/sizeof(float)) {
    float t = (setting.scale/setting.unit_scale) / scale_value[i];;
    if (t > 0.9 && t < 1.1){
      plot_printf(buf, BLEN, "%s%s/",scale_vtext[i],unit_scale_text[setting.unit_scale_index]);
      break;
    }
    i++;
  }
#else
  plot_printf(buf, BLEN, "%.2F/",setting.scale);
#endif
  y = add_quick_menu(buf, x, y, (menuitem_t *)KM_SCALE);

  // Trigger status
  if (is_paused()) {
    color = LCD_BRIGHT_COLOR_GREEN;
    ili9341_set_foreground(color);
    ili9341_drawstring("PAUSED", x, y);
    y += YSTEP + YSTEP/2 ;
  }
  if (setting.trigger == T_SINGLE || setting.trigger == T_NORMAL ) {
    color = LCD_BRIGHT_COLOR_GREEN;
    ili9341_set_foreground(color);
    ili9341_drawstring("ARMED", x, y);
    y += YSTEP + YSTEP/2 ;
  }
  // AM warning
  if (signal_is_AM) {
    color = LCD_BRIGHT_COLOR_RED;
    ili9341_set_foreground(color);
    ili9341_drawstring("AM", x, y);
    y += YSTEP + YSTEP/2 ;
  }
  quick_menu_y[max_quick_menu] = y;
  quick_menu[max_quick_menu++] = (menuitem_t *)NULL;

//  if (setting.mode == M_LOW) {
    // Attenuation
    if (setting.auto_attenuation)
      color = LCD_FG_COLOR;
    else
      color = LCD_BRIGHT_COLOR_GREEN;
    ili9341_set_foreground(color);
    ili9341_drawstring("Atten:", x, y);
    y += YSTEP;
    plot_printf(buf, BLEN, "%.2FdB", get_attenuation());
    y = add_quick_menu(buf, x, y, (menuitem_t *)menu_atten);
//  }

  // Calc
  if (setting.average>0) {
    ili9341_set_foreground(LCD_BRIGHT_COLOR_GREEN);
    ili9341_drawstring("Calc:", x, y);
    y += YSTEP;
    plot_printf(buf, BLEN, "%s",averageText[setting.average]);
    y = add_quick_menu(buf, x, y, (menuitem_t *)menu_average);
  }
  // Spur
#ifdef __SPUR__
  if (setting.spur_removal != S_OFF) {
    if (setting.spur_removal == S_ON)
      ili9341_set_foreground(LCD_BRIGHT_COLOR_GREEN);
    else
      color = LCD_FG_COLOR;

    ili9341_drawstring("Spur:", x, y);
    y += YSTEP;
    if (S_IS_AUTO(setting.spur_removal))
      y = add_quick_menu("AUTO", x, y, (menuitem_t *)menu_stimulus);
    else
      y = add_quick_menu("ON", x, y, (menuitem_t *)menu_stimulus);
  }
  if (setting.mirror_masking) {
    ili9341_set_foreground(LCD_BRIGHT_COLOR_GREEN);
    ili9341_drawstring("Mask:", x, y);

    y += YSTEP;
    y = add_quick_menu("ON", x, y, (menuitem_t *)menu_stimulus);
  }
#endif

  if (setting.subtract_stored) {
    ili9341_set_foreground(LCD_BRIGHT_COLOR_GREEN);
    y = add_quick_menu("Norm.", x, y, (menuitem_t *)menu_storage);
  }

  // RBW
  if (setting.rbw_x10)
    color = LCD_BRIGHT_COLOR_GREEN;
  else
    color = LCD_FG_COLOR;
  ili9341_set_foreground(color);

  ili9341_drawstring("RBW:", x, y);
  y += YSTEP;
  plot_printf(buf, BLEN, "%.1FHz", actual_rbw_x10*100.0);
  y = add_quick_menu(buf, x, y,(menuitem_t *)menu_rbw);

#if 0
  // VBW
  if (setting.frequency_step > 0) {
    ili9341_set_foreground(LCD_FG_COLOR);
    y += YSTEP + YSTEP/2 ;
    ili9341_drawstring("VBW:", x, y);

    y += YSTEP;
    plot_printf(buf, BLEN, "%dkHz",(int)setting.vbw_x10/10.0);
    buf[6]=0;
    ili9341_drawstring(buf, x, y);
  }
#endif
  // Sweep time
  if (setting.step_delay != 0)
    color = LCD_BRIGHT_COLOR_GREEN;
  else
    color = LCD_FG_COLOR;
  ili9341_set_foreground(color);
  buf[0] = ' ';
  strcpy(&buf[1],"Scan:");
  if (setting.step_delay_mode == SD_PRECISE)
    buf[0] = 'P';
  else if (setting.step_delay_mode == SD_FAST)
    buf[0] = 'F';
  else
    strcpy(&buf[0],"Scan:");
  ili9341_drawstring(buf, x, y);

#if 0                   // Activate for sweep time debugging
  y += YSTEP;
  plot_printf(buf, BLEN, "%5.3Fs", (float)setting.sweep_time_us/ONE_SECOND_TIME);
  ili9341_drawstring(buf, x, y);
#endif
  y += YSTEP;
  plot_printf(buf, BLEN, "%5.3Fs", (float)setting.actual_sweep_time_us/ONE_SECOND_TIME);
  y = add_quick_menu(buf, x, y, (menuitem_t *)menu_sweep_speed);


  #if 0                   // Activate for sweep time debugging
  y += YSTEP;
  update_rbw();             // To ensure the calc_min_sweep time shown takes the latest delay into account
  calculate_step_delay();
  uint32_t t = calc_min_sweep_time_us();
  plot_printf(buf, BLEN, "%5.3Fs", (float)t/ONE_SECOND_TIME);
  ili9341_drawstring(buf, x, y);
  y += YSTEP;
  plot_printf(buf, BLEN, "%5.3Fs", (float)setting.additional_step_delay_us/ONE_SECOND_TIME);
  ili9341_drawstring(buf, x, y);
  y += YSTEP + YSTEP/2 ;
#endif
#ifdef TINYSA4
  if (setting.extra_lna){
    ili9341_set_foreground(LCD_BRIGHT_COLOR_GREEN);
    y = add_quick_menu("LNA:ON", x, y, (menuitem_t *)menu_level);
    y += YSTEP;
  }
#endif
  // Cal output
  if (setting.refer >= 0) {
    ili9341_set_foreground(LCD_BRIGHT_COLOR_GREEN);
    ili9341_drawstring("Ref:", x, y);
    y += YSTEP;
    plot_printf(buf, BLEN, "%dMHz",reffer_freq[setting.refer]/1000000);
    buf[6]=0;
    y = add_quick_menu(buf, x, y,(menuitem_t *)menu_reffer);
  }

  // Offset
  if (setting.offset != 0.0) {
    ili9341_set_foreground(LCD_BRIGHT_COLOR_GREEN);
    ili9341_drawstring("Amp:", x, y);
    y += YSTEP;
    plot_printf(buf, BLEN, "%.1fdB",setting.offset);
    y = add_quick_menu(buf, x, y,(menuitem_t *)KM_OFFSET);
  }

  // Repeat
  if (setting.repeat != 1) {
    ili9341_set_foreground(LCD_BRIGHT_COLOR_GREEN);
    ili9341_drawstring("Repeat:", x, y);
    y += YSTEP;
    plot_printf(buf, BLEN, "%d",setting.repeat);
    buf[6]=0;
    y = add_quick_menu(buf, x, y,(menuitem_t *)KM_REPEAT);
  }

  // Trigger
  if (setting.trigger != T_AUTO) {
    if (is_paused() || setting.trigger == T_NORMAL) {
      ili9341_set_foreground(LCD_BRIGHT_COLOR_GREEN);
    } else {
      ili9341_set_foreground(LCD_BRIGHT_COLOR_RED);
    }
    ili9341_drawstring("TRIG:", x, y);

    y += YSTEP;
    if (rounding)
      plot_printf(buf, BLEN, "%4f", value(setting.trigger_level));
    else
      plot_printf(buf, BLEN, "%.4F", value(setting.trigger_level));
//    plot_printf(buf, BLEN, "%4f", value(setting.trigger_level)/setting.unit_scale);
    y = add_quick_menu(buf, x, y,(menuitem_t *)menu_trigger);
  }

  // Mode
  if (level_is_calibrated())
    color = LCD_BRIGHT_COLOR_GREEN;
  else
    color = LCD_BRIGHT_COLOR_RED;
  ili9341_set_foreground(color);
  ili9341_drawstring_7x13(MODE_LOW(setting.mode) ? "LOW" : "HIGH", x, y);

  // Compact status string
//  ili9341_set_background(LCD_FG_COLOR);
  ili9341_set_foreground(LCD_FG_COLOR);
  y += YSTEP + YSTEP/2 ;
  strncpy(buf,"     ",BLEN-1);
  if (setting.auto_attenuation)
    buf[0] = 'a';
  else
    buf[0] = 'A';
  if (setting.auto_IF)
    buf[1] = 'f';
  else
    buf[1] = 'F';
  if (setting.auto_reflevel)
    buf[2] = 'r';
  else
    buf[2] = 'R';
  if (S_IS_AUTO(setting.agc))
    buf[3] = 'g';
  else if (S_STATE(setting.agc))
    buf[3] = 'G';
  if (S_IS_AUTO(setting.lna))
    buf[4] = 'n';
  else if (S_STATE(setting.lna))
    buf[4] = 'N';
  if (S_IS_AUTO(setting.below_IF))
    buf[5] = 'b';
  else if (S_STATE(setting.below_IF))
    buf[5] = 'B';
  ili9341_drawstring(buf, x, y);

  // Version
  y += YSTEP + YSTEP/2 ;
#ifdef TINYSA4
  strncpy(buf,&VERSION[11], BLEN-1);
#else
  strncpy(buf,&VERSION[8], BLEN-1);
#endif
  ili9341_drawstring(buf, x, y);


  if (y >= BATTERY_START && item_space > 0) {
    item_space--;                       // Reduce item spacing
    goto redraw_cal_status;
  }
  if ((y + (max_quick_menu+1) * YSTEP/2) < BATTERY_START && item_space < MAX_ITEM_SPACE) {
    item_space++;                       // Increase item spacing
    goto redraw_cal_status;
  }

//  ili9341_set_background(LCD_BG_COLOR);
  if (!get_waterfall()) {               // Do not draw bottom level if in waterfall mode
    // Bottom level
    y = area_height + OFFSETY;
    if (rounding)
      plot_printf(buf, BLEN, "%4d", (int)(yMax - setting.scale * NGRIDY));
    else
      plot_printf(buf, BLEN, "%+4.3F", ((yMax - setting.scale * NGRIDY)/setting.unit_scale));
    //  buf[5]=0;
    if (level_is_calibrated())
      if (setting.auto_reflevel)
        color = LCD_FG_COLOR;
      else
        color = LCD_BRIGHT_COLOR_GREEN;
    else
      color = LCD_BRIGHT_COLOR_RED;
    ili9341_set_foreground(color);
    ili9341_drawstring(buf, x, y);
  }
}

