/*
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
//#define HAL_USE_SERIAL 1
//#define STM32_SERIAL_USE_USART1  1

#include "ch.h"
#include "hal.h"

//#include "hal_serial.h"

#include "usbcfg.h"
#ifdef __VNA__
#include "si5351.h"
#endif
#include "nanovna.h"
#ifdef __VNA__
#include "fft.h"
#endif

#include <chprintf.h>
#include <string.h>
#include <math.h>

extern freq_t minFreq;
extern freq_t maxFreq;
freq_t frequencyStart;
freq_t frequencyStop;
int32_t frequencyExtra;
#define START_MIN minFreq
#define STOP_MAX maxFreq
/*
 *  Shell settings
 */
// If need run shell as thread (use more amount of memory fore stack), after
// enable this need reduce spi_buffer size, by default shell run in main thread
// #define VNA_SHELL_THREAD

static BaseSequentialStream *shell_stream;

// Shell new line
#define VNA_SHELL_NEWLINE_STR    "\r\n"
// Shell command promt
#define VNA_SHELL_PROMPT_STR     "ch> "
// Shell max arguments
#define VNA_SHELL_MAX_ARGUMENTS   4
// Shell max command line size
#define VNA_SHELL_MAX_LENGTH     48

// Shell command functions prototypes
typedef void (*vna_shellcmd_t)(int argc, char *argv[]);
#define VNA_SHELL_FUNCTION(command_name) \
      static void command_name(int argc, char *argv[])

// Shell command line buffer, args, nargs, and function ptr
static char shell_line[VNA_SHELL_MAX_LENGTH];
static char *shell_args[VNA_SHELL_MAX_ARGUMENTS + 1];
static uint16_t shell_nargs;
static volatile vna_shellcmd_t  shell_function = 0;

//#define ENABLED_DUMP
// Allow get threads debug info
#define ENABLE_THREADS_COMMAND
// Enable vbat_offset command, allow change battery voltage correction in config
#define ENABLE_VBAT_OFFSET_COMMAND
// Info about NanoVNA, need fore soft
#define ENABLE_INFO_COMMAND
// Enable color command, allow change config color for traces, grid, menu
#define ENABLE_COLOR_COMMAND
#ifdef __USE_SERIAL_CONSOLE__
#define ENABLE_USART_COMMAND
#endif
#ifdef __VNA__
static void apply_error_term_at(int i);
static void apply_edelay_at(int i);
static void cal_interpolate(int s);
#endif
void update_frequencies(void);
static void set_frequencies(freq_t start, freq_t stop, uint16_t points);
static bool sweep(bool break_on_operation);
#ifdef __VNA__
static void transform_domain(void);

#define DRIVE_STRENGTH_AUTO (-1)
#define FREQ_HARMONICS (config.harmonic_freq_threshold)
#define IS_HARMONIC_MODE(f) ((f) > FREQ_HARMONICS)
// Obsolete, always use interpolate
#define  cal_auto_interpolate  TRUE

static int8_t drive_strength = DRIVE_STRENGTH_AUTO;
#endif
uint8_t sweep_mode = SWEEP_ENABLE;
uint8_t redraw_request = 0; // contains REDRAW_XXX flags
uint8_t auto_capture = false;
// Version text, displayed in Config->Version menu, also send by info command
const char *info_about[]={
  BOARD_NAME,
  "2019-2020 Copyright @Erik Kaashoek",
  "2016-2020 Copyright @edy555",
  "SW licensed under GPL. See: https://github.com/erikkaashoek/tinySA",
  "Version: " VERSION,
  "Build Time: " __DATE__ " - " __TIME__,
  "Kernel: " CH_KERNEL_VERSION,
  "Compiler: " PORT_COMPILER_NAME,
  "Architecture: " PORT_ARCHITECTURE_NAME " Core Variant: " PORT_CORE_VARIANT_NAME,
  "Port Info: " PORT_INFO,
  "Platform: " PLATFORM_NAME,
  0 // sentinel
};

bool dirty = true;

bool completed = false;

#ifdef TINYSA4
static THD_WORKING_AREA(waThread1, 1124);
#else
static THD_WORKING_AREA(waThread1, 768);
#endif
static THD_FUNCTION(Thread1, arg)
{
  (void)arg;
  chRegSetThreadName("sweep");

#ifndef TINYSA4
  ui_process();
#endif

  while (1) {
//  START_PROFILE
    if (sweep_mode&(SWEEP_ENABLE|SWEEP_ONCE)) {
//      if (dirty)
        completed = sweep(true);
      sweep_mode&=~SWEEP_ONCE;
    } else if (sweep_mode & SWEEP_SELFTEST) {
      // call from lowest level to save stack space
      self_test(setting.test);
//      sweep_mode = SWEEP_ENABLE;
#ifdef __SINGLE_LETTER__
      } else if (sweep_mode & SWEEP_REMOTE) {
      sweep_remote();
#endif
#ifdef __CALIBRATE__
      } else if (sweep_mode & SWEEP_CALIBRATE) {
      // call from lowest level to save stack space
      calibrate();
      sweep_mode = SWEEP_ENABLE;
#endif
      } else {
//      if (setting.mode != -1)
        __WFI();
    }
//  STOP_PROFILE
    // Run Shell command in sweep thread
    if (shell_function) {
      operation_requested = OP_NONE; // otherwise commands  will be aborted
      shell_function(shell_nargs - 1, &shell_args[1]);
      shell_function = 0;
      osalThreadSleepMilliseconds(10);
      if (dirty) {
        if (MODE_OUTPUT(setting.mode))
          draw_menu();    // update screen if in output mode and dirty
        else
          redraw_request |= REDRAW_CAL_STATUS | REDRAW_AREA | REDRAW_FREQUENCY;
      }
//      continue;
    }
//    START_PROFILE
    // Process UI inputs
    if (!(sweep_mode & SWEEP_SELFTEST))
      ui_process();
    // Process collected data, calculate trace coordinates and plot only if scan
    // completed
    if (/* sweep_mode & SWEEP_ENABLE && */ completed) {
//      START_PROFILE;
      // Prepare draw graphics, cache all lines, mark screen cells for redraw
      plot_into_index(measured);
      redraw_request |= REDRAW_CELLS | REDRAW_BATTERY;
//      STOP_PROFILE;
      if (uistat.marker_tracking) {
        int i = marker_search_max();
        if (i != -1 && active_marker != MARKER_INVALID) {
          markers[active_marker].index = i;
          markers[active_marker].frequency = frequencies[i];

          redraw_request |= REDRAW_MARKER;
        }
      }
    }
    // plot trace and other indications as raster
    draw_all(completed);  // flush markmap only if scan completed to prevent
                          // remaining traces
//    STOP_PROFILE
  }

}

#pragma GCC push_options
#pragma GCC optimize ("Os")


int
is_paused(void)
{
  return !(sweep_mode & SWEEP_ENABLE);
}

static inline void
pause_sweep(void)
{
  sweep_mode &= ~SWEEP_ENABLE;
}

static inline void
resume_sweep(void)
{
  sweep_mode |= SWEEP_ENABLE;
}

void
toggle_sweep(void)
{
  sweep_mode ^= SWEEP_ENABLE;
}

#ifdef __VNA__
static float
bessel0(float x)
{
  const float eps = 0.0001;

  float ret = 0;
  float term = 1;
  float m = 0;

  while (term  > eps * ret) {
    ret += term;
    ++m;
    term *= (x*x) / (4*m*m);
  }
  return ret;
}

static float
kaiser_window(float k, float n, float beta)
{
  if (beta == 0.0) return 1.0;
  float r = (2 * k) / (n - 1) - 1;
  return bessel0(beta * sqrt(1 - r * r)) / bessel0(beta);
}

static void
transform_domain(void)
{
  // use spi_buffer as temporary buffer
  // and calculate ifft for time domain
  float* tmp = (float*)spi_buffer;

  uint8_t window_size = POINTS_COUNT, offset = 0;
  uint8_t is_lowpass = FALSE;
  switch (domain_mode & TD_FUNC) {
    case TD_FUNC_BANDPASS:
      offset = 0;
      window_size = POINTS_COUNT;
      break;
    case TD_FUNC_LOWPASS_IMPULSE:
    case TD_FUNC_LOWPASS_STEP:
      is_lowpass = TRUE;
      offset = POINTS_COUNT;
      window_size = POINTS_COUNT * 2;
      break;
  }

  float beta = 0.0;
  switch (domain_mode & TD_WINDOW) {
    case TD_WINDOW_MINIMUM:
      beta = 0.0;  // this is rectangular
      break;
    case TD_WINDOW_NORMAL:
      beta = 6.0;
      break;
    case TD_WINDOW_MAXIMUM:
      beta = 13;
      break;
  }

  for (int ch = 0; ch < 2; ch++) {
    memcpy(tmp, measured[ch], sizeof(measured[0]));
    for (int i = 0; i < POINTS_COUNT; i++) {
      float w = kaiser_window(i + offset, window_size, beta);
      tmp[i * 2 + 0] *= w;
      tmp[i * 2 + 1] *= w;
    }
    for (int i = POINTS_COUNT; i < FFT_SIZE; i++) {
      tmp[i * 2 + 0] = 0.0;
      tmp[i * 2 + 1] = 0.0;
    }
    if (is_lowpass) {
      for (int i = 1; i < POINTS_COUNT; i++) {
        tmp[(FFT_SIZE - i) * 2 + 0] = tmp[i * 2 + 0];
        tmp[(FFT_SIZE - i) * 2 + 1] = -tmp[i * 2 + 1];
      }
    }

    fft256_inverse((float(*)[2])tmp);
    memcpy(measured[ch], tmp, sizeof(measured[0]));
    for (int i = 0; i < POINTS_COUNT; i++) {
      measured[ch][i][0] /= (float)FFT_SIZE;
      if (is_lowpass) {
        measured[ch][i][1] = 0.0;
      } else {
        measured[ch][i][1] /= (float)FFT_SIZE;
      }
    }
    if ((domain_mode & TD_FUNC) == TD_FUNC_LOWPASS_STEP) {
      for (int i = 1; i < POINTS_COUNT; i++) {
        measured[ch][i][0] += measured[ch][i - 1][0];
      }
    }
  }
}
#endif

// Shell commands output
int shell_printf(const char *fmt, ...)
{
  va_list ap;
  int formatted_bytes;
  va_start(ap, fmt);
  formatted_bytes = chvprintf(shell_stream, fmt, ap);
  va_end(ap);
  return formatted_bytes;
}

#ifdef __USE_SERIAL_CONSOLE__
// Serial Shell commands output
int shell_serial_printf(const char *fmt, ...)
{
  va_list ap;
  int formatted_bytes;
  va_start(ap, fmt);
  formatted_bytes = chvprintf(&SD1, fmt, ap);
  va_end(ap);
  return formatted_bytes;
}
#endif

//
// Function used for search substring v in list
// Example need search parameter "center" in "start|stop|center|span|cw" getStringIndex return 2
// If not found return -1
// Used for easy parse command arguments
static int get_str_index(const char *v, const char *list)
{
  int i = 0;
  while (1) {
    const char *p = v;
    while (1) {
      char c = *list;
      if (c == '|') c = 0;
      if (c == *p++) {
        // Found, return index
        if (c == 0) return i;
        list++;    // Compare next symbol
        continue;
      }
      break;  // Not equal, break
    }
    // Set new substring ptr
    while (1) {
      // End of string, not found
      if (*list == 0) return -1;
      if (*list++ == '|') break;
    }
    i++;
  }
  return -1;
}

VNA_SHELL_FUNCTION(cmd_pause)
{
  (void)argc;
  (void)argv;
  pause_sweep();
  draw_cal_status();
}

VNA_SHELL_FUNCTION(cmd_resume)
{
  (void)argc;
  (void)argv;

  // restore frequencies array and cal
  update_frequencies();
#ifdef __VNA__
  if (cal_auto_interpolate && (cal_status & CALSTAT_APPLY))
    cal_interpolate(lastsaveid);
#endif
  resume_sweep();
}

VNA_SHELL_FUNCTION(cmd_reset)
{
  (void)argc;
  (void)argv;

  if (argc == 1) {
    if (get_str_index(argv[0], "dfu") == 0) {
      shell_printf("Performing reset to DFU mode\r\n");
      enter_dfu();
      return;
    }
  }
  shell_printf("Performing reset\r\n");

  rccEnableWWDG(FALSE);
  WWDG->CFR = 0x60;
  WWDG->CR = 0xff;

  /* wait forever */
  while (1)
    ;
}

#ifdef __VNA__
const int8_t gain_table[] = {
  0,  // 0 ~ 300MHz
  40, // 300 ~ 600MHz
  50, // 600 ~ 900MHz
  75, // 900 ~ 1200MHz
  85, // 1200 ~ 1500MHz
  95, // 1500MHz ~
  95, // 1800MHz ~
  95, // 2100MHz ~
  95  // 2400MHz ~
};

#define DELAY_GAIN_CHANGE 2

static int
adjust_gain(uint32_t newfreq)
{
  int new_order = newfreq / FREQ_HARMONICS;
  int old_order = si5351_get_frequency() / FREQ_HARMONICS;
  if (new_order != old_order) {
    tlv320aic3204_set_gain(gain_table[new_order], gain_table[new_order]);
    return DELAY_GAIN_CHANGE;
  }
  return 0;
}
#endif

int set_frequency(freq_t freq)
{
  (void) freq;
#ifdef __VNA__  
  int delay = adjust_gain(freq);
  int8_t ds = drive_strength;
  if (ds == DRIVE_STRENGTH_AUTO) {
    ds = freq > FREQ_HARMONICS ? SI5351_CLK_DRIVE_STRENGTH_8MA : SI5351_CLK_DRIVE_STRENGTH_2MA;
  }
  delay += si5351_set_frequency(freq, ds);
  return delay;
#endif
  return 1;
}

// Use macro, std isdigit more big
#define _isdigit(c) (c >= '0' && c <= '9')
// Rewrite universal standart str to value functions to more compact
//
// Convert string to int32
static long_t my_atoi(const char *p)
{
  long_t value = 0;
  uint32_t c;
  bool neg = false;

  if (*p == '-') {neg = true; p++;}
  if (*p == '+') p++;
  while ((c = *p++ - '0') < 10)
    value = value * 10 + c;
  switch (*(--p)) {
  case 'k': value *= 1000; break;
  case 'M': value *= 1000000; break;
  case 'G': value *= 1000000000; break;
  }
  return neg ? -value : value;
}

// Convert string to uint32
//  0x - for hex radix
//  0o - for oct radix
//  0b - for bin radix
//  default dec radix
freq_t my_atoui(const char *p)
{
  freq_t value = 0, radix = 10, c;
  if (*p == '+') p++;
  if (*p == '0') {
    switch (p[1]) {
      case 'x': radix = 16; break;
      case 'o': radix =  8; break;
      case 'b': radix =  2; break;
      default:  goto calculate;
    }
    p+=2;
  }
calculate:
  while (1) {
    c = *p++ - '0';
    // c = to_upper(*p) - 'A' + 10
    if (c >= 'A' - '0') c = (c&(~0x20)) - ('A' - '0') + 10;
    if (c >= radix) break;
    value = value * radix + c;
  }
  switch (*(--p)) {
  case 'k': value *= 1000; break;
  case 'M': value *= 1000000; break;
  case 'G': value *= 1000000000; break;
  }
  return value;
}

float
my_atof(const char *p)
{
  int neg = FALSE;
  if (*p == '-')
    neg = TRUE;
  if (*p == '-' || *p == '+')
    p++;
  float x = my_atoi(p);
  while (_isdigit((int)*p))
    p++;
  if (*p == '.') {
    float d = 1.0f;
    p++;
    while (_isdigit((int)*p)) {
      d /= 10;
      x += d * (*p - '0');
      p++;
    }
  }
  if (*p == 'e' || *p == 'E') {
    p++;
    int exp = my_atoi(p);
    while (exp > 0) {
      x *= 10;
      exp--;
    }
    while (exp < 0) {
      x /= 10;
      exp++;
    }
  }
  switch (*p) {
  case 'k': x *= 1e+3; break;
  case 'M': x *= 1e+6; break;
  case 'G': x *= 1e+9; break;
  case 'm': x /= 1e+3; break;
  case 'u': x /= 1e+6; break;
  case 'n': x /= 1e+9; break;
  case 'p': x /= 1e+12; break;
  }

  if (neg)
    x = -x;
  return x;
}

#ifdef __VNA__
VNA_SHELL_FUNCTION(cmd_offset)
{
  if (argc != 1) {
    shell_printf("usage: offset {frequency offset(Hz)}\r\n");
    return;
  }
  si5351_set_frequency_offset(my_atoi(argv[0]));
}
#endif

VNA_SHELL_FUNCTION(cmd_freq)
{
  if (argc != 1) {
    goto usage;
  }
  freq_t freq = my_atoui(argv[0]);

  pause_sweep();
  set_frequency(freq);
  return;
usage:
  shell_printf("usage: freq {frequency(Hz)}\r\n");
}
#ifdef __VNA__
VNA_SHELL_FUNCTION(cmd_power)
{
  if (argc != 1) {
    shell_printf("usage: power {0-3|-1}\r\n");
    return;
  }
  (void)argv;
  drive_strength = my_atoi(argv[0]);
//  set_frequency(frequency);
}
#endif

#ifdef __USE_RTC__
VNA_SHELL_FUNCTION(cmd_time)
{
  (void)argc;
  (void)argv;
  uint32_t  dt_buf[2];
  dt_buf[0] = rtc_get_tr_bcd(); // TR should be read first for sync
  dt_buf[1] = rtc_get_dr_bcd(); // DR should be read second
  static const uint8_t idx_to_time[] = {6,5,4,2,  1,  0};
  static const char       time_cmd[] = "y|m|d|h|min|sec";
  //            0    1   2       4      5     6
  // time[] ={sec, min, hr, 0, day, month, year, 0}
  uint8_t   *time = (uint8_t*)dt_buf;
  if (argc == 3 &&  get_str_index(argv[0], "b") == 0){
    rtc_set_time(my_atoui(argv[1]), my_atoui(argv[2]));
    return;
  }
  if (argc!=2) goto usage;
  int idx = get_str_index(argv[0], time_cmd);
  uint32_t val = my_atoui(argv[1]);
  if (idx < 0 || val > 99)
    goto usage;
  // Write byte value in struct
  time[idx_to_time[idx]] = ((val/10)<<4)|(val%10); // value in bcd format
  rtc_set_time(dt_buf[1], dt_buf[0]);
  return;
usage:
  shell_printf("20%02X/%02X/%02X %02X:%02X:%02X\r\n"\
               "usage: time {[%s] 0-99} or {b 0xYYMMDD 0xHHMMSS}\r\n", time[6], time[5], time[4], time[2], time[1], time[0], time_cmd);
}
#endif

VNA_SHELL_FUNCTION(cmd_dac)
{
  int value;
  if (argc != 1) {
    shell_printf("usage: dac {value(0-4095)}\r\n"\
                 "current value: %d\r\n", config.dac_value);
    return;
  }
  value = my_atoui(argv[0]);
  config.dac_value = value;
  dacPutChannelX(&DACD2, 0, value);
}

#ifdef __VNA__
VNA_SHELL_FUNCTION(cmd_threshold)
{
  uint32_t value;
  if (argc != 1) {
    shell_printf("usage: threshold {frequency in harmonic mode}\r\n"\
                 "current: %d\r\n", config.harmonic_freq_threshold);
    return;
  }
  value = my_atoui(argv[0]);
  config.harmonic_freq_threshold = value;
}
#endif

VNA_SHELL_FUNCTION(cmd_saveconfig)
{
  (void)argc;
  (void)argv;
  config_save();
  shell_printf("Config saved.\r\n");
}

VNA_SHELL_FUNCTION(cmd_clearconfig)
{
  if (argc != 1) {
    shell_printf("usage: clearconfig {protection key}\r\n");
    return;
  }

  if (get_str_index(argv[0], "1234") != 0) {
    shell_printf("Key unmatched.\r\n");
    return;
  }

  clear_all_config_prop_data();
  shell_printf("Config and all cal data cleared.\r\n"\
               "Do reset manually to take effect. Then do touch cal and save.\r\n");
}

#ifdef __AUDIO__
static struct {
  int16_t rms[2];
  int16_t ave[2];
  int callback_count;

#if 1
  int32_t last_counter_value;
  int32_t interval_cycles;
  int32_t busy_cycles;
#endif
} stat;
int16_t rx_buffer[AUDIO_BUFFER_LEN * 2];

#ifdef ENABLED_DUMP
int16_t dump_buffer[AUDIO_BUFFER_LEN];
int16_t dump_selection = 0;
#endif
volatile uint8_t wait_count = 0;
volatile uint8_t accumerate_count = 0;
#endif

#ifdef __VNA__
const int8_t bandwidth_accumerate_count[] = {
  1, // 1kHz
  3, // 300Hz
  10, // 100Hz
  33, // 30Hz
  100 // 10Hz
};

float measured[2][POINTS_COUNT][2];
#endif
measurement_t measured;
#ifdef __AUDIO__
#ifdef ENABLED_DUMP
static void
duplicate_buffer_to_dump(int16_t *p)
{
  if (dump_selection == 1)
    p = samp_buf;
  else if (dump_selection == 2)
    p = ref_buf;
  memcpy(dump_buffer, p, sizeof dump_buffer);
}
#endif
#ifdef __AUDIO__
void i2s_end_callback(I2SDriver *i2sp, size_t offset, size_t n)
{
#if PORT_SUPPORTS_RT
  int32_t cnt_s = port_rt_get_counter_value();
  int32_t cnt_e;
#endif
  int16_t *p = &rx_buffer[offset];
  (void)i2sp;
  (void)n;

  if (wait_count > 1) {
    --wait_count;
  } else if (wait_count > 0) {
    if (accumerate_count > 0) {
#ifndef TINYSA4
	//     dsp_process(p, n);
#endif
      accumerate_count--;
    }
#ifdef ENABLED_DUMP
    duplicate_buffer_to_dump(p);
#endif
  }

#if PORT_SUPPORTS_RT
  cnt_e = port_rt_get_counter_value();
  stat.interval_cycles = cnt_s - stat.last_counter_value;
  stat.busy_cycles = cnt_e - cnt_s;
  stat.last_counter_value = cnt_s;
#endif
  stat.callback_count++;
}

static const I2SConfig i2sconfig = {
  NULL, // TX Buffer
  rx_buffer, // RX Buffer
  AUDIO_BUFFER_LEN * 2,
  NULL, // tx callback
  i2s_end_callback, // rx callback
  0, // i2scfgr
  2 // i2spr
};
#endif
#endif

#define MAX_DATA    2
VNA_SHELL_FUNCTION(cmd_data)
{
  int i;
  int sel = 0;
  if (argc == 1)
    sel = my_atoi(argv[0]);

  if (sel >= 0 && sel <= MAX_DATA) {
    static const uint8_t sel_conv[]={TRACE_TEMP, TRACE_STORED, TRACE_ACTUAL};
    float *data = measured[sel_conv[sel]];
    for (i = 0; i < sweep_points; i++)
      shell_printf("%f\r\n", value(data[i]));
    return;
  }
  shell_printf("usage: data [0-2]\r\n");
}

#ifdef ENABLED_DUMP
VNA_SHELL_FUNCTION(cmd_dump)
{
  int i, j;
  int len;

  if (argc == 1)
    dump_selection = my_atoi(argv[0]);

  wait_dsp(3);

  len = AUDIO_BUFFER_LEN;
  if (dump_selection == 1 || dump_selection == 2)
    len /= 2;
  for (i = 0; i < len; ) {
    for (j = 0; j < 16; j++, i++) {
      shell_printf("%04x ", 0xffff & (int)dump_buffer[i]);
    }
    shell_printf("\r\n");
  }
}
#endif

#ifdef __REMOTE_DESKTOP__
VNA_SHELL_FUNCTION(cmd_refresh)
{
// read pixel count at one time (PART*2 bytes required for read buffer)
  int m = generic_option_cmd("refresh", "off|on", argc, argv[0]);
  if (m>=0) {
    auto_capture = m;
  }
}

int16_t mouse_x = 0;
int16_t mouse_y = 0;
uint8_t mouse_down = false;

VNA_SHELL_FUNCTION(cmd_touch)
{
  if (argc == 2){
    mouse_x = my_atoi(argv[0]);
    mouse_y = my_atoi(argv[1]);
    mouse_down = true;
    handle_touch_interrupt();
  }
}

VNA_SHELL_FUNCTION(cmd_release)
{
  if (argc==2) {
    mouse_x = my_atoi(argv[0]);
    mouse_y = my_atoi(argv[1]);
  }
  mouse_down = false;
  handle_touch_interrupt();
}
#endif

VNA_SHELL_FUNCTION(cmd_capture)
{
  // read pixel count at one time (PART*2 bytes required for read buffer)
  (void)argc;
  (void)argv;
  int y;
#ifdef TINYSA4  
#if SPI_BUFFER_SIZE < (2*LCD_WIDTH)
#error "Low size of spi_buffer for cmd_capture"
#endif
#else
#if SPI_BUFFER_SIZE < (3*LCD_WIDTH + 1)
#error "Low size of spi_buffer for cmd_capture"
#endif
#endif
  // read 2 row pixel time (read buffer limit by 2/3 + 1 from spi_buffer size)
  for (y = 0; y < LCD_HEIGHT; y += 2) {
    // use uint16_t spi_buffer[2048] (defined in ili9341) for read buffer
    uint8_t *buf = (uint8_t *)spi_buffer;
    ili9341_read_memory(0, y, LCD_WIDTH, 2, spi_buffer);
    streamWrite(shell_stream, (void*)buf, 2 * LCD_WIDTH * sizeof(uint16_t));
  }
}

void send_region(const char *t, int16_t x, int16_t y, int16_t w, int16_t h)
{
  if (SDU1.config->usbp->state == USB_ACTIVE) {
    shell_printf(t);
    struct {
      char new_str[2];
      int16_t x;
      int16_t y;
      int16_t w;
      int16_t h;
    } region={"\r\n", x,y,w,h};
    streamWrite(shell_stream, (void*)&region, sizeof(region));
  }
  else
    auto_capture = false;
}

void send_buffer(uint8_t * buf, int s)
{
  if (SDU1.config->usbp->state == USB_ACTIVE) {
    while (s > 0) {
      streamWrite(shell_stream, (void*) buf, (s > 128 ? 128 : s));
      buf = buf+128;
      s -= 128;
    }
    streamWrite(shell_stream, (void*)"ch> \r\n", 6);
  }
}

#if 0
VNA_SHELL_FUNCTION(cmd_gamma)
{
  float gamma[2];
  (void)argc;
  (void)argv;
  
  pause_sweep();
  chMtxLock(&mutex);
  wait_dsp(4);  
  calculate_gamma(gamma);
  chMtxUnlock(&mutex);

  shell_printf("%d %d\r\n", gamma[0], gamma[1]);
}
#endif
#ifdef __VNA__
static void (*sample_func)(float *gamma) = calculate_gamma;

VNA_SHELL_FUNCTION(cmd_sample)
{
  if (argc != 1) goto usage;
  //                                         0    1   2
  static const char cmd_sample_list[] = "gamma|ampl|ref";
  switch (get_str_index(argv[0], cmd_sample_list)) {
    case 0:
      sample_func = calculate_gamma;
      return;
    case 1:
      sample_func = fetch_amplitude;
      return;
    case 2:
      sample_func = fetch_amplitude_ref;
      return;
    default:
      break;
  }
usage:
  shell_printf("usage: sample {%s}\r\n", cmd_sample_list);
}
#endif
config_t config = {
  .magic =             CONFIG_MAGIC,
  .dac_value =         1922,
//  .touch_cal =         { 693, 605, 124, 171 },  // 2.4 inch LCD panel
#ifdef TINYSA3
  .touch_cal =         { 347, 495, 160, 205 },  // 2.8 inch LCD panel
#endif
#ifdef TINYSA4
  .touch_cal =          { 261, 605, 115, 146 }, // 4 inch panel
#endif
  ._mode     = _MODE_USB,
  ._serial_speed = USART_SPEED_SETTING(SERIAL_DEFAULT_BITRATE),
#ifdef __VNA__
  .harmonic_freq_threshold = 300000000,
#endif
  .lcd_palette = LCD_DEFAULT_PALETTE,
#ifdef TINYSA4
#endif
#ifdef TINYSA3
  .vbat_offset = 500,
  .low_level_offset =       100,    // Uncalibrated
  .high_level_offset =      100,    // Uncalibrated
  .low_correction_frequency = { 10000, 100000, 200000, 500000, 30000000, 140000000, 200000000, 300000000, 330000000, 350000000 },
  .low_correction_value = { +6.0, +2.8, +1.6, -0.4, 0.0, -0.4, +0.4, +3.0, +4.0, +8.1 },
  .high_correction_frequency = { 240000000, 280000000, 300000000, 400000000, 500000000, 600000000, 700000000, 800000000, 900000000, 960000000 },
  .high_correction_value = { 0, 0, 0, 0, 0.0, 0, 0, 0, 0, 0 },
  .setting_frequency_10mhz = 10000000,
  .cor_am = 0,// -10,
  .cor_wfm = 0, //-18,
  .cor_nfm = 0, //-18,
  .ext_zero_level = 128,
#endif
#ifdef TINYSA4
  .vbat_offset = 220,
  .frequency_IF1 = DEFAULT_IF,
  .frequency_IF2 = 0,
  .ultra_threshold = 800000000,
  .low_level_offset =       100.0,    // Uncalibrated
  .high_level_offset =      100.0,    // Uncalibrated
  .low_level_output_offset =   0.0,    // Uncalibrated
  .high_level_output_offset =  0.0,    // Uncalibrated
  .low_correction_frequency = { 10000, 100000, 200000, 500000, 30000000, 140000000, 200000000, 300000000, 330000000, 350000000 },
  .low_correction_value = { 0, 0, 0, 0, 0.0, 0, 0, 0, 0, 0 },
  .high_correction_frequency = { 10000, 100000, 200000, 500000, 50000000, 140000000, 200000000, 300000000, 330000000, 350000000 },
  .high_correction_value = { 0, 0, 0, 0, 0.0, 0, 0, 0, 0, 0 },
  .setting_frequency_30mhz = 30000000,
  .cor_am = 0,
  .cor_wfm = 0,
  .cor_nfm = 0,
  .ultra = false,
  .high_out_adf4350 = true,
  .ext_zero_level = 174,
#endif
  .sweep_voltage = 3.3,
  .switch_offset = 0.0,
};

//properties_t current_props;
//properties_t *active_props = &current_props;

// NanoVNA Default settings
static const trace_t def_trace[TRACES_MAX] = {//enable, type, channel, reserved, scale, refpos
 [TRACE_TEMP]   = { 0},  //Temp
 [TRACE_STORED] = { 0},  //Stored
 [TRACE_ACTUAL] = { 1}   //Actual
};

static const marker_t def_markers[MARKERS_MAX] = {
    { M_ENABLED,            M_REFERENCE | M_TRACKING,   30, 0 },
    { M_DISABLED,           M_NORMAL,                   40, 0 },
    { M_DISABLED,           M_NORMAL,                   60, 0 },
    { M_DISABLED,           M_NORMAL,                   80, 0 }
};

// Load propeties default settings
void load_LCD_properties(void)
{
//Magic add on caldata_save
//current_props.magic = CONFIG_MAGIC;
//  current_props._setting.frequency0   =         0;    // start =  0Hz
//  current_props._setting.frequency1   = 350000000;    // end   = 350MHz
//  current_props._setting.frequency_IF=  433800000,

  setting._sweep_points = POINTS_COUNT;
  #ifdef VNA__
  setting._cal_status   = 0;
//This data not loaded by default
//setting._frequencies[POINTS_COUNT];
//setting._cal_data[5][POINTS_COUNT][2];
//=============================================
  setting._electrical_delay = 0.0;
#endif
  setting.trace_scale = 10.0;
  setting.trace_refpos = 0;
  setting.waterfall = W_OFF;
  memcpy(setting._trace, def_trace, sizeof(def_trace));
  memcpy(setting._markers, def_markers, sizeof(def_markers));
#ifdef __VNA__
  setting._velocity_factor =  0.7;
#endif
  setting._active_marker   = 0;
#ifdef __VNA__
  setting._domain_mode     = 0;
  setting._marker_smith_format = MS_RLC;
#endif
  reset_settings(M_LOW);
//Checksum add on caldata_save
//setting.checksum = 0;
}

#ifdef __VNA__
void
ensure_edit_config(void)
{
  if (active_props == &current_props)
    return;

  //memcpy(&current_props, active_props, sizeof(config_t));
  active_props = &current_props;
  // move to uncal state
  cal_status = 0;
}
#endif

#include "sa_core.c"
#ifdef __AUDIO__
#define DSP_START(delay) wait_count = delay;
#define DSP_WAIT_READY   while (wait_count) __WFI();
#endif
#ifdef __VNA__
#define DELAY_CHANNEL_CHANGE 2

// main loop for measurement
bool sweep(bool break_on_operation)
{
  int i, delay;
  // blink LED while scanning
  palClearPad(GPIOB, GPIOB_LED);
  // Power stabilization after LED off, also align timings on i == 0
  for (i = 0; i < sweep_points; i++) {         // 5300
    if (frequencies[i] == 0) break;
    delay = set_frequency(frequencies[i]);     // 700
    tlv320aic3204_select(0);                   // 60 CH0:REFLECT, reset and begin measure
    dsp_start(delay + ((i == 0) ? 1 : 0));     // 1900
    //================================================
    // Place some code thats need execute while delay
    //================================================
    dsp_wait();
    // calculate reflection coefficient
    (*sample_func)(measured[0][i]);            // 60

    tlv320aic3204_select(1);                   // 60 CH1:TRANSMISSION, reset and begin measure
    dsp_start(DELAY_CHANNEL_CHANGE);           // 1700
    //================================================
    // Place some code thats need execute while delay
    //================================================
    dsp_wait();
    // calculate transmission coefficient
    (*sample_func)(measured[1][i]);            // 60
                                               // ======== 170 ===========
    if (cal_status & CALSTAT_APPLY)
      apply_error_term_at(i);

    if (electrical_delay != 0)
      apply_edelay_at(i);

    // back to toplevel to handle ui operation
    if (operation_requested && break_on_operation)
      return false;
  }
  // blink LED while scanning
  palSetPad(GPIOB, GPIOB_LED);
  return true;
}
#endif

void set_sweep_points(uint16_t points){
  if (points == sweep_points || points > POINTS_COUNT)
    return;

  sweep_points = points;
  update_frequencies();
}

VNA_SHELL_FUNCTION(cmd_scan)
{
  freq_t start, stop;
  uint32_t points = sweep_points;
  uint32_t i;
  if (argc < 2 || argc > 4) {
    shell_printf("usage: scan {start(Hz)} {stop(Hz)} [points] [outmask]\r\n");
    return;
  }

  start = my_atoui(argv[0]);
  stop = my_atoui(argv[1]);
  if (start > stop) {
      shell_printf("frequency range is invalid\r\n");
      return;
  }
  if (argc >= 3) {
    points = my_atoi(argv[2]);
    if (points <= 0 || points > sweep_points) {
      shell_printf("sweep points exceeds range "define_to_STR(POINTS_COUNT)"\r\n");
      return;
    }
  }
  set_frequencies(start, stop, points);
#ifdef __VNA__
  if (cal_auto_interpolate && (cal_status & CALSTAT_APPLY))
    cal_interpolate(lastsaveid);
#endif
  pause_sweep();
  sweep(false);
  // Output data after if set (faster data recive)
  if (argc == 4) {
    uint16_t mask = my_atoui(argv[3]);
    if (mask) {
      for (i = 0; i < points; i++) {
        if (mask & 1) shell_printf("%U ", frequencies[i]);
        if (mask & 2) shell_printf("%f %f ", value(measured[TRACE_ACTUAL][i]), 0.0);
        if (mask & 4) shell_printf("%f %f ", value(measured[TRACE_STORED][i]), 0.0);
        if (mask & 8) shell_printf("%f %f ", value(measured[TRACE_TEMP][i]), 0.0);
        shell_printf("\r\n");
      }
    }
  }
}

static void
update_marker_index(void)
{
  int m, idx;
  freq_t fstart = get_sweep_frequency(ST_START);
  freq_t fstop  = get_sweep_frequency(ST_STOP);
  for (m = 0; m < MARKERS_MAX; m++) {
    if (!markers[m].enabled)
      continue;
    freq_t f = markers[m].frequency;
    if (f == 0) idx = markers[m].index; // Not need update index in no freq
    else if (f < fstart) idx = 0;
    else if (f >= fstop) idx = sweep_points-1;
    else { // Search frequency index for marker frequency
#if 1
      for (idx = 1; idx < sweep_points; idx++) {
        if (frequencies[idx] <= f) continue;
        if (f < (frequencies[idx-1]/2 + frequencies[idx]/2)) idx--; // Correct closest idx
        break;
      }
#else
      float r = ((float)(f - fstart))/(fstop - fstart);
      idx = r * (sweep_points-1);
#endif
    }
    markers[m].index = idx;
    markers[m].frequency = frequencies[idx];
  }
}

void set_marker_frequency(int m, freq_t f)
{
  if (m == MARKER_INVALID || !markers[m].enabled)
    return;
  int i = 1;
  markers[m].mtype &= ~M_TRACKING;
  freq_t s = (frequencies[1] - frequencies[0])/2;
  while (i< sweep_points - 2){
    if (frequencies[i]-s  <= f && f < frequencies[i+1]-s) {     // Avoid rounding error in s!!!!!!!
      markers[m].index = i;
      markers[m].frequency = f;
      return;
    }
    i++;
  }
}

static void
set_frequencies(freq_t start, freq_t stop, uint16_t points)
{
  uint32_t i;
  freq_t step = (points - 1);
  freq_t span = stop - start;
  freq_t delta = span / step;
  freq_t error = span % step;
  freq_t f = start, df = step>>1;
  for (i = 0; i <= step; i++, f+=delta) {
    frequencies[i] = f;
    df+=error;
    if (df >=step) {
      f++;
      df -= step;
    }
  }
  // disable at out of sweep range
  for (; i < POINTS_COUNT; i++)
    frequencies[i] = 0;
  setting.frequency_step = delta;
  dirty = true;
}

void
update_frequencies(void)
{
  freq_t start, stop;
  start = get_sweep_frequency(ST_START);
  stop  = get_sweep_frequency(ST_STOP);

  set_frequencies(start, stop, sweep_points);
  // operation_requested|= OP_FREQCHANGE;

  update_marker_index();

  // set grid layout
  update_grid();
}

void
set_sweep_frequency(int type, freq_t freq)
{
  // Check frequency for out of bounds (minimum SPAN can be any value)
  if (type != ST_SPAN && freq < START_MIN)
    freq = START_MIN;
  if (freq > STOP_MAX)
    freq = STOP_MAX;
  bool cw_mode = FREQ_IS_CW(); // remember old mode
  freq_t center, span;
  switch (type) {
    case ST_START:
      setting.freq_mode &= ~FREQ_MODE_CENTER_SPAN;
      setting.frequency0 = freq;
      // if start > stop then make start = stop
      if (setting.frequency1 < freq) setting.frequency1 = freq;
      break;
    case ST_STOP:
      setting.freq_mode &= ~FREQ_MODE_CENTER_SPAN;
      setting.frequency1 = freq;
      // if start > stop then make start = stop
      if (setting.frequency0 > freq) setting.frequency0 = freq;
      break;
    case ST_CENTER:
      setting.freq_mode |= FREQ_MODE_CENTER_SPAN;
      center = setting.frequency0/2 + setting.frequency1/2;
      span   = (setting.frequency1 - setting.frequency0)/2;
      if (freq < START_MIN + span)
        span = (freq - START_MIN);
      if (freq > STOP_MAX - span)
        span = (STOP_MAX - freq);
      setting.frequency0 = freq - span;
      setting.frequency1 = freq + span;
      break;
    case ST_SPAN:
      setting.freq_mode |= FREQ_MODE_CENTER_SPAN;
      center = setting.frequency0/2 + setting.frequency1/2;
      span = freq/2;
      if (center < START_MIN + span)
        center = START_MIN + span;
      if (center > STOP_MAX - span)
        center = STOP_MAX - span;
      setting.frequency0 = center - span;
      setting.frequency1 = center + span;
      break;
    case ST_CW:
      setting.freq_mode |= FREQ_MODE_CENTER_SPAN;
      setting.frequency0 = freq;
      setting.frequency1 = freq;
      break;
  }
  if (!cw_mode && FREQ_IS_CW()) // switch to CW mode
    setting.sweep_time_us = 0;  // use minimum as start
  update_frequencies();
}

freq_t
get_sweep_frequency(int type)
{
  // Obsolete, ensure correct start/stop, start always must be < stop
  if (setting.frequency0 > setting.frequency1) {
    freq_t t = setting.frequency0;
    setting.frequency0 = setting.frequency1;
    setting.frequency1 = t;
  }
  switch (type) {
    case ST_START:  return setting.frequency0;
    case ST_STOP:   return setting.frequency1;
    case ST_CENTER: return setting.frequency0/2 + setting.frequency1/2;
    case ST_SPAN:   return setting.frequency1 - setting.frequency0;
    case ST_CW:     return setting.frequency0;
  }
  return 0;
}

VNA_SHELL_FUNCTION(cmd_sweep)
{
  if (argc == 0) {
    shell_printf("%D %D %d\r\n", get_sweep_frequency(ST_START), get_sweep_frequency(ST_STOP), sweep_points);
    return;
  } else if (argc > 3) {
    goto usage;
  }
  freq_t value0 = 0;
  freq_t value1 = 0;
  freq_t value2 = 0;
  if (argc >= 1) value0 = my_atoui(argv[0]);
  if (argc >= 2) value1 = my_atoui(argv[1]);
  if (argc >= 3) value2 = my_atoui(argv[2]);
#if MAX_FREQ_TYPE != 5
#error "Sweep mode possibly changed, check cmd_sweep function"
#endif
  // Parse sweep {start|stop|center|span|cw} {freq(Hz)}
  // get enum ST_START, ST_STOP, ST_CENTER, ST_SPAN, ST_CW
  static const char sweep_cmd[] = "start|stop|center|span|cw";
  int type = get_str_index(argv[0], sweep_cmd);
  if (type >=0) {
    set_sweep_frequency(type, value1);
    return;
  }
  //  Parse sweep {start(Hz)} [stop(Hz)]
  set_sweep_frequency(ST_START, value0);
  if (value1)
    set_sweep_frequency(ST_STOP, value1);
  if (value2)
    set_sweep_points(value2);
  return;
usage:
  shell_printf("usage: sweep {start(Hz)} [stop(Hz)] [points]\r\n"\
               "\tsweep {%s} {freq(Hz)}\r\n", sweep_cmd);
}

#ifdef __VNA__
static void
eterm_set(int term, float re, float im)
{
  int i;
  for (i = 0; i < sweep_points; i++) {
    cal_data[term][i][0] = re;
    cal_data[term][i][1] = im;
  }
}

static void
eterm_copy(int dst, int src)
{
  memcpy(cal_data[dst], cal_data[src], sizeof cal_data[dst]);
}

#if 0
const struct open_model {
  float c0;
  float c1;
  float c2;
  float c3;
} open_model = { 50, 0, -300, 27 };
#endif

#if 0
static void
adjust_ed(void)
{
  int i;
  for (i = 0; i < sweep_points; i++) {
    // z=1/(jwc*z0) = 1/(2*pi*f*c*z0)  Note: normalized with Z0
    // s11ao = (z-1)/(z+1) = (1-1/z)/(1+1/z) = (1-jwcz0)/(1+jwcz0)
    // prepare 1/s11ao to avoid dividing complex
    float c = 1000e-15;
    float z0 = 50;
    //float z = 2 * VNA_PI * frequencies[i] * c * z0;
    float z = 0.02;
    cal_data[ETERM_ED][i][0] += z;
  }
}
#endif

static void
eterm_calc_es(void)
{
  int i;
  for (i = 0; i < sweep_points; i++) {
    // z=1/(jwc*z0) = 1/(2*pi*f*c*z0)  Note: normalized with Z0
    // s11ao = (z-1)/(z+1) = (1-1/z)/(1+1/z) = (1-jwcz0)/(1+jwcz0)
    // prepare 1/s11ao for effeiciency
    float c = 50e-15;
    //float c = 1.707e-12;
    float z0 = 50;
    float z = 2 * VNA_PI * frequencies[i] * c * z0;
    float sq = 1 + z*z;
    float s11aor = (1 - z*z) / sq;
    float s11aoi = 2*z / sq;

    // S11mo’= S11mo - Ed
    // S11ms’= S11ms - Ed
    float s11or = cal_data[CAL_OPEN][i][0] - cal_data[ETERM_ED][i][0];
    float s11oi = cal_data[CAL_OPEN][i][1] - cal_data[ETERM_ED][i][1];
    float s11sr = cal_data[CAL_SHORT][i][0] - cal_data[ETERM_ED][i][0];
    float s11si = cal_data[CAL_SHORT][i][1] - cal_data[ETERM_ED][i][1];
    // Es = (S11mo'/s11ao + S11ms’)/(S11mo' - S11ms’)
    float numr = s11sr + s11or * s11aor - s11oi * s11aoi;
    float numi = s11si + s11oi * s11aor + s11or * s11aoi;
    float denomr = s11or - s11sr;
    float denomi = s11oi - s11si;
    sq = denomr*denomr+denomi*denomi;
    cal_data[ETERM_ES][i][0] = (numr*denomr + numi*denomi)/sq;
    cal_data[ETERM_ES][i][1] = (numi*denomr - numr*denomi)/sq;
  }
  cal_status &= ~CALSTAT_OPEN;
  cal_status |= CALSTAT_ES;
}

static void
eterm_calc_er(int sign)
{
  int i;
  for (i = 0; i < sweep_points; i++) {
    // Er = sign*(1-sign*Es)S11ms'
    float s11sr = cal_data[CAL_SHORT][i][0] - cal_data[ETERM_ED][i][0];
    float s11si = cal_data[CAL_SHORT][i][1] - cal_data[ETERM_ED][i][1];
    float esr = cal_data[ETERM_ES][i][0];
    float esi = cal_data[ETERM_ES][i][1];
    if (sign > 0) {
      esr = -esr;
      esi = -esi;
    }
    esr = 1 + esr;
    float err = esr * s11sr - esi * s11si;
    float eri = esr * s11si + esi * s11sr;
    if (sign < 0) {
      err = -err;
      eri = -eri;
    }
    cal_data[ETERM_ER][i][0] = err;
    cal_data[ETERM_ER][i][1] = eri;
  }
  cal_status &= ~CALSTAT_SHORT;
  cal_status |= CALSTAT_ER;
}

// CAUTION: Et is inversed for efficiency
static void
eterm_calc_et(void)
{
  int i;
  for (i = 0; i < sweep_points; i++) {
    // Et = 1/(S21mt - Ex)
    float etr = cal_data[CAL_THRU][i][0] - cal_data[CAL_ISOLN][i][0];
    float eti = cal_data[CAL_THRU][i][1] - cal_data[CAL_ISOLN][i][1];
    float sq = etr*etr + eti*eti;
    float invr = etr / sq;
    float invi = -eti / sq;
    cal_data[ETERM_ET][i][0] = invr;
    cal_data[ETERM_ET][i][1] = invi;
  }
  cal_status &= ~CALSTAT_THRU;
  cal_status |= CALSTAT_ET;
}

#if 0
void apply_error_term(void)
{
  int i;
  for (i = 0; i < sweep_points; i++) {
    // S11m' = S11m - Ed
    // S11a = S11m' / (Er + Es S11m')
    float s11mr = measured[0][i][0] - cal_data[ETERM_ED][i][0];
    float s11mi = measured[0][i][1] - cal_data[ETERM_ED][i][1];
    float err = cal_data[ETERM_ER][i][0] + s11mr * cal_data[ETERM_ES][i][0] - s11mi * cal_data[ETERM_ES][i][1];
    float eri = cal_data[ETERM_ER][i][1] + s11mr * cal_data[ETERM_ES][i][1] + s11mi * cal_data[ETERM_ES][i][0];
    float sq = err*err + eri*eri;
    float s11ar = (s11mr * err + s11mi * eri) / sq;
    float s11ai = (s11mi * err - s11mr * eri) / sq;
    measured[0][i][0] = s11ar;
    measured[0][i][1] = s11ai;

    // CAUTION: Et is inversed for efficiency
    // S21m' = S21m - Ex
    // S21a = S21m' (1-EsS11a)Et
    float s21mr = measured[1][i][0] - cal_data[ETERM_EX][i][0];
    float s21mi = measured[1][i][1] - cal_data[ETERM_EX][i][1];
    float esr = 1 - (cal_data[ETERM_ES][i][0] * s11ar - cal_data[ETERM_ES][i][1] * s11ai);
    float esi = - (cal_data[ETERM_ES][i][1] * s11ar + cal_data[ETERM_ES][i][0] * s11ai);
    float etr = esr * cal_data[ETERM_ET][i][0] - esi * cal_data[ETERM_ET][i][1];
    float eti = esr * cal_data[ETERM_ET][i][1] + esi * cal_data[ETERM_ET][i][0];
    float s21ar = s21mr * etr - s21mi * eti;
    float s21ai = s21mi * etr + s21mr * eti;
    measured[1][i][0] = s21ar;
    measured[1][i][1] = s21ai;
  }
}
#endif

static void apply_error_term_at(int i)
{
    // S11m' = S11m - Ed
    // S11a = S11m' / (Er + Es S11m')
    float s11mr = measured[0][i][0] - cal_data[ETERM_ED][i][0];
    float s11mi = measured[0][i][1] - cal_data[ETERM_ED][i][1];
    float err = cal_data[ETERM_ER][i][0] + s11mr * cal_data[ETERM_ES][i][0] - s11mi * cal_data[ETERM_ES][i][1];
    float eri = cal_data[ETERM_ER][i][1] + s11mr * cal_data[ETERM_ES][i][1] + s11mi * cal_data[ETERM_ES][i][0];
    float sq = err*err + eri*eri;
    float s11ar = (s11mr * err + s11mi * eri) / sq;
    float s11ai = (s11mi * err - s11mr * eri) / sq;
    measured[0][i][0] = s11ar;
    measured[0][i][1] = s11ai;

    // CAUTION: Et is inversed for efficiency
    // S21m' = S21m - Ex
    // S21a = S21m' (1-EsS11a)Et
    float s21mr = measured[1][i][0] - cal_data[ETERM_EX][i][0];
    float s21mi = measured[1][i][1] - cal_data[ETERM_EX][i][1];
    float esr = 1 - (cal_data[ETERM_ES][i][0] * s11ar - cal_data[ETERM_ES][i][1] * s11ai);
    float esi = - (cal_data[ETERM_ES][i][1] * s11ar + cal_data[ETERM_ES][i][0] * s11ai);
    float etr = esr * cal_data[ETERM_ET][i][0] - esi * cal_data[ETERM_ET][i][1];
    float eti = esr * cal_data[ETERM_ET][i][1] + esi * cal_data[ETERM_ET][i][0];
    float s21ar = s21mr * etr - s21mi * eti;
    float s21ai = s21mi * etr + s21mr * eti;
    measured[1][i][0] = s21ar;
    measured[1][i][1] = s21ai;
}

static void apply_edelay_at(int i)
{
  float w = 2 * VNA_PI * electrical_delay * frequencies[i] * 1E-12;
  float s = sin(w);
  float c = cos(w);
  float real = measured[0][i][0];
  float imag = measured[0][i][1];
  measured[0][i][0] = real * c - imag * s;
  measured[0][i][1] = imag * c + real * s;
  real = measured[1][i][0];
  imag = measured[1][i][1];
  measured[1][i][0] = real * c - imag * s;
  measured[1][i][1] = imag * c + real * s;
}

void
cal_collect(int type)
{
  ensure_edit_config();
  int dst, src;
  switch (type) {
    case CAL_LOAD:  cal_status|= CALSTAT_LOAD;  dst = CAL_LOAD;  src = 0; break;
    case CAL_OPEN:  cal_status|= CALSTAT_OPEN;  dst = CAL_OPEN;  src = 0; cal_status&= ~(CALSTAT_ES|CALSTAT_APPLY); break;
    case CAL_SHORT: cal_status|= CALSTAT_SHORT; dst = CAL_SHORT; src = 0; cal_status&= ~(CALSTAT_ER|CALSTAT_APPLY); break;
    case CAL_THRU:  cal_status|= CALSTAT_THRU;  dst = CAL_THRU;  src = 1; break;
    case CAL_ISOLN: cal_status|= CALSTAT_ISOLN; dst = CAL_ISOLN; src = 1; break;
    default:
      return;
  }
  // Run sweep for collect data
  sweep(false);
  // Copy calibration data
  memcpy(cal_data[dst], measured[src], sizeof measured[0]);
  redraw_request |= REDRAW_CAL_STATUS;
}

void
cal_done(void)
{
  ensure_edit_config();
  if (!(cal_status & CALSTAT_LOAD))
    eterm_set(ETERM_ED, 0.0, 0.0);
  //adjust_ed();
  if ((cal_status & CALSTAT_SHORT) && (cal_status & CALSTAT_OPEN)) {
    eterm_calc_es();
    eterm_calc_er(-1);
  } else if (cal_status & CALSTAT_OPEN) {
    eterm_copy(CAL_SHORT, CAL_OPEN);
    eterm_set(ETERM_ES, 0.0, 0.0);
    eterm_calc_er(1);
  } else if (cal_status & CALSTAT_SHORT) {
    eterm_set(ETERM_ES, 0.0, 0.0);
    cal_status &= ~CALSTAT_SHORT;
    eterm_calc_er(-1);
  } else {
    eterm_set(ETERM_ER, 1.0, 0.0);
    eterm_set(ETERM_ES, 0.0, 0.0);
  }
    
  if (!(cal_status & CALSTAT_ISOLN))
    eterm_set(ETERM_EX, 0.0, 0.0);
  if (cal_status & CALSTAT_THRU) {
    eterm_calc_et();
  } else {
    eterm_set(ETERM_ET, 1.0, 0.0);
  }

  cal_status |= CALSTAT_APPLY;
  redraw_request |= REDRAW_CAL_STATUS;
}

static void
cal_interpolate(int s)
{
  const properties_t *src = caldata_ref(s);
  int i, j;
  int eterm;
  if (src == NULL)
    return;

  ensure_edit_config();

  // lower than start freq of src range
  for (i = 0; i < sweep_points; i++) {
    if (frequencies[i] >= src->_frequencies[0])
      break;

    // fill cal_data at head of src range
    for (eterm = 0; eterm < 5; eterm++) {
      cal_data[eterm][i][0] = src->_cal_data[eterm][0][0];
      cal_data[eterm][i][1] = src->_cal_data[eterm][0][1];
    }
  }

  j = 0;
  for (; i < sweep_points; i++) {
    uint32_t f = frequencies[i];
    if (f == 0) goto interpolate_finish;
    for (; j < src->_sweep_points-1; j++) {
      if (src->_frequencies[j] <= f && f < src->_frequencies[j+1]) {
        // found f between freqs at j and j+1
        float k1 = (float)(f - src->_frequencies[j])
                        / (src->_frequencies[j+1] - src->_frequencies[j]);

        // avoid glitch between freqs in different harmonics mode
        if (IS_HARMONIC_MODE(src->_frequencies[j]) != IS_HARMONIC_MODE(src->_frequencies[j+1])) {
          // assume f[j] < f[j+1]
          k1 = IS_HARMONIC_MODE(f) ? 1.0 : 0.0;
        }

        float k0 = 1.0 - k1;
        for (eterm = 0; eterm < 5; eterm++) {
          cal_data[eterm][i][0] = src->_cal_data[eterm][j][0] * k0 + src->_cal_data[eterm][j+1][0] * k1;
          cal_data[eterm][i][1] = src->_cal_data[eterm][j][1] * k0 + src->_cal_data[eterm][j+1][1] * k1;
        }
        break;
      }
    }
    if (j == src->_sweep_points-1)
      break;
  }

  // upper than end freq of src range
  for (; i < sweep_points; i++) {
    // fill cal_data at tail of src
    for (eterm = 0; eterm < 5; eterm++) {
      cal_data[eterm][i][0] = src->_cal_data[eterm][src->_sweep_points-1][0];
      cal_data[eterm][i][1] = src->_cal_data[eterm][src->_sweep_points-1][1];
    }
  }
interpolate_finish:
  cal_status |= src->_cal_status | CALSTAT_APPLY | CALSTAT_INTERPOLATED;
  redraw_request |= REDRAW_CAL_STATUS;
}

VNA_SHELL_FUNCTION(cmd_cal)
{
  static const char *items[] = { "load", "open", "short", "thru", "isoln", "Es", "Er", "Et", "cal'ed" };

  if (argc == 0) {
    int i;
    for (i = 0; i < 9; i++) {
      if (cal_status & (1<<i))
        shell_printf("%s ", items[i]);
    }
    shell_printf("\r\n");
    return;
  }
  redraw_request|=REDRAW_CAL_STATUS;
  //                                     0    1     2    3     4    5  6   7     8    9 10
  static const char cmd_cal_list[] = "load|open|short|thru|isoln|done|on|off|reset|data|in";
  switch (get_str_index(argv[0], cmd_cal_list)) {
    case 0:
      cal_collect(CAL_LOAD);
      return;
    case 1:
      cal_collect(CAL_OPEN);
      return;
    case 2:
      cal_collect(CAL_SHORT);
      return;
    case 3:
      cal_collect(CAL_THRU);
      return;
    case 4:
      cal_collect(CAL_ISOLN);
      return;
    case 5:
      cal_done();
      return;
    case 6:
      cal_status |= CALSTAT_APPLY;
      return;
    case 7:
      cal_status &= ~CALSTAT_APPLY;
      return;
    case 8:
      cal_status = 0;
      return;
    case 9:
      shell_printf("%f %f\r\n", cal_data[CAL_LOAD][0][0], cal_data[CAL_LOAD][0][1]);
      shell_printf("%f %f\r\n", cal_data[CAL_OPEN][0][0], cal_data[CAL_OPEN][0][1]);
      shell_printf("%f %f\r\n", cal_data[CAL_SHORT][0][0], cal_data[CAL_SHORT][0][1]);
      shell_printf("%f %f\r\n", cal_data[CAL_THRU][0][0], cal_data[CAL_THRU][0][1]);
      shell_printf("%f %f\r\n", cal_data[CAL_ISOLN][0][0], cal_data[CAL_ISOLN][0][1]);
      return;
    case 10:
      cal_interpolate((argc > 1) ? my_atoi(argv[1]) : 0);
      return;
    default:
      break;
  }
  shell_printf("usage: cal [%s]\r\n", cmd_cal_list);
}
#endif

VNA_SHELL_FUNCTION(cmd_save)
{
  if (argc != 1)
    goto usage;

  int id = my_atoi(argv[0]);
  if (id < 0 || id >= SAVEAREA_MAX)
    goto usage;
  caldata_save(id);
  redraw_request |= REDRAW_CAL_STATUS;
  return;

 usage:
  shell_printf("save {id}\r\n");
}

VNA_SHELL_FUNCTION(cmd_recall)
{
  if (argc != 1)
    goto usage;

  int id = my_atoi(argv[0]);
  if (id < 0 || id >= SAVEAREA_MAX)
    goto usage;
  // Check for success
  if (caldata_recall(id) == -1)
    shell_printf("Err, default load\r\n");
  update_frequencies();
  redraw_request |= REDRAW_CAL_STATUS;
  return;
 usage:
  shell_printf("recall {id}\r\n");
}

const char * const trc_channel_name[] = {
  [TRACE_ACTUAL] = "ACTUAL",
  [TRACE_STORED] = "STORED",
  [TRACE_TEMP]   = "COMPUTED",
};

void set_trace_scale(float scale)
{
  if (setting.trace_scale == scale) return;
  setting.trace_scale = scale;
  redraw_request |= REDRAW_AREA | REDRAW_CAL_STATUS;
}

void set_trace_refpos(float refpos)
{
  if (setting.trace_refpos == refpos) return;
  setting.trace_refpos = refpos;
  redraw_request |= REDRAW_AREA | REDRAW_CAL_STATUS;
}

VNA_SHELL_FUNCTION(cmd_trace)
{
  int t;
  if (argc == 0) {
    for (t = 0; t < TRACES_MAX; t++) {
      if (trace[t].enabled) {
        const char *type = unit_string[setting.unit]; // get_trace_typename(t);
        const char *channel = trc_channel_name[t];
        float scale = get_trace_scale();
        float refpos = get_trace_refpos();
        shell_printf("%d %s %s %f %f\r\n", t, type, channel, scale, refpos);
      }
    }
    return;
  }

  if ('0' <= argv[0][0] && argv[0][0] <= '9') {
    t = my_atoi(argv[0]);
    if (argc != 1 || t < 0 || t >= TRACES_MAX)
      goto usage;
    const char *type = "LOGMAG";//unit_string[setting.unit];
    const char *channel = trc_channel_name[t];
    shell_printf("%d %s %s\r\n", t, type, channel);
    return;
  }
#if MAX_UNIT_TYPE != 4
#error "Unit type enum possibly changed, check cmd_trace function"
#endif
  static const char cmd_type_list[] = "dBm|dBmV|dBuV|V|W";
  if (argc == 1) {
    int type = get_str_index(argv[0], cmd_type_list);
    if (type >= 0) {
      set_unit(type);
      goto update;
    }
//    goto usage;
  }
  static const char cmd_store_list[] = "store|clear|subtract";
  if (argc == 1) {
    int type = get_str_index(argv[0], cmd_store_list);
    if (type >= 0) {
      switch(type) {
      case 0:
        set_storage();
        goto update;
      case 1:
        set_clear_storage();
        goto update;
      case 2:
        set_subtract_storage();
        goto update;
      }
    }
//    goto usage;
  }
  //                                            0      1
  static const char cmd_scale_ref_list[] = "scale|reflevel";
  if (argc == 2) {
    switch (get_str_index(argv[0], cmd_scale_ref_list)) {
    case 0:
      if (get_str_index(argv[1],"auto") == 0) {
        set_auto_reflevel(true);
      } else {
        user_set_scale(my_atof(argv[1]));
      }
      goto update;
    case 1:
      //trace[t].refpos = my_atof(argv[2]);
      if (get_str_index(argv[1],"auto") == 0) {
        set_auto_reflevel(true);
      } else {
        user_set_reflevel(my_atof(argv[1]));
      }
      goto update;
    }
    goto usage;
  }
update:
redraw_request |= REDRAW_CAL_STATUS;
  return;
usage:
  shell_printf("trace {%s}\r\n"\
               "trace {%s}\r\n"\
               "trace {%s} {value|auto}\r\n", cmd_store_list, cmd_type_list, cmd_scale_ref_list);
}


#ifdef __VNA__
void set_electrical_delay(float picoseconds)
{
  if (electrical_delay != picoseconds) {
    electrical_delay = picoseconds;
    force_set_markmap();
  }
  redraw_request |= REDRAW_MARKER;
}

float get_electrical_delay(void)
{
  return electrical_delay;
}

VNA_SHELL_FUNCTION(cmd_edelay)
{
  if (argc == 0) {
    shell_printf("%f\r\n", electrical_delay);
    return;
  }
  if (argc > 0) {
    set_electrical_delay(my_atof(argv[0]));
  }
}
#endif


VNA_SHELL_FUNCTION(cmd_marker)
{
  int t;
  if (argc == 0) {
    for (t = 0; t < MARKERS_MAX; t++) {
      if (markers[t].enabled) {
        shell_printf("%d %d %D %f\r\n", t+1, markers[t].index, markers[t].frequency, value(actual_t[markers[t].index]));
      }
    }
    return;
  }
  redraw_request |= REDRAW_MARKER;
  if (get_str_index(argv[0], "off") == 0) {
    active_marker = MARKER_INVALID;
    for (t = 0; t < MARKERS_MAX; t++)
      markers[t].enabled = FALSE;
    return;
  }
  t = my_atoi(argv[0])-1;
  if (t < 0 || t >= MARKERS_MAX)
    goto usage;
  if (argc == 1) {
  display_marker:
    shell_printf("%d %d %D %.2f\r\n", t+1, markers[t].index, markers[t].frequency, value(actual_t[markers[t].index]));
    active_marker = t;
    // select active marker
    markers[t].enabled = TRUE;
    return;
  }
  static const char cmd_marker_list[] = "on|off|peak";
  switch (get_str_index(argv[1], cmd_marker_list)) {
    case 0: markers[t].enabled = TRUE; active_marker = t; return;
    case 1: markers[t].enabled =FALSE; if (active_marker == t) active_marker = MARKER_INVALID; return;
    case 2: markers[t].enabled = TRUE; active_marker = t;
      int i = marker_search_max();
      if (i == -1) i = 0;
      markers[active_marker].index = i;
      markers[active_marker].frequency = frequencies[i];
      goto display_marker;
    default:
      // select active marker and move to index or frequency
      markers[t].enabled = TRUE;
      freq_t value = my_atoui(argv[1]);
      markers[t].mtype &= ~M_TRACKING;
      active_marker = t;
      if (value > sweep_points)
        set_marker_frequency(active_marker, value);
      else {
        markers[t].index = value;
        markers[t].frequency = frequencies[value];
      }
      return;
  }
 usage:
  shell_printf("marker [n] [%s|{freq}|{index}]\r\n", cmd_marker_list);
}

VNA_SHELL_FUNCTION(cmd_touchcal)
{
  (void)argc;
  (void)argv;
  //extern int16_t touch_cal[4];
  int i;

  shell_printf("first touch upper left, then lower right...");
  touch_cal_exec();
  shell_printf("done\r\n");

  shell_printf("touch cal params: ");
  for (i = 0; i < 4; i++) {
    shell_printf("%d ", config.touch_cal[i]);
  }
  shell_printf("\r\n");
}

VNA_SHELL_FUNCTION(cmd_touchtest)
{
  (void)argc;
  (void)argv;
  do {
    touch_draw_test();
  } while (argc);
}

VNA_SHELL_FUNCTION(cmd_frequencies)
{
  int i;
  (void)argc;
  (void)argv;
  for (i = 0; i < sweep_points; i++) {
    if (frequencies[i] != 0)
      shell_printf("%U\r\n", frequencies[i]);
  }
}

#ifdef __VNA__
static void
set_domain_mode(int mode) // accept DOMAIN_FREQ or DOMAIN_TIME
{
  if (mode != (domain_mode & DOMAIN_MODE)) {
    domain_mode = (domain_mode & ~DOMAIN_MODE) | (mode & DOMAIN_MODE);
    redraw_request |= REDRAW_FREQUENCY;
    uistat.lever_mode = LM_MARKER;
  }
}

static void
set_timedomain_func(int func) // accept TD_FUNC_LOWPASS_IMPULSE, TD_FUNC_LOWPASS_STEP or TD_FUNC_BANDPASS
{
  domain_mode = (domain_mode & ~TD_FUNC) | (func & TD_FUNC);
}

static void
set_timedomain_window(int func) // accept TD_WINDOW_MINIMUM/TD_WINDOW_NORMAL/TD_WINDOW_MAXIMUM
{
  domain_mode = (domain_mode & ~TD_WINDOW) | (func & TD_WINDOW);
}

VNA_SHELL_FUNCTION(cmd_transform)
{
  int i;
  if (argc == 0) {
    goto usage;
  }
  //                                         0   1       2    3        4       5      6       7
  static const char cmd_transform_list[] = "on|off|impulse|step|bandpass|minimum|normal|maximum";
  for (i = 0; i < argc; i++) {
    switch (get_str_index(argv[i], cmd_transform_list)) {
      case 0:
        set_domain_mode(DOMAIN_TIME);
        return;
      case 1:
        set_domain_mode(DOMAIN_FREQ);
        return;
      case 2:
        set_timedomain_func(TD_FUNC_LOWPASS_IMPULSE);
        return;
      case 3:
        set_timedomain_func(TD_FUNC_LOWPASS_STEP);
        return;
      case 4:
        set_timedomain_func(TD_FUNC_BANDPASS);
        return;
      case 5:
        set_timedomain_window(TD_WINDOW_MINIMUM);
        return;
      case 6:
        set_timedomain_window(TD_WINDOW_NORMAL);
        return;
      case 7:
        set_timedomain_window(TD_WINDOW_MAXIMUM);
        return;
      default:
        goto usage;
    }
  }
  return;
usage:
  shell_printf("usage: transform {%s} [...]\r\n", cmd_transform_list);
}
#endif

VNA_SHELL_FUNCTION(cmd_test)
{
  (void)argc;
  (void)argv;

#if 0
  int i;
  for (i = 0; i < 100; i++) {
    palClearPad(GPIOB, GPIOB_LED);
    set_frequency(10000000);
    palSetPad(GPIOB, GPIOB_LED);
    chThdSleepMilliseconds(50);

    palClearPad(GPIOB, GPIOB_LED);
    set_frequency(90000000);
    palSetPad(GPIOB, GPIOB_LED);
    chThdSleepMilliseconds(50);
  }
#endif

#if 0
  int i;
  int mode = 0;
  if (argc >= 1)
    mode = my_atoi(argv[0]);

  for (i = 0; i < 20; i++) {
    palClearPad(GPIOB, GPIOB_LED);
    ili9341_test(mode);
    palSetPad(GPIOB, GPIOB_LED);
    chThdSleepMilliseconds(50);
  }
#endif

#if 0
  //extern adcsample_t adc_samples[2];
  //shell_printf("adc: %d %d\r\n", adc_samples[0], adc_samples[1]);
  int i;
  int x, y;
  for (i = 0; i < 50; i++) {
    test_touch(&x, &y);
    shell_printf("adc: %d %d\r\n", x, y);
    chThdSleepMilliseconds(200);
  }
  //extern int touch_x, touch_y;
  //shell_printf("adc: %d %d\r\n", touch_x, touch_y);
#endif

  while (argc > 1) {
    int x, y;
    touch_position(&x, &y);
    shell_printf("touch: %d %d\r\n", x, y);
    chThdSleepMilliseconds(200);
  }
}

#ifdef __VNA__
VNA_SHELL_FUNCTION(cmd_gain)
{
  int rvalue;
  int lvalue = 0;
  if (argc != 1 && argc != 2) {
    shell_printf("usage: gain {lgain(0-95)} [rgain(0-95)]\r\n");
    return;
  }
  rvalue = my_atoi(argv[0]);
  if (argc == 2) 
    lvalue = my_atoi(argv[1]);
  tlv320aic3204_set_gain(lvalue, rvalue);
}

VNA_SHELL_FUNCTION(cmd_port)
{
  int port;
  if (argc != 1) {
    shell_printf("usage: port {0:TX 1:RX}\r\n");
    return;
  }
  port = my_atoi(argv[0]);
  tlv320aic3204_select(port);
}

VNA_SHELL_FUNCTION(cmd_stat)
{
  int16_t *p = &rx_buffer[0];
  int32_t acc0, acc1;
  int32_t ave0, ave1;
  int32_t count = AUDIO_BUFFER_LEN;
  int i;
  (void)argc;
  (void)argv;
  acc0 = acc1 = 0;
  for (i = 0; i < AUDIO_BUFFER_LEN*2; i += 2) {
    acc0 += p[i];
    acc1 += p[i+1];
  }
  ave0 = acc0 / count;
  ave1 = acc1 / count;
  acc0 = acc1 = 0;
  for (i = 0; i < AUDIO_BUFFER_LEN*2; i += 2) {
    acc0 += (p[i] - ave0)*(p[i] - ave0);
    acc1 += (p[i+1] - ave1)*(p[i+1] - ave1);
  }
  stat.rms[0] = sqrtf(acc0 / count);
  stat.rms[1] = sqrtf(acc1 / count);
  stat.ave[0] = ave0;
  stat.ave[1] = ave1;

  shell_printf("average: %d %d\r\n", stat.ave[0], stat.ave[1]);
  shell_printf("rms: %d %d\r\n", stat.rms[0], stat.rms[1]);
  shell_printf("callback count: %d\r\n", stat.callback_count);
  //shell_printf("interval cycle: %d\r\n", stat.interval_cycles);
  //shell_printf("busy cycle: %d\r\n", stat.busy_cycles);
  //shell_printf("load: %d\r\n", stat.busy_cycles * 100 / stat.interval_cycles);
//  extern int awd_count;
//  shell_printf("awd: %d\r\n", awd_count);
}
#endif

#ifndef VERSION
#define VERSION "unknown"
#endif

const char NANOVNA_VERSION[] = VERSION;

VNA_SHELL_FUNCTION(cmd_version)
{
  (void)argc;
  (void)argv;
  shell_printf("%s\r\n", NANOVNA_VERSION);
}

VNA_SHELL_FUNCTION(cmd_vbat)
{
  (void)argc;
  (void)argv;
  shell_printf("%d mV\r\n", adc_vbat_read());
}

#ifdef ENABLE_VBAT_OFFSET_COMMAND
VNA_SHELL_FUNCTION(cmd_vbat_offset)
{
  if (argc != 1) {
    shell_printf("%d\r\n", config.vbat_offset);
    return;
  }
  config.vbat_offset = (int16_t)my_atoi(argv[0]);
}
#endif

#ifdef ENABLE_INFO_COMMAND
VNA_SHELL_FUNCTION(cmd_info)
{
  (void)argc;
  (void)argv;
  int i = 0;
  while (info_about[i])
    shell_printf("%s\r\n", info_about[i++]);
}
#endif

#ifdef ENABLE_COLOR_COMMAND
VNA_SHELL_FUNCTION(cmd_color)
{
  uint32_t color;
  int i;
  if (argc != 2) {
    shell_printf("usage: color {id} {rgb24}\r\n");
    for (i=0; i < MAX_PALETTE; i++) {
      color = GET_PALTETTE_COLOR(i);
      color = HEXRGB(color);
      shell_printf(" %2d: 0x%06x\r\n", i, color);
    }
    return;
  }
  i = my_atoi(argv[0]);
  if (i >= MAX_PALETTE)
    return;
  color = RGBHEX(my_atoui(argv[1]));
  config.lcd_palette[i] = color;

  // Redraw all
  redraw_request|= REDRAW_AREA;
}
#endif

#ifdef ENABLE_THREADS_COMMAND
#if CH_CFG_USE_REGISTRY == FALSE
#error "Threads Requite enabled CH_CFG_USE_REGISTRY in chconf.h"
#endif
const char *states[] = {CH_STATE_NAMES};
VNA_SHELL_FUNCTION(cmd_threads) 
{
  thread_t *tp;
  (void)argc;
  (void)argv;
  shell_printf("stklimit|        |stk free|    addr|refs|prio|    state|        name"VNA_SHELL_NEWLINE_STR);
  tp = chRegFirstThread();
  do {
    uint32_t max_stack_use = 0U;
#if (CH_DBG_ENABLE_STACK_CHECK == TRUE) || (CH_CFG_USE_DYNAMIC == TRUE)
    uint32_t stklimit = (uint32_t)tp->wabase;
#if CH_DBG_FILL_THREADS == TRUE
    uint8_t *p = (uint8_t *)tp->wabase; while(p[max_stack_use]==CH_DBG_STACK_FILL_VALUE) max_stack_use++;
#endif
#else
    uint32_t stklimit = 0U;
#endif
    shell_printf("%08x|%08x|%08x|%08x|%4u|%4u|%9s|%12s"VNA_SHELL_NEWLINE_STR,
             stklimit, (uint32_t)tp->ctx.sp, max_stack_use, (uint32_t)tp,
             (uint32_t)tp->refs - 1, (uint32_t)tp->prio, states[tp->state],
             tp->name == NULL ? "" : tp->name);
    tp = chRegNextThread(tp);
  } while (tp != NULL);
}
#endif

#ifdef ENABLE_USART_COMMAND
VNA_SHELL_FUNCTION(cmd_usart)
{
  uint32_t time = 2000; // 200ms wait answer by default
  if (argc == 0 || argc > 2 || (config._mode & _MODE_SERIAL)) return;
  if (argc == 2) time = my_atoui(argv[1])*10;
  sdWriteTimeout(&SD1, (uint8_t *)argv[0], strlen(argv[0]), time);
  sdWriteTimeout(&SD1, (uint8_t *)VNA_SHELL_NEWLINE_STR, sizeof(VNA_SHELL_NEWLINE_STR)-1, time);
  uint32_t size;
  uint8_t buffer[64];
  while ((size = sdReadTimeout(&SD1, buffer, sizeof(buffer), time)))
    streamWrite(&SDU1, buffer, size);
}
#endif
#include "sa_cmd.c"

//=============================================================================
VNA_SHELL_FUNCTION(cmd_help);

#pragma pack(push, 2)
typedef struct {
  const char           *sc_name;
  vna_shellcmd_t    sc_function;
  uint16_t flags;
} VNAShellCommand;
#pragma pack(pop)

// Some commands can executed only in sweep thread, not in main cycle
#define CMD_WAIT_MUTEX  1
static const VNAShellCommand commands[] =
{
    {"version"     , cmd_version     , 0},
    {"reset"       , cmd_reset       , 0},
    {"freq"        , cmd_freq        , CMD_WAIT_MUTEX},
#ifdef __VNA__
    {"offset"      , cmd_offset      , 0},
#endif
#ifdef __USE_RTC__
    {"time"        , cmd_time        , 0},
#endif
    {"dac"         , cmd_dac         , 0},
    {"sweep_voltage",cmd_sweep_voltage,0},
    {"saveconfig"  , cmd_saveconfig  , 0},
    {"clearconfig" , cmd_clearconfig , 0},
    {"data"        , cmd_data        , CMD_WAIT_MUTEX},
#ifdef ENABLED_DUMP
    {"dump"        , cmd_dump        , 0},
#endif
    {"frequencies" , cmd_frequencies , 0},
#ifdef __VNA__
    {"port"        , cmd_port        , 0},
    {"stat"        , cmd_stat        , 0},
    {"gain"        , cmd_gain        , 0},
    {"power"       , cmd_power       , 0},
    {"sample"      , cmd_sample      , 0},
#endif
//  {"gamma"       , cmd_gamma       , 0},
    {"scan"        , cmd_scan        , CMD_WAIT_MUTEX},
    {"scanraw"     , cmd_scanraw     , CMD_WAIT_MUTEX},
    {"zero"        , cmd_zero        , CMD_WAIT_MUTEX},
    {"sweep"       , cmd_sweep       , 0},
    {"test"        , cmd_test        , 0},
    {"touchcal"    , cmd_touchcal    , CMD_WAIT_MUTEX},
    {"touchtest"   , cmd_touchtest   , CMD_WAIT_MUTEX},
    {"pause"       , cmd_pause       , CMD_WAIT_MUTEX},
    {"resume"      , cmd_resume      , CMD_WAIT_MUTEX},
    {"caloutput"   , cmd_caloutput   , 0},
#ifdef __VNA__
    {"cal"         , cmd_cal         , CMD_WAIT_MUTEX},
#endif
    {"save"        , cmd_save        , 0},
    {"recall"      , cmd_recall      , CMD_WAIT_MUTEX},
    {"trace"       , cmd_trace       , CMD_WAIT_MUTEX},
    {"trigger"     , cmd_trigger     , 0},
    {"marker"      , cmd_marker      , 0},
#ifdef ENABLE_USART_COMMAND
    {"usart"       , cmd_usart       , CMD_WAIT_MUTEX},
#endif
#ifdef __VNA__
    {"edelay"      , cmd_edelay      , 0},
#endif
    {"capture"     , cmd_capture     , CMD_WAIT_MUTEX},
#ifdef __REMOTE_DESKTOP__
    {"refresh"     , cmd_refresh     , 0},
    {"touch"       , cmd_touch       , 0},
    {"release"     , cmd_release     , 0},
#endif
    {"vbat"        , cmd_vbat        , 0},     // Uses same adc as touch!!!!!
#ifdef ENABLE_VBAT_OFFSET_COMMAND
    {"vbat_offset" , cmd_vbat_offset , 0},
#endif
#ifdef __VNA__
    {"transform"   , cmd_transform   , 0},
    {"threshold"   , cmd_threshold   , 0},
#endif
    {"help"        , cmd_help        , 0},
#ifdef ENABLE_INFO_COMMAND
    {"info"        , cmd_info        , 0},
#endif
#ifdef ENABLE_COLOR_COMMAND
    {"color"       , cmd_color       , 0},
#endif
    { "if", cmd_if,    0 },
#ifdef TINYSA4
    { "if1", cmd_if1,    0 },
    { "lna2", cmd_lna2,    0 },
    { "agc", cmd_agc,    0 },
#endif
    { "attenuate", cmd_attenuate,    0 },
    { "level", cmd_level,    0 },
    { "sweeptime", cmd_sweeptime,    0 },
    { "leveloffset", cmd_leveloffset,    0 },
    { "levelchange", cmd_levelchange,    0 },
    { "modulation", cmd_modulation,    0 },
    { "rbw", cmd_rbw,    0 },
    { "mode", cmd_mode,    CMD_WAIT_MUTEX },
#ifdef __SPUR__
    { "spur", cmd_spur,    0 },
#endif
#ifdef TINYSA4
    { "lna", cmd_lna,    0 },
    { "ultra", cmd_ultra,    0 },
    { "ultra_start", cmd_ultra_start, CMD_WAIT_MUTEX },
#endif
    { "load", cmd_load,    0 },
    { "offset", cmd_offset, 0},
    { "output", cmd_output,    0 },
    { "deviceid", cmd_deviceid,    0 },
    { "selftest", cmd_selftest,    0 },
    { "correction", cmd_correction,    0 },
    { "calc", cmd_calc, 0},
 #ifdef ENABLE_THREADS_COMMAND
     {"threads"     , cmd_threads     , 0},
 #endif
#ifdef __SINGLE_LETTER__
    { "y", cmd_y,    CMD_WAIT_MUTEX },
   { "i", cmd_i,	CMD_WAIT_MUTEX },
   { "v", cmd_v,	CMD_WAIT_MUTEX },
   { "a", cmd_a,	CMD_WAIT_MUTEX },
   { "b", cmd_b,	CMD_WAIT_MUTEX },
   { "t", cmd_t,	CMD_WAIT_MUTEX },
   { "e", cmd_e,	CMD_WAIT_MUTEX },
   { "s", cmd_s,	CMD_WAIT_MUTEX },
   { "m", cmd_m,	CMD_WAIT_MUTEX },
   { "p", cmd_p,	CMD_WAIT_MUTEX },
   { "w", cmd_w,	CMD_WAIT_MUTEX },
   { "o", cmd_o,    CMD_WAIT_MUTEX },
   { "d", cmd_d,    CMD_WAIT_MUTEX },
   { "f", cmd_f,    CMD_WAIT_MUTEX },
#endif
#ifdef TINYSA4
   { "g", cmd_g,    CMD_WAIT_MUTEX },
#endif
#ifdef __ADF4351__
    { "x", cmd_x,    CMD_WAIT_MUTEX },
#endif
    {NULL          , NULL            , 0}
};

VNA_SHELL_FUNCTION(cmd_help)
{
  (void)argc;
  (void)argv;
  const VNAShellCommand *scp = commands;
  shell_printf("Commands:");
  while (scp->sc_name != NULL
#ifdef __SINGLE_LETTER__
      && scp->sc_function != cmd_y
#endif
      )   {
    shell_printf(" %s", scp->sc_name);
    scp++;
  }
  shell_printf(VNA_SHELL_NEWLINE_STR);
  return;
}

/*
 * VNA shell functions
 */
// Check Serial connection requirements
#ifdef __USE_SERIAL_CONSOLE__
#if HAL_USE_SERIAL == FALSE
#error "For serial console need HAL_USE_SERIAL as TRUE in halconf.h"
#endif

// Before start process command from shell, need select input stream
#define PREPARE_STREAM shell_stream = (config._mode&_MODE_SERIAL) ? (BaseSequentialStream *)&SD1 : (BaseSequentialStream *)&SDU1;

// Update Serial connection speed and settings
void shell_update_speed(void){
  // Update Serial speed settings
  SerialConfig s_config = {USART_GET_SPEED(config._serial_speed), 0, USART_CR2_STOP1_BITS, 0 };
  sdStop(&SD1);
  sdStart(&SD1, &s_config);  // USART config
}

// Check USB connection status
static bool usb_IsActive(void){
  return usbGetDriverStateI(&USBD1) == USB_ACTIVE;
}
void shell_reset_console(void){
  // Reset I/O queue over USB (for USB need also connect/disconnect)
  if (usb_IsActive()){
    if (config._mode & _MODE_SERIAL)
      sduDisconnectI(&SDU1);
    else
      sduConfigureHookI(&SDU1);
  }
  // Reset I/O queue over Serial
  oqResetI(&SD1.oqueue);
  iqResetI(&SD1.iqueue);
}

// Check active connection for Shell
static bool shell_check_connect(void){
  // Serial connection always active
  if (config._mode & _MODE_SERIAL)
    return true;
  // USB connection can be USB_SUSPENDED
  return usb_IsActive();
}

static void shell_init_connection(void){
/*
 * Initializes and start serial-over-USB CDC driver SDU1, connected to USBD1
 */
  sduObjectInit(&SDU1);
  sduStart(&SDU1, &serusbcfg);

/*
 * Set Serial speed settings for SD1
 */
  shell_update_speed();

/*
 * Activates the USB driver and then the USB bus pull-up on D+.
 * Note, a delay is inserted in order to not have to disconnect the cable
 * after a reset.
 */
  usbDisconnectBus(&USBD1);
  chThdSleepMilliseconds(100);
  usbStart(&USBD1, &usbcfg);
  usbConnectBus(&USBD1);

/*
 *  Set I/O stream (SDU1 or SD1) for shell
 */
  PREPARE_STREAM;
}

#else
// Only USB console, shell_stream always on USB
#define PREPARE_STREAM

#if 0           // Not used
// Check connection as Active, if no suspend input
static bool shell_check_connect(void){
  return SDU1.config->usbp->state == USB_ACTIVE;
}
#endif

// Init shell I/O connection over USB
static void shell_init_connection(void){
/*
 * Initializes and start serial-over-USB CDC driver SDU1, connected to USBD1
 */
  sduObjectInit(&SDU1);
  sduStart(&SDU1, &serusbcfg);

/*
 * Activates the USB driver and then the USB bus pull-up on D+.
 * Note, a delay is inserted in order to not have to disconnect the cable
 * after a reset.
 */
  usbDisconnectBus(&USBD1);
  chThdSleepMilliseconds(100);
  usbStart(&USBD1, &usbcfg);
  usbConnectBus(&USBD1);

/*
 *  Set I/O stream SDU1 for shell
 */
  shell_stream = (BaseSequentialStream *)&SDU1;
}
#endif

//
// Read command line from shell_stream
//
static int VNAShell_readLine(char *line, int max_size)
{
  // Read line from input stream
  uint8_t c;
  char *ptr = line;
  // Prepare I/O for shell_stream
  PREPARE_STREAM;
  while (1) {
    // Return 0 only if stream not active
    if (streamRead(shell_stream, &c, 1) == 0)
      return 0;
    // Backspace or Delete
    if (c == 8 || c == 0x7f) {
      if (ptr != line) {
        static const char backspace[] = {0x08, 0x20, 0x08, 0x00};
        shell_printf(backspace);
        ptr--;
      }
      continue;
    }
    // New line (Enter)
    if (c == '\r') {
      shell_printf(VNA_SHELL_NEWLINE_STR);
      *ptr = 0;
      return 1;
    }
    // Others (skip)
    if (c < 0x20)
      continue;
    // Store
    if (ptr < line + max_size - 1) {
      streamPut(shell_stream, c); // Echo
      *ptr++ = (char)c;
    }
  }
  return 0;
}

//
// Parse and run command line
//
static void VNAShell_executeLine(char *line)
{
  // Parse and execute line
  char *lp = line, *ep;
  shell_nargs = 0;
  while (*lp != 0) {
    // Skipping white space and tabs at string begin.
    while (*lp == ' ' || *lp == '\t') lp++;
    // If an argument starts with a double quote then its delimiter is another quote, else
    // delimiter is white space.
    ep = (*lp == '"') ? strpbrk(++lp, "\"") : strpbrk(lp, " \t");
    // Store in args string
    shell_args[shell_nargs++] = lp;
    // Stop, end of input string
    if ((lp = ep) == NULL) break;
    // Argument limits check
    if (shell_nargs > VNA_SHELL_MAX_ARGUMENTS) {
      shell_printf("too many arguments, max " define_to_STR(
          VNA_SHELL_MAX_ARGUMENTS) "" VNA_SHELL_NEWLINE_STR);
      return;
    }
    // Set zero at the end of string and continue check
    *lp++ = 0;
  }
  if (shell_nargs == 0) return;
  // Execute line
  const VNAShellCommand *scp;
  for (scp = commands; scp->sc_name != NULL; scp++) {
    if (get_str_index(scp->sc_name, shell_args[0]) == 0) {
      if (scp->flags & CMD_WAIT_MUTEX) {
        shell_function = scp->sc_function;
        operation_requested|=OP_CONSOLE;
        // Wait execute command in sweep thread
        do {
          osalThreadSleepMilliseconds(100);
        } while (shell_function);
      } else {
        operation_requested = false; // otherwise commands  will be aborted
        scp->sc_function(shell_nargs - 1, &shell_args[1]);
        if (dirty) {
          operation_requested = true;   // ensure output is updated
          if (MODE_OUTPUT(setting.mode))
            draw_menu();    // update screen if in output mode and dirty
          else
            redraw_request |= REDRAW_CAL_STATUS | REDRAW_AREA | REDRAW_FREQUENCY;
        }
      }
      return;
    }
  }
  shell_printf("%s?" VNA_SHELL_NEWLINE_STR, shell_args[0]);
}

#ifdef VNA_SHELL_THREAD
static THD_WORKING_AREA(waThread2, /* cmd_* max stack size + alpha */442);
THD_FUNCTION(myshellThread, p)
{
  (void)p;
  chRegSetThreadName("shell");
  shell_printf(VNA_SHELL_NEWLINE_STR"tinySA Shell"VNA_SHELL_NEWLINE_STR);
  while (true) {
    shell_printf(VNA_SHELL_PROMPT_STR);
    if (VNAShell_readLine(shell_line, VNA_SHELL_MAX_LENGTH))
      VNAShell_executeLine(shell_line);
    else // Putting a delay in order to avoid an endless loop trying to read an unavailable stream.
      osalThreadSleepMilliseconds(100);
  }
}
#endif

#ifdef __VNA__
// I2C clock bus setting: depend from STM32_I2C1SW in mcuconf.h
static const I2CConfig i2ccfg = {
  .timingr  = STM32_TIMINGR_PRESC(0U)  |            /* 72MHz I2CCLK. ~ 600kHz i2c   */
//  STM32_TIMINGR_SCLDEL(10U) | STM32_TIMINGR_SDADEL(4U) |
//  STM32_TIMINGR_SCLH(31U)   | STM32_TIMINGR_SCLL(79U),

  STM32_TIMINGR_SCLDEL(15U) | STM32_TIMINGR_SDADEL(15U) |
  STM32_TIMINGR_SCLH(35U)   | STM32_TIMINGR_SCLL(85U),

//  STM32_TIMINGR_SCLDEL(15U) | STM32_TIMINGR_SDADEL(15U) |
//  STM32_TIMINGR_SCLH(35U)   | STM32_TIMINGR_SCLL(55U),

//  STM32_TIMINGR_SCLDEL(10U) | STM32_TIMINGR_SDADEL(4U) |
//  STM32_TIMINGR_SCLH(48U)   | STM32_TIMINGR_SCLL(90U),
  .cr1      = 0,
  .cr2      = 0
};
#endif

static DACConfig dac1cfg1 = {
  //init:         2047U,
  init:         1922U,
  datamode:     DAC_DHRM_12BIT_RIGHT
};

#pragma GCC pop_options

static const GPTConfig gpt4cfg = {
  1000000, // 1 MHz timer clock.
  NULL, // No callback
  0, 0
};

void my_microsecond_delay(int t)
{
#ifdef TINYSA4
  if (t>1) gptPolledDelay(&GPTD4, t); // t us delay
#else
  if (t>1) gptPolledDelay(&GPTD14, t); // t us delay
#endif
}
#if 0
/*
 * UART driver configuration structure.
 */
static UARTConfig uart_cfg_1 = {
    NULL,   //txend1,
    NULL,   //txend2,
    NULL,   //rxend,
    NULL,   //rxchar,
    NULL,   //rxerr,
    800000,
    0,
    0,      //USART_CR2_LINEN,
    0
};
#endif

#if 0
static const SerialConfig LCD_config =
{
  9600,
  0,
  USART_CR2_STOP2_BITS,
  0
};


void myWrite(char *buf)
{
  int len = strlen(buf);
  while(len-- > 0) {
    sdPut(&SD1,*buf++);
    osalThreadSleepMicroseconds(1000);
  }
}

static int serial_count = 0;
int mySerialReadline(unsigned char *buf, int len)
{
  int i;
  do {
    i =  sdReadTimeout(&SD1,&buf[serial_count], 20-serial_count,TIME_IMMEDIATE);
    serial_count += i;
    if (i > 0)
      osalThreadSleepMicroseconds(1000);
  } while (serial_count < len && i > 0);
  if (buf[serial_count-1] == '\n') {
    serial_count = 0;
    return(i);
  } else
    return 0;
}
#endif

/* Main thread stack size defined in makefile USE_PROCESS_STACKSIZE = 0x200
 * Profile stack usage (enable threads command by def ENABLE_THREADS_COMMAND) show:
 *Stack maximum usage = 472 bytes (need test more and run all commands), free stack = 40 bytes
 */

int main(void)
{
  halInit();
  chSysInit();

  //palSetPadMode(GPIOB, 8, PAL_MODE_ALTERNATE(1) | PAL_STM32_OTYPE_OPENDRAIN);
  //palSetPadMode(GPIOB, 9, PAL_MODE_ALTERNATE(1) | PAL_STM32_OTYPE_OPENDRAIN);
#ifdef __VNA__
  i2cStart(&I2CD1, &i2ccfg);
  si5351_init();
#endif

#ifdef __SI4432__
 /*
  * Powercycle the RF part to reset SI4432
  */

#if 0
  palClearPad(GPIOB, GPIOA_RF_PWR);
  chThdSleepMilliseconds(200);
#endif
#ifdef TINYSA4
  palSetPad(GPIOB, GPIOA_RF_PWR);
#else
  palSetPad(GPIOB, GPIO_RF_PWR);
#endif
  chThdSleepMilliseconds(500);
#endif

#if 0
  palSetPadMode(GPIOA, 9, PAL_MODE_INPUT_ANALOG);
  palSetPadMode(GPIOA, 10, PAL_MODE_OUTPUT_PUSHPULL);
  int s;

  adc_stop();

  // drive high to low on Y line (coordinates from left to right)
  palSetPad(GPIOB, GPIOB_YN);
  palClearPad(GPIOA, GPIOA_YP);
  // Set Y line as output
  palSetPadMode(GPIOB, GPIOB_YN, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPadMode(GPIOA, GPIOA_YP, PAL_MODE_OUTPUT_PUSHPULL);
  // Set X line as input
  palSetPadMode(GPIOB, GPIOB_XN, PAL_MODE_INPUT);        // Hi-z mode
  palSetPadMode(GPIOA, GPIOA_XP, PAL_MODE_INPUT_ANALOG); // <- ADC_TOUCH_X channel


  while (1) {
//      palSetPad(GPIOA, 10);
//      shell_printf("%d\n\r", adc_single_read(ADC_CHSELR_CHSEL9));
//      palClearPad(GPIOA, 10);
      shell_printf("%d\n\r", adc_single_read(ADC_TOUCH_X));
  }
#endif

  /*
 * SPI LCD Initialize
 */
  ili9341_init();

/*
 *  Initiate 1 micro second timer
 */
#ifdef TINYSA4
 gptStart(&GPTD4, &gpt4cfg);
  gptPolledDelay(&GPTD4, 10); // 10 us delay
#else
  gptStart(&GPTD14, &gpt4cfg);
  gptPolledDelay(&GPTD14, 10); // 10 us delay
#endif

/* restore config */
  config_recall();
  config.cor_am = 0;        // Should be removed from config
  config.cor_nfm = 0;
  config.cor_wfm = 0;

  if (caldata_recall(0) == -1) {
    load_LCD_properties();
  }

/*
 * Init Shell console connection data (after load config for settings)
 */

  shell_init_connection();

#ifdef TINYSA4
  dac1cfg1.init = config.dac_value;
#else
  dac1cfg1.init = 0;
#endif

/*
 * Starting DAC1 driver, setting up the output pin as analog as suggested
 * by the Reference Manual.
 */
  dacStart(&DACD2, &dac1cfg1);

  setup_sa();
  set_sweep_points(POINTS_COUNT);

  #ifdef __AUDIO__
/*
 * I2S Initialize
 */
  tlv320aic3204_init();
  i2sInit();
  i2sObjectInit(&I2SD2);
  i2sStart(&I2SD2, &i2sconfig);
  i2sStartExchange(&I2SD2);
#endif
  area_height = AREA_HEIGHT_NORMAL;
  ui_init();
  //Initialize graph plotting
  plot_init();

//  if (setting.mode != -1) {
//    menu_mode_cb(setting.mode,0);
//  }
  redraw_frame();
#ifdef TINYSA3
  set_mode(M_HIGH);
  set_sweep_frequency(ST_STOP, (freq_t) 30000000);
  sweep(false);
  osalThreadSleepMilliseconds(100);

  set_mode(M_LOW);
  set_sweep_frequency(ST_STOP, (freq_t) 4000000);
  sweep(false);
#endif

  if (caldata_recall(0) == -1) {
    load_LCD_properties();
  }

  set_refer_output(-1);
//  ui_mode_menu();       // Show menu when autostarting mode
  ui_mode_normal();

  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO-1, Thread1, NULL);


  while (1) {
    if (SDU1.config->usbp->state == USB_ACTIVE) {
#ifdef VNA_SHELL_THREAD
#if CH_CFG_USE_WAITEXIT == FALSE
#error "VNA_SHELL_THREAD use chThdWait, need enable CH_CFG_USE_WAITEXIT in chconf.h"
#endif
      thread_t *shelltp = chThdCreateStatic(waThread2, sizeof(waThread2),
                                            NORMALPRIO + 1,
                                            myshellThread, NULL);
      chThdWait(shelltp);
#else
      shell_printf(VNA_SHELL_NEWLINE_STR"tinySA Shell"VNA_SHELL_NEWLINE_STR);
      do {
        shell_printf(VNA_SHELL_PROMPT_STR);
        if (VNAShell_readLine(shell_line, VNA_SHELL_MAX_LENGTH))
          VNAShell_executeLine(shell_line);
        else
          chThdSleepMilliseconds(200);
      } while (SDU1.config->usbp->state == USB_ACTIVE);
#endif
    }
    chThdSleepMilliseconds(1000);
  }
}

/* The prototype shows it is a naked function - in effect this is just an
assembly function. */
void HardFault_Handler(void);

void hard_fault_handler_c(uint32_t *sp) __attribute__((naked));


void HardFault_Handler(void)
{
  uint32_t *sp;
  //__asm volatile ("mrs %0, msp \n\t": "=r" (sp) );
  __asm volatile("mrs %0, psp \n\t" : "=r"(sp));
  hard_fault_handler_c(sp);
}

void hard_fault_handler_c(uint32_t *sp)
{
#ifdef TINYSA4
  uint32_t r0  = sp[0];
  uint32_t r1  = sp[1];
  uint32_t r2  = sp[2];
  uint32_t r3  = sp[3];
  register uint32_t  r4 __asm("r4");
  register uint32_t  r5 __asm("r5");
  register uint32_t  r6 __asm("r6");
  register uint32_t  r7 __asm("r7");
  register uint32_t  r8 __asm("r8");
  register uint32_t  r9 __asm("r9");
  register uint32_t r10 __asm("r10");
  register uint32_t r11 __asm("r11");
  uint32_t r12 = sp[4];
  uint32_t lr  = sp[5];
  uint32_t pc  = sp[6];
  uint32_t psr = sp[7];
  int y = 0;
  int x = OFFSETX + 1;
  static  char buf[96];
  ili9341_set_background(LCD_BG_COLOR);
  ili9341_set_foreground(LCD_FG_COLOR);

  plot_printf(buf, sizeof(buf), "SP  0x%08x",  (uint32_t)sp);ili9341_drawstring(buf, x, y+=FONT_STR_HEIGHT);
  plot_printf(buf, sizeof(buf), "R0  0x%08x",  r0);ili9341_drawstring(buf, x, y+=FONT_STR_HEIGHT);
  plot_printf(buf, sizeof(buf), "R1  0x%08x",  r1);ili9341_drawstring(buf, x, y+=FONT_STR_HEIGHT);
  plot_printf(buf, sizeof(buf), "R2  0x%08x",  r2);ili9341_drawstring(buf, x, y+=FONT_STR_HEIGHT);
  plot_printf(buf, sizeof(buf), "R3  0x%08x",  r3);ili9341_drawstring(buf, x, y+=FONT_STR_HEIGHT);
  plot_printf(buf, sizeof(buf), "R4  0x%08x",  r4);ili9341_drawstring(buf, x, y+=FONT_STR_HEIGHT);
  plot_printf(buf, sizeof(buf), "R5  0x%08x",  r5);ili9341_drawstring(buf, x, y+=FONT_STR_HEIGHT);
  plot_printf(buf, sizeof(buf), "R6  0x%08x",  r6);ili9341_drawstring(buf, x, y+=FONT_STR_HEIGHT);
  plot_printf(buf, sizeof(buf), "R7  0x%08x",  r7);ili9341_drawstring(buf, x, y+=FONT_STR_HEIGHT);
  plot_printf(buf, sizeof(buf), "R8  0x%08x",  r8);ili9341_drawstring(buf, x, y+=FONT_STR_HEIGHT);
  plot_printf(buf, sizeof(buf), "R9  0x%08x",  r9);ili9341_drawstring(buf, x, y+=FONT_STR_HEIGHT);
  plot_printf(buf, sizeof(buf), "R10 0x%08x", r10);ili9341_drawstring(buf, x, y+=FONT_STR_HEIGHT);
  plot_printf(buf, sizeof(buf), "R11 0x%08x", r11);ili9341_drawstring(buf, x, y+=FONT_STR_HEIGHT);
  plot_printf(buf, sizeof(buf), "R12 0x%08x", r12);ili9341_drawstring(buf, x, y+=FONT_STR_HEIGHT);
  plot_printf(buf, sizeof(buf), "LR  0x%08x",  lr);ili9341_drawstring(buf, x, y+=FONT_STR_HEIGHT);
  plot_printf(buf, sizeof(buf), "PC  0x%08x",  pc);ili9341_drawstring(buf, x, y+=FONT_STR_HEIGHT);
  plot_printf(buf, sizeof(buf), "PSR 0x%08x", psr);ili9341_drawstring(buf, x, y+=FONT_STR_HEIGHT);
#ifdef ENABLE_THREADS_COMMAND
  thread_t *tp;
  tp = chRegFirstThread();
  do {
    uint32_t max_stack_use = 0U;
#if (CH_DBG_ENABLE_STACK_CHECK == TRUE) || (CH_CFG_USE_DYNAMIC == TRUE)
    uint32_t stklimit = (uint32_t)tp->wabase;
#if CH_DBG_FILL_THREADS == TRUE
    uint8_t *p = (uint8_t *)tp->wabase; while(p[max_stack_use]==CH_DBG_STACK_FILL_VALUE) max_stack_use++;
#endif
#else
    uint32_t stklimit = 0U;
#endif
    plot_printf(buf, sizeof(buf), "%08x|%08x|%08x|%08x|%4u|%4u|%9s|%12s",
             stklimit, (uint32_t)tp->ctx.sp, max_stack_use, (uint32_t)tp,
             (uint32_t)tp->refs - 1, (uint32_t)tp->prio, states[tp->state],
             tp->name == NULL ? "" : tp->name);
    ili9341_drawstring(buf, x, y+=FONT_STR_HEIGHT);
    tp = chRegNextThread(tp);
  } while (tp != NULL);
#endif
  shell_printf("===================================\r\n");
#else
  ili9341_set_background(LCD_BG_COLOR);
  ili9341_set_foreground(LCD_FG_COLOR);
  ili9341_drawstring("FATAL ERROR", OFFSETX, 120);
   (void)sp;
#endif
  while (true) {
  }
}



