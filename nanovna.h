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
#ifdef TINYSA_F072
#error "Remove comment for #ifdef TINYSA_F303"
#endif
#ifndef TINYSA4
#define TINYSA4
#endif
#define TINYSA4_PROTO
#endif

#ifdef TINYSA_F072
#ifdef TINYSA_F303
#error "Remove comment for #ifdef TINYSA_F072"
#endif
#ifndef TINYSA3
#define TINYSA3
#endif
#endif
// Need enable HAL_USE_SPI in halconf.h
#define __USE_DISPLAY_DMA__

//#define __DEBUG_SPUR__

#define __SA__
#ifdef TINYSA3
#define __SI4432__
#endif
#ifdef TINYSA4
#define __SI4463__
#define __SI4468__
#define __ADF4351__
#endif
#define __PE4302__
//#define __SIMULATION__
#define __SCROLL__                // Add waterfall option
#define __ICONS__
#define __MEASURE__
#define __LINEARITY__             // Not available
#define __SELFTEST__              // Add selftest option (not fully disable it)
#define __CALIBRATE__             // Add calibration menu and functions
#define __FAST_SWEEP__            // Pre-fill SI4432 RSSI buffer  to get fastest sweep in zero span mode
// #define __AUDIO__
//#define __HAM_BAND__
#define __SPUR__                  // Does spur reduction by shifting IF
//#define __USE_SERIAL_CONSOLE__  // Enable serial I/O connection (need enable HAL_USE_SERIAL as TRUE in halconf.h)
#define __SINGLE_LETTER__         // Add fast console commands
#define __NICE_BIG_FONT__         // Add not scaled big font for menus
#define __QUASI_PEAK__            // Add quasi peak average option
#define __REMOTE_DESKTOP__        // Add remote desktop option
#define __LISTEN__
#ifdef TINYSA4
#define  __HARMONIC__
#endif

#ifdef TINYSA3
#define DEFAULT_IF  433800000
#define DEFAULT_SPUR_IF 434000000
#define DEFAULT_MAX_FREQ    350000000
#define HIGH_MIN_FREQ_MHZ   240
#define HIGH_MAX_FREQ_MHZ   960
#endif
#ifdef TINYSA4
#define DEFAULT_IF  ((freq_t)977100000)
#define DEFAULT_SPUR_OFFSET ((freq_t)1500000)
#define DEFAULT_MAX_FREQ    ((freq_t)800000000)
#define HIGH_MIN_FREQ_MHZ   136// 825
#define HIGH_MAX_FREQ_MHZ   1130
#define ULTRA_MAX_FREQ      5350000000ULL
//#define ULTRA_MAX_FREQ      2900000000ULL
#define MAX_LO_FREQ         4350000000ULL
#define LOW_MAX_FREQ         800000000ULL
#endif
/*
 * main.c
 */
#ifdef __SA__
#ifdef TINYSA4
#define POINTS_COUNT     450
#else
#define POINTS_COUNT     290
#endif
#define MARKER_COUNT    4

#define TRACES_MAX 3
#define TRACE_ACTUAL    0
#define TRACE_STORED    1
#define TRACE_TEMP      2
//#define TRACE_AGE       3
#define TRACE_INVALID  -1

#define actual_t  measured[TRACE_ACTUAL]
#define stored_t  measured[TRACE_STORED]
#define temp_t    measured[TRACE_TEMP]
// #define age_t     measured[TRACE_AGE]

#ifdef TINYSA3
typedef uint32_t freq_t;
 typedef int32_t long_t;
#endif
#ifdef TINYSA4
 typedef uint64_t freq_t;
 typedef int64_t long_t;
#endif

#define CORRECTION_POINTS  10       // Frequency dependent level correction table entries

typedef float measurement_t[TRACES_MAX][POINTS_COUNT];
extern measurement_t measured;
#endif

#ifdef __REMOTE_DESKTOP__
extern uint8_t auto_capture;
extern int16_t mouse_x;
extern int16_t mouse_y;
extern uint8_t mouse_down;
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
  ST_START=0, ST_STOP, ST_CENTER, ST_SPAN, ST_CW, ST_DUMMY      // Last is used in marker ops
};

void set_sweep_points(uint16_t points);
void update_frequencies(void);
void set_sweep_frequency(int type, freq_t frequency);
freq_t get_sweep_frequency(int type);
void my_microsecond_delay(int t);
float my_atof(const char *p);
int shell_printf(const char *fmt, ...);
#ifdef __REMOTE_DESKTOP__
void send_region(const char *t, int16_t x, int16_t y, int16_t w, int16_t h);
void send_buffer(uint8_t * buf, int s);
#endif
void set_marker_frequency(int m, freq_t f);
void toggle_sweep(void);
void toggle_mute(void);
void load_default_properties(void);

enum {
  AV_OFF, AV_MIN, AV_MAX_HOLD, AV_MAX_DECAY, AV_4, AV_16, AV_QUASI
};
enum {
  M_LOW, M_HIGH, M_GENLOW, M_GENHIGH, M_ULTRA
};

enum {
  MO_NONE, MO_AM, MO_NFM, MO_WFM, MO_EXTERNAL,
};

#define MODE_OUTPUT(x)  ((x) == M_GENLOW || (x) == M_GENHIGH )
#ifdef __ULTRA__
#define MODE_INPUT(x)  ((x) == M_LOW || (x) == M_HIGH || (x) == M_ULTRA )
#else
#define MODE_INPUT(x)  ((x) == M_LOW || (x) == M_HIGH )
#endif
#define MODE_HIGH(x)  ((x) == M_HIGH || (x) == M_GENHIGH )
#define MODE_LOW(x)  ((x) == M_LOW || (x) == M_GENLOW )

#ifdef __SI4432__
#define SI4432_RX                          0
#define SI4432_LO                          1
#define MODE_SELECT(x) (MODE_HIGH(x) ? SI4432_LO : SI4432_RX)
#endif
#ifdef __SI4468__
// Not use mode
#define MODE_SELECT(x) (MODE_HIGH(x) ? 1 : 0)
#endif

#define SWEEP_ENABLE    0x01
#define SWEEP_ONCE      0x02
#define SWEEP_CALIBRATE 0x04
#define SWEEP_SELFTEST  0x08
#define SWEEP_REMOTE    0x10
//#define SWEEP_FACTORY    0x20


extern uint8_t sweep_mode;
extern bool completed;
extern const char *info_about[];

#ifdef TINYSA4
void toggle_extra_lna(void);
void set_extra_lna(int t);
#endif

// ------------------------------- sa_core.c ----------------------------------


extern float level_min;
extern float level_max;
extern float level_range;

extern const char * const unit_string[];
#ifdef TINYSA4
extern float *drive_dBm;
#else
extern const int8_t drive_dBm [];
#endif
extern uint8_t signal_is_AM;
extern const uint32_t reffer_freq[];
extern freq_t minFreq;
extern freq_t maxFreq;
int level_is_calibrated(void);
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
float get_level(void);
void set_harmonic(int);
//extern int setting.harmonic;
int search_is_greater(void);
void set_auto_attenuation(void);
void set_auto_reflevel(bool);
int is_paused(void);
void set_actual_power(float);
void SetGenerate(int);
void set_RBW(uint32_t rbw_x10);
void set_lo_drive(int d);
void set_rx_drive(int d);
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
void toggle_spur(void);
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
void disable_waterfall(void);
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
void set_attack(int);
void set_noise(int);
void toggle_tracking_output(void);
extern int32_t frequencyExtra;
void set_modulation(int);
void set_modulation_frequency(int);
int search_maximum(int m, freq_t center, int span);
//extern int setting.modulation;
void set_measurement(int);
// extern int settingSpeed;
//extern int setting.step_delay;
void sweep_remote(void);
extern int generic_option_cmd( const char *cmd, const char *cmd_list, int argc, char *argv);

#ifdef TINYSA4
void clear_frequency_cache(void);
void toggle_high_out_adf4350(void);
extern int high_out_adf4350;
void set_30mhz(freq_t);
void toggle_ultra(void);
void set_IF2(int f);
void set_R(int f);
extern void set_modulo(uint32_t f);
extern void fill_spur_table(void);
#else
void set_10mhz(freq_t);
#endif

#ifdef __AUDIO__
/*
 * dsp.c
 */
// 5ms @ 48kHz
#define AUDIO_BUFFER_LEN 96

extern int16_t rx_buffer[AUDIO_BUFFER_LEN * 2];

#define STATE_LEN 32
#define SAMPLE_LEN 48

#ifdef ENABLED_DUMP
extern int16_t ref_buf[];
extern int16_t samp_buf[];
#endif
#endif
#ifdef __VNA__
void dsp_process(int16_t *src, size_t len);
void reset_dsp_accumerator(void);
void calculate_gamma(float *gamma);
void fetch_amplitude(float *gamma);
void fetch_amplitude_ref(float *gamma);
#endif

#ifdef __AUDIO__
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
#define CHART_BOTTOM   (LCD_HEIGHT-10)
#define SCROLL_GRIDY      (HEIGHT_SCROLL / NGRIDY)
#define NOSCROLL_GRIDY    (CHART_BOTTOM / NGRIDY)
#else
#define GRIDY             (CHART_BOTTOM / NGRIDY)
#endif

#define BATTERY_START   (LCD_HEIGHT-40)

#define WIDTH  (LCD_WIDTH - 1 - OFFSETX)
#define HEIGHT (GRIDY*NGRIDY)

#define FREQUENCIES_XPOS1 OFFSETX
#define FREQUENCIES_XPOS2 (LCD_WIDTH-120)
#define FREQUENCIES_YPOS  (LCD_HEIGHT-8)

//
#define CELLOFFSETX 0
#define AREA_WIDTH_NORMAL  (CELLOFFSETX + WIDTH  + 1)
#define AREA_HEIGHT_NORMAL (              HEIGHT + 1)

#define GRID_X_TEXT       (AREA_WIDTH_NORMAL - 7*5)

// Marker start drag distance (can be bigger for various display resolution)
#define MARKER_PICKUP_DISTANCE 20

// Smith/polar chart
//#define P_CENTER_X (CELLOFFSETX + WIDTH/2)
//#define P_CENTER_Y (HEIGHT/2)
//#define P_RADIUS   (HEIGHT/2)

// Menu Button
// Maximum menu buttons count
#ifdef TINYSA4
#define MENU_BUTTON_MAX         9
#else
#define MENU_BUTTON_MAX         8
#endif
#define MENU_BUTTON_WIDTH      80
#define MENU_BUTTON_HEIGHT     (LCD_HEIGHT/MENU_BUTTON_MAX-1)
#define MENU_BUTTON_BORDER      1
#define KEYBOARD_BUTTON_BORDER  2
#define FORM_BUTTON_BORDER      2

// Form button (at center screen better be less LCD_WIDTH - 2*OFFSETX)
#define MENU_FORM_WIDTH    (LCD_WIDTH - 2*OFFSETX)

// Num Input height at bottom
#define NUM_INPUT_HEIGHT   32

extern uint16_t area_width;
extern uint16_t area_height;

// Define marker size (can be 0 or 1)
#ifdef TINYSA3
#define _MARKER_SIZE_         0
#endif
#ifdef TINYSA4
#define _MARKER_SIZE_         1
#endif
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

#ifdef __NICE_BIG_FONT__
#define wFONT_START_CHAR   0x17
#define wFONT_MAX_WIDTH      12
#define wFONT_GET_HEIGHT     14
#define wFONT_STR_HEIGHT     16
#define wFONT_GET_DATA(ch)   (   &x10x14_bits[(ch-wFONT_START_CHAR)*2*wFONT_GET_HEIGHT  ])
#define wFONT_GET_WIDTH(ch)  (14-(x10x14_bits[(ch-wFONT_START_CHAR)*2*wFONT_GET_HEIGHT+1]&0x7))
#else
#define wFONT_MAX_WIDTH      12
#define wFONT_GET_HEIGHT     14
#endif

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

// Max palette indexes in config
#define MAX_PALETTE     32

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
  uint32_t deviceid;
  uint16_t lcd_palette[MAX_PALETTE];
  int16_t  touch_cal[4];
  uint32_t _serial_speed;
#ifdef __VNA__
  freq_t harmonic_freq_threshold;
#endif
  uint16_t dac_value;
  uint16_t vbat_offset;
  float low_level_offset;
  float high_level_offset;
  float low_level_output_offset;
  float high_level_output_offset;
  float  low_correction_value[CORRECTION_POINTS];
  float  high_correction_value[CORRECTION_POINTS];
  freq_t low_correction_frequency[CORRECTION_POINTS];
  freq_t high_correction_frequency[CORRECTION_POINTS];
#ifdef TINYSA4
  freq_t  setting_frequency_30mhz;
#else
  freq_t  setting_frequency_10mhz;
#endif

  uint16_t gridlines;
  uint16_t hambands;
#ifdef TINYSA4
  freq_t frequency_IF1;
  freq_t frequency_IF2;
  freq_t ultra_threshold;
  int8_t    ultra;
#endif
  uint8_t   _mode;
  int8_t    cor_am;
  int8_t    cor_wfm;
  int8_t    cor_nfm;
  uint8_t high_out_adf4350;
  float sweep_voltage;
  float switch_offset;
  int16_t   ext_zero_level;
  uint32_t    dummy;
//  uint8_t _reserved[22];
  freq_t checksum;
} config_t;

extern config_t config;
//#define settingLevelOffset config.level_offset
float get_level_offset(void);

extern uint8_t in_selftest;
extern int display_test(void);

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
int  shell_serial_printf(const char *fmt, ...);


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
  uint8_t enabled;
  uint8_t mtype;
  int16_t index;
  freq_t frequency;
} marker_t;

#define MARKERS_MAX 4
#define MARKER_INVALID -1

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
void draw_frequencies(void);
void draw_all(bool flush);

void draw_cal_status(void);

//void markmap_all_markers(void);

int distance_to_index(int8_t t, uint16_t idx, int16_t x, int16_t y);
int search_nearest_index(int x, int y, int t);

int marker_search_max(void);
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
extern  uint8_t redraw_request;

/*
 * ili9341.c
 */
// Set display buffers count for cell render (if use 2 and DMA, possible send data and prepare new in some time)

#ifdef __USE_DISPLAY_DMA__
// Cell size = sizeof(spi_buffer), but need wait while cell data send to LCD
//#define DISPLAY_CELL_BUFFER_COUNT     1
// Cell size = sizeof(spi_buffer)/2, while one cell send to LCD by DMA, CPU render to next cell
#define DISPLAY_CELL_BUFFER_COUNT     2
#else
// Always one if no DMA mode
#define DISPLAY_CELL_BUFFER_COUNT     1
#endif

// One pixel size
typedef uint16_t pixel_t;

#define CELLWIDTH  (64/DISPLAY_CELL_BUFFER_COUNT)
#define CELLHEIGHT (32)

// Define size of screen buffer in pixels (one pixel 16bit size)
#define SPI_BUFFER_SIZE     (CELLWIDTH * CELLHEIGHT * DISPLAY_CELL_BUFFER_COUNT)

// SPI bus revert byte order
// 16-bit gggBBBbb RRRrrGGG
#define RGB565(r,g,b)  ( (((g)&0x1c)<<11) | (((b)&0xf8)<<5) | ((r)&0xf8) | (((g)&0xe0)>>5) )
#define RGBHEX(hex) ( (((hex)&0x001c00)<<3) | (((hex)&0x0000f8)<<5) | (((hex)&0xf80000)>>16) | (((hex)&0x00e000)>>13) )
#define HEXRGB(hex) ( (((hex)>>3)&0x001c00) | (((hex)>>5)&0x0000f8) | (((hex)<<16)&0xf80000) | (((hex)<<13)&0x00e000) )

// Define LCD display driver and screen size
#ifdef TINYSA4
#define LCD_DRIVER_ST7796S
#define LCD_WIDTH                   480
#define LCD_HEIGHT                  320
#else
#define LCD_DRIVER_ILI9341
#define LCD_WIDTH                   320
#define LCD_HEIGHT                  240
#endif

#define LCD_BG_COLOR             0
#define LCD_FG_COLOR             1
#define LCD_GRID_COLOR           2
#define LCD_MENU_COLOR           3
#define LCD_MENU_TEXT_COLOR      4
#define LCD_MENU_ACTIVE_COLOR    5
#define LCD_TRACE_1_COLOR        6
#define LCD_TRACE_2_COLOR        7
#define LCD_TRACE_3_COLOR        8
#define LCD_TRACE_4_COLOR        9
#define LCD_NORMAL_BAT_COLOR    10
#define LCD_LOW_BAT_COLOR       11
#define LCD_TRIGGER_COLOR       12
#define LCD_RISE_EDGE_COLOR     13
#define LCD_FALLEN_EDGE_COLOR   14
#define LCD_SWEEP_LINE_COLOR    15
#define LCD_BW_TEXT_COLOR       16
#define LCD_INPUT_TEXT_COLOR    17
#define LCD_INPUT_BG_COLOR      18
#define LCD_BRIGHT_COLOR_BLUE   19
#define LCD_BRIGHT_COLOR_RED    20
#define LCD_BRIGHT_COLOR_GREEN  21
#define LCD_DARK_GREY           22
#define LCD_LIGHT_GREY          23
#define LCD_HAM_COLOR           24
#define LCD_GRID_VALUE_COLOR    25
#define LCD_M_REFERENCE         26
#define LCD_M_DELTA             27
#define LCD_M_NOISE             28
#define LCD_M_DEFAULT           29

#define LCD_DEFAULT_PALETTE {\
[LCD_BG_COLOR         ] = RGB565(  0,  0,  0), \
[LCD_FG_COLOR         ] = RGB565(255,255,255), \
[LCD_GRID_COLOR       ] = RGB565(128,128,128), \
[LCD_MENU_COLOR       ] = RGB565(230,230,230), \
[LCD_MENU_TEXT_COLOR  ] = RGB565(  0,  0,  0), \
[LCD_MENU_ACTIVE_COLOR] = RGB565(210,210,210), \
[LCD_TRACE_1_COLOR    ] = RGB565(255,255,  0), \
[LCD_TRACE_2_COLOR    ] = RGB565(  0,255,  0), \
[LCD_TRACE_3_COLOR    ] = RGB565(255,  0,  0), \
[LCD_TRACE_4_COLOR    ] = RGB565(255,  0,255), \
[LCD_NORMAL_BAT_COLOR ] = RGB565( 31,227,  0), \
[LCD_LOW_BAT_COLOR    ] = RGB565(255,  0,  0), \
[LCD_TRIGGER_COLOR    ] = RGB565(  0,  0,255), \
[LCD_RISE_EDGE_COLOR  ] = RGB565(255,255,255), \
[LCD_FALLEN_EDGE_COLOR] = RGB565(128,128,128), \
[LCD_SWEEP_LINE_COLOR ] = RGB565(  0,255,  0), \
[LCD_BW_TEXT_COLOR    ] = RGB565(128,128,128), \
[LCD_INPUT_TEXT_COLOR ] = RGB565(  0,  0,  0), \
[LCD_INPUT_BG_COLOR   ] = RGB565(255,255,255), \
[LCD_BRIGHT_COLOR_BLUE] = RGB565(  0,  0,255), \
[LCD_BRIGHT_COLOR_RED ] = RGB565(255,128,128), \
[LCD_BRIGHT_COLOR_GREEN]= RGB565(  0,255,  0), \
[LCD_DARK_GREY        ] = RGB565(140,140,140), \
[LCD_LIGHT_GREY       ] = RGB565(220,220,220), \
[LCD_HAM_COLOR        ] = RGB565( 80, 80, 80), \
[LCD_GRID_VALUE_COLOR ] = RGB565(196,196,196), \
[LCD_M_REFERENCE      ] = RGB565(255,255,255), \
[LCD_M_DELTA          ] = RGB565(  0,255,  0), \
[LCD_M_NOISE          ] = RGB565(  0,255,255), \
[LCD_M_DEFAULT        ] = RGB565(255,255,  0), \
}

#define GET_PALTETTE_COLOR(idx)  config.lcd_palette[idx]

extern uint16_t foreground_color;
extern uint16_t background_color;

extern pixel_t spi_buffer[SPI_BUFFER_SIZE];

// Used for easy define big Bitmap as 0bXXXXXXXXX image
#define _BMP8(d)                                                        ((d)&0xFF)
#define _BMP16(d)                                      (((d)>>8)&0xFF), ((d)&0xFF)
#define _BMP24(d)                    (((d)>>16)&0xFF), (((d)>>8)&0xFF), ((d)&0xFF)
#define _BMP32(d)  (((d)>>24)&0xFF), (((d)>>16)&0xFF), (((d)>>8)&0xFF), ((d)&0xFF)

void ili9341_init(void);
void ili9341_test(int mode);
void ili9341_bulk(int x, int y, int w, int h);              // send data to display, in DMA mode use it, but wait DMA complete
void ili9341_fill(int x, int y, int w, int h);

// Double buffer mode parser
#if DISPLAY_CELL_BUFFER_COUNT == 1
#define ili9341_get_cell_buffer()             spi_buffer
#define ili9341_bulk_continue                 ili9341_bulk
#define ili9341_bulk_finish()                 {}
#else
pixel_t *ili9341_get_cell_buffer(void);                     // get buffer for cell render
void ili9341_bulk_continue(int x, int y, int w, int h);     // send data to display, in DMA mode use it, no wait DMA complete
void ili9341_bulk_finish(void);                             // wait DMA complete (need call at end)
#endif

void ili9341_set_foreground(uint16_t fg_idx);
void ili9341_set_background(uint16_t bg_idx);

void ili9341_clear_screen(void);
void ili9341_blitBitmap(int x, int y, int width, int height, const uint8_t *bitmap);
void ili9341_drawchar(uint8_t ch, int x, int y);
void ili9341_drawstring(const char *str, int x, int y);
void ili9341_drawstring_7x13(const char *str, int x, int y);
void ili9341_drawstring_10x14(const char *str, int x, int y);
void ili9341_drawstringV(const char *str, int x, int y);
int  ili9341_drawchar_size(uint8_t ch, int x, int y, uint8_t size);
void ili9341_drawstring_size(const char *str, int x, int y, uint8_t size);
void ili9341_drawfont(uint8_t ch, int x, int y);
void ili9341_read_memory(int x, int y, int w, int h, uint16_t* out);
void ili9341_line(int x0, int y0, int x1, int y1);
void show_version(void);

/*
 * flash.c
 */
typedef struct setting
{
  uint32_t magic;
  bool auto_reflevel;          // bool
  bool auto_attenuation;       // bool
  bool mirror_masking;         // bool
  bool subtract_stored;        // bool
  bool show_stored;            // bool
  bool tracking_output;        // bool
  bool mute;                   // bool
  bool auto_IF;                // bool

  uint8_t mode;                // enum
  uint8_t below_IF;            // enum
  uint8_t unit;                // enum
  uint8_t agc;                 // enum
  uint8_t lna;                 // enum
  uint8_t modulation;          // enum
  uint8_t trigger;             // enum
  uint8_t trigger_mode;        // enum
  uint8_t trigger_direction;   // enum
  uint8_t step_delay_mode;     // enum
  uint8_t waterfall;           // enum
  uint8_t average;             // enum
  uint8_t measurement;         // enum
  uint8_t spur_removal;        // enum

  int8_t  tracking;            // -1...1 !!! need convert to bool
  uint8_t atten_step;          //  0...1 !!! need convert to bool
  int8_t _active_marker;       // -1...MARKER_MAX
  uint8_t unit_scale_index;    // table index
  uint8_t repeat;              // 1...100
  uint8_t noise;               // 2...50
  uint8_t lo_drive;            // 0-3 , 3dB steps
  uint8_t rx_drive;            // 0-15 , 7=+20dBm, 3dB steps
  uint8_t test;                // current test number
  uint8_t harmonic;            // used harmonic number 1...5
  uint8_t fast_speedup;        // 0 - 20

  uint16_t linearity_step;     // range equal POINTS_COUNT
  uint16_t _sweep_points;
  int16_t attenuate_x2;        // 0...60 !!! in calculation can be < 0

  uint16_t step_delay;         // KM_SAMPLETIME   250...10000, 0=auto
  uint16_t offset_delay;       // KM_OFFSET_DELAY 250...10000, 0=auto

  uint16_t freq_mode;           //  0...1!!! need convert to bool or bit field
  int16_t  refer;               // -1 disabled

  uint16_t modulation_frequency;  // 50...6000
  int decay;                      // KM_DECAY   < 1000000
  int attack;                     // KM_ATTACK  <   20000

#ifdef TINYSA4
  int32_t slider_position;
  freq_t  slider_span;
#else
  int32_t slider_position;
  int32_t slider_span;
#endif

  uint32_t rbw_x10;
  uint32_t vbw_x10;

  float reflevel;
  float scale;
  float offset;
  float trigger_level;
  float level;
  float level_sweep;

  float unit_scale;
  float normalize_level;     // Level to set normalize to, zero if not doing anything

  freq_t frequency_step;
  freq_t frequency0;
  freq_t frequency1;
  freq_t frequency_IF;

  float trace_scale;
  float trace_refpos;
  trace_t _trace[TRACES_MAX];
  marker_t _markers[MARKERS_MAX];

  systime_t sweep_time_us;
  systime_t measure_sweep_time_us;
  systime_t actual_sweep_time_us;
  systime_t additional_step_delay_us;

  freq_t  *correction_frequency;
  float   *correction_value;

#ifdef TINYSA4
  bool    extra_lna;
  uint8_t ultra;    // enum ??
  int R;            // KM_R
#endif
  int test_argument;            // used for tests
  uint32_t checksum;            // must be last and at 4 byte boundary
}setting_t;

extern setting_t setting;

void reset_settings(int m);

void set_trace_scale(float scale);
void set_trace_refpos(float refpos);
#define get_trace_scale()  setting.trace_scale
#define get_trace_refpos() setting.trace_refpos

#define S_IS_AUTO(x) ((x)&2)
#define S_STATE(X) ((X)&1)
enum { S_OFF=0, S_ON=1, S_AUTO_OFF=2, S_AUTO_ON=3 };

enum { SD_NORMAL, SD_PRECISE, SD_FAST, SD_MANUAL };

enum {W_OFF, W_SMALL, W_BIG};

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

extern freq_t frequencies[POINTS_COUNT];
extern const float unit_scale_value[];
extern const char  unit_scale_text[];
#ifdef TINYSA4
extern int debug_frequencies;
#endif
#if 1   // Still sufficient flash
// Flash save area - flash7  : org = 0x0801B000, len = 20k in *.ld file
// 2k - for config save
// 9 * 2k for setting_t + stored trace
#define SAVEAREA_MAX 9
// STM32 minimum page size for write
#define FLASH_PAGESIZE          0x800
// config save area (flash7 addr)
#ifdef TINYSA3
#define SAVE_CONFIG_ADDR        0x0801B000
#endif

#ifdef TINYSA4
#define SAVE_CONFIG_ADDR        0x0803B000
#endif

#define SAVE_CONFIG_SIZE        0x00000800
// setting_t save area (save area + config size)
#define SAVE_PROP_CONFIG_ADDR   (SAVE_CONFIG_ADDR + SAVE_CONFIG_SIZE)
#define SAVE_PROP_CONFIG_SIZE   0x00000800
// Should include all save slots
#define SAVE_CONFIG_AREA_SIZE   (SAVE_CONFIG_SIZE + SAVEAREA_MAX * SAVE_PROP_CONFIG_SIZE)


#else
#define SAVEAREA_MAX 4
// Begin addr                   0x0801C000
#define SAVE_CONFIG_AREA_SIZE   0x00004000
// config save area
#define SAVE_CONFIG_ADDR        0x0801C000
// properties_t save area
#define SAVE_PROP_CONFIG_0_ADDR 0x0801C800
#define SAVE_PROP_CONFIG_1_ADDR 0x0801D000
#define SAVE_PROP_CONFIG_2_ADDR 0x0801D800
#define SAVE_PROP_CONFIG_3_ADDR 0x0801E000
#define SAVE_PROP_CONFIG_4_ADDR 0x0801e800
#endif

#if 0
typedef struct properties {
  uint32_t magic;
  preset_t setting;
//  freq_t _frequency0;
//  freq_t _frequency1;
  uint16_t _sweep_points;
#ifdef __VNA__
  uint16_t _cal_status;

#endif
#ifdef __SA__
//  freq_t _frequency_IF; //IF frequency
#endif
//  freq_t _frequencies[POINTS_COUNT];
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

#define CONFIG_MAGIC 0x434f4e4b /* 'CONF' */

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
int invoke_quick_menu(int);

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
  uint8_t auto_center_marker;
  char text[28];
} uistat_t;

typedef struct ui_button {
  uint16_t fg;
  uint16_t bg;
  uint8_t  border;
  int8_t   icon;
  union {
    int32_t  i;
    uint32_t u;
    float f;
    const char *text;
  } param_1, param_2;    // void data for label printf
  char text[32];
} ui_button_t;

typedef struct ui_slider {
  uint8_t keypad;
  uint8_t has_steps;
  uint16_t slider_position;
  uint16_t slider_step;
  float min_value;
  float max_value;
} ui_slider_t;

extern uistat_t uistat;
void ui_init(void);
void ui_show(void);
void ui_hide(void);

void touch_position(int *x, int *y);
void handle_touch_interrupt(void);

#define TOUCH_THRESHOLD 2000

void touch_cal_exec(void);
void touch_draw_test(void);
void enter_dfu(void);

/*
 * adc.c
 */
#ifdef TINYSA4
#define rccEnableWWDG(lp) rccEnableAPB1(RCC_APB1ENR_WWDGEN, lp)
#define ADC_TOUCH_X  ADC_CHANNEL_IN3
#define ADC_TOUCH_Y  ADC_CHANNEL_IN4
#else
#define ADC_TOUCH_X  ADC_CHSELR_CHSEL6
#define ADC_TOUCH_Y  ADC_CHSELR_CHSEL7
#endif

void adc_init(void);
uint16_t adc_single_read(uint32_t chsel);
void adc_start_analog_watchdog(void);
void adc_stop_analog_watchdog(void);
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

#define ARRAY_COUNT(a)    (sizeof(a)/sizeof(*(a)))
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

#define DEVICE_TO_PURE_RSSI(rssi) ((rssi)<<4)
#define PURE_TO_DEVICE_RSSI(rssi) ((rssi)>>4)
#define float_TO_PURE_RSSI(rssi)  ((rssi)*32)
#define PURE_TO_float(rssi)       ((rssi)/32.0)

extern uint16_t actual_rbw_x10;

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
pureRSSI_t perform(bool b, int i, freq_t f, int e);
void interpolate_maximum(int m);
void calibrate_modulation(int modulation, int8_t *correction);

enum {
  M_OFF, M_IMD, M_OIP3, M_PHASE_NOISE, M_STOP_BAND, M_PASS_BAND, M_LINEARITY, M_AM, M_FM, M_THD
};

enum {
  T_AUTO, T_NORMAL, T_SINGLE, T_DONE, T_UP, T_DOWN, T_MODE, T_PRE, T_POST, T_MID
};


extern const int SI4432_RBW_count;
extern void SI4432_Listen(int s);

#ifdef TINYSA4
// si4432.c

extern void ADF4351_mux(int R);
extern void ADF4351_force_refresh(void);
extern void ADF4351_CP(int p);
extern void ADF4351_modulo(int m);
extern void ADF4351_csr(int c);
extern void ADF4351_fastlock(int c);
extern int SI4463_R;
extern volatile int64_t ADF4350_modulo;
extern void SI446x_set_AGC_LNA(uint8_t v);
extern void SI4463_init_rx(void);
extern void SI4463_init_tx(void);
extern void SI4463_start_tx(uint8_t CHANNEL);
extern void SI4463_set_output_level(int t);
extern freq_t SI4463_set_freq(freq_t freq);
extern uint16_t set_rbw(uint16_t rbw_x10);
extern uint16_t force_rbw(int f);
extern void SI4463_do_api(void* data, uint8_t len, void* out, uint8_t outLen);
extern void SI4463_set_gpio(int i, int s);
extern void si_set_offset(int16_t offset);
extern void si_fm_offset(int16_t offset);
extern bool ADF4351_frequency_changed;
extern bool SI4463_frequency_changed;
extern bool SI4463_offset_changed;
extern int old_R;
#endif
/*EOF*/
