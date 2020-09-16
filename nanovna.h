/* Copyright (c) 2014-2015, TAKAHASHI Tomohiro (TTRFTECH) edy555@gmail.com
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
#include "ch.h"

#ifdef TINYSA_F303
#include "adc_F303.h"
#endif
// Need enable HAL_USE_SPI in halconf.h
#define __USE_DISPLAY_DMA__

#define __SA__
#define __SI4432__
#define __PE4302__
//#define __SIMULATION__
//#define __PIPELINE__
#define __SCROLL__
#define __ICONS__
#define __MEASURE__
#define __SELFTEST__
#define __CALIBRATE__
#define __FAST_SWEEP__          // Pre-fill SI4432 RSSI buffer  to get fastest sweep in zero span mode
#define __HAM_BAND__
//#define __ULTRA__             // Add harmonics mode on low input.
//#define __ULTRA_SA__            // Adds ADF4351 control for extra high 1st IF stage
#define __SPUR__                // Does spur reduction by shifting IF
#define __USE_SERIAL_CONSOLE__  // Enable serial I/O connection (need enable HAL_USE_SERIAL as TRUE in halconf.h)
/*
 * main.c
 */
#ifdef __SA__
#define POINTS_COUNT     290
#define MARKER_COUNT    4

#define TRACES_MAX 3
#define TRACE_AGE       3
#define TRACE_ACTUAL    2
#define TRACE_STORED    1
#define TRACE_TEMP      0
// #define age_t     measured[TRACE_AGE]
#define stored_t  measured[TRACE_STORED]
#define actual_t  measured[TRACE_ACTUAL]
#define temp_t    measured[TRACE_TEMP]

#define CORRECTION_POINTS  10       // Frequency dependent level correction table entries

typedef float measurement_t[TRACES_MAX][POINTS_COUNT];
extern measurement_t measured;
#endif

#ifdef __VNA__
// Minimum frequency set
#define START_MIN                50000
// Maximum frequency set
#define STOP_MAX                 2700000000U
// Frequency offset (sin_cos table in dsp.c generated for this offset, if change need create new table)
#define FREQUENCY_OFFSET         5000
// Speed of light const
#define SPEED_OF_LIGHT           299792458
// pi const
#define VNA_PI                   3.14159265358979323846

#define POINTS_COUNT 101
extern float measured[2][POINTS_COUNT][2];

#define CAL_LOAD 0
#define CAL_OPEN 1
#define CAL_SHORT 2
#define CAL_THRU 3
#define CAL_ISOLN 4

#define CALSTAT_LOAD (1<<0)
#define CALSTAT_OPEN (1<<1)
#define CALSTAT_SHORT (1<<2)
#define CALSTAT_THRU (1<<3)
#define CALSTAT_ISOLN (1<<4)
#define CALSTAT_ES (1<<5)
#define CALSTAT_ER (1<<6)
#define CALSTAT_ET (1<<7)
#define CALSTAT_ED CALSTAT_LOAD
#define CALSTAT_EX CALSTAT_ISOLN
#define CALSTAT_APPLY (1<<8)
#define CALSTAT_INTERPOLATED (1<<9)

#define ETERM_ED 0 /* error term directivity */
#define ETERM_ES 1 /* error term source match */
#define ETERM_ER 2 /* error term refrection tracking */
#define ETERM_ET 3 /* error term transmission tracking */
#define ETERM_EX 4 /* error term isolation */

#define DOMAIN_MODE (1<<0)
#define DOMAIN_FREQ (0<<0)
#define DOMAIN_TIME (1<<0)
#define TD_FUNC (0b11<<1)
#define TD_FUNC_BANDPASS (0b00<<1)
#define TD_FUNC_LOWPASS_IMPULSE (0b01<<1)
#define TD_FUNC_LOWPASS_STEP    (0b10<<1)
#define TD_WINDOW (0b11<<3)
#define TD_WINDOW_NORMAL (0b00<<3)
#define TD_WINDOW_MINIMUM (0b01<<3)
#define TD_WINDOW_MAXIMUM (0b10<<3)

#define FFT_SIZE 256

void cal_collect(int type);
void cal_done(void);
#endif
#define MAX_FREQ_TYPE 5
enum stimulus_type {
  ST_START=0, ST_STOP, ST_CENTER, ST_SPAN, ST_CW
};

void set_sweep_points(uint16_t points);
void update_frequencies(void);
void set_sweep_frequency(int type, uint32_t frequency);
uint32_t get_sweep_frequency(int type);
void my_microsecond_delay(int t);
float my_atof(const char *p);
int shell_printf(const char *fmt, ...);

void set_marker_frequency(int m, uint32_t f);
void toggle_sweep(void);
void toggle_mute(void);
void load_default_properties(void);

enum {
  AV_OFF, AV_MIN, AV_MAX_HOLD, AV_MAX_DECAY, AV_4, AV_16
};
enum {
  M_LOW, M_HIGH, M_GENLOW, M_GENHIGH, M_ULTRA
};

enum {
  MO_NONE, MO_AM_1kHz, MO_AM_10Hz, MO_NFM, MO_WFM, MO_EXTERNAL,
};

#define MODE_OUTPUT(x)  ((x) == M_GENLOW || (x) == M_GENHIGH )
#ifdef __ULTRA__
#define MODE_INPUT(x)  ((x) == M_LOW || (x) == M_HIGH || (x) == M_ULTRA )
#else
#define MODE_INPUT(x)  ((x) == M_LOW || (x) == M_HIGH )
#endif
#define MODE_HIGH(x)  ((x) == M_HIGH || (x) == M_GENHIGH )
#define MODE_LOW(x)  ((x) == M_LOW || (x) == M_GENLOW )
#define MODE_SELECT(x) (MODE_HIGH(x) ? 1 : 0)

#define SWEEP_ENABLE    0x01
#define SWEEP_ONCE      0x02
#define SWEEP_CALIBRATE 0x04
#define SWEEP_SELFTEST  0x08
#define SWEEP_REMOTE    0x10
//#define SWEEP_FACTORY    0x20


extern int8_t sweep_mode;
extern bool completed;
extern const char *info_about[];

// ------------------------------- sa_core.c ----------------------------------
void reset_settings(int);
void update_min_max_freq(void);
//void ui_process_touch(void);
void SetPowerGrid(int);
void SetRefLevel(float);
void set_refer_output(int);
void toggle_below_IF(void);
int get_refer_output(void);
void set_attenuation(float);
float get_attenuation(void);
void set_harmonic(int);
//extern int setting.harmonic;
int search_is_greater(void);
void set_auto_attenuation(void);
void set_auto_reflevel(int);
int is_paused(void);
void set_actual_power(float);
void SetGenerate(int);
void set_RBW(uint32_t rbw_x10);
void set_drive(int d);
void set_IF(int f);
void set_step_delay(int t);
void set_offset_delay(int t);
void set_repeat(int);
void set_level_sweep(float);
void set_level(float);
void set_sweep_time_us(uint32_t);
//extern int setting.repeat;
//extern int setting.rbw;
#ifdef __SPUR__
//extern int setting.spur;
void set_spur(int v);
void toggle_mirror_masking(void);
#endif
void set_average(int);
int GetAverage(void);
//extern int setting.average;
void  set_storage(void);
void  set_clear_storage(void);
void  set_subtract_storage(void);
void  toggle_normalize(void);
void toggle_waterfall(void);
void set_mode(int);
int GetMode(void);
void set_reflevel(float);
void user_set_reflevel(float);
#define REFLEVEL_MAX 9999.0
#define REFLEVEL_MIN    1.0e-12
void set_scale(float);
void user_set_scale(float);
void AllDirty(void);
void MenuDirty(void);
void toggle_LNA(void);
void toggle_AGC(void);
void redrawHisto(void);
void self_test(int);
void set_decay(int);
void set_noise(int);
void toggle_tracking_output(void);
extern int32_t frequencyExtra;
void set_10mhz(uint32_t);
void set_modulation(int);
//extern int setting.modulation;
void set_measurement(int);
// extern int settingSpeed;
//extern int setting.step_delay;
void sweep_remote(void);
#ifdef __VNA__
/*
 * dsp.c
 */
// 5ms @ 48kHz
#define AUDIO_BUFFER_LEN 96

extern int16_t rx_buffer[];

#define STATE_LEN 32
#define SAMPLE_LEN 48

#ifdef ENABLED_DUMP
extern int16_t ref_buf[];
extern int16_t samp_buf[];
#endif

void dsp_process(int16_t *src, size_t len);
void reset_dsp_accumerator(void);
void calculate_gamma(float *gamma);
void fetch_amplitude(float *gamma);
void fetch_amplitude_ref(float *gamma);
#endif

#ifdef __VNA__
/*
 * tlv320aic3204.c
 */

extern void tlv320aic3204_init(void);
extern void tlv320aic3204_set_gain(int lgain, int rgain);
extern void tlv320aic3204_select(int channel);

#endif
/*
 * plot.c
 */

// Offset of plot area
#define OFFSETX 30
#define OFFSETY 0

#define NGRIDY 10
// GRIDX calculated depends from frequency span
#ifdef __SCROLL__
extern  uint16_t _grid_y;
#define GRIDY  _grid_y
extern uint16_t graph_bottom;
#define BIG_WATERFALL   90
#define SMALL_WATERFALL 180
#define NO_WATERFALL    CHART_BOTTOM
#define CHART_BOTTOM   230
#define SCROLL_GRIDY      (HEIGHT_SCROLL / NGRIDY)
#define NOSCROLL_GRIDY    (CHART_BOTTOM / NGRIDY)
#else
#define GRIDY             (230 / NGRIDY)
#endif

#define WIDTH  (LCD_WIDTH - 1 - OFFSETX)
#define HEIGHT (GRIDY*NGRIDY)

#define CELLWIDTH  (32)
#define CELLHEIGHT (32)

#define FREQUENCIES_XPOS1 OFFSETX
#define FREQUENCIES_XPOS2 200
#define FREQUENCIES_YPOS  (LCD_HEIGHT-7)

//
#define CELLOFFSETX 0
#define AREA_WIDTH_NORMAL  (CELLOFFSETX + WIDTH  + 1)
#define AREA_HEIGHT_NORMAL (              HEIGHT + 1)

#define GRID_X_TEXT       (AREA_WIDTH_NORMAL - 7*5)

// Smith/polar chart
#define P_CENTER_X (CELLOFFSETX + WIDTH/2)
#define P_CENTER_Y (HEIGHT/2)
#define P_RADIUS   (HEIGHT/2)

// Menu Button
// Maximum menu buttons count
#define MENU_BUTTON_MAX         8
#define MENU_BUTTON_WIDTH      80
#define MENU_BUTTON_HEIGHT     28
#define MENU_BUTTON_BORDER      1
#define KEYBOARD_BUTTON_BORDER  2
#define FORM_BUTTON_BORDER      2

// Form button (at center screen better be less LCD_WIDTH - 2*OFFSETX)
#define MENU_FORM_WIDTH    256

// Num Input height at bottom
#define NUM_INPUT_HEIGHT   32

extern int16_t area_width;
extern int16_t area_height;

// Define marker size (can be 0 or 1)
#define _MARKER_SIZE_         0
// font
extern const uint8_t x5x7_bits [];
extern const uint8_t x7x11b_bits [];
extern const uint8_t x10x14_bits[];
extern const uint8_t numfont16x22[];

#define FONT_START_CHAR    0x17
#define FONT_MAX_WIDTH        7
#define FONT_WIDTH            5
#define FONT_GET_HEIGHT       7
#define FONT_STR_HEIGHT       8
#define FONT_GET_DATA(ch)    (  &x5x7_bits[(ch-FONT_START_CHAR)*FONT_GET_HEIGHT])
#define FONT_GET_WIDTH(ch)   (8-(x5x7_bits[(ch-FONT_START_CHAR)*FONT_GET_HEIGHT]&7))

#define bFONT_START_CHAR   0x17
#define bFONT_MAX_WIDTH       8
#define bFONT_WIDTH           7
#define bFONT_GET_HEIGHT     11
#define bFONT_STR_HEIGHT     11

#define bFONT_GET_DATA(ch)   (  &x7x11b_bits[(ch-bFONT_START_CHAR)*bFONT_GET_HEIGHT])
#define bFONT_GET_WIDTH(ch)  (8-(x7x11b_bits[(ch-bFONT_START_CHAR)*bFONT_GET_HEIGHT]&7))

#define wFONT_START_CHAR   0x17
#define wFONT_MAX_WIDTH      12
#define wFONT_GET_HEIGHT     14
#define wFONT_STR_HEIGHT     16
#define wFONT_GET_DATA(ch)   (   &x10x14_bits[(ch-wFONT_START_CHAR)*2*wFONT_GET_HEIGHT  ])
#define wFONT_GET_WIDTH(ch)  (14-(x10x14_bits[(ch-wFONT_START_CHAR)*2*wFONT_GET_HEIGHT+1]&0x7))

#define NUM_FONT_GET_WIDTH      16
#define NUM_FONT_GET_HEIGHT     22
#define NUM_FONT_GET_DATA(ch)   (&numfont16x22[ch*2*NUM_FONT_GET_HEIGHT])

#if 1
#define KP_WIDTH                  ((LCD_WIDTH) / 4)// numeric keypad button width
#define KP_HEIGHT                 ((LCD_HEIGHT - NUM_INPUT_HEIGHT) / 4) // numeric keypad button height
// Key x, y position (0 - 15) on screen
#define KP_GET_X(posx)            ((posx) * KP_WIDTH)                   // numeric keypad left
#define KP_GET_Y(posy)            ((posy) * KP_HEIGHT)                  // numeric keypad top
#else
#define KP_WIDTH     (LCD_HEIGHT/5)
#define KP_HEIGHT    (LCD_HEIGHT/5)
// Key x, y position (0 - 15) on screen
#define KP_GET_X(posx) ((posx)*KP_WIDTH + (LCD_WIDTH-MENU_BUTTON_WIDTH-5-KP_WIDTH*4))
#define KP_GET_Y(posy) ((posy)*KP_HEIGHT + 12 )
#endif

#define S_DELTA    "\027"  // 0x17
#define S_SARROW   "\030"  // 0x18
#define S_INFINITY "\031"  // 0x19
#define S_LARROW   "\032"  // 0x1A
#define S_RARROW   "\033"  // 0x1B
#define S_PI       "\034"  // 0x1C
#define S_MICRO    "\035"  // 0x1D
#define S_OHM      "\036"  // 0x1E
#define S_DEGREE   "\037"  // 0x1F

// trace 
#define MAX_TRACE_TYPE 12
enum trace_type {
  TRC_LOGMAG=0, TRC_PHASE, TRC_DELAY, TRC_SMITH, TRC_POLAR, TRC_LINEAR, TRC_SWR, TRC_REAL, TRC_IMAG, TRC_R, TRC_X, TRC_OFF
};
// Mask for define rectangular plot
#define RECTANGULAR_GRID_MASK ((1<<TRC_LOGMAG)|(1<<TRC_PHASE)|(1<<TRC_DELAY)|(1<<TRC_LINEAR)|(1<<TRC_SWR)|(1<<TRC_REAL)|(1<<TRC_IMAG)|(1<<TRC_R)|(1<<TRC_X))

// LOGMAG: SCALE, REFPOS, REFVAL
// PHASE: SCALE, REFPOS, REFVAL
// DELAY: SCALE, REFPOS, REFVAL
// SMITH: SCALE, <REFPOS>, <REFVAL>
// LINMAG: SCALE, REFPOS, REFVAL
// SWR: SCALE, REFPOS, REFVAL

// Electrical Delay
// Phase

#define MAX_UNIT_TYPE 4
enum unit_type {
  U_DBM=0, U_DBMV, U_DBUV, U_VOLT, U_WATT, U_DBC //  dBc only for displaying delta marker info
};
#define UNIT_IS_LINEAR(T) ( T >= U_VOLT ? true : false)
#define UNIT_IS_LOG(T) ( T >= U_VOLT ? false : true)

float value(float);

typedef struct trace {
  uint8_t enabled;
  uint8_t type;
  uint8_t channel;
  uint8_t reserved;
  float scale;
  float refpos;
} trace_t;

#define FREQ_MODE_START_STOP    0x0
#define FREQ_MODE_CENTER_SPAN   0x1
#define FREQ_MODE_DOTTED_GRID   0x2

// Connection flag
#define _MODE_CONNECTION_MASK  0x04
#define _MODE_SERIAL           0x04
#define _MODE_USB              0x00

typedef struct config {
  int32_t magic;
  uint16_t dac_value;
  uint16_t grid_color;
  uint16_t menu_normal_color;
  uint16_t menu_active_color;
  uint16_t trace_color[TRACES_MAX];
  int16_t  touch_cal[4];
  uint8_t  _mode;
  uint32_t _serial_speed;
#ifdef __VNA__
  uint32_t harmonic_freq_threshold;
#endif
  uint16_t vbat_offset;
  float low_level_offset;
  float high_level_offset;
  uint32_t correction_frequency[CORRECTION_POINTS];
  float    correction_value[CORRECTION_POINTS];
  uint32_t deviceid;
  uint16_t ham_color;
  uint16_t gridlines;
  uint16_t hambands;
//  uint8_t _reserved[22];
  uint32_t checksum;
} config_t;

extern config_t config;
//#define settingLevelOffset config.level_offset
float get_level_offset(void);

void set_trace_type(int t, int type);
void set_trace_channel(int t, int channel);
void set_trace_scale(float scale);
void set_trace_refpos(float refpos);
float get_trace_scale(int t);
float get_trace_refpos(int t);
const char *get_trace_typename(int t);
extern int in_selftest;

//
// Shell config functions and macros
// Serial connect definitions not used if Serial mode disabled
// Minimum speed - USART_SPEED_MULTIPLIER
// Maximum speed - USART_SPEED_MULTIPLIER * 256
// Can be: 19200, 38400, 57600, 76800, 115200, 230400, 460800, 921600, 1843200, 3686400
#define USART_SPEED_MULTIPLIER          19200
#define USART_SPEED_SETTING(speed)     ((speed)/USART_SPEED_MULTIPLIER - 1)
#define USART_GET_SPEED(idx)           (((idx) + 1) * USART_SPEED_MULTIPLIER)
void shell_update_speed(void);
void shell_reset_console(void);

#ifdef __VNA
void set_electrical_delay(float picoseconds);
float get_electrical_delay(void);
float groupdelay_from_array(int i, float array[POINTS_COUNT][2]);
#endif
// marker
enum {
  M_NORMAL=0,M_REFERENCE=1, M_DELTA=2, M_NOISE=4, M_TRACKING=8, M_DELETE=16  // Tracking must be last.
};

enum {
  M_DISABLED = false, M_ENABLED = true
};

typedef struct {
  int8_t enabled;
  int8_t mtype;
  int16_t index;
  uint32_t frequency;
} marker_t;

#define MARKERS_MAX 4

extern int8_t previous_marker;
extern int8_t marker_tracking;

void plot_init(void);
void update_grid(void);
void request_to_redraw_grid(void);
void redraw_frame(void);
//void redraw_all(void);
void request_to_draw_cells_behind_menu(void);
void request_to_draw_cells_behind_numeric_input(void);
void redraw_marker(int marker);
void markmap_all_markers(void);
void plot_into_index(measurement_t measured);
void force_set_markmap(void);
void draw_frequencies(void);
void draw_all(bool flush);

void draw_cal_status(void);

//void markmap_all_markers(void);

void marker_position(int m, int t, int *x, int *y);
int search_nearest_index(int x, int y, int t);
void set_marker_search(int mode);
int marker_search(void);
int marker_search_max(void);
int marker_search_left(int from);
int marker_search_right(int from);
int marker_search_left_max(int from);
int marker_search_right_max(int from);
int marker_search_left_min(int from);
int marker_search_right_min(int from);

// _request flag for update screen
#define REDRAW_CELLS      (1<<0)
#define REDRAW_FREQUENCY  (1<<1)
#define REDRAW_CAL_STATUS (1<<2)
#define REDRAW_MARKER     (1<<3)
#define REDRAW_BATTERY    (1<<4)
#define REDRAW_AREA       (1<<5)
#define REDRAW_TRIGGER    (1<<6)
extern volatile uint8_t redraw_request;

/*
 * ili9341.c
 */
// SPI bus revert byte order
//gggBBBbb RRRrrGGG
#define byteReverse16(x) (uint16_t)(((x) << 8) & 0xff00) | (((x) >> 8) & 0xff)
#define RGB565(r,g,b)     byteReverse16( ((((uint16_t)(r))<<8)&0b1111100000000000) | ((((uint16_t)(g))<<3)&0b0000011111100000) | ((((uint16_t)(b))>>3)&0b0000000000011111) )

//#define RGB565(r,g,b)  ( (((g)&0x1c)<<11) | (((b)&0xf8)<<5) | ((r)&0xf8) | (((g)&0xe0)>>5) )
#define RGBHEX(hex) ( (((hex)&0x001c00)<<3) | (((hex)&0x0000f8)<<5) | (((hex)&0xf80000)>>16) | (((hex)&0x00e000)>>13) )

// Define size of screen buffer in pixels (one pixel 16bit size)
#define SPI_BUFFER_SIZE             (CELLWIDTH*CELLHEIGHT)

#define LCD_WIDTH                   320
#define LCD_HEIGHT                  240

#define DEFAULT_FG_COLOR            RGB565(255,255,255)
#define DEFAULT_BG_COLOR            RGB565(  0,  0,  0)
#define DARK_GREY                   RGB565(140,140,140)
#define LIGHT_GREY                  RGB565(220,220,220)
#define DEFAULT_GRID_COLOR          RGB565(128,128,128)
#define DEFAULT_HAM_COLOR           RGB565(80,80,80)
#define DEFAULT_GRID_VALUE_COLOR    RGB565(196,196,196)
#define DEFAULT_MENU_COLOR          RGB565(255,255,255)
#define DEFAULT_MENU_TEXT_COLOR     RGB565(  0,  0,  0)
#define DEFAULT_MENU_ACTIVE_COLOR   RGB565(180,255,180)
#define DEFAULT_TRACE_1_COLOR       RGB565(255,  0,  0)  /* RGB565(255,255,  0) */
#define DEFAULT_TRACE_2_COLOR       RGB565(  0,255,  0)/* RGB565(  0,255,255) */
#define DEFAULT_TRACE_3_COLOR       RGB565(255,255,  0)/* RGB565(  0,255,  0) */
#define DEFAULT_TRIGGER_COLOR       RGB565(  0,  0,255)/* RGB565(  0   0,255) */
//#define DEFAULT_TRACE_4_COLOR       RGB565(255,  0,255)
#define DEFAULT_NORMAL_BAT_COLOR    RGB565( 31,227,  0)
#define DEFAULT_LOW_BAT_COLOR       RGB565(255,  0,  0)
#define DEFAULT_SPEC_INPUT_COLOR    RGB565(128,255,128);
#define BRIGHT_COLOR_BLUE  RGB565(0,0,255)
#define BRIGHT_COLOR_RED  RGB565(255,128,128)
#define BRIGHT_COLOR_GREEN  RGB565(0,255,0)

extern uint16_t foreground_color;
extern uint16_t background_color;

extern uint16_t spi_buffer[SPI_BUFFER_SIZE];

// Used for easy define big Bitmap as 0bXXXXXXXXX image
#define _BMP8(d)                                                        ((d)&0xFF)
#define _BMP16(d)                                      (((d)>>8)&0xFF), ((d)&0xFF)
#define _BMP24(d)                    (((d)>>16)&0xFF), (((d)>>8)&0xFF), ((d)&0xFF)
#define _BMP32(d)  (((d)>>24)&0xFF), (((d)>>16)&0xFF), (((d)>>8)&0xFF), ((d)&0xFF)

void ili9341_init(void);
void ili9341_test(int mode);
void ili9341_bulk(int x, int y, int w, int h);
void ili9341_fill(int x, int y, int w, int h, uint16_t color);

#if 1
void ili9341_set_foreground(uint16_t fg);
void ili9341_set_background(uint16_t fg);
#else
#define ili9341_set_foreground(fg) {  foreground_color = fg; }
#define ili9341_set_background(bg) {  background_color = bg;}
#endif

void ili9341_clear_screen(void);
void blit8BitWidthBitmap(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t *bitmap);
void blit16BitWidthBitmap(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *bitmap);
void ili9341_drawchar(uint8_t ch, int x, int y);
void ili9341_drawstring(const char *str, int x, int y);
void ili9341_drawstring_7x13(const char *str, int x, int y);
void ili9341_drawstring_10x14(const char *str, int x, int y);
void ili9341_drawstringV(const char *str, int x, int y);
int  ili9341_drawchar_size(uint8_t ch, int x, int y, uint8_t size);
void ili9341_drawstring_size(const char *str, int x, int y, uint8_t size);
void ili9341_drawfont(uint8_t ch, int x, int y);
void ili9341_read_memory(int x, int y, int w, int h, int len, uint16_t* out);
void ili9341_line(int x0, int y0, int x1, int y1);
void show_version(void);

/*
 * flash.c
 */


typedef struct setting
{
  uint32_t magic;
//  uint32_t _frequency0;
//  uint32_t _frequency1;
  int mode;
  uint16_t _sweep_points;
  float attenuate;
  int auto_attenuation;
  int atten_step;
  uint32_t rbw_x10;
  int below_IF;
  int average;
  int show_stored;
  int subtract_stored;
  int drive; // 0-7 , 7=+20dBm, 3dB steps
  int agc;
  int lna;
  int auto_reflevel;
  float reflevel;
  float scale;
  int tracking;
  int modulation;
  int step_delay;
  uint32_t frequency_step;
  int test;
  int harmonic;
  int decay;
  int noise;
  uint32_t vbw_x10;
  int  tracking_output;
  int repeat;
  uint32_t frequency0;
  uint32_t frequency1;
  uint32_t frequency_IF;
  int freq_mode;
  int measurement;
  int refer;
  int spur_removal;
  int mirror_masking;
  trace_t _trace[TRACES_MAX];
  marker_t _markers[MARKERS_MAX];
  int8_t _active_marker;
  int8_t unit;
  float offset;
  float trigger_level;
  int trigger_direction;
  int trigger;
  int linearity_step;
  float level;
  float level_sweep;
  uint32_t sweep_time_us;
  systime_t measure_sweep_time_us;
  uint32_t actual_sweep_time_us;
  uint32_t additional_step_delay_us;
  int test_argument;
  int auto_IF;
  unsigned int unit_scale_index;
  float unit_scale;
  int mute;
  int step_delay_mode;
  int offset_delay;
  int fast_speedup;
  uint32_t checksum;
}setting_t;

extern setting_t setting;

extern int setting_frequency_10mhz;
void reset_settings(int m);


#define S_IS_AUTO(x) ((x)&2)
#define S_STATE(X) ((X)&1)
enum { S_OFF=0, S_ON=1, S_AUTO_OFF=2, S_AUTO_ON=3 };

enum { SD_NORMAL, SD_PRECISE, SD_FAST, SD_MANUAL };

#ifdef __FAST_SWEEP__
#define MINIMUM_SWEEP_TIME  1800U    // Minimum sweep time on zero span in uS
#else
#define MINIMUM_SWEEP_TIME  15000U   // Minimum sweep time on zero span in uS
#endif
#define MAXIMUM_SWEEP_TIME  600000000U // Maximum sweep time uS
#define ONE_SECOND_TIME     1000000U // One second uS
#define ONE_MS_TIME         1000U    // One ms uS

#define REPEAT_TIME         111         // Time per extra repeat in uS
#define MEASURE_TIME        127         // Time per single point measurement with vbwstep =1 without step delay in uS

extern uint32_t frequencies[POINTS_COUNT];
extern const float unit_scale_value[];
extern const char * const unit_scale_text[];

#if 1   // Still sufficient flash
// Flash save area - flash7  : org = 0x0801B000, len = 20k in *.ld file
// 2k - for config save
// 9 * 2k for setting_t + stored trace
#define SAVEAREA_MAX 9
// STM32 minimum page size for write
#define FLASH_PAGESIZE          0x800
// config save area (flash7 addr)
#define SAVE_CONFIG_ADDR        0x0801B000
#define SAVE_CONFIG_SIZE        0x00000800
// setting_t save area (save area + config size)
#define SAVE_PROP_CONFIG_ADDR   (SAVE_CONFIG_ADDR + SAVE_CONFIG_SIZE)
#define SAVE_PROP_CONFIG_SIZE   0x00000800
// Should include all save slots
#define SAVE_CONFIG_AREA_SIZE   (SAVE_CONFIG_SIZE + SAVEAREA_MAX * SAVE_PROP_CONFIG_SIZE)
#else           // Just in case flash runs out
// Flash save area - flash7  : org = 0x0801D000, len = 12k in *.ld file
// 2k - for config save
// 9 * 2k for setting_t + stored trace
#define SAVEAREA_MAX 5
// STM32 minimum page size for write
#define FLASH_PAGESIZE          0x800
// config save area (flash7 addr)
#define SAVE_CONFIG_ADDR        0x0801D000
#define SAVE_CONFIG_SIZE        0x00000800
// setting_t save area (save area + config size)
#define SAVE_PROP_CONFIG_ADDR   (SAVE_CONFIG_ADDR + SAVE_CONFIG_SIZE)
#define SAVE_PROP_CONFIG_SIZE   0x00000800
// Should include all save slots
#define SAVE_CONFIG_AREA_SIZE   (SAVE_CONFIG_SIZE + SAVEAREA_MAX * SAVE_PROP_CONFIG_SIZE)
#endif

#if 0
typedef struct properties {
  uint32_t magic;
  preset_t setting;
//  uint32_t _frequency0;
//  uint32_t _frequency1;
  uint16_t _sweep_points;
#ifdef __VNA__
  uint16_t _cal_status;

#endif
#ifdef __SA__
//  uint32_t _frequency_IF; //IF frequency
#endif
//  uint32_t _frequencies[POINTS_COUNT];
#ifdef __VNA__
  float _cal_data[5][POINTS_COUNT][2];
  float _electrical_delay; // picoseconds
#endif
  trace_t _trace[TRACES_MAX];
  marker_t _markers[MARKERS_MAX];
  int8_t _active_marker;
#ifdef __VNA__
  float _velocity_factor; // %
  uint8_t _domain_mode; /* 0bxxxxxffm : where ff: TD_FUNC m: DOMAIN_MODE */
  uint8_t _marker_smith_format;
  uint8_t _bandwidth;
#endif  
  uint8_t _reserved[2];
  uint32_t checksum;
} properties_t;

#endif

//sizeof(properties_t) == 0x1200

#define CONFIG_MAGIC 0x434f4e46 /* 'CONF' */

extern int16_t lastsaveid;
//extern properties_t *active_props;

//extern properties_t current_props;

//#define frequency0 current_props._frequency0
//#define frequency1 current_props._frequency1
#define sweep_points setting._sweep_points
#ifdef __VNA__
#define cal_status current_props._cal_status
#endif
#ifdef __SA__
//#define frequency_IF current_props._frequency_IF
#endif
//#define frequencies current_props._frequencies
#ifdef __VNA__
#define cal_data active_props->_cal_data
#define electrical_delay current_props._electrical_delay
#endif
#define trace setting._trace
#define markers setting._markers
#define active_marker setting._active_marker
#ifdef __VNA__
#define domain_mode current_props._domain_mode
#define velocity_factor current_props._velocity_factor
#define marker_smith_format current_props._marker_smith_format
#define bandwidth current_props._bandwidth
#endif

#define FREQ_IS_STARTSTOP() (!(setting.freq_mode&FREQ_MODE_CENTER_SPAN))
#define FREQ_IS_CENTERSPAN() (setting.freq_mode&FREQ_MODE_CENTER_SPAN)
#define FREQ_IS_CW() (setting.frequency0 == setting.frequency1)
int caldata_recall(uint16_t id);
int caldata_save(uint16_t id);
//const properties_t *caldata_ref(int id);
int config_save(void);
int config_recall(void);

void clear_all_config_prop_data(void);

/*
 * ui.c
 */
extern void ui_init(void);
extern void ui_process(void);
int current_menu_is_form(void);

void ui_mode_normal(void);
void ui_mode_menu(void);
void menu_push_lowoutput(void);
void menu_push_highoutput(void);
void menu_move_top(void);
void draw_menu(void);
int check_touched(void);

// Irq operation process set
#define OP_NONE       0x00
#define OP_LEVER      0x01
#define OP_TOUCH      0x02
#define OP_CONSOLE    0x04
//#define OP_FREQCHANGE 0x04
extern volatile uint8_t operation_requested;

// lever_mode
enum lever_mode {
  LM_MARKER, LM_SEARCH, LM_CENTER, LM_SPAN, LM_EDELAY
};

// marker smith value format
enum marker_smithvalue {
  MS_LIN, MS_LOG, MS_REIM, MS_RX, MS_RLC
};

typedef struct uistat {
  float value; // for editing at numeric input area
  int8_t digit; /* 0~5 */
  int8_t digit_mode;
  int8_t current_trace; /* 0..3 */
//  uint32_t previous_value;
  uint8_t lever_mode;
  uint8_t marker_delta;
  uint8_t marker_noise;
  uint8_t marker_tracking;
  char text[20];
} uistat_t;

typedef struct ui_button {
  uint16_t fg;
  uint16_t bg;
  uint8_t  border;
  int8_t   icon;
  union {
    int32_t  i;
    uint32_t u;
    const char *text;
  } param_1, param_2;    // void data for label printf
  char text[32];
} ui_button_t;

extern uistat_t uistat;
void ui_init(void);
void ui_show(void);
void ui_hide(void);

void touch_start_watchdog(void);
void touch_position(int *x, int *y);
void handle_touch_interrupt(void);

#define TOUCH_THRESHOLD 2000

void touch_cal_exec(void);
void touch_draw_test(void);
void enter_dfu(void);

/*
 * adc.c
 */
#define ADC_TOUCH_X  ADC_CHSELR_CHSEL6
#define ADC_TOUCH_Y  ADC_CHSELR_CHSEL7

void adc_init(void);
uint16_t adc_single_read(uint32_t chsel);
void adc_start_analog_watchdogd(uint32_t chsel);
void adc_stop(void);
void adc_interrupt(void);
int16_t adc_vbat_read(void);

/*
 * misclinous
 */
int plot_printf(char *str, int, const char *fmt, ...);
#define PULSE do { palClearPad(GPIOC, GPIOC_LED); palSetPad(GPIOC, GPIOC_LED);} while(0)
//extern int setting_attenuate;
//extern int settingPowerCal;
//extern int setting_step_delay;
//extern int actualStepDelay;
//extern int setting_mode;

// Speed profile definition
#define START_PROFILE   systime_t time = chVTGetSystemTimeX();
#define RESTART_PROFILE   time = chVTGetSystemTimeX();
#define STOP_PROFILE    {char string_buf[12];plot_printf(string_buf, sizeof string_buf, "%06d", chVTGetSystemTimeX() - time);ili9341_drawstring(string_buf, 0, FREQUENCIES_YPOS);}
#define DELTA_TIME (time = chVTGetSystemTimeX() - time)
// Macros for convert define value to string
#define STR1(x)  #x
#define define_to_STR(x)  STR1(x)

// sa_core.c

typedef uint8_t  deviceRSSI_t;
typedef int16_t  pureRSSI_t;

// RSSI values conversion macro
// External programm zero level settings (need decrease on this value -)
#define EXT_ZERO_LEVEL            (128)
#define DEVICE_TO_PURE_RSSI(rssi) ((rssi)<<4)
#define float_TO_PURE_RSSI(rssi)  ((rssi)*32)
#define PURE_TO_float(rssi)       ((rssi)/32.0)

extern uint16_t actual_rbw_x10;

int get_waterfall(void);
void toggle_tracking(void);
void toggle_hambands(void);
void reset_calibration(void);
void set_reflevel(float);
void set_offset(float);
void set_unit(int);
void set_switches(int);
void set_gridlines(int);
void set_trigger_level(float);
void set_trigger(int);
void update_rbw(void);
void set_fast_speedup(int);
//extern int setting_measurement;
void self_test(int);
//extern int setting_test;
void wait_user(void);
void calibrate(void);
float to_dBm(float);
uint32_t calc_min_sweep_time_us(void);
pureRSSI_t perform(bool b, int i, uint32_t f, int e);

enum {
  M_OFF, M_IMD, M_OIP3, M_PHASE_NOISE, M_STOP_BAND, M_PASS_BAND, M_LINEARITY, M_AM, M_FM
};

enum {
  T_AUTO, T_NORMAL, T_SINGLE, T_DONE, T_UP, T_DOWN
};
/*EOF*/
