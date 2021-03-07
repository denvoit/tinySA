/* All rights reserved.
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

#include <math.h>
#include <string.h>
#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "nanovna.h"

#pragma GCC push_options
#pragma GCC optimize ("O2")      // Makes the code just a bit faster, disable during debugging.

#ifdef __SCROLL__
uint16_t _grid_y = (CHART_BOTTOM / NGRIDY);
uint16_t graph_bottom = CHART_BOTTOM;
#endif
static void cell_draw_marker_info(int x0, int y0);
static void cell_grid_line_info(int x0, int y0);
static void cell_blit_bitmap(int x, int y, uint16_t w, uint16_t h, const uint8_t *bitmap);
static void draw_battery_status(void);
static void update_waterfall(void);
void cell_draw_test_info(int x0, int y0);
#ifndef wFONT_GET_DATA
static void cell_drawstring_size(char *str, int x, int y, int size);
#endif
static int16_t grid_offset;
static int16_t grid_width;
static freq_t grid_span;

uint16_t area_width  = AREA_WIDTH_NORMAL;
uint16_t area_height; // initialized in main()  = AREA_HEIGHT_NORMAL;

// Cell render use spi buffer
pixel_t *cell_buffer = (pixel_t *)spi_buffer;

// Check buffer size
#if CELLWIDTH*CELLHEIGHT > SPI_BUFFER_SIZE
#error "Too small spi_buffer size SPI_BUFFER_SIZE < CELLWIDTH*CELLHEIGH"
#endif

// indicate dirty cells (not redraw if cell data not changed)
#define MAX_MARKMAP_X    ((LCD_WIDTH+CELLWIDTH-1)/CELLWIDTH)
#define MAX_MARKMAP_Y    ((LCD_HEIGHT+CELLHEIGHT-1)/CELLHEIGHT)
// Define markmap mask size
#if MAX_MARKMAP_X <= 8
typedef uint8_t map_t;
#elif MAX_MARKMAP_X <= 16
typedef uint16_t map_t;
#elif MAX_MARKMAP_X <= 32
typedef uint32_t map_t;
#endif

static map_t   markmap[2][MAX_MARKMAP_Y+1];
static uint8_t current_mappage = 0;

// Trace data cache, for faster redraw cells
// Set data size types depend from screen resolution
// Set type for x resolution
#if LCD_WIDTH < 256
typedef uint8_t   index_x_t;
#else
typedef uint16_t  index_x_t;
#endif
// Set type for y resolution
#if LCD_HEIGHT < 256
typedef uint8_t   index_y_t;
#else
typedef uint16_t  index_y_t;
#endif

static index_x_t trace_index_x[POINTS_COUNT];
static index_y_t trace_index_y[TRACES_MAX][POINTS_COUNT];

uint16_t marker_color(int mtype)
{
  if (mtype & M_REFERENCE)
    return LCD_M_REFERENCE;
  if (mtype & M_DELTA)
    return LCD_M_DELTA;
  if (mtype & M_NOISE)
    return LCD_M_NOISE;
  return LCD_M_DEFAULT;
}

//#if 4 != M_TRACKING
//#error "Wrong marker numbers"
//#endif

char marker_letter[5] =
{
 'R',
 ' ',
 'D',
 'N',
 'T'
};

#if 1
#define float2int(v) ((int)(v))
#else
static int 
float2int(float v) 
{
  if (v < 0) return v - 0.5;
  if (v > 0) return v + 0.5;
  return 0;
}
#endif

void update_grid(void)
{
  freq_t gdigit = 1000000000;
  freq_t fstart = get_sweep_frequency(ST_START);
  freq_t fspan  = get_sweep_frequency(ST_SPAN);
  freq_t grid;

  if (fspan == 0) {
    fspan = setting.actual_sweep_time_us; // Time in uS
    fstart = 0;
  }
  if (config.gridlines < 3)
    config.gridlines = 6;

  while (gdigit > 100) {
    grid = 5 * gdigit;
    if (fspan / grid >= config.gridlines)
      break;
    grid = 2 * gdigit;
    if (fspan / grid >= config.gridlines)
      break;
    grid = gdigit;
    if (fspan / grid >= config.gridlines)
      break;
    gdigit /= 10;
  }

  grid_span = grid;
  grid_offset = (WIDTH) * ((fstart % grid) / 100) / (fspan / 100);
  grid_width = (WIDTH) * (grid / 100) / (fspan / 1000);

  if (get_waterfall()){
    ili9341_set_background(LCD_BG_COLOR);
    ili9341_fill(OFFSETX, graph_bottom, LCD_WIDTH - OFFSETX, CHART_BOTTOM - graph_bottom);
  }
  redraw_request |= REDRAW_FREQUENCY | REDRAW_AREA;
}

#if 0
static int
rectangular_grid(int x, int y)
{
  //#define FREQ(x) (((x) * (fspan / 1000) / (WIDTH-1)) * 1000 + fstart)
  //int32_t n = FREQ(x-1) / fgrid;
  //int32_t m = FREQ(x) / fgrid;
  //if ((m - n) > 0)
  //if (((x * 6) % (WIDTH-1)) < 6)
  //if (((x - grid_offset) % grid_width) == 0)
  if (x == 0 || x == WIDTH-1)
    return 1;
  if ((y % GRIDY) == 0)
    return 1;
  if ((((x + grid_offset) * 10) % grid_width) < 10)
    return 1;
  return 0;
}
#endif

#ifdef __HAM_BAND__
typedef const struct {
  freq_t start;
  freq_t stop;
} ham_bands_t;

const ham_bands_t ham_bands[] =
{
  {135700, 137800},
  {472000, 479000},
  {1800000, 2000000},
  {3500000, 3800000},
  {5250000, 5450000},
  {7000000, 7200000},
  {10100000, 10150000},
  {14000000, 14350000},
  {18068000, 18168000},
  {21000000, 21450000},
  {24890000, 24990000},
  {28000000, 29700000},
  {50000000, 52000000},
  {70000000, 70500000},
  {144000000, 146000000}
};

int ham_band(int x)      // Search which index in the frequency tabled matches with frequency  f using actual_rbw
{
  if (!config.hambands)
    return false;
  freq_t f = frequencies[x];
  int L = 0;
  int R =  (sizeof ham_bands)/sizeof(freq_t) - 1;
  while (L <= R) {
    int m = (L + R) / 2;
    if (ham_bands[m].stop < f)
      L = m + 1;
    else if (ham_bands[m].start > f)
      R = m - 1;
    else
       return true; // index is m
  }
  return false;
}
#endif

static int
rectangular_grid_x(int x)
{
  x -= CELLOFFSETX;
  if (x < 0) return 0;
  if (x == 0 || x == WIDTH)
    return 1;
  if ((((x + grid_offset) * 10) % grid_width) < 10)
    return 1;
  return 0;
}

static int
rectangular_grid_y(int y)
{
  if (y < 0)
    return 0;
  if ((y % GRIDY) == 0)
    return 1;
  return 0;
}

#if 0
int
set_strut_grid(int x)
{
  uint16_t *buf = spi_buffer;
  int y;

  for (y = 0; y < HEIGHT; y++) {
    int c = rectangular_grid(x, y);
    c |= smith_grid(x, y);
    *buf++ = c;
  }
  return y;
}

void
draw_on_strut(int v0, int d, int color)
{
  int v;
  int v1 = v0 + d;
  if (v0 < 0) v0 = 0;
  if (v1 < 0) v1 = 0;
  if (v0 >= HEIGHT) v0 = HEIGHT-1;
  if (v1 >= HEIGHT) v1 = HEIGHT-1;
  if (v0 == v1) {
    v = v0; d = 2;
  } else if (v0 < v1) {
    v = v0; d = v1 - v0 + 1;
  } else {
    v = v1; d = v0 - v1 + 1;
  }
  while (d-- > 0)
    spi_buffer[v++] |= color;
}
#endif

/*
 * calculate log10f(abs(gamma))
 */ 
float
index_to_value(const int i)
{
  return(value(actual_t[i]));
}

// Function for convert to different type of values from dBm
// Replaced some equal functions and use recalculated constants:
// powf(10,x) =  expf(x * logf(10))
// log10f(x)  =  logf(x)/logf(10)
float
value(const float v)
{
  switch(setting.unit)
  {
  case U_DBMV:
    return v + (30.0 + 20.0*log10f(sqrtf(50.0)));
  case U_DBUV:
    return v + (90.0 + 20.0*log10f(sqrtf(50.0)));
  case U_VOLT:
//  return powf(10.0, (v-30.0)/20.0) * sqrtf(50.0);                     // powf(10.0, v           /20.0) * powf(10, -30.0/20.0) * sqrtf(50)
    return expf(v*(logf(10.0)/20.0)) * (powf(10, -30.0/20.0)*sqrtf(50));//       expf(v*logf(10.0)/20.0) * powf(10, -30.0/20.0) * sqrtf(50)
  case U_WATT:
//    return powf(10.0, v/10.0)/1000.0;               // powf(10.0, v           /10.0) / 1000.0
    return expf(v*(logf(10.0)/10.0)) / 1000.0;        //       expf(v*logf(10.0)/10.0) / 1000.0
  }
//  case U_DBM:
    return v;  // raw data is in logmag*10 format
}

float
to_dBm(const float v)
{
  switch(setting.unit)
  {
  case U_DBMV:
    return v - (30.0 - 20.0*log10f(sqrtf(50.0)));
  case U_DBUV:
    return v - (90.0 - 20.0*log10f(sqrtf(50.0)));
  case U_VOLT:
//  return log10f(v/(sqrtf(50.0)))* 20.0             + 30.0;
    return   logf(v/(sqrtf(50.0)))*(20.0/logf(10.0)) + 30.0;
  case U_WATT:
//  return log10f(v*1000.0)* 10.0;
    return   logf(v*1000.0)*(10.0/logf(10.0));
  }
//  case U_DBM:
    return v;  // raw data is in logmag*10 format
}

static void
trace_into_index_x_array(index_x_t *x, uint16_t points){
  // Not need update if index calculated for this points count
  static uint16_t old_points = 0;
  if (old_points == points) return;
  old_points = points;
  points-=1;
  for (int i=0; i<= points;i++)
    x[i] = (i * WIDTH + (points>>1)) / points + CELLOFFSETX;
}

//
// Optimized by speed/size array processing of value(const float v) function
// on screen need calculate as:
// y = (ref-v)/scale
// and align by top/bottom
static void
trace_into_index_y_array(index_y_t *y, float *array, int points)
{
  float scale = GRIDY / get_trace_scale();
  float ref   = get_trace_refpos();
  float mult = 0, vmult = 1.0;
  float ref_shift = 0;
  switch (setting.unit){
    case U_DBM: break;
    case U_DBMV: ref_shift = 30.0 - 20.0*log10f(sqrtf(50.0));break;
    case U_DBUV: ref_shift = 90.0 - 20.0*log10f(sqrtf(50.0));break;
    case U_VOLT: vmult = powf(10, -30.0/20.0) * sqrtf(50.0); mult = logf(10.0)/20.0;break;
    case U_WATT: vmult = 1.0/1000.0;                         mult = logf(10.0)/10.0;break;
    default:
    return;
  }
  // Universal formula look like this:
  // v = (refpos - (mult ? expf(value*mult) : value) - ref_shift) * vmult) * scale;
  // v = ((refpos - ref_shift) * scale) - (mult ? expf(value*mult) : value) * (vmult * scale)
  // Made precalculated constants:
  ref = (ref - ref_shift) * scale + 0.5; // add 0.5 for floor on int convert
  scale  = scale * vmult;
  int max = NGRIDY * GRIDY, i;
  for (i=0;i<points;i++){
    float value = array[i];
    if (mult) value = expf(value*mult);
    value = ref - value * scale;
    int v = value;
         if (v <   0) v = 0;
    else if (v > max) v = max;
    y[i] = v;
  }
  return;
}

void trace_get_value_string(     // Only used at one place
    int t, char *buf, int len,
    int i, float coeff[POINTS_COUNT],
    int ri, int mtype,
    freq_t i_freq, freq_t ref_freq)
{
  (void) t;
  float v;
  char buf2[16];
  char buf3[8];
  char *ptr2 = buf2;
  freq_t dfreq = 0;
  float rlevel = 0;
  int ii = i;
  int unit_index = setting.unit;
  if (mtype & M_DELTA) {
    *ptr2++ = S_DELTA[0];
    unit_index = setting.unit+5;
    if (ri > i) {
      dfreq = ref_freq - i_freq;
      ii = ri - i;
      *ptr2++ = '-';
    } else {
      dfreq = i_freq - ref_freq;
      ii = i - ri;
      *ptr2++ = '+';
    }
    rlevel = value(coeff[ri]);
  } else {
    dfreq = i_freq;
  }
  if (FREQ_IS_CW()) {
    float t = ii*(setting.actual_sweep_time_us)/(sweep_points - 1);
#if 1
    plot_printf(ptr2, sizeof(buf2) - 2, "%.3Fs" , t/ONE_SECOND_TIME);
#else
    if (t>ONE_SECOND_TIME){
      ptr2+=plot_printf(ptr2, sizeof(buf2) - 2, "%4f" , t/ONE_SECOND_TIME);
      *ptr2++= 'S';
      *ptr2++=0;
    }
    else if (t>1000.0) {
        ptr2+=plot_printf(ptr2, sizeof(buf2) - 2, "%4f" , t/ONE_MS_TIME);
        *ptr2++= 'm';
        *ptr2++= 'S';
        *ptr2++= 0;
    }
    else {
        ptr2+=plot_printf(&buf2[1], sizeof(buf2) -1, "%4f" , t);
        *ptr2++= 'u';
        *ptr2++= 'S';
        *ptr2++= 0;
    }
#endif
  } else {
#if 0
  freq_t resolution = get_sweep_frequency(ST_SPAN);
  if (resolution  <= 2000*POINTS_COUNT)
    plot_printf(ptr2, sizeof(buf2) - 2, "%3.3f" , (dfreq + 500) / 1000000.0);
  else if (resolution  <= 20000*POINTS_COUNT)
    plot_printf(ptr2, sizeof(buf2) - 2, "%3.2f" , (dfreq + 5000) / 1000000.0);
  else
    plot_printf(ptr2, sizeof(buf2) - 2, "%3.1f" , (dfreq + 50000) / 1000000.0);
  }
#else
    plot_printf(ptr2, sizeof(buf2) - 2, "%9.5QHz" , dfreq);
  }
#endif
    v = value(coeff[i]);
    if (mtype & M_NOISE){
//    v-= log10f(actual_rbw_x10*100.0) *  10.0;
      v-=   logf(actual_rbw_x10*100.0) * (10.0/logf(10.0));
    }
    if (v == -INFINITY)
      plot_printf(buf, len, "-INF");
    else {
      v = v - rlevel;
      if (UNIT_IS_LINEAR(setting.unit)) {
        plot_printf(buf3, sizeof(buf3), "%.3F", v); // 5 characters incl u,m,etc...
      } else {
        plot_printf(buf3, sizeof(buf3), "%.1f", v);
      }
      plot_printf(buf, len, "%s %s%s%s", buf2, buf3, unit_string[unit_index],(mtype & M_NOISE?"/Hz":""));
    }
}

static inline void
swap_markmap(void)
{
  current_mappage^= 1;
}

static void
clear_markmap(void)
{
  memset(markmap[current_mappage], 0, sizeof markmap[current_mappage]);
}

static void
force_set_markmap(void)
{
  memset(markmap[current_mappage], 0xff, sizeof markmap[current_mappage]);
}

/*
 * Force region of screen update
 */
static void
invalidate_rect_func(int x0, int y0, int x1, int y1)
{
  int x, y;
  map_t *map = &markmap[current_mappage][0];
  for (y = y0; y <= y1; y++)
    if ((uint32_t)y < MAX_MARKMAP_Y)
      for (x = x0; x <= x1; x++)
        map[y]|= 1 << x;
}
#define invalidate_rect(x0, y0, x1, y1) invalidate_rect_func((x0)/CELLWIDTH, (y0)/CELLHEIGHT, (x1)/CELLWIDTH, (y1)/CELLHEIGHT)

#define SWAP(x,y) {int t=x;x=y;y=t;}
static void
mark_cells_from_index(void)
{
  int t, i, j;
  /* mark cells between each neighber points */
  map_t *map = &markmap[current_mappage][0];
  index_x_t *index_x = trace_index_x;
  for (t = 0; t < TRACES_MAX; t++) {
    if (!trace[t].enabled)
      continue;
    index_y_t *index_y = trace_index_y[t];
    int m0 = index_x[0] / CELLWIDTH;
    int n0 = index_y[0] / CELLHEIGHT;
    map[n0] |= 1 << m0;
    for (i = 1; i < sweep_points; i++) {
      int m1 = index_x[i] / CELLWIDTH;
      int n1 = index_y[i] / CELLHEIGHT;
      if (m0 == m1 && n0 == n1)
        continue;
      int x0 = m0; int x1 = m1; if (x0>x1) SWAP(x0, x1); m0 = m1;
      int y0 = n0; int y1 = n1; if (y0>y1) SWAP(y0, y1); n0 = n1;
      for (; y0 <= y1; y0++)
        for (j = x0; j <= x1; j++)
          map[y0] |= 1 << j;
    }
  }
}

static inline void
markmap_upperarea(void)
{
  // Hardcoded, Text info from upper area
  invalidate_rect(0, 0, AREA_WIDTH_NORMAL, 31);
}

static uint16_t get_trigger_level(void){
  index_y_t trigger;
  trace_into_index_y_array(&trigger, &setting.trigger_level, 1);
  return trigger;
}

static inline void
markmap_trigger_area(void){
  uint16_t tp = get_trigger_level();
  markmap[current_mappage][tp/CELLWIDTH] = (map_t)0xFFFFFFFF;
}

//
// in most cases _compute_outcode clip calculation not give render line speedup
//
static inline void
cell_drawline(int x0, int y0, int x1, int y1, int c)
{
  if (x0 < 0 && x1 < 0) return;
  if (y0 < 0 && y1 < 0) return;
  if (x0 >= CELLWIDTH && x1 >= CELLWIDTH) return;
  if (y0 >= CELLHEIGHT && y1 >= CELLHEIGHT) return;

  // modifed Bresenham's line algorithm, see https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
  // Draw from top to bottom (most graph contain vertical lines)
  if (y1 < y0) { SWAP(x0, x1); SWAP(y0, y1); }
  int dx =-(x1 - x0), sx = 1; if (dx > 0) { dx = -dx; sx = -sx; }
  int dy = (y1 - y0);
  int err = ((dy + dx) < 0 ? -dx : -dy) / 2;
  // Fast skip points while y0 < 0
  if (y0 < 0){
    while(1){
      int e2 = err;
      if (e2 > dx) { err-= dy; x0+=sx;}
      if (e2 < dy) { err-= dx; y0++; if (y0 == 0) break;}
    }
  }
  // align y by CELLWIDTH for faster calculations
  y0*=CELLWIDTH;
  y1*=CELLWIDTH;
  while (1) {
    if ((uint32_t)x0 < CELLWIDTH)
      cell_buffer[y0 + x0]|= c;
    if (x0 == x1 && y0 == y1)
      return;
    int e2 = err;
    if (e2 > dx) { err-= dy; x0+=sx;}
    if (e2 < dy) { err-= dx; y0+=CELLWIDTH; if (y0>=CELLHEIGHT*CELLWIDTH) return;} // stop after cell bottom
  }
}

// Give a little speedup then draw rectangular plot (50 systick on all calls, all render req 700 systick)
// Write more difficult algoritm for seach indexes not give speedup
static int
search_index_range_x(int x1, int x2, int *i0, int *i1)
{
  int i, j;
  int head = 0;
  int tail = sweep_points;
  int idx_x;
  index_x_t *index_x = trace_index_x;
  // Search index point in cell
  while (1) {
    i = (head + tail) / 2;
    idx_x = index_x[i];
    if (idx_x >= x2) { // index after cell
      if (tail == i)
        return false;
      tail = i;
    }
    else if (idx_x < x1) {    // index before cell
      if (head == i)
        return false;
      head = i;
    }
    else  // index in cell (x =< idx_x < cell_end)
      break;
  }
  j = i;
  // Search index left from point
  do {
    j--;
  } while (j > 0 && x1 <= index_x[j]);
  *i0 = j;
  // Search index right from point
  do {
    i++;
  } while (i < sweep_points-1 && index_x[i] < x2);
  *i1 = i;

  return TRUE;
}
#if 0       // Not used as refpos is always at top of screen
#define REFERENCE_WIDTH    6
#define REFERENCE_HEIGHT   5
#define REFERENCE_X_OFFSET 5
#define REFERENCE_Y_OFFSET 2

// Reference bitmap
static const uint8_t reference_bitmap[]={
  _BMP8(0b11000000),
  _BMP8(0b11110000),
  _BMP8(0b11111100),
  _BMP8(0b11110000),
  _BMP8(0b11000000),
#endif

#if _MARKER_SIZE_ == 0
#define MARKER_WIDTH       7
#define MARKER_HEIGHT     10
#define X_MARKER_OFFSET    3
#define Y_MARKER_OFFSET   10
#define MARKER_BITMAP(i)  (&marker_bitmap[(i)*MARKER_HEIGHT])
static const uint8_t marker_bitmap[]={
// Marker Back plate
  _BMP8(0b11111110),
  _BMP8(0b11111110),
  _BMP8(0b11111110),
  _BMP8(0b11111110),
  _BMP8(0b11111110),
  _BMP8(0b11111110),
  _BMP8(0b11111110),
  _BMP8(0b01111100),
  _BMP8(0b00111000),
  _BMP8(0b00010000),
  // Marker 1
  _BMP8(0b00000000),
  _BMP8(0b00010000),
  _BMP8(0b00110000),
  _BMP8(0b00010000),
  _BMP8(0b00010000),
  _BMP8(0b00010000),
  _BMP8(0b00111000),
  _BMP8(0b00000000),
  _BMP8(0b00000000),
  _BMP8(0b00000000),
  // Marker 2
  _BMP8(0b00000000),
  _BMP8(0b00111000),
  _BMP8(0b01000100),
  _BMP8(0b00000100),
  _BMP8(0b00111000),
  _BMP8(0b01000000),
  _BMP8(0b01111100),
  _BMP8(0b00000000),
  _BMP8(0b00000000),
  _BMP8(0b00000000),
  // Marker 3
  _BMP8(0b00000000),
  _BMP8(0b00111000),
  _BMP8(0b01000100),
  _BMP8(0b00011000),
  _BMP8(0b00000100),
  _BMP8(0b01000100),
  _BMP8(0b00111000),
  _BMP8(0b00000000),
  _BMP8(0b00000000),
  _BMP8(0b00000000),
  // Marker 4
  _BMP8(0b00000000),
  _BMP8(0b00001000),
  _BMP8(0b00011000),
  _BMP8(0b00101000),
  _BMP8(0b01001000),
  _BMP8(0b01001000),
  _BMP8(0b01111100),
  _BMP8(0b00001000),
  _BMP8(0b00000000),
  _BMP8(0b00000000),
};

#elif _MARKER_SIZE_ == 1
#define MARKER_WIDTH       10
#define MARKER_HEIGHT      13
#define X_MARKER_OFFSET     4
#define Y_MARKER_OFFSET    13
#define MARKER_BITMAP(i)   (&marker_bitmap[(i)*2*MARKER_HEIGHT])
static const uint8_t marker_bitmap[]={
  // Marker Back plate
  _BMP16(0b1111111110000000),
  _BMP16(0b1111111110000000),
  _BMP16(0b1111111110000000),
  _BMP16(0b1111111110000000),
  _BMP16(0b1111111110000000),
  _BMP16(0b1111111110000000),
  _BMP16(0b1111111110000000),
  _BMP16(0b1111111110000000),
  _BMP16(0b1111111110000000),
  _BMP16(0b0111111100000000),
  _BMP16(0b0011111000000000),
  _BMP16(0b0001110000000000),
  _BMP16(0b0000100000000000),
  // Marker 1
  _BMP16(0b0000000000000000),
  _BMP16(0b0000110000000000),
  _BMP16(0b0001110000000000),
  _BMP16(0b0010110000000000),
  _BMP16(0b0000110000000000),
  _BMP16(0b0000110000000000),
  _BMP16(0b0000110000000000),
  _BMP16(0b0000110000000000),
  _BMP16(0b0000110000000000),
  _BMP16(0b0001111000000000),
  _BMP16(0b0000000000000000),
  _BMP16(0b0000000000000000),
  _BMP16(0b0000000000000000),
  // Marker 2
  _BMP16(0b0000000000000000),
  _BMP16(0b0001111000000000),
  _BMP16(0b0011001100000000),
  _BMP16(0b0011001100000000),
  _BMP16(0b0000011000000000),
  _BMP16(0b0000110000000000),
  _BMP16(0b0001100000000000),
  _BMP16(0b0011000000000000),
  _BMP16(0b0011111100000000),
  _BMP16(0b0000000000000000),
  _BMP16(0b0000000000000000),
  _BMP16(0b0000000000000000),
  _BMP16(0b0000000000000000),
  // Marker 3
  _BMP16(0b0000000000000000),
  _BMP16(0b0011111000000000),
  _BMP16(0b0110001100000000),
  _BMP16(0b0110001100000000),
  _BMP16(0b0000001100000000),
  _BMP16(0b0000111000000000),
  _BMP16(0b0000001100000000),
  _BMP16(0b0110001100000000),
  _BMP16(0b0110001100000000),
  _BMP16(0b0011111000000000),
  _BMP16(0b0000000000000000),
  _BMP16(0b0000000000000000),
  _BMP16(0b0000000000000000),
  // Marker 4
  _BMP16(0b0000000000000000),
  _BMP16(0b0000011000000000),
  _BMP16(0b0000111000000000),
  _BMP16(0b0001111000000000),
  _BMP16(0b0011011000000000),
  _BMP16(0b0110011000000000),
  _BMP16(0b0110011000000000),
  _BMP16(0b0111111100000000),
  _BMP16(0b0000011000000000),
  _BMP16(0b0000011000000000),
  _BMP16(0b0000000000000000),
  _BMP16(0b0000000000000000),
  _BMP16(0b0000000000000000),
};
#endif

static void
markmap_marker(int marker)
{
  if (!markers[marker].enabled)
    return;
  if (!trace[TRACE_ACTUAL].enabled)
    return;
  int idx = markers[marker].index;
  int x = trace_index_x[idx] - X_MARKER_OFFSET;
  int y = trace_index_y[TRACE_ACTUAL][idx] - Y_MARKER_OFFSET;
  invalidate_rect(x, y, x+MARKER_WIDTH-1, y+MARKER_HEIGHT-1);
}

void
markmap_all_markers(void)
{
  int i;
  for (i = 0; i < MARKERS_MAX; i++) {
    if (!markers[i].enabled)
      continue;
    markmap_marker(i);
  }
  markmap_upperarea();
}

int
distance_to_index(int8_t t, uint16_t idx, int16_t x, int16_t y)
{
  x-= trace_index_x[idx];
  y-= trace_index_y[t][idx];
  return x*x + y*y;
}

int
search_nearest_index(int x, int y, int t)
{
  int min_i = -1;
  int min_d = MARKER_PICKUP_DISTANCE * MARKER_PICKUP_DISTANCE;
  int i;
  for (i = 0; i < sweep_points; i++) {
    int d = distance_to_index(t, i, x , y);
    if (d < min_d) {
      min_d = d;
      min_i = i;
    }
  }
  return min_i;
}

void
plot_into_index(measurement_t measured)
{
  int t;
//  START_PROFILE
  trace_into_index_x_array(trace_index_x, sweep_points);
  for (t = 0; t < TRACES_MAX; t++) {
    if (!trace[t].enabled)
      continue;
    trace_into_index_y_array(trace_index_y[t], measured[t], sweep_points);
  }
//  STOP_PROFILE
//  START_PROFILE
  mark_cells_from_index();
  markmap_all_markers();
//  STOP_PROFILE
}

static void
draw_cell(int m, int n)
{
  int x0 = m * CELLWIDTH;
  int y0 = n * CELLHEIGHT;
  int w = CELLWIDTH;
  int h = CELLHEIGHT;
  int x, y;
  int i;
  int t;
  uint16_t c;
  // Clip cell by area
  if (w > area_width - x0)
    w = area_width - x0;
  if (h > area_height - y0)
    h = area_height - y0;
  if (w <= 0 || h <= 0)
    return;
//  PULSE;
  cell_buffer = ili9341_get_cell_buffer();
  // Clear buffer ("0 : height" lines)
#if 0
  // use memset 350 system ticks for all screen calls
  // as understand it use 8 bit set, slow down on 32 bit systems
  memset(spi_buffer, LCD_BG_COLOR, (h*CELLWIDTH)*sizeof(uint16_t));
#else
  // use direct set  35 system ticks for all screen calls
#if CELLWIDTH%8 != 0
#error "CELLWIDTH % 8 should be == 0 for speed, or need rewrite cell cleanup"
#endif
  // Set LCD_BG_COLOR for 8 pixels in one cycle
  int count = h*CELLWIDTH / 8;
  uint32_t *p = (uint32_t *)cell_buffer;
  uint32_t clr = GET_PALTETTE_COLOR(LCD_BG_COLOR) | (GET_PALTETTE_COLOR(LCD_BG_COLOR) << 16);
  while (count--) {
    p[0] = clr;
    p[1] = clr;
    p[2] = clr;
    p[3] = clr;
    p += 4;
  }
#endif

// Draw grid
#if 1
  c = GET_PALTETTE_COLOR(LCD_GRID_COLOR);
  // Generate grid type list
  uint32_t trace_type = 0;
  for (t = 0; t < TRACES_MAX; t++) {
    if (trace[t].enabled) {
      trace_type |= RECTANGULAR_GRID_MASK;
    }
  }
  // Draw rectangular plot (40 system ticks for all screen calls)
  if (trace_type & RECTANGULAR_GRID_MASK) {
    for (x = 0; x < w; x++) {
#ifdef __HAM_BAND__
      if (ham_band(x+x0)) {
        for (y = 0; y < h; y++) cell_buffer[y * CELLWIDTH + x] = GET_PALTETTE_COLOR(LCD_HAM_COLOR);
      }
#endif
      if (rectangular_grid_x(x + x0)) {
        for (y = 0; y < h; y++) cell_buffer[y * CELLWIDTH + x] = c;
      }
    }
    for (y = 0; y < h; y++) {
      if (rectangular_grid_y(y + y0)) {
        for (x = 0; x < w; x++)
          if ((uint32_t)(x + x0 - CELLOFFSETX) <= WIDTH)
            cell_buffer[y * CELLWIDTH + x] = c;
      }
    }
  }

//  PULSE;
#endif
// Draw trigger line
  if (setting.trigger != T_AUTO) {
	c = GET_PALTETTE_COLOR(LCD_TRIGGER_COLOR);
    int tp = get_trigger_level() - y0;
    if (tp>=0 && tp < h)
      for (x = 0; x < w; x++)
        if ((uint32_t)(x + x0 - CELLOFFSETX) <= WIDTH + CELLOFFSETX)
          cell_buffer[tp * CELLWIDTH + x] = c;
  }

#if 1
  // Only right cells
  if (m >= (GRID_X_TEXT)/CELLWIDTH)
    cell_grid_line_info(x0, y0);
#endif

// Draw traces (50-600 system ticks for all screen calls, depend from lines
// count and size)
#if 1
  int i0 = 0;
  int i1 = 0;
  index_x_t *index_x = trace_index_x;
  search_index_range_x(x0, x0 + w, &i0, &i1);
  for (t = 0; t < TRACES_MAX; t++) {
    if (!trace[t].enabled)
      continue;
    c = GET_PALTETTE_COLOR(LCD_TRACE_1_COLOR + t);
    index_y_t *index_y = trace_index_y[t];
    for (i = i0; i < i1; i++) {
      int x1 = index_x[i  ] - x0;
      int x2 = index_x[i+1] - x0;
      int y1 = index_y[i  ] - y0;
      int y2 = index_y[i+1] - y0;
      cell_drawline(x1, y1, x2, y2, c);
    }
  }
#else
  for (x = 0; x < area_width; x += 6)
    cell_drawline(x - x0, 0 - y0, area_width - x - x0, area_height - y0,
                  config.trace_color[0]);
#endif
//  PULSE;
// draw marker symbols on each trace (<10 system ticks for all screen calls)
#if 1
  if (trace[TRACE_ACTUAL].enabled) {
    for (i = 0; i < MARKERS_MAX; i++) {
      if (!markers[i].enabled)
        continue;
      int idx = markers[i].index;
      int x = trace_index_x[idx] - x0 - X_MARKER_OFFSET;
      int y = trace_index_y[TRACE_ACTUAL][idx] - y0 - Y_MARKER_OFFSET;
      // Check marker icon on cell
      if ((uint32_t)(x+MARKER_WIDTH ) < (CELLWIDTH  + MARKER_WIDTH ) &&
          (uint32_t)(y+MARKER_HEIGHT) < (CELLHEIGHT + MARKER_HEIGHT)){
          // Draw marker plate
          ili9341_set_foreground(LCD_TRACE_1_COLOR + TRACE_ACTUAL);
          cell_blit_bitmap(x, y, MARKER_WIDTH, MARKER_HEIGHT, MARKER_BITMAP(0));
          // Draw marker number
          ili9341_set_foreground(LCD_BG_COLOR);
          cell_blit_bitmap(x, y, MARKER_WIDTH, MARKER_HEIGHT, MARKER_BITMAP(i+1));
      }
    }
  }
#endif
// Draw trace and marker info on the top (50 system ticks for all screen calls)
#if 1
  if (n == 0)
    cell_draw_marker_info(x0, y0);
#endif
#ifdef __SELFTEST__
  cell_draw_test_info(x0, y0);
#endif
  //  PULSE;
#if 0
// Draw reference position (<10 system ticks for all screen calls)
  for (t = 0; t < TRACES_MAX; t++) {
    if (!trace[t].enabled)
      continue;
    uint32_t trace_type = (1 << trace[t].type);
    if (trace_type & ((1 << TRC_SMITH) | (1 << TRC_POLAR)))
      continue;
    int x = 0 - x0 + CELLOFFSETX - REFERENCE_X_OFFSET;
    if (x + REFERENCE_WIDTH >= 0 && x - REFERENCE_WIDTH < CELLWIDTH) {
      int y = HEIGHT - float2int((get_trace_refpos(t) * GRIDY)) - y0 - REFERENCE_Y_OFFSET;
      if (y + REFERENCE_HEIGHT >= 0 && y - REFERENCE_HEIGHT < CELLHEIGHT){
        ili9341_set_foreground(GET_PALTETTE_COLOR(LCD_TRACE_1_COLOR + t));
        cell_blit_bitmap(x , y, REFERENCE_WIDTH, REFERENCE_HEIGHT, reference_bitmap);
      }
    }
  }
#endif
// Need right clip cell render (25 system ticks for all screen calls)
#if 1
  if (w < CELLWIDTH) {
    pixel_t *src = cell_buffer + CELLWIDTH;
    pixel_t *dst = cell_buffer + w;
    for (y = h; --y; src += CELLWIDTH - w)
      for (x = w; x--;)
        *dst++ = *src++;
  }
#endif
  // Draw cell (500 system ticks for all screen calls)
  ili9341_bulk_continue(OFFSETX + x0, OFFSETY + y0, w, h);
}

static void
draw_all_cells(bool flush_markmap)
{
  int m, n;
//  START_PROFILE
  for (m = 0; m < (area_width+CELLWIDTH-1) / CELLWIDTH; m++)
    for (n = 0; n < (area_height+CELLHEIGHT-1) / CELLHEIGHT; n++)
      if ((markmap[0][n] | markmap[1][n]) & (1 << m))
        draw_cell(m, n);
#if 0
  // Used for debug control cell update
  ili9341_bulk_finish();
  for (m = 0; m < (area_width+CELLWIDTH-1) / CELLWIDTH; m++)
    for (n = 0; n < (area_height+CELLHEIGHT-1) / CELLHEIGHT; n++) {
      ili9341_set_background(((markmap[0][n] | markmap[1][n]) & (1 << m)) ? LCD_LOW_BAT_COLOR : LCD_NORMAL_BAT_COLOR);
      ili9341_fill(m*CELLWIDTH+OFFSETX, n*CELLHEIGHT, 2, 2);
    }
#endif
  if (flush_markmap) {
    // keep current map for update
    swap_markmap();
    // clear map for next plotting
    clear_markmap();
  }
  // Flush LCD buffer, wait completion (need call after end use ili9341_bulk_continue mode)
  ili9341_bulk_finish();
//  STOP_PROFILE
}

void
draw_all(bool flush)
{
  if (redraw_request & REDRAW_AREA)       // this set all area for update
    force_set_markmap();
  else {
    if (redraw_request & REDRAW_MARKER)   // update marker area
      markmap_upperarea();
    if (redraw_request & REDRAW_TRIGGER)  // update trigger area
      markmap_trigger_area();
  }
  if (redraw_request & (REDRAW_CELLS | REDRAW_MARKER | REDRAW_AREA | REDRAW_TRIGGER)){
    draw_all_cells(flush);
#ifdef __SCROLL__
  //  START_PROFILE
  if (setting.waterfall)
    update_waterfall();
  //  STOP_PROFILE
#endif
  }
  if (redraw_request & REDRAW_CAL_STATUS)
    draw_cal_status();                      // calculates the actual sweep time, must be before draw_frequencies
  if (redraw_request & REDRAW_FREQUENCY)
    draw_frequencies();
  if (redraw_request & REDRAW_BATTERY)
    draw_battery_status();
  redraw_request = 0;
}

//
// Call this function then need fast draw marker and marker info
// Used in ui.c for leveler move marker, drag marker and etc.
void
redraw_marker(int marker)
{
  if (marker < 0)
    return;
  // mark map on new position of marker
  markmap_marker(marker);

  // mark cells on marker info
  markmap_upperarea();

  draw_all_cells(TRUE);
  // Force redraw all area after (disable artifacts after fast marker update area)
  redraw_request|=REDRAW_AREA;
}

#if 0  // Not used
void
request_to_draw_cells_behind_menu(void)
{
  // Values Hardcoded from ui.c
  if (current_menu_is_form())
    invalidate_rect(0, 0, LCD_WIDTH-1, LCD_HEIGHT-1);
  else
    invalidate_rect(LCD_WIDTH-MENU_BUTTON_WIDTH-OFFSETX, 0, LCD_WIDTH-OFFSETX, LCD_HEIGHT-1);
  redraw_request |= REDRAW_CELLS;
}

void
request_to_draw_cells_behind_numeric_input(void)
{
  // Values Hardcoded from ui.c
  invalidate_rect(0, LCD_HEIGHT-NUM_INPUT_HEIGHT, LCD_WIDTH-1, LCD_HEIGHT-1);
  redraw_request |= REDRAW_CELLS;
}
#endif

static void
cell_blit_bitmap(int x, int y, uint16_t w, uint16_t h, const uint8_t *bmp)
{
  if (x <= -w)
    return;
  uint8_t bits = 0;
  int c = h+y, r;
  for (; y < c; y++) {
    for (r = 0; r < w; r++, bits<<=1) {
      if ((r&7)==0) bits = *bmp++;
      if ((0x80 & bits) == 0) continue;    // no pixel
      if ((uint32_t)(y+0) >= CELLHEIGHT) continue; // y   < 0 || y   >= CELLHEIGHT
      if ((uint32_t)(x+r) >= CELLWIDTH ) continue; // x+r < 0 || x+r >= CELLWIDTH
      cell_buffer[y*CELLWIDTH + x + r] = foreground_color;
    }
  }
}

void
cell_drawstring(char *str, int x, int y)
{
  if ((uint32_t)(y+FONT_GET_HEIGHT) >= CELLHEIGHT + FONT_GET_HEIGHT)
    return;
  while (*str) {
    if (x >= CELLWIDTH)
      return;
    uint16_t ch = *str++;
    uint16_t w = FONT_GET_WIDTH(ch);
    cell_blit_bitmap(x, y, w, FONT_GET_HEIGHT, FONT_GET_DATA(ch));
    x += w;
  }
}

static void
cell_drawstring_7x13(char *str, int x, int y)
{
  if ((uint32_t)(y+bFONT_GET_HEIGHT) >= CELLHEIGHT + bFONT_GET_HEIGHT)
    return;
  while (*str) {
    if (x >= CELLWIDTH)
      return;
    uint8_t ch = *str++;
    uint16_t w = bFONT_GET_WIDTH(ch);
    cell_blit_bitmap(x, y, w, bFONT_GET_HEIGHT, bFONT_GET_DATA(ch));
    x += w;
  }
}

void
cell_drawstring_10x14(char *str, int x, int y)
{
#ifdef wFONT_GET_DATA
  if ((uint32_t)(y+wFONT_GET_HEIGHT) >= CELLHEIGHT + wFONT_GET_HEIGHT)
    return;
  while (*str) {
    if (x >= CELLWIDTH)
      return;
    uint8_t ch = *str++;
    uint16_t w = wFONT_GET_WIDTH(ch);
    cell_blit_bitmap(x, y, w <=8 ? 9 : w, wFONT_GET_HEIGHT, wFONT_GET_DATA(ch));
    x+=w;
  }
#else
  cell_drawstring_size(str, x, y, 2);
#endif
}

#ifndef wFONT_GET_DATA
static int
cell_drawchar_size(uint8_t ch, int x, int y, int size)
{
  uint8_t bits;
  int c, r, ch_size;
  const uint8_t *char_buf = FONT_GET_DATA(ch);
  ch_size = FONT_GET_WIDTH(ch);
  //  if (y <= -FONT_GET_HEIGHT || y >= CELLHEIGHT || x <= -ch_size || x >= CELLWIDTH)
  //    return ch_size;
  if (x <= -ch_size*size)
    return ch_size*size;
  for (c = 0; c < FONT_GET_HEIGHT; c++) {
    for (int i=0; i < size; i++) {
      bits = *char_buf;
      if ((y + c*size+i) < 0 || (y + c*size+i) >= CELLHEIGHT)
        continue;
      for (r = 0; r < ch_size; r++) {
        for (int j = 0; j < size; j++) {
          if ((x+r*size + j) >= 0 && (x+r*size+j) < CELLWIDTH && (0x80 & bits))
            cell_buffer[(y+c*size+i)*CELLWIDTH + (x+r*size+j)] = foreground_color;
        }
        bits <<= 1;
      }
    }
    char_buf++;
  }
  return ch_size*size;
}

void
cell_drawstring_size(char *str, int x, int y, int size)
{
  if (y <= -FONT_GET_HEIGHT*2 || y >= CELLHEIGHT)
    return;
  while (*str) {
    if (x >= CELLWIDTH)
      return;
    x += cell_drawchar_size(*str++, x, y, size);
  }
}
#endif

extern float temppeakLevel;

static void cell_grid_line_info(int x0, int y0)
{
  char buf[32];
  int xpos = GRID_X_TEXT - x0;
  int ypos = 0 - y0 + 2;
  ili9341_set_foreground(LCD_GRID_VALUE_COLOR);
  float   ref = get_trace_refpos();
  float scale = get_trace_scale();
  for (int i = 0; i < NGRIDY; i++){
    if (ypos >= CELLHEIGHT) break;
    if (ypos >= -FONT_GET_HEIGHT){
      plot_printf(buf, sizeof buf, "% 7.3F", ref);
      cell_drawstring(buf, xpos, ypos);
    }
    ypos+=GRIDY;
    ref-=scale;
  }
}

static void cell_draw_marker_info(int x0, int y0)
{
  char buf[32];
  int t;
  int ref_marker = 0;
  int j = 0;
//  int count = 0;
  int active=0;
  for (int i = 0; i < MARKER_COUNT; i++) {
    if (markers[i].enabled) {
      if (markers[i].mtype & M_REFERENCE) {
        ref_marker = i;
      }
      active++;
    }
  }
  if (setting.measurement == M_THD && active >= 1)
    active = 2;
  for (int i = 0; i < MARKER_COUNT; i++) {
    if (i == 3) {
      if (setting.measurement == M_PASS_BAND) {
        freq_t f;
        if (markers[2].frequency>markers[1].frequency)
          f = markers[2].frequency-markers[1].frequency;
        else
          f = markers[1].frequency-markers[2].frequency;
        plot_printf(buf, sizeof buf, "WIDTH: %8.3QHz", f);
    show_computed:
        j = 3;
        int xpos = 1 + (j%2)*(WIDTH/2) + CELLOFFSETX - x0;
        int ypos = 1 + (j/2)*(16) - y0;
        cell_drawstring_7x13(buf, xpos, ypos);
        //        cell_drawstring(buf, xpos, ypos);
      } else if (setting.measurement == M_AM){
#ifdef AM_IN_VOLT
        int old_unit = setting.unit;
        setting.unit = U_VOLT;
        float level = (index_to_value(markers[1].index) + index_to_value(markers[2].index))/2 / index_to_value(markers[0].index);
        setting.unit = old_unit;
        int depth = (int)( level * 2.0 * 80.0) + 20;
#else
        float delta = actual_t[markers[1].index] - actual_t[markers[2].index];
        if (delta < -5 || delta > 5)
          break;
        float level = (actual_t[markers[1].index] + actual_t[markers[2].index])/2.0 -  actual_t[markers[0].index];
        if (level < -70 || level > 0)
          break;
//      int depth = powf(10.0, 2.0 + (level + 6.02)             /20.0 );
        int depth =      expf((2.0 + (level + 6.02))*(logf(10.0)/20.0));
#endif
        plot_printf(buf, sizeof buf, "DEPTH: %3d%%", depth);
        goto show_computed;
      } else if (setting.measurement == M_FM){
        freq_t dev = markers[1].frequency + actual_rbw_x10 * 100;      // Temp value to prevent calculation of negative deviation
        if ( markers[2].frequency < dev)
          break;
        dev = ( markers[2].frequency - dev ) >> 1;
        plot_printf(buf, sizeof buf, "DEVIATION:%6.1QHz", dev);
        goto show_computed;
      } else if (setting.measurement == M_THD && markers[0].enabled && (markers[0].index << 5) > sweep_points ) {
        int old_unit = setting.unit;
        setting.unit = U_WATT;
        float p = index_to_value(markers[0].index);
        int h_i = 2;
        freq_t f = markers[0].frequency;
        float h = 0.0;
        while (f * h_i < frequencies[sweep_points-1]) {
          if (search_maximum(1, f*h_i, 4*h_i) )             // use marker 1 for searching harmonics
            h += index_to_value(markers[1].index);
          h_i++;
        }
        float thd = 100.0 * sqrtf(h/p);
        setting.unit = old_unit;
        ili9341_set_foreground(marker_color(markers[0].mtype));
        plot_printf(buf, sizeof buf, "THD: %4.1f%%", thd);
//        j = 1;
        int xpos = 1 + (j%2)*(WIDTH/2) + CELLOFFSETX - x0;
        int ypos = 1 + (j/2)*(16) - y0;
        cell_drawstring_7x13(buf, xpos, ypos);
//        cell_drawstring(buf, xpos, ypos);
        break;
      }
    } else
    if (i >= 2 && setting.measurement == M_OIP3 && markers[2].enabled && markers[3].enabled) {
      float il = index_to_value(markers[2].index);
      float ir = index_to_value(markers[3].index);
      float sl = index_to_value(markers[0].index);
      float sr = index_to_value(markers[1].index);

      float ip = sl+ (sr - il)/2;
      plot_printf(buf, sizeof buf, "OIP3: %4.1fdB", ip);
      j = 2;
      int xpos = 1 + (j%2)*(WIDTH/2) + CELLOFFSETX - x0;
      int ypos = 1 + (j/2)*(16) - y0;
//      cell_drawstring_7x13(buf, xpos, ypos);
      cell_drawstring(buf, xpos, ypos);

      ip = sr+ (sl - ir)/2;
      plot_printf(buf, sizeof buf, "OIP3: %4.1fdB", ip);
      j = 3;
      xpos = 1 + (j%2)*(WIDTH/2) + CELLOFFSETX - x0;
      ypos = 1 + (j/2)*(16) - y0;
//      cell_drawstring_7x13(buf, xpos, ypos);
      cell_drawstring(buf, xpos, ypos);
      break;
    }
#if 0
    if (i >= 2 && in_selftest) {
      plot_printf(buf, sizeof buf, "DO NOT SWITCH OFF!!");
      j = 2;
      int xpos = 1 + CELLOFFSETX +25 - x0;
      int ypos = 1 + 16 - y0;

      cell_drawstring_7x13(buf, xpos, ypos);
      break;
    }
#endif
    if (!markers[i].enabled)
      continue;
    int idx = markers[i].index;
    int ridx = markers[ref_marker].index;
    for (t = TRACE_ACTUAL; t <= TRACE_ACTUAL; t++) { // Only show info on actual trace
      if (!trace[t].enabled)
        continue;
      int k = 0;
      if (i == active_marker) {
//        ili9341_set_foreground(LCD_BG_COLOR);
//        ili9341_set_background(marker_color(markers[i].mtype));
        buf[k++] = S_SARROW[0];
      } else {
//        ili9341_set_background(LCD_BG_COLOR);
//        ili9341_set_foreground(marker_color(markers[i].mtype));
        buf[k++] = ' ';
//        buf[k++] = ' ';
      }
      buf[k++] = i+'1';
      if (markers[i].mtype & M_REFERENCE)
        buf[k++] = 'R';
      if (markers[i].mtype & M_TRACKING)
        buf[k++] = 'T';
      if (markers[i].mtype & M_DELTA)
        buf[k++] = 'D';
      if (markers[i].mtype & M_NOISE)
        buf[k++] = 'N';
      buf[k++] = ' ';
//      buf[k++] = 0;
      ili9341_set_background(LCD_BG_COLOR);
      uint16_t color;
      if ((!setting.subtract_stored) &&     // Disabled when normalized
          ((setting.mode == M_LOW && temppeakLevel - get_attenuation() + setting.offset > -10) ||
           (setting.mode == M_HIGH && temppeakLevel - get_attenuation()+ setting.offset > -29) ))
        color = LCD_BRIGHT_COLOR_RED;
      else
        color = marker_color(markers[i].mtype);
      ili9341_set_foreground(color);
//      if (setting.unit)
//        cell_drawstring(buf, xpos, ypos);
//      else
//        cell_drawstring_7x13(buf, xpos, ypos);
      trace_get_value_string(
          t, &buf[k], (sizeof buf) - k,
          idx, measured[t], ridx, markers[i].mtype,markers[i].frequency, markers[ref_marker].frequency);
#if 1
      int xpos = 1 + (j%2)*(WIDTH/2) + CELLOFFSETX - x0;
//      int ypos = 1 + (j/2)*(13) - y0;
      int ypos = 1 + (j/2)*(16) - y0;
#else
      int xpos = 1 + CELLOFFSETX - x0;
      int ypos = 1 + j*(FONT_GET_HEIGHT*2+1) - y0;
#endif
      if (/* strlen(buf)*7> WIDTH/2 && */active > 1)
        cell_drawstring(buf, xpos, ypos);
      else
        cell_drawstring_7x13(buf, xpos, ypos);
      j++;
   }
  }
}

void
draw_frequencies(void)
{
  char buf1[32];
  char buf2[32];
  if (MODE_OUTPUT(setting.mode))     // No frequencies during output
    return;
  if (current_menu_is_form() && !in_selftest)
    return;

  if (FREQ_IS_CW()) {
    plot_printf(buf1, sizeof(buf1), " CW %QHz", get_sweep_frequency(ST_CW));
    // Show user actual select sweep time?
    uint32_t t = setting.actual_sweep_time_us;
    plot_printf(buf2, sizeof(buf2), " TIME %.3Fs", (float)t/ONE_SECOND_TIME);

  } else if (FREQ_IS_STARTSTOP()) {
    plot_printf(buf1, sizeof(buf1), " START %.3QHz    %5.1QHz/", get_sweep_frequency(ST_START), grid_span);
    plot_printf(buf2, sizeof(buf2), " STOP %.3QHz", get_sweep_frequency(ST_STOP));
  } else if (FREQ_IS_CENTERSPAN()) {
    plot_printf(buf1, sizeof(buf1), " CENTER %.3QHz    %5.1QHz/", get_sweep_frequency(ST_CENTER), grid_span);
    plot_printf(buf2, sizeof(buf2), " SPAN %.3QHz", get_sweep_frequency(ST_SPAN));
  }
  ili9341_set_foreground(LCD_FG_COLOR);
  ili9341_set_background(LCD_BG_COLOR);
  ili9341_fill(FREQUENCIES_XPOS1, FREQUENCIES_YPOS, LCD_WIDTH - FREQUENCIES_XPOS1, LCD_HEIGHT - FREQUENCIES_YPOS);
  if (uistat.lever_mode == LM_CENTER)
    buf1[0] = S_SARROW[0];
  if (uistat.lever_mode == LM_SPAN)
    buf2[0] = S_SARROW[0];
//  int p2 = FREQUENCIES_XPOS2;
//  if (FREQ_IS_CW()) {
    int p2 = LCD_WIDTH - FONT_WIDTH*strlen(buf2);
//  }
  ili9341_drawstring(buf2, p2, FREQUENCIES_YPOS);
  ili9341_drawstring(buf1, FREQUENCIES_XPOS1, FREQUENCIES_YPOS);
}

// Draw battery level
#define BATTERY_TOP_LEVEL       4100
#define BATTERY_BOTTOM_LEVEL    3200
#define BATTERY_WARNING_LEVEL   3300

static void draw_battery_status(void)
{
  int16_t vbat = adc_vbat_read();
  if (vbat <= 0)
    return;
  uint8_t string_buf[16];
  // Set battery color
  ili9341_set_foreground(vbat < BATTERY_WARNING_LEVEL ? LCD_LOW_BAT_COLOR : LCD_NORMAL_BAT_COLOR);
  ili9341_set_background(LCD_BG_COLOR);

  // Prepare battery bitmap image
  // Battery top
  int x = 0;
  string_buf[x++] = 0b00000000;
  string_buf[x++] = 0b00111100;
  string_buf[x++] = 0b00111100;
  string_buf[x++] = 0b11111111;
  // Fill battery status
  for (int power=BATTERY_TOP_LEVEL; power > BATTERY_BOTTOM_LEVEL; ){
    if ((x&3) == 0) {string_buf[x++] = 0b10000001; continue;}
    string_buf[x++] = (power > vbat) ? 0b10000001 : // Empty line
                                       0b10111101;  // Full line
    power-=100;
  }
  // Battery bottom
  string_buf[x++] = 0b10000001;
  string_buf[x++] = 0b11111111;
  // Draw battery
  ili9341_blitBitmap(7, BATTERY_START, 8, x, string_buf);
  plot_printf((char*)string_buf, sizeof string_buf, "%.2fv", vbat/1000.0);
  ili9341_drawstring((char*)string_buf, 1, BATTERY_START+x+3);
}

void
request_to_redraw_grid(void)
{
  redraw_request |= REDRAW_AREA;
}

void
redraw_frame(void)
{
  ili9341_set_background(LCD_BG_COLOR);
  ili9341_clear_screen();
  draw_frequencies();
  draw_cal_status();
}

int display_test(void)
{
//  return true;
  // write and read display, return false on fail.
  for (int h = 0; h < LCD_HEIGHT; h++) {
      for (int w = 0; w < LCD_WIDTH; w++) {
        spi_buffer[w] =  ((w*h) & 0xfff);
      }
      ili9341_bulk(0, h, LCD_WIDTH, 1);
      for (int w = 0; w < LCD_WIDTH; w++) {
        spi_buffer[w] = 0;
      }
      ili9341_read_memory(0, h, LCD_WIDTH, 1, spi_buffer);
      for (int w = 0; w < LCD_WIDTH; w++) {
        if (spi_buffer[w] != ((w*h) & 0xfff))
          return false;
      }
  }
  return true;
}

//#define _USE_WATERFALL_PALETTE
#ifdef  _USE_WATERFALL_PALETTE
#include "waterfall.c"
#endif

static void update_waterfall(void){
  int i;
  int w_width = area_width < WIDTH ? area_width : WIDTH;
//  START_PROFILE;
  for (i = CHART_BOTTOM-1; i >=graph_bottom+1; i--) {		// Scroll down
    ili9341_read_memory(OFFSETX, i, w_width, 1, spi_buffer);
    ili9341_bulk(OFFSETX, i+1, w_width, 1);
  }
  index_y_t *index;
  if (setting.average == AV_OFF)
    index = trace_index_y[TRACE_ACTUAL];
  else
    index = trace_index_y[TRACE_TEMP];
  int j = 0;
  for (i=0; i< sweep_points; i++) {			// Add new topline
    uint16_t color;
#ifdef _USE_WATERFALL_PALETTE
    uint16_t y = _PALETTE_ALIGN(CELL_Y(index[i])); // should be always in range 0 - graph_bottom
//    y = (uint8_t)i;  // for test
    color = waterfall_palette[y];
#elif 0
    uint16_t y = index[i]; // should be always in range 0 - graph_bottom
    uint16_t ratio = (graph_bottom - y)*2;
//    ratio = (i*2);    // Uncomment for testing the waterfall colors
    int16_t b = 255 - ratio;
    if (b > 255) b = 255;
    if (b < 0) b = 0;
    int16_t r = ratio - 255;
    if (r > 255) r = 255;
    if (r < 0) r = 0;
    int16_t g = 255 - b - r;
#define gamma_correct(X) X = (X < 64 ? X * 2 : X < 128 ? 128 + (X-64) : X < 192 ? 192 + (X - 128)/2 : 225 + (X - 192) / 4)
    gamma_correct(r);
    gamma_correct(g);
    gamma_correct(b);
    color = RGB565(r, g, b);
#else
    uint16_t y = SMALL_WATERFALL - index[i]* (graph_bottom == BIG_WATERFALL ? 2 : 1); // should be always in range 0 - graph_bottom *2 depends on height of scroll
    // Calculate gradient palette for range 0 .. 192
    // idx     r   g   b
    //   0 - 127   0   0
    //  32 - 255 127   0
    //  64 - 255 255 127
    //  96 - 255 255 255
    // 128 - 127 255 255
    // 160 -   0 127 255
    // 192 -   0   0 127
    // 224 -   0   0   0
//    y = (uint8_t)i;  // for test
         if (y <  32) color = RGB565( 127+((y-  0)*4),   0+((y-  0)*4),               0);
    else if (y <  64) color = RGB565(             255, 127+((y- 32)*4),   0+((y- 32)*4));
    else if (y <  96) color = RGB565(             255,             255, 127+((y- 64)*4));
    else if (y < 128) color = RGB565( 252-((y- 96)*4),             255,             255);
    else if (y < 160) color = RGB565( 124-((y-128)*4), 252-((y-128)*4),             255);
    else              color = RGB565(               0, 124-((y-160)*4), 252-((y-160)*4));

#endif
    while (j * sweep_points  < (i+1) * WIDTH) {   // Scale waterfall to WIDTH points
      spi_buffer[j++] = color;
    }
  }
  ili9341_bulk(OFFSETX, graph_bottom+1, w_width, 1);
//  STOP_PROFILE;
}

int get_waterfall(void)
{
  return(setting.waterfall);
}

//extern float peakLevel;
//extern float min_level;
//int w_max = -130;
//int w_min = 0;
void
toggle_waterfall(void)
{
  if (setting.waterfall == W_OFF) {
//    w_min = (int)min_level;
//    w_max = (int)peakLevel;
//    if (w_max < w_min + 20)
//      w_max = w_min + 20;
    graph_bottom = SMALL_WATERFALL;
    setting.waterfall = W_SMALL;
  } else if (setting.waterfall == W_SMALL) {
    graph_bottom = BIG_WATERFALL;
    setting.waterfall = W_BIG;
  } else {
    graph_bottom = NO_WATERFALL;
    setting.waterfall = W_OFF;
  }
  _grid_y = graph_bottom / NGRIDY;
  ili9341_set_background(LCD_BG_COLOR);
  ili9341_fill(OFFSETX, graph_bottom, LCD_WIDTH - OFFSETX, CHART_BOTTOM - graph_bottom);
  request_to_redraw_grid();
}

void
disable_waterfall(void)
{
  graph_bottom = NO_WATERFALL;
  setting.waterfall = W_OFF;
  _grid_y = graph_bottom / NGRIDY;
  ili9341_set_background(LCD_BG_COLOR);
  ili9341_fill(OFFSETX, graph_bottom, LCD_WIDTH - OFFSETX, CHART_BOTTOM - graph_bottom);
  request_to_redraw_grid();
}

void
plot_init(void)
{
  force_set_markmap();
}

#pragma GCC pop_options
