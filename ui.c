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
#include "hal.h"
#include "chprintf.h"
#include "nanovna.h"
//#include <stdlib.h>
#include <string.h>
#include <math.h>

#pragma GCC push_options
#pragma GCC optimize ("Os")

uistat_t uistat = {
 digit: 6,
 current_trace: 0,
 lever_mode: LM_MARKER,
 marker_delta: FALSE,
 marker_noise: FALSE,
 marker_tracking : FALSE,
 auto_center_marker : FALSE,
 text : "",
};

#define NO_EVENT                    0
#define EVT_BUTTON_SINGLE_CLICK     0x01
#define EVT_BUTTON_DOUBLE_CLICK     0x02
#define EVT_BUTTON_DOWN_LONG        0x04
#define EVT_UP                  0x10
#define EVT_DOWN                0x20
#define EVT_REPEAT              0x40

#define BUTTON_DOWN_LONG_TICKS      5000   /* 500ms */
#define BUTTON_DOUBLE_TICKS         2500   /* 250ms */
#define BUTTON_REPEAT_TICKS         400    /*  40ms */
#define BUTTON_DEBOUNCE_TICKS       200

/* lever switch assignment */
#define BIT_UP1     3
#define BIT_PUSH    2
#define BIT_DOWN1   1

#define READ_PORT() palReadPort(GPIOA)
#define BUTTON_MASK 0b1110

static uint16_t last_button = 0b0000;
static uint32_t last_button_down_ticks;
static uint32_t last_button_repeat_ticks;

volatile uint8_t operation_requested = OP_NONE;

int8_t previous_marker = MARKER_INVALID;

enum {
  UI_NORMAL, UI_MENU, UI_KEYPAD
};

#ifdef __VNA__
enum {
  KM_START, KM_STOP, KM_CENTER, KM_SPAN, KM_CW, KM_SCALE, KM_REFPOS, KM_EDELAY, KM_VELOCITY_FACTOR, KM_SCALEDELAY
};
#endif

#define NUMINPUT_LEN 10
static uint8_t ui_mode = UI_NORMAL;
static uint8_t keypad_mode;
static char    kp_buf[NUMINPUT_LEN+1];
static int8_t  kp_index = 0;
static char   *kp_help_text = NULL;
static uint8_t menu_current_level = 0;
static int  selection = 0;

static const uint8_t slider_bitmap[]=
{
  _BMP8(0b11111110),
  _BMP8(0b11111110),
  _BMP8(0b01111100),
  _BMP8(0b00111000),
  _BMP8(0b00010000)
};

// Button definition (used in MT_ADV_CALLBACK for custom)
#define BUTTON_ICON_NONE            -1
#define BUTTON_ICON_NOCHECK          0
#define BUTTON_ICON_CHECK            1
#define BUTTON_ICON_CHECK_AUTO       2
#define BUTTON_ICON_CHECK_MANUAL     3
#define BUTTON_ICON_GROUP            4
#define BUTTON_ICON_GROUP_CHECKED    5

#define CHECK_ICON(S) ((S) ? BUTTON_ICON_CHECK : BUTTON_ICON_NOCHECK)
#define GROUP_ICON(S) ((S) ? BUTTON_ICON_GROUP_CHECKED : BUTTON_ICON_GROUP)
#define AUTO_ICON(S)  (S>=2?BUTTON_ICON_CHECK_AUTO:S)            // Depends on order of ICONs!!!!!

#define BUTTON_BORDER_NONE           0x00
#define BUTTON_BORDER_WIDTH_MASK     0x0F

// Define mask for draw border (if 1 use light color, if 0 dark)
#define BUTTON_BORDER_TYPE_MASK      0xF0
#define BUTTON_BORDER_TOP            0x10
#define BUTTON_BORDER_BOTTOM         0x20
#define BUTTON_BORDER_LEFT           0x40
#define BUTTON_BORDER_RIGHT          0x80

#define BUTTON_BORDER_FLAT           0x00
#define BUTTON_BORDER_RISE           (BUTTON_BORDER_TOP|BUTTON_BORDER_RIGHT)
#define BUTTON_BORDER_FALLING        (BUTTON_BORDER_BOTTOM|BUTTON_BORDER_LEFT)

// Set structure align as WORD (save flash memory)
#pragma pack(push, 2)
typedef struct {
  uint8_t type;
  uint8_t data;
  char *label;
  const void *reference;
} menuitem_t;
#pragma pack(pop)

// Touch screen
#define EVT_TOUCH_NONE     0
#define EVT_TOUCH_DOWN     1
#define EVT_TOUCH_PRESSED  2
#define EVT_TOUCH_RELEASED 3
#define EVT_TOUCH_LONGPRESS 4

#define TOUCH_INTERRUPT_ENABLED   1
static uint8_t touch_status_flag = 0;
static int8_t last_touch_status = EVT_TOUCH_NONE;
static int16_t last_touch_x;
static int16_t last_touch_y;

#define KP_CONTINUE 0
#define KP_DONE 1
#define KP_CANCEL 2

static void ui_mode_keypad(int _keypad_mode);
// static void draw_menu(void);
static void leave_ui_mode(void);
static void erase_menu_buttons(void);
static void ui_process_keypad(void);
static void choose_active_marker(void);
static void menu_move_back(bool leave_ui);
static void menu_push_submenu(const menuitem_t *submenu);
//static const menuitem_t menu_marker_type[];

static int btn_check(void)
{
  systime_t ticks;
  // Debounce input
  while(TRUE){
    ticks = chVTGetSystemTimeX();
    if(ticks - last_button_down_ticks > BUTTON_DEBOUNCE_TICKS)
      break;
    chThdSleepMilliseconds(10);
  }
  int status = 0;
  uint16_t cur_button = READ_PORT() & BUTTON_MASK;
  // Detect only changed and pressed buttons
  uint16_t button_set = (last_button ^ cur_button) & cur_button;
  last_button_down_ticks = ticks;
  last_button = cur_button;

  if (button_set & (1<<BIT_PUSH))
    status |= EVT_BUTTON_SINGLE_CLICK;
  if (button_set & (1<<BIT_UP1))
    status |= EVT_UP;
  if (button_set & (1<<BIT_DOWN1))
    status |= EVT_DOWN;
  return status;
}

static int btn_wait_release(void)
{
  while (TRUE) {
    systime_t ticks = chVTGetSystemTimeX();
    systime_t dt = ticks - last_button_down_ticks;
    // Debounce input
//    if (dt < BUTTON_DEBOUNCE_TICKS){
//      chThdSleepMilliseconds(10);
//      continue;
//    }
    chThdSleepMilliseconds(1);
    uint16_t cur_button = READ_PORT() & BUTTON_MASK;
    uint16_t changed = last_button ^ cur_button;
    if (dt >= BUTTON_DOWN_LONG_TICKS && (cur_button & (1<<BIT_PUSH)))
      return EVT_BUTTON_DOWN_LONG;
    else if (changed & (1<<BIT_PUSH)) { // release
      last_button = cur_button;
      last_button_down_ticks = ticks;
      return EVT_BUTTON_SINGLE_CLICK;
    }
    if (changed) {
      // finished
      last_button = cur_button;
      last_button_down_ticks = ticks;
      return 0;
    }

    if (dt > BUTTON_DOWN_LONG_TICKS &&
      ticks > last_button_repeat_ticks) {
      int status = 0;
      if (cur_button & (1<<BIT_DOWN1))
        status |= EVT_DOWN | EVT_REPEAT;
      if (cur_button & (1<<BIT_UP1))
        status |= EVT_UP | EVT_REPEAT;
      last_button_repeat_ticks = ticks + BUTTON_REPEAT_TICKS;
      return status;
    }
  }
}


#define SOFTWARE_TOUCH
//*******************************************************************************
// Software Touch module
//*******************************************************************************
#ifdef SOFTWARE_TOUCH
// ADC read count for measure X and Y (2^N count)
#define TOUCH_X_N 2
#define TOUCH_Y_N 2
static int
touch_measure_y(void)
{
  // drive low to high on X line (At this state after touch_prepare_sense)
//  palSetPadMode(GPIOB, GPIOB_XN, PAL_MODE_OUTPUT_PUSHPULL); //
//  palSetPadMode(GPIOA, GPIOA_XP, PAL_MODE_OUTPUT_PUSHPULL); //
  // drive low to high on X line (coordinates from top to bottom)
  palClearPad(GPIOB, GPIOB_XN);
//  palSetPad(GPIOA, GPIOA_XP);

  // open Y line (At this state after touch_prepare_sense)
//  palSetPadMode(GPIOB, GPIOB_YN, PAL_MODE_INPUT);        // Hi-z mode
  palSetPadMode(GPIOA, GPIOA_YP, PAL_MODE_INPUT_ANALOG);   // <- ADC_TOUCH_Y channel

//  chThdSleepMilliseconds(20);
  uint32_t v = 0, cnt = 1<<TOUCH_Y_N;
  do{v+=adc_single_read(ADC_TOUCH_Y);}while(--cnt);
  return v>>TOUCH_Y_N;
}

static int
touch_measure_x(void)
{
  // drive high to low on Y line (coordinates from left to right)
  palSetPad(GPIOB, GPIOB_YN);
  palClearPad(GPIOA, GPIOA_YP);
  // Set Y line as output
  palSetPadMode(GPIOB, GPIOB_YN, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPadMode(GPIOA, GPIOA_YP, PAL_MODE_OUTPUT_PUSHPULL);
  // Set X line as input
  palSetPadMode(GPIOB, GPIOB_XN, PAL_MODE_INPUT);        // Hi-z mode
  palSetPadMode(GPIOA, GPIOA_XP, PAL_MODE_INPUT_ANALOG); // <- ADC_TOUCH_X channel

  uint32_t v = 0, cnt = 1<<TOUCH_X_N;
  do{v+=adc_single_read(ADC_TOUCH_X);}while(--cnt);
  return v>>TOUCH_X_N;
}
// Manually measure touch event
static inline int
touch_status(void)
{
  return adc_single_read(ADC_TOUCH_Y) > TOUCH_THRESHOLD;
}

static void
touch_prepare_sense(void)
{
  // Set Y line as input
  palSetPadMode(GPIOB, GPIOB_YN, PAL_MODE_INPUT);          // Hi-z mode
  palSetPadMode(GPIOA, GPIOA_YP, PAL_MODE_INPUT_PULLDOWN); // Use pull
  // drive high on X line (for touch sense on Y)
  palSetPad(GPIOB, GPIOB_XN);
  palSetPad(GPIOA, GPIOA_XP);
  // force high X line
  palSetPadMode(GPIOB, GPIOB_XN, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPadMode(GPIOA, GPIOA_XP, PAL_MODE_OUTPUT_PUSHPULL);

//  chThdSleepMilliseconds(10); // Wait 10ms for denounce touch
}

static void
touch_start_watchdog(void)
{
  if (touch_status_flag&TOUCH_INTERRUPT_ENABLED) return;
  touch_status_flag^=TOUCH_INTERRUPT_ENABLED;
  adc_start_analog_watchdog();
}

static void
touch_stop_watchdog(void)
{
  if (!(touch_status_flag&TOUCH_INTERRUPT_ENABLED)) return;
  touch_status_flag^=TOUCH_INTERRUPT_ENABLED;
  adc_stop_analog_watchdog();
}

// Touch panel timer check (check press frequency 20Hz)
static const GPTConfig gpt3cfg = {
  20,     // 200Hz timer clock. 200/10 = 20Hz touch check
  NULL,   // Timer callback.
  0x0020, // CR2:MMS=02 to output TRGO
  0
};

//
// Touch init function init timer 3 trigger adc for check touch interrupt, and run measure
//
static void touch_init(void){
  // Prepare pin for measure touch event
  touch_prepare_sense();
  // Start touch interrupt, used timer_3 ADC check threshold:
  gptStart(&GPTD3, &gpt3cfg);         // Init timer 3
  gptStartContinuous(&GPTD3, 10);     // Start timer 3 vs timer 10 interval
  touch_start_watchdog();             // Start ADC watchdog (measure by timer 3 interval and trigger interrupt if touch pressed)
}

// Main software touch function, should:
// set last_touch_x and last_touch_x
// return touch status
static int
touch_check(void)
{
  touch_stop_watchdog();

  int stat = touch_status();
  if (stat) {
    int y = touch_measure_y();
    int x = touch_measure_x();
    touch_prepare_sense();
    if (touch_status())
    {
      last_touch_x = x;
      last_touch_y = y;
    }
#ifdef __REMOTE_DESKTOP__
    mouse_down = false;
  }
  if (!stat) {
    stat = mouse_down;
    if (mouse_down) {
      last_touch_x = mouse_x;
      last_touch_y = mouse_y;
    }
#endif
  }
  #if 0                                           // Long press detection
  systime_t ticks = chVTGetSystemTimeX();

  if (stat && !last_touch_status) {         // new button, initialize
    prev_touch_time = ticks;
  }
  dt = ticks - prev_touch_time;

  if (stat && stat == last_touch_status && dt > BUTTON_DOWN_LONG_TICKS) {return EVT_TOUCH_LONGPRESS;}
#endif
  if (stat != last_touch_status) {
    last_touch_status = stat;
    return stat ? EVT_TOUCH_PRESSED : EVT_TOUCH_RELEASED;
  }
  return stat ? EVT_TOUCH_DOWN : EVT_TOUCH_NONE;
}
//*******************************************************************************
// End Software Touch module
//*******************************************************************************
#endif // end SOFTWARE_TOUCH

void
touch_wait_release(void)
{
  while (touch_check() != EVT_TOUCH_NONE)
    chThdSleepMilliseconds(20);
}
#if 0
static inline void
touch_wait_pressed(void)
{
  while (touch_check() != EVT_TOUCH_PRESSED)
    ;
}
#endif

static inline void
touch_wait_released(void)
{
  while (touch_check() != EVT_TOUCH_RELEASED)
    ;
}

void
touch_cal_exec(void)
{
  int x1, x2, y1, y2;
  ili9341_set_foreground(LCD_FG_COLOR);
  ili9341_set_background(LCD_BG_COLOR);
  ili9341_clear_screen();
  ili9341_line(0, 0, 0, 32);
  ili9341_line(0, 0, 32, 0);
  ili9341_line(0, 0, 32, 32);
  ili9341_drawstring("TOUCH UPPER LEFT", 40, 40);
  touch_wait_released();
//  touch_wait_release();
  x1 = last_touch_x;
  y1 = last_touch_y;

  ili9341_clear_screen();
  ili9341_line(LCD_WIDTH-1, LCD_HEIGHT-1, LCD_WIDTH-1, LCD_HEIGHT-32);
  ili9341_line(LCD_WIDTH-1, LCD_HEIGHT-1, LCD_WIDTH-32, LCD_HEIGHT-1);
  ili9341_line(LCD_WIDTH-1, LCD_HEIGHT-1, LCD_WIDTH-32, LCD_HEIGHT-32);
  ili9341_drawstring("TOUCH LOWER RIGHT", LCD_WIDTH-17*(FONT_WIDTH)-30, LCD_HEIGHT-FONT_GET_HEIGHT-35);

  touch_wait_released();
 // touch_wait_release();
  x2 = last_touch_x;
  y2 = last_touch_y;

  config.touch_cal[0] = x1;
  config.touch_cal[1] = y1;
  config.touch_cal[2] = (x2 - x1) * 16 / LCD_WIDTH;
  config.touch_cal[3] = (y2 - y1) * 16 / LCD_HEIGHT;

  config_save();            // Auto save touch calibration

  //redraw_all();
}

void
touch_draw_test(void)
{
  int x0, y0;
  int x1, y1;

  ili9341_set_foreground(LCD_FG_COLOR);
  ili9341_set_background(LCD_BG_COLOR);
  ili9341_clear_screen();
  ili9341_drawstring("TOUCH TEST: DRAG PANEL, PRESS BUTTON TO FINISH", OFFSETX, LCD_HEIGHT - FONT_GET_HEIGHT);

  int old_button_state = 0;
  while (touch_check() != EVT_TOUCH_PRESSED) {
    int button_state = READ_PORT() & BUTTON_MASK;
    if (button_state != old_button_state) {
      char buf[20];
      plot_printf(buf, sizeof buf, "STATE: %4d       ", button_state);
      ili9341_drawstring_7x13(buf, 120, 120);
      old_button_state = button_state;
    }

  }

  do {
    if (touch_check() == EVT_TOUCH_PRESSED){
      touch_position(&x0, &y0);
      do {
        chThdSleepMilliseconds(50);
        touch_position(&x1, &y1);
        ili9341_line(x0, y0, x1, y1);
        x0 = x1;
        y0 = y1;
      } while (touch_check() != EVT_TOUCH_RELEASED);
    }
  }while (!(btn_check() & EVT_BUTTON_SINGLE_CLICK));
}


void
touch_position(int *x, int *y)
{
#ifdef __REMOTE_DESKTOP__
  *x = (mouse_down ? mouse_x : (last_touch_x - config.touch_cal[0]) * 16 / config.touch_cal[2]);
  *y = (mouse_down ? mouse_y : (last_touch_y - config.touch_cal[1]) * 16 / config.touch_cal[3]);
#else
  *x = (last_touch_x - config.touch_cal[0]) * 16 / config.touch_cal[2];
  *y = (last_touch_y - config.touch_cal[1]) * 16 / config.touch_cal[3];
#endif
}

void
show_version(void)
{
  int x = 5, y = 5, i = 0;
  ili9341_set_foreground(LCD_FG_COLOR);
  ili9341_set_background(LCD_BG_COLOR);

  ili9341_clear_screen();
  uint16_t shift = 0b0000010000111110;
  ili9341_drawstring_10x14(info_about[i++], x , y);
  while (info_about[i]) {
    do {shift>>=1; y+=5;} while (shift&1);
    ili9341_drawstring(info_about[i++], x, y+=5);
  }
#ifdef TINYSA3
  if (has_esd)
    ili9341_drawstring("ESD protected", x, y+=5);
#endif
#ifdef TINYSA4
extern const char *states[];
  #define ENABLE_THREADS_COMMAND

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
    char buf[96];
    plot_printf(buf, sizeof(buf), "%08x|%08x|%08x|%08x|%4u|%4u|%9s|%12s",
             stklimit, (uint32_t)tp->ctx.sp, max_stack_use, (uint32_t)tp,
             (uint32_t)tp->refs - 1, (uint32_t)tp->prio, states[tp->state],
             tp->name == NULL ? "" : tp->name);
    ili9341_drawstring(buf, x, y+=FONT_STR_HEIGHT);
    tp = chRegNextThread(tp);
  } while (tp != NULL);
#endif

#endif		// TINYSA4

  uint16_t cnt = 0;
  while (true) {
    if (touch_check() == EVT_TOUCH_PRESSED)
      break;
    if (btn_check() & EVT_BUTTON_SINGLE_CLICK)
      break;
    chThdSleepMilliseconds(40);
    if ((cnt++)&0x07) continue; // Not update time so fast
#ifdef __USE_RTC__
    uint32_t tr = rtc_get_tr_bin(); // TR read first
    uint32_t dr = rtc_get_dr_bin(); // DR read second
    char buf[96];
    plot_printf(buf, sizeof(buf), "Time: 20%02d/%02d/%02d %02d:%02d:%02d" " (LS%c)",
      RTC_DR_YEAR(dr),
      RTC_DR_MONTH(dr),
      RTC_DR_DAY(dr),
      RTC_TR_HOUR(dr),
      RTC_TR_MIN(dr),
      RTC_TR_SEC(dr),
      (RCC->BDCR & STM32_RTCSEL_MASK) == STM32_RTCSEL_LSE ? 'E' : 'I');
    ili9341_drawstring(buffer, x, y);
#endif
#if 0
    uint32_t vbat=adc_vbat_read();
    plot_printf(buf, sizeof(buf), "Batt: %d.%03dV", vbat/1000, vbat%1000);
    ili9341_drawstring(buf, x, y + FONT_STR_HEIGHT + 2);
#endif
  }
}

void
enter_dfu(void)
{
  int x = 5, y = 5;
  ili9341_set_foreground(LCD_FG_COLOR);
  ili9341_set_background(LCD_BG_COLOR);
  // leave a last message 
  ili9341_clear_screen();
  ili9341_drawstring_7x13("DFU: Device Firmware Update Mode\n"
                          "To exit DFU mode, please reset device yourself.", x, y);
  // see __early_init in ./NANOVNA_STM32_F072/board.c
  *((unsigned long *)BOOT_FROM_SYTEM_MEMORY_MAGIC_ADDRESS) = BOOT_FROM_SYTEM_MEMORY_MAGIC;
  NVIC_SystemReset();
}

static void
select_lever_mode(int mode)
{
  if (uistat.lever_mode != mode) {
    uistat.lever_mode = mode;
    redraw_request |= REDRAW_FREQUENCY | REDRAW_MARKER;
  }
}

// type of menu item 
enum {
  MT_NONE,                      // sentinel menu
  MT_BLANK,                     // blank menu (nothing draw)
  MT_SUBMENU,                   // enter to submenu
  MT_CALLBACK,                  // call user function
  MT_ADV_CALLBACK,              // adv call user function
  MT_CANCEL,                    // menu, step back on one level up
  MT_TITLE,                     // Title
  MT_KEYPAD,
  MT_ICON = 0x10,
  MT_HIGH = 0x20,               // Only applicable to high mode
  MT_LOW = 0x40,                // Only applicable to low mode
  MT_FORM = 0x80,               // Large button menu
};
//#define MT_BACK     0x40
//#define MT_LEAVE    0x20
#define MT_MASK(x) (0xF & (x))

// Call back functions for MT_CALLBACK type
typedef void (*menuaction_cb_t)(int item, uint16_t data);
#define UI_FUNCTION_CALLBACK(ui_function_name) void ui_function_name(int item, uint16_t data)

typedef void (*menuaction_acb_t)(int item, uint16_t data, ui_button_t *b);
#define UI_FUNCTION_ADV_CALLBACK(ui_function_name) void ui_function_name(int item, uint16_t data, ui_button_t *b)

#ifdef __VNA__
static void
menu_calop_cb(int item, uint8_t data)
{
  cal_collect(data);
  selection = item+1;
  draw_cal_status();
  draw_menu();
}

static void
menu_caldone_cb(int item, uint8_t data)
{
  extern const menuitem_t menu_save[];
  //extern const menuitem_t menu_cal[];
  (void)item;
  (void)data;
  cal_done();
  draw_cal_status();
  menu_move_back(false);
  menu_push_submenu(menu_save);
}

static void
menu_cal2_cb(int item, uint8_t data)
{
  (void)data;
  switch (item) {
  case 2: // RESET
    cal_status = 0;
    break;
  case 3: // CORRECTION
    // toggle applying correction
    cal_status ^= CALSTAT_APPLY;
    break;
  }
  draw_menu();
  draw_cal_status();
  //menu_move_back(false);
}

static void
menu_recall_cb(int item, uint8_t data)
{
  (void)item;
  caldata_recall(data);
  menu_move_back(false);
  ui_mode_normal();
  update_grid();
  draw_cal_status();
}

static void
menu_config_cb(int item, uint8_t data)
{
  (void)data;
  switch (item) {
  case 0:
      touch_cal_exec();
      break;
  case 1:
      touch_draw_test();
      break;
  case 3:
      show_version();
      break;
  }
  redraw_frame();
  request_to_redraw_grid();
  draw_menu();
}

static void
menu_config_save_cb(int item, uint8_t data)
{
  (void)item;
  (void)data;
  config_save();
  menu_move_back(false);
  ui_mode_normal();
}

static void
menu_dfu_cb(int item, uint8_t data)
{
  (void)item;
  (void)data;
  enter_dfu();
}

static void
menu_save_cb(int item, uint8_t data)
{
  (void)item;
  if (caldata_save(data) == 0) {
    menu_move_back(false);
    ui_mode_normal();
    draw_cal_status();
  }
}

static void 
choose_active_trace(void)
{
  int i;
  if (trace[uistat.current_trace].enabled)
    // do nothing
    return;
  for (i = 0; i < TRACES_MAX; i++)
    if (trace[i].enabled) {
      uistat.current_trace = i;
      return;
    }
}

static void
menu_trace_cb(int item, uint8_t data)
{
  (void)item;
  if (trace[data].enabled) {
    if (data == uistat.current_trace) {
      // disable if active trace is selected
      trace[data].enabled = FALSE;
      choose_active_trace();
    } else {
      // make active selected trace
      uistat.current_trace = data;
    }
  } else {
    trace[data].enabled = TRUE;
    uistat.current_trace = data;
  }
  request_to_redraw_grid();
  draw_menu();
}

static void
menu_format_cb(int item, uint8_t data)
{
  (void)item;
  set_trace_type(uistat.current_trace, data);
  request_to_redraw_grid();
  ui_mode_normal();
  //redraw_all();
}

static void
menu_channel_cb(int item, uint8_t data)
{
  (void)item;
  set_trace_channel(uistat.current_trace, data);
  menu_move_back(false);
  ui_mode_normal();
}

static void
menu_transform_window_cb(int item, uint8_t data)
{
  (void)item;
  // TODO
  domain_mode = (domain_mode & ~TD_WINDOW) | data;
  ui_mode_normal();
}

static void
menu_transform_cb(int item, uint8_t data)
{
  (void)item;
  (void)data;
  domain_mode ^= DOMAIN_TIME;
  select_lever_mode(LM_MARKER);
  draw_frequencies();
  ui_mode_normal();
}

static void
menu_velocity_cb(int item, uint8_t data)
{
  (void)item;
  (void)data;
  if (btn_wait_release() & EVT_BUTTON_DOWN_LONG) {
      ui_mode_numeric(KM_VELOCITY_FACTOR);
      ui_process_numeric();
  } else {
      ui_mode_keypad(KM_VELOCITY_FACTOR);
  }
}

static void
menu_transform_filter_cb(int item, uint8_t data)
{
  (void)item;
  domain_mode = (domain_mode & ~TD_FUNC) | data;
  ui_mode_normal();
}

static void
menu_bandwidth_cb(int item)
{
  bandwidth = item;
  draw_menu();
}

static void 
choose_active_marker(void)
{
  int i;
  for (i = 0; i < MARKERS_MAX; i++)
    if (markers[i].enabled) {
      active_marker = i;
      return;
    }
  active_marker = -1;
}

static void
menu_scale_cb(int item, uint8_t data)
{
  (void)item;
#ifdef __VNA__
  if (data == KM_SCALE && trace[uistat.current_trace].type == TRC_DELAY) {
    data = KM_SCALEDELAY;
  }
#endif
  if (btn_wait_release() & EVT_BUTTON_DOWN_LONG) {
    ui_mode_numeric(data);
    ui_process_numeric();
  } else {
    ui_mode_keypad(data);
  }
}

static void
menu_stimulus_cb(int item, uint8_t data)
{
  (void)data;
  switch (item) {
  case 0: /* START */
  case 1: /* STOP */
  case 2: /* CENTER */
  case 3: /* SPAN */
  case 4: /* CW */
    if (btn_wait_release() & EVT_BUTTON_DOWN_LONG) {
      ui_mode_numeric(item);
      ui_process_numeric();
    } else {
      ui_mode_keypad(item);
    }
    break;
  case 5: /* PAUSE */
    toggle_sweep();
    //menu_move_back(false);
    //ui_mode_normal();
    draw_menu();
    break;
  }
}
#endif

static freq_t
get_marker_frequency(int marker)
{
  if (marker < 0 || marker >= MARKERS_MAX)
    return 0;
  if (!markers[marker].enabled)
    return 0;
  return frequencies[markers[marker].index];
}

static UI_FUNCTION_CALLBACK(menu_marker_op_cb)
{
  (void)item;
  freq_t freq = get_marker_frequency(active_marker);
  if (freq == 0)
    return; // no active marker

  switch (data) {
  case 0: /* MARKER->START */
  case 1: /* MARKER->STOP */
  case 2: /* MARKER->CENTER */
    set_sweep_frequency(data, freq);
    if (data == 2) {
      uistat.lever_mode = LM_SPAN;
      uistat.auto_center_marker = true;
    }
    break;
  case 3: /* MARKERS->SPAN */
    {
      if (previous_marker == MARKER_INVALID || active_marker == previous_marker) {
        // if only 1 marker is active, keep center freq and make span the marker comes to the edge
        freq_t center = get_sweep_frequency(ST_CENTER);
        freq_t span = center > freq ? center - freq : freq - center;
        set_sweep_frequency(ST_SPAN, span * 2);
      } else {
        // if 2 or more marker active, set start and stop freq to each marker
        freq_t freq2 = get_marker_frequency(previous_marker);
        if (freq2 == 0)
          return;
        if (freq > freq2) {
          freq2 = freq;
          freq = get_marker_frequency(previous_marker);
        }
        set_sweep_frequency(ST_START, freq);
        set_sweep_frequency(ST_STOP, freq2);
      }
    }
    break;
  case 4: // marker -> ref level
    {
    float l = actual_t[markers[active_marker].index];
    float s_max = value(l)/setting.scale;
    user_set_reflevel(setting.scale*(floorf(s_max)+2));
    }
    break;
#ifdef __VNA__
  case 4: /* MARKERS->EDELAY */
    { 
      if (uistat.current_trace == -1)
        break;
      float (*array)[2] = measured[trace[uistat.current_trace].channel];
      float v = groupdelay_from_array(markers[active_marker].index, array);
      set_electrical_delay(electrical_delay + (v / 1e-12));
    }
    break;
#endif
  }
  menu_move_back(true);
  redraw_request |= REDRAW_CAL_STATUS;
  //redraw_all();
}

static UI_FUNCTION_CALLBACK(menu_marker_search_cb)
{
  (void)item;
  int i = -1;
  if (active_marker == MARKER_INVALID)
    return;
  markers[active_marker].mtype &= ~M_TRACKING;
  switch (data) {
  case 0: /* search Left */
    i = marker_search_left_min(markers[active_marker].index);
    break;
  case 1: /* search right */
    i = marker_search_right_min(markers[active_marker].index);
    break;
  case 2: /* search Left */
    i = marker_search_left_max(markers[active_marker].index);
    break;
  case 3: /* search right */
    i = marker_search_right_max(markers[active_marker].index);
    break;
  case 4: /* peak search */
    i = marker_search_max();
    break;
  }
  if (i != -1) {
    markers[active_marker].index = i;
    if (data > 1) // Maximum related
      interpolate_maximum(active_marker);
    else
      markers[active_marker].frequency = frequencies[i];
  }
  redraw_marker(active_marker);
//  if (data == 4)
    select_lever_mode(LM_MARKER);   // Allow any position with level
//  else
//    select_lever_mode(LM_SEARCH); // Jump from maximum to maximum
}

static UI_FUNCTION_ADV_CALLBACK(menu_marker_tracking_acb){
  (void)item;
  (void)data;
  if (active_marker == MARKER_INVALID) return;
  if(b){
    b->icon = markers[active_marker].mtype & M_TRACKING ? BUTTON_ICON_CHECK : BUTTON_ICON_NOCHECK;
    return;
  }
  markers[active_marker].mtype ^= M_TRACKING;
}

#ifdef __VNA__
static void
menu_marker_smith_cb(int item, uint8_t data)
{
  (void)item;
  marker_smith_format = data;
  redraw_marker(active_marker);
  draw_menu();
}
#endif

static void
active_marker_select(int item)  // used only to select an active marker from the modify marker selection menu
{
  if (item == -1) {
    active_marker = previous_marker;
    previous_marker = MARKER_INVALID;
    if (active_marker == MARKER_INVALID) {
      choose_active_marker();
    }
  } else {
    if (previous_marker != active_marker) {
      previous_marker = active_marker;
      active_marker = item;
    } else {
      active_marker = item;
    }
  }
}
#ifdef __VNA__
static void
menu_marker_sel_cb(int item, uint8_t data)
{
  (void)data;
//  int t;
  if (item >= 0 && item < MARKERS_MAX) {
    if (markers[item].enabled) {
      if (item == active_marker) {
        // disable if active trace is selected
        markers[item].enabled = M_DISABLED;
        active_marker_select(-1);
      } else {
        active_marker_select(item);
      }
    } else {
      markers[item].enabled = M_ENABLED;
      active_marker_select(item);
      markers[item].mtype = M_NORMAL;
      markers[item].mtype |= (uistat.marker_delta ? M_DELTA : 0);
      markers[item].mtype |= (uistat.marker_noise ? M_NOISE : 0);
      markers[item].mtype |= (uistat.marker_tracking ? M_TRACKING : 0);
    }
 //   if (markers[item].enabled)
 //     menu_push_submenu(menu_marker_type);
#if 0
  } else if (item == 4) { /* all off */
      for (t = 0; t < MARKERS_MAX; t++)
        markers[t].enabled = M_DISABLED;
      previous_marker = -1;
      active_marker = -1;      
#endif
  } else if (item == 4) { /* marker delta */
    uistat.marker_delta = !uistat.marker_delta;
  } else if (item == 5) { /* marker noise */
    uistat.marker_noise = !uistat.marker_noise;
    // if (uistat.marker_noise) uistat.marker_delta = true;     //Default behavior
  } else if (item == 6) { /* marker tracking */
    uistat.marker_tracking = !uistat.marker_tracking;
    // if (uistat.marker_tracking) uistat.marker_noise = false; //Default behavior
  }
  redraw_marker(active_marker);
  draw_menu();
}

static const menuitem_t menu_calop[] = {
  { MT_CALLBACK, CAL_OPEN,  "OPEN",  menu_calop_cb },
  { MT_CALLBACK, CAL_SHORT, "SHORT", menu_calop_cb },
  { MT_CALLBACK, CAL_LOAD,  "LOAD",  menu_calop_cb },
  { MT_CALLBACK, CAL_ISOLN, "ISOLN", menu_calop_cb },
  { MT_CALLBACK, CAL_THRU,  "THRU",  menu_calop_cb },
  { MT_CALLBACK, 0,         "DONE",  menu_caldone_cb },
  { MT_CANCEL,   0, S_LARROW" BACK", NULL },
  { MT_NONE,     0, NULL, NULL } // sentinel
};

const menuitem_t menu_save[] = {
  { MT_CALLBACK, 0, "SAVE 0", menu_save_cb },
  { MT_CALLBACK, 1, "SAVE 1", menu_save_cb },
  { MT_CALLBACK, 2, "SAVE 2", menu_save_cb },
  { MT_CALLBACK, 3, "SAVE 3", menu_save_cb },
  { MT_CALLBACK, 4, "SAVE 4", menu_save_cb },
  { MT_CANCEL, 0, S_LARROW" BACK", NULL },
  { MT_NONE, 0, NULL, NULL } // sentinel
};

const menuitem_t menu_cal[] = {
  { MT_SUBMENU,  0, "CALIBRATE", menu_calop },
  { MT_SUBMENU,  0, "SAVE", menu_save },
  { MT_CALLBACK, 0, "RESET", menu_cal2_cb },
  { MT_CALLBACK, 0, "CORRECTION", menu_cal2_cb },
  { MT_CANCEL, 0, S_LARROW" BACK", NULL },
  { MT_NONE, 0, NULL, NULL } // sentinel
};

const menuitem_t menu_trace[] = {
  { MT_CALLBACK, 0, "TRACE 0", menu_trace_cb },
  { MT_CALLBACK, 1, "TRACE 1", menu_trace_cb },
  { MT_CALLBACK, 2, "TRACE 2", menu_trace_cb },
  { MT_CALLBACK, 3, "TRACE 3", menu_trace_cb },
  { MT_CANCEL, 0, S_LARROW" BACK", NULL },
  { MT_NONE, 0, NULL, NULL } // sentinel
};

const menuitem_t menu_format2[] = {
  { MT_CALLBACK, TRC_POLAR, "POLAR", menu_format_cb },
  { MT_CALLBACK, TRC_LINEAR, "LINEAR", menu_format_cb },
  { MT_CALLBACK, TRC_REAL, "REAL", menu_format_cb },
  { MT_CALLBACK, TRC_IMAG, "IMAG", menu_format_cb },
  { MT_CALLBACK, TRC_R, "RESISTANCE", menu_format_cb },
  { MT_CALLBACK, TRC_X, "REACTANCE", menu_format_cb },
  { MT_CANCEL, 0, S_LARROW" BACK", NULL },
  { MT_NONE, 0, NULL, NULL } // sentinel
};

const menuitem_t menu_format[] = {
  { MT_CALLBACK, TRC_LOGMAG, "LOGMAG", menu_format_cb },
  { MT_CALLBACK, TRC_PHASE, "PHASE", menu_format_cb },
  { MT_CALLBACK, TRC_DELAY, "DELAY", menu_format_cb },
  { MT_CALLBACK, TRC_SMITH, "SMITH", menu_format_cb },
  { MT_CALLBACK, TRC_SWR, "SWR", menu_format_cb },
  { MT_SUBMENU, 0, S_RARROW" MORE", menu_format2 },
  //{ MT_CALLBACK, TRC_LINEAR, "LINEAR", menu_format_cb },
  //{ MT_CALLBACK, TRC_SWR, "SWR", menu_format_cb },
  { MT_CANCEL, 0, S_LARROW" BACK", NULL },
  { MT_NONE, 0, NULL, NULL } // sentinel
};

const menuitem_t menu_scale[] = {
  { MT_CALLBACK, KM_SCALE, "SCALE/DIV", menu_scale_cb },
  { MT_CALLBACK, KM_REFPOS, "\2REFERENCE\0POSITION", menu_scale_cb },
  { MT_CALLBACK, KM_EDELAY, "\2ELECTRICAL\0DELAY", menu_scale_cb },
  { MT_CANCEL, 0, S_LARROW" BACK", NULL },
  { MT_NONE, 0, NULL, NULL } // sentinel
};

const menuitem_t menu_channel[] = {
  { MT_CALLBACK, 0, "\2CH0\0REFLECT", menu_channel_cb },
  { MT_CALLBACK, 1, "\2CH1\0THROUGH", menu_channel_cb },
  { MT_CANCEL, 0, S_LARROW" BACK", NULL },
  { MT_NONE, 0, NULL, NULL } // sentinel
};

const menuitem_t menu_transform_window[] = {
  { MT_CALLBACK, TD_WINDOW_MINIMUM, "MINIMUM", menu_transform_window_cb },
  { MT_CALLBACK, TD_WINDOW_NORMAL,   "NORMAL", menu_transform_window_cb },
  { MT_CALLBACK, TD_WINDOW_MAXIMUM, "MAXIMUM", menu_transform_window_cb },
  { MT_CANCEL, 0, S_LARROW" BACK", NULL },
  { MT_NONE, 0, NULL, NULL } // sentinel
};

const menuitem_t menu_transform[] = {
  { MT_CALLBACK, 0, "\2TRANSFORM\0ON", menu_transform_cb },
  { MT_CALLBACK, TD_FUNC_LOWPASS_IMPULSE, "\2LOW PASS\0IMPULSE", menu_transform_filter_cb },
  { MT_CALLBACK, TD_FUNC_LOWPASS_STEP, "\2LOW PASS\0STEP", menu_transform_filter_cb },
  { MT_CALLBACK, TD_FUNC_BANDPASS, "BANDPASS", menu_transform_filter_cb },
  { MT_SUBMENU, 0, "WINDOW", menu_transform_window },
  { MT_CALLBACK, 0, "\2VELOCITY\0FACTOR", menu_velocity_cb },
  { MT_CANCEL, 0, S_LARROW" BACK", NULL },
  { MT_NONE, 0, NULL, NULL } // sentinel
};

const menuitem_t menu_bandwidth[] = {
  { MT_CALLBACK, 0, "1 kHz", menu_bandwidth_cb },
  { MT_CALLBACK, 0, "300 Hz", menu_bandwidth_cb },
  { MT_CALLBACK, 0, "100 Hz", menu_bandwidth_cb },
  { MT_CALLBACK, 0, "30 Hz", menu_bandwidth_cb },
  { MT_CALLBACK, 0, "10 Hz", menu_bandwidth_cb },
  { MT_CANCEL, 0, S_LARROW" BACK", NULL },
  { MT_NONE, 0, NULL, NULL } // sentinel
};

const menuitem_t menu_display[] = {
  { MT_SUBMENU, 0, "TRACE", menu_trace },
  { MT_SUBMENU, 0, "FORMAT", menu_format },
  { MT_SUBMENU, 0, "SCALE", menu_scale },
  { MT_SUBMENU, 0, "CHANNEL", menu_channel },
  { MT_SUBMENU, 0, "TRANSFORM", menu_transform },
  { MT_SUBMENU, 0, "BANDWIDTH", menu_bandwidth },  
  { MT_CANCEL, 0, S_LARROW" BACK", NULL },
  { MT_NONE, 0, NULL, NULL } // sentinel
};

const menuitem_t menu_stimulus[] = {
  { MT_CALLBACK, 0, "START", menu_stimulus_cb },
  { MT_CALLBACK, 0, "STOP", menu_stimulus_cb },
  { MT_CALLBACK, 0, "CENTER", menu_stimulus_cb },
  { MT_CALLBACK, 0, "SPAN", menu_stimulus_cb },
  { MT_CALLBACK, 0, "CW FREQ", menu_stimulus_cb },
  { MT_CALLBACK, 0, "\2PAUSE\0SWEEP", menu_stimulus_cb },
  { MT_CANCEL, 0, S_LARROW" BACK", NULL },
  { MT_NONE, 0, NULL, NULL } // sentinel
};

const menuitem_t menu_marker_sel[] = {
  { MT_CALLBACK, 1, "MARKER 1", menu_marker_sel_cb },
  { MT_CALLBACK, 2, "MARKER 2", menu_marker_sel_cb },
  { MT_CALLBACK, 3, "MARKER 3", menu_marker_sel_cb },
  { MT_CALLBACK, 4, "MARKER 4", menu_marker_sel_cb },
  { MT_CALLBACK, 0, "ALL OFF", menu_marker_sel_cb },
  { MT_CALLBACK, 0, "DELTA", menu_marker_sel_cb },
  { MT_CANCEL, 0, S_LARROW" BACK", NULL },
  { MT_NONE, 0, NULL, NULL } // sentinel
};

const menuitem_t menu_marker_ops[] = {
  { MT_CALLBACK, ST_START, S_RARROW"START", menu_marker_op_cb },
  { MT_CALLBACK, ST_STOP, S_RARROW"STOP", menu_marker_op_cb },
  { MT_CALLBACK, ST_CENTER, S_RARROW"CENTER", menu_marker_op_cb },
  { MT_CALLBACK, ST_SPAN, S_RARROW"SPAN", menu_marker_op_cb },
  { MT_CALLBACK, 0, S_RARROW"EDELAY", menu_marker_op_cb },
  { MT_CANCEL, 0, S_LARROW" BACK", NULL },
  { MT_NONE, 0, NULL, NULL } // sentinel
};

const menuitem_t menu_marker_search[] = {
  //{ MT_CALLBACK, "OFF", menu_marker_search_cb },
  { MT_CALLBACK, 0, "MAXIMUM", menu_marker_search_cb },
  { MT_CALLBACK, 0, "MINIMUM", menu_marker_search_cb },
  { MT_CALLBACK, 0, "\2SEARCH\0" S_LARROW" LEFT", menu_marker_search_cb },
  { MT_CALLBACK, 0, "\2SEARCH\0" S_RARROW" RIGHT", menu_marker_search_cb },
  { MT_CALLBACK, 0, "TRACKING", menu_marker_search_cb },
  { MT_CANCEL, 0, S_LARROW" BACK", NULL },
  { MT_NONE, 0, NULL, NULL } // sentinel
};

const menuitem_t menu_marker_smith[] = {
  { MT_CALLBACK, MS_LIN, "LIN", menu_marker_smith_cb },
  { MT_CALLBACK, MS_LOG, "LOG", menu_marker_smith_cb },
  { MT_CALLBACK, MS_REIM,"Re+Im", menu_marker_smith_cb },
  { MT_CALLBACK, MS_RX,  "R+Xj", menu_marker_smith_cb },
  { MT_CALLBACK, MS_RLC, "R+L/C", menu_marker_smith_cb },
  { MT_CANCEL, 0, S_LARROW" BACK", NULL },
  { MT_NONE, 0, NULL, NULL } // sentinel
};

const menuitem_t menu_marker[] = {
  { MT_SUBMENU, 0, "\2SELECT\0MARKER", menu_marker_sel },
  { MT_SUBMENU, 0, "SEARCH", menu_marker_search },
  { MT_SUBMENU, 0, "OPERATIONS", menu_marker_ops },
  { MT_SUBMENU, 0, "\2SMITH\0VALUE", menu_marker_smith },
  { MT_CANCEL, 0, S_LARROW" BACK", NULL },
  { MT_NONE, 0, NULL, NULL } // sentinel
};

const menuitem_t menu_recall[] = {
  { MT_CALLBACK, 0, "RECALL 0", menu_recall_cb },
  { MT_CALLBACK, 1, "RECALL 1", menu_recall_cb },
  { MT_CALLBACK, 2, "RECALL 2", menu_recall_cb },
  { MT_CALLBACK, 3, "RECALL 3", menu_recall_cb },
  { MT_CALLBACK, 4, "RECALL 4", menu_recall_cb },
  { MT_CANCEL, 0, S_LARROW" BACK", NULL },
  { MT_NONE, 0, NULL, NULL } // sentinel
};

const menuitem_t menu_dfu[] = {
  { MT_CALLBACK, 0, "\2RESET AND\0ENTER DFU", menu_dfu_cb },
  { MT_CANCEL, 0, S_LARROW"CANCEL", NULL },
  { MT_NONE, 0, NULL, NULL } // sentinel
};

const menuitem_t menu_config[] = {
  { MT_CALLBACK, 0, "TOUCH CAL", menu_config_cb },
  { MT_CALLBACK, 0, "TOUCH TEST", menu_config_cb },
  { MT_CALLBACK, 0, "SAVE", menu_config_save_cb },
  { MT_CALLBACK, 0, "VERSION", menu_config_cb },
  { MT_SUBMENU, 0, S_RARROW"DFU", menu_dfu },
  { MT_CANCEL, 0, S_LARROW" BACK", NULL },
  { MT_NONE, 0, NULL, NULL } // sentinel
};

const menuitem_t menu_top[] = {
  { MT_SUBMENU, 0, "DISPLAY", menu_display },
  { MT_SUBMENU, 0, "MARKER", menu_marker },
  { MT_SUBMENU, 0, "STIMULUS", menu_stimulus },
  { MT_SUBMENU, 0, "CAL", menu_cal },
  { MT_SUBMENU, 0, "RECALL", menu_recall },
  { MT_SUBMENU, 0, "CONFIG", menu_config },
  { MT_NONE, 0, NULL, NULL } // sentinel
};
#endif

#include "ui_sa.c"

#define MENU_STACK_DEPTH_MAX 7
const menuitem_t *menu_stack[MENU_STACK_DEPTH_MAX] = {
  menu_top, NULL, NULL, NULL
};

static void
ensure_selection(void)
{
  const menuitem_t *menu = menu_stack[menu_current_level];
  if (selection < 0) {selection = -1; return;}
  int i;
  if (MT_MASK(menu[0].type) == MT_TITLE && selection == 0) {
    selection = 1;
    return;
  }
  for (i = 0; MT_MASK(menu[i].type) != MT_NONE; i++)
    ;
  if (selection >= i)
    selection = i-1;
}

static void
menu_move_back(bool leave_ui)
{
  if (menu_current_level == 0)
    return;
  erase_menu_buttons();
  if ( menu_is_form(menu_stack[menu_current_level  ]) &&
      !menu_is_form(menu_stack[menu_current_level-1]))
    redraw_request|=REDRAW_AREA|REDRAW_BATTERY|REDRAW_FREQUENCY|REDRAW_CAL_STATUS; // redraw all if switch from form to normal menu mode
  menu_current_level--;
  selection = -1;

  if (leave_ui){
    ui_mode_normal();
    return;
  }
  ui_mode_menu();
}

static void
menu_push_submenu(const menuitem_t *submenu)
{
  erase_menu_buttons();
  if (menu_current_level < MENU_STACK_DEPTH_MAX-1)
    menu_current_level++;
  menu_stack[menu_current_level] = submenu;
  ui_mode_menu();
}

void
menu_push_lowoutput(void)
{
  menu_push_submenu(menu_lowoutputmode);
}

void
menu_push_highoutput(void)
{
  menu_push_submenu(menu_highoutputmode);
}

int current_menu_is_form(void)
{
  return menu_is_form(menu_stack[menu_current_level]);
}

/*
static void
menu_move_top(void)
{
  if (menu_current_level == 0)
    return;
  menu_current_level = 0;
  ensure_selection();
  erase_menu_buttons();
  draw_menu();
}
*/

static void
menu_invoke(int item)
{
  const menuitem_t *menu = menu_stack[menu_current_level];
  menu = &menu[item];

  switch (MT_MASK(menu->type)) {
  case MT_NONE:
  case MT_BLANK:
    ui_mode_normal();
    break;

  case MT_CANCEL:
    menu_move_back(false);
    break;

  case MT_CALLBACK: {
    uistat.auto_center_marker = false;
    menuaction_cb_t cb = (menuaction_cb_t)menu->reference;
    if (cb) (*cb)(item, menu->data);
//    if (!(menu->type & MT_FORM))
    redraw_request |= REDRAW_CAL_STATUS;
    break;
  }
  case MT_ADV_CALLBACK: {
    uistat.auto_center_marker = false;
    menuaction_acb_t cb = (menuaction_acb_t)menu->reference;
    if (cb) (*cb)(item, menu->data, NULL);
//    if (!(menu->type & MT_FORM))
    redraw_request |= REDRAW_CAL_STATUS | REDRAW_BATTERY;
    break;
  }
  case MT_SUBMENU:
    menu_push_submenu((const menuitem_t*)menu->reference);
    break;

  case MT_KEYPAD:
    uistat.auto_center_marker = false;
    if (menu->type & MT_FORM) {
      redraw_frame();         // Remove form numbers
    }
    kp_help_text = (char *)menu->reference;
    if (menu->data <= KM_CW) {      // One of the frequency input keypads
      if (MODE_LOW(setting.mode))
        kp_help_text = "0..350MHz";
      else
        kp_help_text = "240..960Mhz";
    }
    ui_mode_keypad(menu->data);
    redraw_request |= REDRAW_CAL_STATUS;
    break;
  }
  // Redraw menu after if UI in menu mode
  if (ui_mode == UI_MENU)
    draw_menu();
}

#ifdef __VNA__
// Key names
#define KP_0          0
#define KP_1          1
#define KP_2          2
#define KP_3          3
#define KP_4          4
#define KP_5          5
#define KP_6          6
#define KP_7          7
#define KP_8          8
#define KP_9          9
#define KP_PERIOD    10
#define KP_MINUS     11
#define KP_X1        12
#define KP_K         13
#define KP_M         14
#define KP_G         15
#define KP_BS        16
#define KP_INF       17
#define KP_DB        18
#define KP_PLUSMINUS 19
#define KP_KEYPAD    20
#define KP_N         21
#define KP_P         22

typedef struct {
  uint8_t x:4;
  uint8_t y:4;
  int8_t  c;
} keypads_t;

static const keypads_t *keypads;

static const keypads_t keypads_freq[] = {
  { 1, 3, KP_PERIOD },
  { 0, 3, KP_0 },
  { 0, 2, KP_1 },
  { 1, 2, KP_2 },
  { 2, 2, KP_3 },
  { 0, 1, KP_4 },
  { 1, 1, KP_5 },
  { 2, 1, KP_6 },
  { 0, 0, KP_7 },
  { 1, 0, KP_8 },
  { 2, 0, KP_9 },
  { 3, 0, KP_G },
  { 3, 1, KP_M },
  { 3, 2, KP_K },
  { 3, 3, KP_X1 },
  { 2, 3, KP_BS },
  { 0, 0, -1 }
};

static const keypads_t keypads_scale[] = {
  { 1, 3, KP_PERIOD },
  { 0, 3, KP_0 },
  { 0, 2, KP_1 },
  { 1, 2, KP_2 },
  { 2, 2, KP_3 },
  { 0, 1, KP_4 },
  { 1, 1, KP_5 },
  { 2, 1, KP_6 },
  { 0, 0, KP_7 },
  { 1, 0, KP_8 },
  { 2, 0, KP_9 },
  { 3, 3, KP_X1 },
  { 2, 3, KP_BS },
  { 0, 0, -1 }
};

static const keypads_t keypads_time[] = {
  { 1, 3, KP_PERIOD },
  { 0, 3, KP_0 },
  { 0, 2, KP_1 },
  { 1, 2, KP_2 },
  { 2, 2, KP_3 },
  { 0, 1, KP_4 },
  { 1, 1, KP_5 },
  { 2, 1, KP_6 },
  { 0, 0, KP_7 },
  { 1, 0, KP_8 },
  { 2, 0, KP_9 },
  { 3, 1, KP_N },
  { 3, 2, KP_P },
  { 3, 3, KP_MINUS },
  { 2, 3, KP_BS },
  { 0, 0, -1 }
};

static const keypads_t * const keypads_mode_tbl[] = {
  keypads_freq, // start
  keypads_freq, // stop
  keypads_freq, // center
  keypads_freq, // span
  keypads_freq, // cw freq
  keypads_scale, // scale
  keypads_scale, // refpos
  keypads_time, // electrical delay
  keypads_scale, // velocity factor
  keypads_time // scale of delay
};

static const char * const keypad_mode_label[] = {
  "START", "STOP", "CENTER", "SPAN", "CW FREQ", "SCALE", "REFPOS", "EDELAY", "VELOCITY%", "DELAY"
};
#endif

static const char * const keypad_scale_text[] = {"0", "1", "2", "5", "10", "20" , "50", "100", "200", "500"};
//static const int  keypad_scale_value[] = { 1, 2, 5, 10, 20 , 50, 100, 200, 500};

static void
draw_button(uint16_t x, uint16_t y, uint16_t w, uint16_t h, ui_button_t *b)
{
  uint16_t bw = b->border&BUTTON_BORDER_WIDTH_MASK;
  ili9341_set_background(b->bg);ili9341_fill(x + bw, y + bw, w - (bw * 2), h - (bw * 2));
  if (bw==0) return;
  uint16_t br = LCD_RISE_EDGE_COLOR;
  uint16_t bd = LCD_FALLEN_EDGE_COLOR;
  uint16_t type = b->border;
  ili9341_set_background(type&BUTTON_BORDER_TOP    ? br : bd);ili9341_fill(x,          y,           w, bw); // top
  ili9341_set_background(type&BUTTON_BORDER_RIGHT  ? br : bd);ili9341_fill(x + w - bw, y,          bw,  h); // right
  ili9341_set_background(type&BUTTON_BORDER_LEFT   ? br : bd);ili9341_fill(x,          y,          bw,  h); // left
  ili9341_set_background(type&BUTTON_BORDER_BOTTOM ? br : bd);ili9341_fill(x,          y + h - bw,  w, bw); // bottom
}

static void
draw_keypad(void)
{
  int i = 0;
  ui_button_t button;
  button.fg = LCD_MENU_TEXT_COLOR;
  while (keypads[i].c >= 0) {
    button.bg = LCD_MENU_COLOR;
    if (i == selection){
      button.bg = LCD_MENU_ACTIVE_COLOR;
      button.border = KEYBOARD_BUTTON_BORDER|BUTTON_BORDER_FALLING;
    }
    else
      button.border = KEYBOARD_BUTTON_BORDER|BUTTON_BORDER_RISE;
    int x = KP_GET_X(keypads[i].x);
    int y = KP_GET_Y(keypads[i].y);
    draw_button(x, y, KP_WIDTH, KP_HEIGHT, &button);
    ili9341_set_foreground(button.fg);
    ili9341_set_background(button.bg);
    if (keypads[i].c < KP_0) { // KP_0
      ili9341_drawfont(keypads[i].c,
                     x + (KP_WIDTH - NUM_FONT_GET_WIDTH) / 2,
                     y + (KP_HEIGHT - NUM_FONT_GET_HEIGHT) / 2);
    } else {
      const char *t = keypad_scale_text[keypads[i].c - KP_0];
      ili9341_drawstring_10x14(t,
                     x + (KP_WIDTH  - wFONT_MAX_WIDTH*strlen(t)) / 2,
                     y + (KP_HEIGHT - wFONT_GET_HEIGHT) / 2);
    }
    i++;
  }
}

static int
menu_is_multiline(const char *label)
{
  int n = 1;
  while (*label)
    if (*label++ == '\n')
      n++;
  return n;
}

static void
draw_numeric_input(const char *buf)
{
  int i;
  int x;
  uint16_t xsim = 0b0010010000000000;

  ili9341_set_foreground(LCD_INPUT_TEXT_COLOR);
  ili9341_set_background(LCD_INPUT_BG_COLOR);
  for (i = 0, x = 64; i < 10 && buf[i]; i++, xsim<<=1) {
    int c = buf[i];
    if (c == '.')
      c = KP_PERIOD;
    else if (c == '-')
      c = KP_MINUS;
    else// if (c >= '0' && c <= '9')
      c = c - '0';

    if (c >= 0) // c is number
      ili9341_drawfont(c, x, LCD_HEIGHT-NUM_INPUT_HEIGHT+4);
    else // erase
      break;

    x += xsim&0x8000 ? NUM_FONT_GET_WIDTH+2+8 : NUM_FONT_GET_WIDTH+2;
  }
  // erase last
  ili9341_fill(x, LCD_HEIGHT-NUM_INPUT_HEIGHT+4, LCD_WIDTH-x-1, NUM_FONT_GET_WIDTH+2+8);
  if (buf[0] == 0 && kp_help_text != NULL) {
    int  lines = menu_is_multiline(kp_help_text);
    ili9341_set_foreground(LCD_INPUT_TEXT_COLOR);
    ili9341_drawstring_7x13(kp_help_text, 64+NUM_FONT_GET_WIDTH+2, LCD_HEIGHT-(lines*bFONT_GET_HEIGHT+NUM_INPUT_HEIGHT)/2);
  }
}

static void
draw_numeric_area_frame(void)
{
  ili9341_set_foreground(LCD_INPUT_TEXT_COLOR);
  ili9341_set_background(LCD_INPUT_BG_COLOR);

  ili9341_fill(0, LCD_HEIGHT-NUM_INPUT_HEIGHT, LCD_WIDTH, NUM_INPUT_HEIGHT);
  char *name = keypads_mode_tbl[keypad_mode].name;
  int lines = menu_is_multiline(name);
  ili9341_drawstring_7x13(name, 10, LCD_HEIGHT-NUM_INPUT_HEIGHT + (NUM_INPUT_HEIGHT-lines*bFONT_STR_HEIGHT)/2);
  //ili9341_drawfont(KP_KEYPAD, 300, 216);
  draw_numeric_input("");
}

#ifdef __VNA__
static void
menu_item_modify_attribute(const menuitem_t *menu, int item,
                           uint16_t *fg, uint16_t *bg)
{
  if (menu == menu_trace && item < TRACES_MAX) {
    if (trace[item].enabled)
      *bg = config.trace_color[item];
  } else if (menu == menu_marker_sel) {
    if (item < 4) {
      if (markers[item].enabled) {
        *bg = LCD_MENU_TEXT_COLOR;
        *fg = config.menu_normal_color;
      }
    } else if (item == 5) {
      if (uistat.marker_delta) {
        *bg = LCD_MENU_TEXT_COLOR;
        *fg = config.menu_normal_color;
      }
    }
  } else if (menu == menu_marker_search) {
    if (item == 4 && uistat.marker_tracking) {
      *bg = LCD_MENU_TEXT_COLOR;
      *fg = config.menu_normal_color;
    }
  } else if (menu == menu_marker_smith) {

    if (marker_smith_format == item) {
      *bg = LCD_MENU_TEXT_COLOR;
      *fg = config.menu_normal_color;
    }
  } else if (menu == menu_calop) {
    if ((item == 0 && (cal_status & CALSTAT_OPEN))
        || (item == 1 && (cal_status & CALSTAT_SHORT))
        || (item == 2 && (cal_status & CALSTAT_LOAD))
        || (item == 3 && (cal_status & CALSTAT_ISOLN))
        || (item == 4 && (cal_status & CALSTAT_THRU))) {
      domain_mode = (domain_mode & ~DOMAIN_MODE) | DOMAIN_FREQ;
      *bg = LCD_MENU_TEXT_COLOR;
      *fg = config.menu_normal_color;
    }
  } else if (menu == menu_stimulus) {
    if (item == 5 /* PAUSE */ && !(sweep_mode&SWEEP_ENABLE)) {
      *bg = LCD_MENU_TEXT_COLOR;
      *fg = config.menu_normal_color;
    }
  } else if (menu == menu_cal) {
    if (item == 3 /* CORRECTION */ && (cal_status & CALSTAT_APPLY)) {
      *bg = LCD_MENU_TEXT_COLOR;
      *fg = config.menu_normal_color;
    }
  } else if (menu == menu_bandwidth) {
      if (item == bandwidth) {
        *bg = 0x0000;
        *fg = 0xffff;
      }
  } else if (menu == menu_transform) {
      if ((item == 0 && (domain_mode & DOMAIN_MODE) == DOMAIN_TIME)
       || (item == 1 && (domain_mode & TD_FUNC) == TD_FUNC_LOWPASS_IMPULSE)
       || (item == 2 && (domain_mode & TD_FUNC) == TD_FUNC_LOWPASS_STEP)
       || (item == 3 && (domain_mode & TD_FUNC) == TD_FUNC_BANDPASS)
       ) {
        *bg = LCD_MENU_TEXT_COLOR;
        *fg = config.menu_normal_color;
      }
  } else if (menu == menu_transform_window) {
      if ((item == 0 && (domain_mode & TD_WINDOW) == TD_WINDOW_MINIMUM)
       || (item == 1 && (domain_mode & TD_WINDOW) == TD_WINDOW_NORMAL)
       || (item == 2 && (domain_mode & TD_WINDOW) == TD_WINDOW_MAXIMUM)
       ) {
        *bg = LCD_MENU_TEXT_COLOR;
        *fg = config.menu_normal_color;
      }
  }
}
#endif

#ifndef __VNA__
extern void menu_item_modify_attribute(
    const menuitem_t *menu, int item, ui_button_t *button);
#endif

static bool menuDisabled(uint8_t type){
  if ((type & MT_LOW) && !MODE_LOW(setting.mode))
    return true;
  if ((type & MT_HIGH) && !MODE_HIGH(setting.mode))
    return true;
  if (type == MT_BLANK)
    return true;
  return false;
}

#define ICON_WIDTH        16
#define ICON_HEIGHT       11

static const uint8_t check_box[] = {
  _BMP16(0b0011111111110000),
  _BMP16(0b0010000000010000),
  _BMP16(0b0010000000010000),
  _BMP16(0b0010000000010000),
  _BMP16(0b0010000000010000),
  _BMP16(0b0010000000010000),
  _BMP16(0b0010000000010000),
  _BMP16(0b0010000000010000),
  _BMP16(0b0010000000010000),
  _BMP16(0b0010000000010000),
  _BMP16(0b0011111111110000),

  _BMP16(0b0011111111110000),
  _BMP16(0b0010000000001000),
  _BMP16(0b0010000000011000),
  _BMP16(0b0010000000110000),
  _BMP16(0b0010000001100000),
  _BMP16(0b0010100011010000),
  _BMP16(0b0010110110010000),
  _BMP16(0b0010011100010000),
  _BMP16(0b0010001000010000),
  _BMP16(0b0010000000010000),
  _BMP16(0b0011111111110000),

  _BMP16(0b0011111111111000),
  _BMP16(0b0010000000001000),
  _BMP16(0b0010001111101000),
  _BMP16(0b0010011001101000),
  _BMP16(0b0010110001101000),
  _BMP16(0b0010110001101000),
  _BMP16(0b0010111111101000),
  _BMP16(0b0010110001101000),
  _BMP16(0b0010110001101000),
  _BMP16(0b0010000000001000),
  _BMP16(0b0011111111111000),

  _BMP16(0b0011111111111000),
  _BMP16(0b0010000000001000),
  _BMP16(0b0010110001101000),
  _BMP16(0b0010110001101000),
  _BMP16(0b0010111011101000),
  _BMP16(0b0010111111101000),
  _BMP16(0b0010110101101000),
  _BMP16(0b0010110101101000),
  _BMP16(0b0010110001101000),
  _BMP16(0b0010000000001000),
  _BMP16(0b0011111111111000),

  _BMP16(0b0000000000000000),
  _BMP16(0b0000011110000000),
  _BMP16(0b0000100001000000),
  _BMP16(0b0001000000100000),
  _BMP16(0b0010000000010000),
  _BMP16(0b0010000000010000),
  _BMP16(0b0010000000010000),
  _BMP16(0b0010000000010000),
  _BMP16(0b0001000000100000),
  _BMP16(0b0000100001000000),
  _BMP16(0b0000011110000000),

  _BMP16(0b0000000000000000),
  _BMP16(0b0000011110000000),
  _BMP16(0b0000100001000000),
  _BMP16(0b0001001100100000),
  _BMP16(0b0010011110010000),
  _BMP16(0b0010111111010000),
  _BMP16(0b0010111111010000),
  _BMP16(0b0010011110010000),
  _BMP16(0b0001001100100000),
  _BMP16(0b0000100001000000),
  _BMP16(0b0000011110000000),
};
static const char *step_text[5] = {"-10dB", "-1dB", "set", "+1dB", "+10dB"};
static char step_text_freq[5][10] = { "-100MHz", "-10MHz", "set", "+10MHz", "+100MHz" };

static void
draw_menu_buttons(const menuitem_t *menu)
{
  int i = 0;
  int y = 0;
  ui_button_t button;
  for (i = 0; i < MENU_BUTTON_MAX; i++) {
    if (menuDisabled(menu[i].type))            //not applicable to mode
      continue;
    if (MT_MASK(menu[i].type) == MT_NONE)
      break;
    button.icon = BUTTON_ICON_NONE;
    // Border width
    button.border = MENU_BUTTON_BORDER;

    if (MT_MASK(menu[i].type) == MT_TITLE) {
      button.fg = LCD_FG_COLOR;
      button.bg = LCD_BG_COLOR;
      button.border = 0; // no border for title
    } else {
      button.bg = LCD_MENU_COLOR;
      button.fg = LCD_MENU_TEXT_COLOR;
    }

    if (i == selection){
      button.bg = LCD_MENU_ACTIVE_COLOR;
      button.border|= BUTTON_BORDER_FALLING;
    }
    else
      button.border|= BUTTON_BORDER_RISE;

    // Need replace this obsolete bad function on new MT_ADV_CALLBACK variant
    menu_item_modify_attribute(menu, i, &button);      // before plot_printf to create status text

    // MT_ADV_CALLBACK - allow change button data in callback, more easy and correct
    if (MT_MASK(menu[i].type) == MT_ADV_CALLBACK){
      menuaction_acb_t cb = (menuaction_acb_t)menu[i].reference;
      if (cb) (*cb)(i, menu[i].data, &button);
    }
    // Only keypad retrieves value
    if (menu[i].type & MT_FORM && MT_MASK(menu[i].type) == MT_KEYPAD) {
      keypad_mode = menu[i].data;
      fetch_numeric_target();
      button.param_1.text = uistat.text;
    }

    // Prepare button label
    plot_printf(button.text, sizeof button.text, menu[i].label, button.param_1.u, button.param_2.u);

    if (menu[i].type & MT_FORM) {
      int button_width = MENU_FORM_WIDTH;
      int button_start = (LCD_WIDTH - MENU_FORM_WIDTH)/2; // At center of screen
      int button_height = MENU_BUTTON_HEIGHT;
      draw_button(button_start, y, button_width, button_height, &button);

      ili9341_set_foreground(button.fg);
      ili9341_set_background(button.bg);
      uint16_t text_offs = button_start + 6;
      if (button.icon >=0){
        ili9341_blitBitmap(button_start+3, y+(MENU_BUTTON_HEIGHT-ICON_HEIGHT)/2, ICON_WIDTH, ICON_HEIGHT, &check_box[button.icon*2*ICON_HEIGHT]);
        text_offs = button_start+6+ICON_WIDTH+1;
      }
#ifdef __ICONS__
      if (menu[i].type & MT_ICON) {
        ili9341_blitBitmap(button_start+MENU_FORM_WIDTH-2*FORM_ICON_WIDTH-8,y+(button_height-FORM_ICON_HEIGHT)/2,FORM_ICON_WIDTH,FORM_ICON_HEIGHT,& left_icons[((menu[i].data >>4)&0xf)*2*FORM_ICON_HEIGHT]);
        ili9341_blitBitmap(button_start+MENU_FORM_WIDTH-  FORM_ICON_WIDTH-8,y+(button_height-FORM_ICON_HEIGHT)/2,FORM_ICON_WIDTH,FORM_ICON_HEIGHT,&right_icons[((menu[i].data >>0)&0xf)*2*FORM_ICON_HEIGHT]);
      }
#endif
      int local_slider_positions = 0;
      int local_text_shift = 0;
      if (MT_MASK(menu[i].type) == MT_KEYPAD) {
        local_text_shift = 2;
        if (menu[i].data == KM_CENTER) {
          local_slider_positions =  LCD_WIDTH/2+setting.slider_position;
          plot_printf(step_text_freq[0], sizeof step_text_freq[0], "-%3.0FHz", (float)setting.slider_span);
          plot_printf(step_text_freq[1], sizeof step_text_freq[1], "-%3.0FHz", (float)setting.slider_span/10);
          plot_printf(step_text_freq[3], sizeof step_text_freq[3], "+%3.0FHz", (float)setting.slider_span/10);
          plot_printf(step_text_freq[4], sizeof step_text_freq[4], "+%3.0FHz", (float)setting.slider_span);
          for (int i=0; i <= 4; i++) {
            ili9341_drawstring(step_text_freq[i], button_start+12 + i * MENU_FORM_WIDTH/5, y+button_height-9);
          }
          goto draw_divider;
        } else if (menu[i].data == KM_LOWOUTLEVEL) {
          local_slider_positions = ((get_level() - level_min) * (MENU_FORM_WIDTH-8)) / level_range + OFFSETX+4;
          for (int i=0; i <= 4; i++) {
            ili9341_drawstring(step_text[i], button_start+12 + i * MENU_FORM_WIDTH/5, y+button_height-9);
          }
        draw_divider:
          for (int i = 1; i <= 4; i++) {
            ili9341_line(button_start + i * MENU_FORM_WIDTH/5, y+button_height-9, button_start + i * MENU_FORM_WIDTH/5, y+button_height);
          }
        draw_slider:
          if (local_slider_positions < button_start)
            local_slider_positions = button_start;
          ili9341_blitBitmap(local_slider_positions - 4, y, 7, 5, slider_bitmap);
        } else if (menu[i].data == KM_HIGHOUTLEVEL) {
          local_slider_positions = ((get_level() - level_min ) * (MENU_FORM_WIDTH-8)) / level_range + OFFSETX+4;
          goto draw_slider;
        }
      }
//      ili9341_drawstring_size(button.text, text_offs, y+(button_height-2*FONT_GET_HEIGHT)/2-local_text_shift, 2);
      ili9341_drawstring_10x14(button.text, text_offs, y+(button_height-wFONT_GET_HEIGHT)/2-local_text_shift);
    } else {
      int button_width = MENU_BUTTON_WIDTH;
      int button_start = LCD_WIDTH - MENU_BUTTON_WIDTH;
      int button_height = MENU_BUTTON_HEIGHT;
      draw_button(button_start, y, button_width, button_height, &button);
      ili9341_set_foreground(button.fg);
      ili9341_set_background(button.bg);
      uint16_t text_offs = button_start + 7;
      if (button.icon >=0){
        ili9341_blitBitmap(button_start+2, y+(MENU_BUTTON_HEIGHT-ICON_HEIGHT)/2, ICON_WIDTH, ICON_HEIGHT, &check_box[button.icon*2*ICON_HEIGHT]);
        text_offs = button_start+2+ICON_WIDTH;
      }
      int lines = menu_is_multiline(button.text);
#define BIG_BUTTON_FONT 1
#ifdef BIG_BUTTON_FONT
      ili9341_drawstring_7x13(button.text, text_offs, y+(button_height-lines*bFONT_GET_HEIGHT)/2);
#else
      ili9341_drawstring(button.text, text_offs, y+(button_height-linesFONT_GET_HEIGHT)/2);
#endif
    }
    y += MENU_BUTTON_HEIGHT;
  }
  // Cleanup other buttons (less flicker)
  ili9341_set_background(LCD_BG_COLOR);
  for (; y < MENU_BUTTON_MAX*MENU_BUTTON_HEIGHT; y+=MENU_BUTTON_HEIGHT)
    ili9341_fill(LCD_WIDTH-MENU_BUTTON_WIDTH, y, MENU_BUTTON_WIDTH, MENU_BUTTON_HEIGHT);
//  if (menu[i].type & MT_FORM)
//    draw_battery_status();
}

static systime_t prev_touch_time = 0;
static int prev_touch_button = -1;

enum { SL_UNKNOWN, SL_SPAN, SL_MOVE};

void set_keypad_value(int v)
{
  keypad_mode = v;
  set_numeric_value();
}

void check_frequency_slider(freq_t slider_freq)
{

  if ( (maxFreq - minFreq) < (freq_t)setting.slider_span) {
    setting.slider_span = maxFreq - minFreq;                         // absolute mode with max step size
  }
  freq_t half_span = setting.slider_span >> 1;
  int temp = (setting.slider_span / (MENU_FORM_WIDTH-8));
  if (minFreq + half_span > slider_freq) {
    setting.slider_position -= (minFreq + half_span - slider_freq) / temp;            // reposition if needed
  }
  if (maxFreq < slider_freq + half_span) {
    setting.slider_position += (slider_freq + half_span - maxFreq) / temp;            // reposition if needed
  }
}

static void
menu_select_touch(int i, int pos)
{
  long_t step = 0;
  int do_exit = false;
  selection = i;
  draw_menu();
#if 1               // drag values
  const menuitem_t *menu = menu_stack[menu_current_level];
  int old_keypad_mode = keypad_mode;
  int keypad = menu[i].data;
  prev_touch_time = chVTGetSystemTimeX();

    int touch_x, touch_y,  prev_touch_x = 0;
//    touch_position(&touch_x, &touch_y);
    systime_t dt = 0;
    int mode = SL_UNKNOWN;
    while (touch_check() != EVT_TOUCH_NONE) {

      systime_t ticks = chVTGetSystemTimeX();
      dt = ticks - prev_touch_time;

      if (dt > BUTTON_DOWN_LONG_TICKS) {
        touch_position(&touch_x, &touch_y);
        if (touch_x !=  prev_touch_x /* - 1 || prev_touch_x + 1 < touch_x */ ) {
        keypad_mode = keypad;
        fetch_numeric_target();
        int new_slider = touch_x - LCD_WIDTH/2;   // Can have negative outcome
        if (new_slider < - (MENU_FORM_WIDTH-8)/2)
          new_slider = -(MENU_FORM_WIDTH-8)/2;
        if (new_slider > (MENU_FORM_WIDTH-8)/2)
          new_slider = (MENU_FORM_WIDTH-8)/2;
        if (menu_is_form(menu) && MT_MASK(menu[i].type) == MT_KEYPAD && keypad == KM_CENTER){
#define TOUCH_DEAD_ZONE 40
          if (mode == SL_UNKNOWN ) {
            if (setting.slider_position - TOUCH_DEAD_ZONE < new_slider && new_slider < setting.slider_position + TOUCH_DEAD_ZONE) { // Pick up slider
              mode = SL_MOVE;
            } else {
              mode = SL_SPAN;
              goto first_span;
            }
          }
          if (mode == SL_MOVE ) {
              uistat.value+= (int)(setting.slider_span/(MENU_FORM_WIDTH-8))*(new_slider - setting.slider_position);
              if (uistat.value < minFreq)
                uistat.value = minFreq;
              if (uistat.value > maxFreq)
                uistat.value = maxFreq;
              setting.slider_position = new_slider;
              set_keypad_value(keypad);
              dirty = false;
              perform(false, 0, (freq_t)uistat.value, false);
              draw_menu();
          } else if (mode == SL_SPAN ){
              freq_t slider_freq;
            first_span:
              slider_freq = (freq_t) uistat.value;
              int pw=new_slider + LCD_WIDTH/2;
              setting.slider_position = pw - LCD_WIDTH/2;   // Show delta on slider
              setting.slider_span = 10;
              while (pw>0) {
                setting.slider_span += setting.slider_span;
                pw -= 12;
                if (pw <=0)
                  break;
                setting.slider_span += setting.slider_span + (setting.slider_span >>1);
                pw -= 12;
                if (pw<=0)
                  break;
                setting.slider_span *= 2;
                pw -= 12;
              }
              if ((freq_t)setting.slider_span > (maxFreq - minFreq))
                setting.slider_span = (maxFreq - minFreq);
              freq_t old_minFreq = minFreq;           // Save when in high mode
              minFreq = 0;                              // And set minFreq to 0 for span display
              uistat.value = setting.slider_span;
              set_keypad_value(keypad);
#if 1
              plot_printf(center_text, sizeof center_text, "RANGE: %%s");
#else
              center_text[0] = 'S';
              center_text[1] = 'P';
              center_text[2] = 'A';
              center_text[3] = 'N';
#endif
              draw_menu();                               // Show slider span
              minFreq = old_minFreq;                     // and restore minFreq
              uistat.value = (float) slider_freq;        // and restore current slider freq
              set_keypad_value(keypad);
#if 1
              plot_printf(center_text, sizeof center_text, "FREQ: %%s");
#else
              center_text[0] = 'F';
              center_text[1] = 'R';
              center_text[2] = 'E';
              center_text[3] = 'Q';
#endif
              setting.slider_position = 0;                         // reset slider after span change
              check_frequency_slider(slider_freq);
         }
        } else if (menu_is_form(menu) && MT_MASK(menu[i].type) == MT_KEYPAD && keypad == KM_LOWOUTLEVEL) {
            uistat.value =  setting.external_gain + ((touch_x - OFFSETX+4) * level_range ) / (MENU_FORM_WIDTH-8) + level_min ;
         apply_step:
            set_keypad_value(keypad);
         apply:
            perform(false, 0, get_sweep_frequency(ST_CENTER), false);
            draw_menu();
//          }
//        } else if (MT_MASK(menu[i].type) == MT_ADV_CALLBACK && menu[i].reference == menu_sdrive_acb) {
        } else if (menu_is_form(menu) && MT_MASK(menu[i].type) == MT_KEYPAD && keypad == KM_HIGHOUTLEVEL) {
            set_level( (touch_x - OFFSETX+4) *(level_range) / (MENU_FORM_WIDTH-8) + level_min );
            goto apply;
        }
        keypad_mode = old_keypad_mode;
      }
      }
      prev_touch_x = touch_x;
    }
    if (dt > BUTTON_DOWN_LONG_TICKS || do_exit) {
      selection = -1;
      draw_menu();
//      redraw_request = 0; // reset all (not need update after)
      return;
    }
    if (menu_is_form(menu) && MT_MASK(menu[i].type) == MT_KEYPAD && keypad == KM_LOWOUTLEVEL) {
      switch (pos) {
      case 0:
        step = -10;
        break;
      case 1:
        step = -1;
        break;
      case 2:
        goto nogo;
      case 3:
        step = +1;
        break;
      case 4:
        step = +10;
        break;
      }
      uistat.value = setting.external_gain + get_level() + step;
      do_exit = true;
      goto apply_step;
    } else if (menu_is_form(menu) && MT_MASK(menu[i].type) == MT_KEYPAD && keypad == KM_CENTER) {
      switch (pos) {
      case 0:
        step = setting.slider_span;
        step =-step;
        break;
      case 1:
        step = setting.slider_span/10;
        step =-step;
        break;
      case 2:
        goto nogo;
      case 3:
        step = setting.slider_span/10;
        break;
      case 4:
        step = setting.slider_span;
        break;
      }
      if (step < 0 && get_sweep_frequency(ST_CENTER) < (freq_t)(-step))
        uistat.value = 0;
      else
        uistat.value = get_sweep_frequency(ST_CENTER) + step;
      do_exit = true;
      setting.slider_position = 0;                         // reset slider after step
      check_frequency_slider(uistat.value);
      goto apply_step;
    }

nogo:
    setting.slider_position = 0;            // Reset slider when entering frequency
    prev_touch_button = -1;
#endif

//  touch_wait_release();
  selection = -1;
  menu_invoke(i);
}


static void
menu_apply_touch(int touch_x, int touch_y)
{
  const menuitem_t *menu = menu_stack[menu_current_level];
  int i;
  int y = 0;
  for (i = 0; i < MENU_BUTTON_MAX; i++) {
    if (menuDisabled(menu[i].type))            //not applicable to mode
      continue;
    if (MT_MASK(menu[i].type) == MT_TITLE) {
      y += MENU_BUTTON_HEIGHT;
      continue;
    }
    if (MT_MASK(menu[i].type) == MT_NONE) {
      break;
    }
    int active_button_start;
    if (menu[i].type & MT_FORM) {
      active_button_start = (LCD_WIDTH - MENU_FORM_WIDTH)/2;
//      active_button_stop = LCD_WIDTH - active_button_start;
    } else {
      active_button_start = LCD_WIDTH - MENU_BUTTON_WIDTH;
//      active_button_stop = LCD_WIDTH;
    }
    if (y < touch_y && touch_y < y+MENU_BUTTON_HEIGHT) {
      if (touch_x > active_button_start) {
        menu_select_touch(i, (( touch_x - active_button_start) * 5 ) / MENU_FORM_WIDTH);
        return;
      }
    }
    y += MENU_BUTTON_HEIGHT;
  }
  if (menu_is_form(menu))
    return;
  touch_wait_release();
  ui_mode_normal();
}

void
draw_menu(void)
{
  draw_menu_buttons(menu_stack[menu_current_level]);
}

static void
erase_menu_buttons(void)
{
// Not need, screen redraw in all cases
//  ili9341_fill(area_width, 0, LCD_WIDTH - area_width, area_height, LCD_BG_COLOR);
 // if (current_menu_is_form())
 //   ili9341_fill(OFFSETX, 0,LCD_WIDTH-OFFSETX, MENU_BUTTON_HEIGHT*MENU_BUTTON_MAX, LCD_BG_COLOR);
 // else
 //   ili9341_fill(LCD_WIDTH-MENU_BUTTON_WIDTH, 0, MENU_BUTTON_WIDTH, MENU_BUTTON_HEIGHT*MENU_BUTTON_MAX, LCD_BG_COLOR);
  draw_frequencies();
}

#if 0
static void
erase_numeric_input(void)
{
  ili9341_set_background(LCD_BG_COLOR);
  ili9341_fill(0, LCD_HEIGHT-NUM_INPUT_HEIGHT, LCD_WIDTH, NUM_INPUT_HEIGHT);
}
#endif

static void
leave_ui_mode()
{
//  if (ui_mode == UI_MENU) {
//    request_to_draw_cells_behind_menu();
//    erase_menu_buttons();
//  }
  ili9341_set_background(LCD_BG_COLOR);
  // Erase bottom area (not redraw on area update)
  if (MENU_BUTTON_HEIGHT*MENU_BUTTON_MAX - area_height > 0)
    ili9341_fill(LCD_WIDTH-MENU_BUTTON_WIDTH, area_height, MENU_BUTTON_WIDTH, MENU_BUTTON_HEIGHT*MENU_BUTTON_MAX - area_height);
  if (setting.waterfall)
    toggle_waterfall();
  redraw_request|=REDRAW_AREA | REDRAW_FREQUENCY | REDRAW_CAL_STATUS | REDRAW_BATTERY;
}

#ifdef __VNA__
static void
fetch_numeric_target(void)
{
  switch (keypad_mode) {
  case KM_START:
    uistat.value = get_sweep_frequency(ST_START);
    break;
  case KM_STOP:
    uistat.value = get_sweep_frequency(ST_STOP);
    break;
  case KM_CENTER:
    uistat.value = get_sweep_frequency(ST_CENTER);
    break;
  case KM_SPAN:
    uistat.value = get_sweep_frequency(ST_SPAN);
    break;
  case KM_CW:
    uistat.value = get_sweep_frequency(ST_CW);
    break;
  case KM_SCALE:
    uistat.value = get_trace_scale(uistat.current_trace) * 1000;
    break;
  case KM_REFPOS:
    uistat.value = get_trace_refpos(uistat.current_trace) * 1000;
    break;
  case KM_EDELAY:
    uistat.value = get_electrical_delay();
    break;
  case KM_VELOCITY_FACTOR:
    uistat.value = velocity_factor * 100;
    break;
  case KM_SCALEDELAY:
    uistat.value = get_trace_scale(uistat.current_trace) * 1e12;
    break;
  }

  {
    uint32_t x = uistat.value;
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
    set_sweep_frequency(ST_START, uistat.value);
    break;
  case KM_STOP:
    set_sweep_frequency(ST_STOP, uistat.value);
    break;
  case KM_CENTER:
    set_sweep_frequency(ST_CENTER, uistat.value);
    break;
  case KM_SPAN:
    set_sweep_frequency(ST_SPAN, uistat.value);
    break;
  case KM_CW:
    set_sweep_frequency(ST_CW, uistat.value);
    break;
  case KM_SCALE:
    set_trace_scale(uistat.current_trace, uistat.value / 1000.0);
    break;
  case KM_REFPOS:
    set_trace_refpos(uistat.current_trace, uistat.value / 1000.0);
    break;
  case KM_EDELAY:
    set_electrical_delay(uistat.value);
    break;
  case KM_VELOCITY_FACTOR:
    velocity_factor = uistat.value/100.0;
    break;
  }
}
#endif

void
ui_mode_menu(void)
{
//  if (ui_mode == UI_MENU)
//    return;
  ui_mode = UI_MENU;
  ensure_selection();
  if (current_menu_is_form()) {
    redraw_frame();
    area_width = 0;
    area_height = 0;
  } else {
    area_width = AREA_WIDTH_NORMAL - MENU_BUTTON_WIDTH;
    area_height = AREA_HEIGHT_NORMAL;
  }
  draw_menu();
  redraw_request|=REDRAW_BATTERY|REDRAW_CAL_STATUS;
}

static void
ui_mode_keypad(int _keypad_mode)
{
  if (ui_mode == UI_KEYPAD && keypad_mode == _keypad_mode )
    return;

  // keypads array
  keypad_mode = _keypad_mode;
  keypads = keypads_mode_tbl[_keypad_mode].keypad_type;

  ui_mode = UI_KEYPAD;
  if (!current_menu_is_form())
    draw_menu();
  draw_keypad();
  draw_numeric_area_frame();
  ui_process_keypad();
}

void
ui_mode_normal(void)
{
  if (ui_mode == UI_NORMAL)
    return;
  leave_ui_mode();
  area_width  = AREA_WIDTH_NORMAL;
  area_height = AREA_HEIGHT_NORMAL;
  ui_mode = UI_NORMAL;
}

static void
lever_move_marker(int status)
{
  uint16_t step = 1<<2;
  do {
    if (active_marker != MARKER_INVALID && markers[active_marker].enabled) {
      int idx = (int)markers[active_marker].index;
      if (status & EVT_DOWN) {
        idx -= step>>2;
        if (idx < 0) idx = 0 ;
      }
      if (status & EVT_UP) {
        idx += step>>2;
        if (idx  > sweep_points-1) idx = sweep_points-1 ;
      }
      markers[active_marker].index = idx;
      markers[active_marker].frequency = frequencies[idx];
      redraw_marker(active_marker);
      markers[active_marker].mtype &= ~M_TRACKING;    // Disable tracking when dragging marker
      step++;
    }
    status = btn_wait_release();
  } while (status != 0);
}

static void
lever_search_marker(int status)
{
  int i = -1;
  if (active_marker != MARKER_INVALID) {
    if (status & EVT_DOWN)
      i = marker_search_left_max(markers[active_marker].index);
    else if (status & EVT_UP)
      i = marker_search_right_max(markers[active_marker].index);
    if (i != -1) {
      markers[active_marker].index = i;
      interpolate_maximum(active_marker);
//      markers[active_marker].frequency = frequencies[i];
    }
    redraw_marker(active_marker);
  }
}

// ex. 10942 -> 10000
//      6791 ->  5000
//       341 ->   200
static freq_t
step_round(freq_t v)
{
  // decade step
  freq_t x = 1;
  for (x = 1; x*10 <= v; x*= 10)
    ;

  // 1-2-5 step
  if (x * 2 > v)
    return x;
  else if (x * 5 > v)
    return x * 2;
  else
    return x * 5;
}

static void
lever_zoom_span(int status)
{
  freq_t span = get_sweep_frequency(ST_SPAN);
  if (uistat.auto_center_marker) {
    freq_t freq = get_marker_frequency(active_marker);
    search_maximum(active_marker, freq, 10 );
    if (freq != 0)
      set_sweep_frequency(ST_CENTER, freq);
  }
  if (status & EVT_UP) {
    span = step_round(span - 1);
  } else if (status & EVT_DOWN) {
    span = step_round(span + 1);
    span = step_round(span * 3);
  }
  set_sweep_frequency(ST_SPAN, span);
}

static void
lever_zoom_time(int status)
{
  uint32_t time = setting.sweep_time_us; // in uS
  if (time < MINIMUM_SWEEP_TIME)
    time = MINIMUM_SWEEP_TIME;
  if (status & EVT_UP) {
    time = time*10/25;
  } else if (status & EVT_DOWN) {
    time = time*25/10;
  }
  time = step_round(time);
  set_sweep_time_us(time);
}

static void
lever_move(int status, int mode)
{
  freq_t center = get_sweep_frequency(mode);
  freq_t span = get_sweep_frequency(ST_SPAN);
  span = step_round(span / 3);
  if (status & EVT_UP) {
    set_sweep_frequency(mode, center + span);
  } else if (status & EVT_DOWN) {
    set_sweep_frequency(mode, center - span);
  }
}

#define STEPRATIO 0.2
#ifdef __VNA__
static void
lever_edelay(int status)
{
  float value = get_electrical_delay();
  float ratio = STEPRATIO;
  if (value < 0)
    ratio = -ratio;
  if (status & EVT_UP) {
    value = (1 - ratio) * value;
  } else if (status & EVT_DOWN) {
    value = (1 + ratio) * value;
  }
  set_electrical_delay(value);
}
#endif
static void
ui_process_normal(void)
{
  int status = btn_check();
  if (status != 0) {
    if (status & EVT_BUTTON_SINGLE_CLICK) {
      ui_mode_menu();
    } else {
      switch (uistat.lever_mode) {
      case LM_MARKER: lever_move_marker(status);   break;
      case LM_SEARCH: lever_search_marker(status); break;
      case LM_CENTER:
        lever_move(status, FREQ_IS_STARTSTOP() ? ST_START : ST_CENTER);
        break;
      case LM_SPAN:
        if (FREQ_IS_STARTSTOP())
          lever_move(status, ST_STOP);
        else {
          if (FREQ_IS_CW())
            lever_zoom_time(status);
          else
            lever_zoom_span(status);
        }
        break;
#ifdef __VNA__
      case LM_EDELAY:
        lever_edelay(status);
        break;
#endif
      }
    }
  }
}

#ifdef __LISTEN__
bool
ui_process_listen_lever(void)
{
  int status = btn_check();
  if (status != 0) {
    if (status & EVT_BUTTON_SINGLE_CLICK) {
      return false;
    } else {
      lever_move_marker(status);
    }
  }
  return true;
}
#endif

static void
ui_process_menu(void)
{
  // Flag show, can close menu if user come out from it
  // if false user must select some thing
  const menuitem_t *menu = menu_stack[menu_current_level];
  int status = btn_check();
  if (status != 0) {
    if (selection >=0 && status & EVT_BUTTON_SINGLE_CLICK) {
      menu_invoke(selection);
    } else {
      do {
        if (status & EVT_UP) {
          // skip menu item if disabled
          while (menuDisabled(menu[selection+1].type))
            selection++;
          // close menu if next item is sentinel, else step up
          if (menu[selection+1].type != MT_NONE)
            selection++;
          else if (!(menu[0].type & MT_FORM))  // not close if type = form menu
            goto menuclose;
        }
        if (status & EVT_DOWN) {
          // skip menu item if disabled
          while (selection > 0 && menuDisabled(menu[selection-1].type))
            selection--;
          // close menu if item is 0, else step down
          if (selection > 0)
            selection--;
          else if (!(menu[0].type & MT_FORM)) // not close if type = form menu
            goto menuclose;
        }
//activate:
        ensure_selection();
        draw_menu();
        chThdSleepMilliseconds(50); // Add delay for not move so fast in menu
        status = btn_wait_release();
      } while (status != 0);
    }
  }
  return;

menuclose:
  ui_mode_normal();
}

static int
keypad_click(int key)
{
  int c = keypads[key].c;
  if ((c >= KP_X1 && c <= KP_G) || c == KP_m || c == KP_u || c == KP_n) {
    float scale = 1.0;
    if (c >= KP_X1 && c <= KP_G) {
      int n = c - KP_X1;
      while (n-- > 0)
        scale *= 1000.0;
    } else if (c == KP_m) {
      scale /= 1000.0;
    } else if (c == KP_u) {
      scale /= 1000000.0;
    } else if (c == KP_n) {
      scale /= 1000000000.0;
    }
    /* numeric input done */
    uistat.value = my_atof(kp_buf) * scale;
    set_numeric_value();
    return KP_DONE;
  } else if (c <= 9 && kp_index < NUMINPUT_LEN) {
    kp_buf[kp_index++] = '0' + c;
  } else if (c>=KP_0) {
    kp_buf[kp_index++] = keypad_scale_text[c-KP_0][0];
    if (c >=KP_10)
      kp_buf[kp_index++] = '0';
    if (c >=KP_100)
      kp_buf[kp_index++] = '0';
  } else if (c == KP_PERIOD && kp_index < NUMINPUT_LEN) {
    // check period in former input
    int j;
    for (j = 0; j < kp_index && kp_buf[j] != '.'; j++)
      ;
    // append period if there are no period
    if (kp_index == j)
      kp_buf[kp_index++] = '.';
  } else if (c == KP_MINUS) {
    if (kp_index == 0)
      kp_buf[kp_index++] = '-';
  } else if (c == KP_BS) {
    if (kp_index == 0) {
      return KP_CANCEL;
    }
    --kp_index;
  }
  kp_buf[kp_index] = '\0';
  draw_numeric_input(kp_buf);
  return KP_CONTINUE;
}

static int
keypad_apply_touch(void)
{
  int touch_x, touch_y;
  int i = 0;

  touch_position(&touch_x, &touch_y);

  while (keypads[i].c >= 0) {
    int x = KP_GET_X(keypads[i].x);
    int y = KP_GET_Y(keypads[i].y);
    if (x < touch_x && touch_x < x+KP_WIDTH && y < touch_y && touch_y < y+KP_HEIGHT) {
      // draw focus
      selection = i;
      draw_keypad();
      touch_wait_release();
      // erase focus
      selection = -1;
      draw_keypad();
      return i;
    }
    i++;
  }
  return -1;
}

static void
ui_process_keypad(void)
{
  int status;
  kp_index = 0;
  int keypads_last_index;
  for (keypads_last_index = 0; keypads[keypads_last_index+1].c >= 0; keypads_last_index++)
    ;
  while (TRUE) {
    status = btn_check();
    if (status & (EVT_UP|EVT_DOWN)) {
      int s = status;
      do {
        if (s & EVT_UP)
          if (--selection < 0)
            selection = keypads_last_index;
        if (s & EVT_DOWN)
          if (++selection > keypads_last_index)
            selection = 0;
        draw_keypad();
        s = btn_wait_release();
      } while (s != 0);
    }

    if (status == EVT_BUTTON_SINGLE_CLICK) {
      if (keypad_click(selection))
        /* exit loop on done or cancel */
        break;
    }

    if (touch_check() == EVT_TOUCH_PRESSED) {
      int key = keypad_apply_touch();
      if (key >= 0 && keypad_click(key))
        /* exit loop on done or cancel */
        break;
    }
  }
  kp_help_text = NULL;
  redraw_frame();
  if (current_menu_is_form()) {
    ui_mode_menu(); //Reactivate menu after keypad
    selection = -1;
  } else {
    ui_mode_normal();
  }
  //redraw_all();
}

static void
ui_process_lever(void)
{
  switch (ui_mode) {
  case UI_NORMAL:
    ui_process_normal();
    break;
  case UI_MENU:
    ui_process_menu();
    break;
  case UI_KEYPAD:
    ui_process_keypad();
    break;
  }
}

static void
drag_marker(int t, int m)
{
  /* wait touch release */
  do {
    int touch_x, touch_y;
    int index;
    touch_position(&touch_x, &touch_y);
    touch_x -= OFFSETX;
    touch_y -= OFFSETY;
    index = search_nearest_index(touch_x, touch_y, t);
    if (index >= 0) {
      markers[m].index = index;
      markers[m].frequency = frequencies[index];
      redraw_marker(m);
    }
  } while (touch_check()!= EVT_TOUCH_RELEASED);
}

static int
touch_pickup_marker(int touch_x, int touch_y)
{
  int m, t;
  touch_x -= OFFSETX;
  touch_y -= OFFSETY;

  int i = MARKER_INVALID, mt;
  int min_dist = MARKER_PICKUP_DISTANCE * MARKER_PICKUP_DISTANCE;
  // Search closest marker to touch position
  for (t = 0; t < TRACES_MAX; t++) {
    if (!trace[t].enabled)
      continue;
    for (m = 0; m < MARKERS_MAX; m++) {
      if (!markers[m].enabled)
        continue;
      // Get distance to marker from touch point
      int dist = distance_to_index(t, markers[m].index, touch_x, touch_y);
      if (dist < min_dist) {
        min_dist = dist;
        i  = m;
        mt = t;
      }
    }
  }
  // Marker not found
  if (i == MARKER_INVALID)
    return FALSE;
  // Marker found, set as active and start drag it
  if (active_marker != i) {
    previous_marker = active_marker;
    active_marker = i;
  }
  // Disable tracking
  markers[i].mtype &= ~M_TRACKING;    // Disable tracking when dragging marker
  // Leveler mode = marker move
  select_lever_mode(LM_MARKER);
  // select trace
  uistat.current_trace = mt;
  // drag marker until release
  drag_marker(mt, i);
  return TRUE;
}

static int touch_quick_menu(int touch_x, int touch_y)
{
  if (touch_x <OFFSETX)
  {
    touch_wait_release();
    return invoke_quick_menu(touch_y);
  }
  return FALSE;
}

static int
touch_lever_mode_select(int touch_x, int touch_y)
{
  if (touch_y > HEIGHT) {
    touch_wait_release();
    // Touch on left frequency field side
    if (touch_x < FREQUENCIES_XPOS2 - 50) {
      if (uistat.lever_mode == LM_CENTER){
        ui_mode_keypad(FREQ_IS_CENTERSPAN() ? KM_CENTER : KM_START);
        return TRUE;
      }
    }
    else if (touch_x <  FREQUENCIES_XPOS2 + 50) {
      // toggle frequency mode start/stop <=> center/span
      setting.freq_mode^= FREQ_MODE_CENTER_SPAN;
      redraw_request |= REDRAW_FREQUENCY;
      return true;
    }
    else if (uistat.lever_mode == LM_SPAN) {
      ui_mode_keypad(FREQ_IS_CW() ? KM_SWEEP_TIME : (FREQ_IS_CENTERSPAN() ? KM_SPAN : KM_STOP));
      return TRUE;
    }
    select_lever_mode(touch_x < FREQUENCIES_XPOS2 ? LM_CENTER : LM_SPAN);
    return TRUE;
  }
  if (touch_x < OFFSETX)
  {
    return invoke_quick_menu(touch_y);
  }
  else if (touch_y < 25) {
#ifdef __VNA__
    if (touch_x < FREQUENCIES_XPOS2 && get_electrical_delay() != 0.0) {
      select_lever_mode(LM_EDELAY);
    } else {
#endif
      select_lever_mode(LM_MARKER);
#ifdef __VNA__
      }
#endif
    return TRUE;
  }
  return FALSE;
}

static int
touch_marker_select(int touch_x, int touch_y)
{
  int selected_marker = 0;
  if (current_menu_is_form() || touch_x > LCD_WIDTH-MENU_BUTTON_WIDTH || touch_x < 25 || touch_y > 30)
    return FALSE;
  if (touch_y > 15)
    selected_marker = 2;
  selected_marker += (touch_x >150 ? 1 : 0);
  for (int i = 0; i < MARKERS_MAX; i++) {
    if (markers[i].enabled) {
      if (selected_marker == 0) {
        if (active_marker == i) {
          extern const menuitem_t menu_marker_modify[];
          touch_wait_release();
          selection = -1;
          menu_current_level = 0;
          menu_push_submenu(menu_marker_modify);
          break;
        }
        active_marker = i;
        redraw_marker(active_marker);
        break;
      }
      selected_marker --;
    }
  }
  if (touch_y < 25) {
#ifdef __VNA__
    if (touch_x < FREQUENCIES_XPOS2 && get_electrical_delay() != 0.0) {
      select_lever_mode(LM_EDELAY);
    } else {
#endif
      select_lever_mode(LM_MARKER);
#ifdef __VNA__
      }
#endif
    return TRUE;
  }
  return FALSE;
}

static
void ui_process_touch(void)
{
  int touch_x, touch_y;
  int status = touch_check();
  if (status == EVT_TOUCH_PRESSED || status == EVT_TOUCH_DOWN) {
    touch_position(&touch_x, &touch_y);
    switch (ui_mode) {
    case UI_NORMAL:
      if (touch_quick_menu(touch_x, touch_y))
        break;
      // Try drag marker
      if (touch_pickup_marker(touch_x, touch_y))
        break;
      if (touch_marker_select(touch_x, touch_y))
        break;
      // Try select lever mode (top and bottom screen)
      if (touch_lever_mode_select(touch_x, touch_y)) {
//        touch_wait_release();
        break;
      }

      // switch menu mode after release
      touch_wait_release();
      selection = -1; // hide keyboard mode selection
      ui_mode_menu();
      break;
    case UI_MENU:
      menu_apply_touch(touch_x, touch_y);
      break;
    }
  }
}

static uint16_t previous_button_state = 0;
#ifdef __REMOTE_DESKTOP__
static uint16_t previous_mouse_state = 0;
static int16_t previous_mouse_x = 0;
static int16_t previous_mouse_y = 0;
#endif

void
ui_process(void)
{
  int button_state = READ_PORT() & BUTTON_MASK;
  if (ui_mode == UI_NORMAL && current_menu_is_form()) {     //   Force into menu mode
    selection = -1; // hide keyboard mode selection
    ui_mode_menu();
  }
  if (operation_requested&OP_LEVER || previous_button_state != button_state) {
    ui_process_lever();
    previous_button_state = button_state;
    operation_requested = OP_NONE;
  }
  if (operation_requested&OP_TOUCH
#ifdef __REMOTE_DESKTOP__
	  || previous_mouse_state != mouse_down || previous_mouse_x != mouse_x || previous_mouse_y != mouse_y
#endif
  ) {
    ui_process_touch();
	operation_requested = OP_NONE;
  }
  touch_start_watchdog();
}

/* Triggered when the button is pressed or released. The LED4 is set to ON.*/
static void extcb1(EXTDriver *extp, expchannel_t channel)
{
  (void)extp;
  (void)channel;
  operation_requested|=OP_LEVER;
  // cur_button = READ_PORT() & BUTTON_MASK;
}

static const EXTConfig extcfg = {
  {
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_RISING_EDGE | EXT_CH_MODE_AUTOSTART | EXT_MODE_GPIOA, extcb1},
    {EXT_CH_MODE_RISING_EDGE | EXT_CH_MODE_AUTOSTART | EXT_MODE_GPIOA, extcb1},
    {EXT_CH_MODE_RISING_EDGE | EXT_CH_MODE_AUTOSTART | EXT_MODE_GPIOA, extcb1},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL}
  }
};

void
handle_touch_interrupt(void)
{
  operation_requested|= OP_TOUCH;
}

void
ui_init()
{
  adc_init();
  // Activates the EXT driver 1.
  extStart(&EXTD1, &extcfg);
  // Init touch subsystem
  touch_init();
}

void wait_user(void)
{
  touch_wait_released();
#if 0
  operation_requested = OP_NONE;
  while (true) {
    if (operation_requested & OP_TOUCH)
      break;
    if (operation_requested & OP_LEVER)
      break;
  }
#endif
}

int check_touched(void)
{
  int touched = false;
  if (touch_check() == EVT_TOUCH_RELEASED)
    touched = true;
  return touched;
}



#pragma GCC pop_options
