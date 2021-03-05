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
#pragma GCC optimize ("Os")      // Makes the code just a bit faster, disable during debugging.


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
typedef uint16_t pixel_t;
pixel_t *cell_buffer = (pixel_t *)spi_buffer;
// Cell size
// Depends from spi_buffer size, CELLWIDTH*CELLHEIGHT*sizeof(pixel) <= sizeof(spi_buffer)
//#define CELLWIDTH  (64) // moved to nanovna.h
//#define CELLHEIGHT (32)
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

map_t   markmap[2][MAX_MARKMAP_Y+1];
uint8_t current_mappage = 0;

// Trace data cache, for faster redraw cells
//   CELL_X[16:31] x position
//   CELL_Y[ 0:15] y position
typedef uint32_t  index_t;
static index_t trace_index[TRACES_MAX][POINTS_COUNT];

#define INDEX(x, y) ((((index_t)x)<<16)|(((index_t)y)))
#define CELL_X(i)  (int)(((i)>>16))
#define CELL_Y(i)  (int)(((i)&0xFFFF))

//#define float2int(v) ((int)(v))
static int 
float2int(float v) 
{
  if (v < 0) return v - 0.5;
  if (v > 0) return v + 0.5;
  return 0;
}

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

#ifdef __VNA__
static inline int
circle_inout(int x, int y, int r)
{
  int d = x*x + y*y - r*r;
  if (d < -r)
    return 1;
  if (d >  r)
    return -1;
  return 0;
}

static int
polar_grid(int x, int y)
{
  int d;

  // offset to center
  x -= P_CENTER_X;
  y -= P_CENTER_Y;

  // outer circle
  d = circle_inout(x, y, P_RADIUS);
  if (d < 0) return 0;
  if (d == 0) return 1;

  // vertical and horizontal axis
  if (x == 0 || y == 0) return 1;

  d = circle_inout(x, y, P_RADIUS / 5);
  if (d == 0) return 1;
  if (d > 0) return 0;

  d = circle_inout(x, y, P_RADIUS * 2 / 5);
  if (d == 0) return 1;
  if (d > 0) return 0;

  // cross sloping lines
  if (x == y || x == -y) return 1;

  d = circle_inout(x, y, P_RADIUS * 3 / 5);
  if (d == 0) return 1;
  if (d > 0) return 0;

  d = circle_inout(x, y, P_RADIUS * 4 / 5);
  if (d == 0) return 1;
  return 0;
}

/*
 * Constant Resistance circle: (u - r/(r+1))^2 + v^2 = 1/(r+1)^2
 * Constant Reactance circle:  (u - 1)^2 + (v-1/x)^2 = 1/x^2
 */
static int
smith_grid(int x, int y)
{
  int d;

  // offset to center
  x -= P_CENTER_X;
  y -= P_CENTER_Y;

  // outer circle
  d = circle_inout(x, y, P_RADIUS);
  if (d < 0) return 0;
  if (d == 0) return 1;

  // horizontal axis
  if (y == 0) return 1;

  // shift circle center to right origin
  x -= P_RADIUS;

  // Constant Reactance Circle: 2j : R/2 = P_RADIUS/2
  if (circle_inout(x, y + P_RADIUS / 2, P_RADIUS / 2) == 0) return 1;
  if (circle_inout(x, y - P_RADIUS / 2, P_RADIUS / 2) == 0) return 1;

  // Constant Resistance Circle: 3 : R/4 = P_RADIUS/4
  d = circle_inout(x + P_RADIUS / 4, y, P_RADIUS / 4);
  if (d > 0) return 0;
  if (d == 0) return 1;

  // Constant Reactance Circle: 1j : R = P_RADIUS
  if (circle_inout(x, y + P_RADIUS, P_RADIUS) == 0) return 1;
  if (circle_inout(x, y - P_RADIUS, P_RADIUS) == 0) return 1;

  // Constant Resistance Circle: 1 : R/2
  d = circle_inout(x + P_RADIUS / 2, y, P_RADIUS / 2);
  if (d > 0) return 0;
  if (d == 0) return 1;

  // Constant Reactance Circle: 1/2j : R*2
  if (circle_inout(x, y + P_RADIUS * 2, P_RADIUS * 2) == 0) return 1;
  if (circle_inout(x, y - P_RADIUS * 2, P_RADIUS * 2) == 0) return 1;

  // Constant Resistance Circle: 1/3 : R*3/4
  if (circle_inout(x + P_RADIUS * 3 / 4, y, P_RADIUS * 3 / 4) == 0) return 1;
  return 0;
}

#if 0
static int
smith_grid2(int x, int y, float scale)
{
  int d;

  // offset to center
  x -= P_CENTER_X;
  y -= P_CENTER_Y;

  // outer circle
  d = circle_inout(x, y, P_RADIUS);
  if (d < 0)
    return 0;
  if (d == 0)
    return 1;

  // shift circle center to right origin
  x -= P_RADIUS * scale;

  // Constant Reactance Circle: 2j : R/2 = 58
  if (circle_inout(x, y+58*scale, 58*scale) == 0)
    return 1;
  if (circle_inout(x, y-58*scale, 58*scale) == 0)
    return 1;
#if 0
  // Constant Resistance Circle: 3 : R/4 = 29
  d = circle_inout(x+29*scale, y, 29*scale);
  if (d > 0) return 0;
  if (d == 0) return 1;
  d = circle_inout(x-29*scale, y, 29*scale);
  if (d > 0) return 0;
  if (d == 0) return 1;
#endif

  // Constant Reactance Circle: 1j : R = 116
  if (circle_inout(x, y+116*scale, 116*scale) == 0)
    return 1;
  if (circle_inout(x, y-116*scale, 116*scale) == 0)
    return 1;

  // Constant Resistance Circle: 1 : R/2 = 58
  d = circle_inout(x+58*scale, y, 58*scale);
  if (d > 0) return 0;
  if (d == 0) return 1;
  d = circle_inout(x-58*scale, y, 58*scale);
  if (d > 0) return 0;
  if (d == 0) return 1;

  // Constant Reactance Circle: 1/2j : R*2 = 232
  if (circle_inout(x, y+232*scale, 232*scale) == 0)
    return 1;
  if (circle_inout(x, y-232*scale, 232*scale) == 0)
    return 1;

#if 0
  // Constant Resistance Circle: 1/3 : R*3/4 = 87
  d = circle_inout(x+87*scale, y, 87*scale);
  if (d > 0) return 0;
  if (d == 0) return 1;
  d = circle_inout(x+87*scale, y, 87*scale);
  if (d > 0) return 0;
  if (d == 0) return 1;
#endif

  // Constant Resistance Circle: 0 : R
  d = circle_inout(x+P_RADIUS*scale, y, P_RADIUS*scale);
  if (d > 0) return 0;
  if (d == 0) return 1;
  d = circle_inout(x-P_RADIUS*scale, y, P_RADIUS*scale);
  if (d > 0) return 0;
  if (d == 0) return 1;

  // Constant Resistance Circle: -1/3 : R*3/2 = 174
  d = circle_inout(x+174*scale, y, 174*scale);
  if (d > 0) return 0;
  if (d == 0) return 1;
  d = circle_inout(x-174*scale, y, 174*scale);
  //if (d > 0) return 0;
  if (d == 0) return 1;
  return 0;
}
#endif

#if 0
const int cirs[][4] = {
  { 0, 58/2, 58/2, 0 },    // Constant Reactance Circle: 2j : R/2 = 58
  { 29/2, 0, 29/2, 1 },    // Constant Resistance Circle: 3 : R/4 = 29
  { 0, 115/2, 115/2, 0 },  // Constant Reactance Circle: 1j : R = 115
  { 58/2, 0, 58/2, 1 },    // Constant Resistance Circle: 1 : R/2 = 58
  { 0, 230/2, 230/2, 0 },  // Constant Reactance Circle: 1/2j : R*2 = 230
  { 86/2, 0, 86/2, 1 },    // Constant Resistance Circle: 1/3 : R*3/4 = 86
  { 0, 460/2, 460/2, 0 },  // Constant Reactance Circle: 1/4j : R*4 = 460
  { 115/2, 0, 115/2, 1 },  // Constant Resistance Circle: 0 : R
  { 173/2, 0, 173/2, 1 },  // Constant Resistance Circle: -1/3 : R*3/2 = 173
  { 0, 0, 0, 0 } // sentinel
};

static int
smith_grid3(int x, int y)
{
  int d;

  // offset to center
  x -= P_CENTER_X;
  y -= P_CENTER_Y;

  // outer circle
  d = circle_inout(x, y, P_RADIUS);
  if (d < 0)
    return 0;
  if (d == 0)
    return 1;

  // shift circle center to right origin
  x -= P_RADIUS /2;

  int i;
  for (i = 0; cirs[i][2]; i++) {
    d = circle_inout(x+cirs[i][0], y+cirs[i][1], cirs[i][2]);
    if (d == 0)
      return 1;
    if (d > 0 && cirs[i][3])
      return 0;
    d = circle_inout(x-cirs[i][0], y-cirs[i][1], cirs[i][2]);
    if (d == 0)
      return 1;
    if (d > 0 && cirs[i][3])
      return 0;
  }
  return 0;
}
#endif
#endif
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

#define SQRT_50 ((float)7.0710678118654)
#define LOG_10_SQRT_50 ((float)0.84948500216800)
#define POW_30_20   ((float) 0.215443469)
#define POW_SQRT    ((float)0.2236067950725555419921875)
#define LOG_10_SQRT_50_x20_plus30 ((float)46.98970004336)
#define LOG_10_SQRT_50_x20_plus90 ((float)106.98970004336)

/*
 * calculate log10f(abs(gamma))
 */ 


float
index_to_value(const int i)
{
  return(value(actual_t[i]));
}

float
value(const float v)
{
  switch(setting.unit)
  {
  case U_DBMV:
//    return v + 30.0 + 20.0*log10f(sqrtf(50));
    return v + LOG_10_SQRT_50_x20_plus30; // + 30.0 + 20.0*LOG_10_SQRT_50;     //TODO convert constants to single float number as GCC compiler does runtime calculation
    break;
  case U_DBUV:
//    return v + 90.0 + 20.0*log10f(sqrtf(50.0));     //TODO convert constants to single float number as GCC compiler does runtime calculation
    return v + LOG_10_SQRT_50_x20_plus90; // 90.0 + 20.0*LOG_10_SQRT_50;
    break;
  case U_VOLT:
//    return powf(10.0, (v-30.0)/20.0) * sqrtf(50.0);
//    return powf(10.0, (v-30.0)/20.0) * SQRT_50;  //
    return powf(10.0, v/20) * POW_SQRT;      //  powf(10.0,v/20.0) * powf(10, -30.0/20.0) * sqrtf(50)
    break;
  case U_WATT:
    return powf((float)10.0, v/10.0)/1000.0;  // 
    break;
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
//  return v - 30.0 - 20.0*log10f(sqrtf(50));
    return v - LOG_10_SQRT_50_x20_plus30; // (30.0 + 20.0*LOG_10_SQRT_50);
    break;
  case U_DBUV:
//  return v - 90.0 - 20.0*log10f(sqrtf(50.0));     //TODO convert constants to single float number as GCC compiler does runtime calculation
    return v - LOG_10_SQRT_50_x20_plus90; // (90.0 + 20.0*LOG_10_SQRT_50);
    break;
  case U_VOLT:
//  return log10f( v / (sqrtf(50.0))) * 20.0 + 30.0 ;
    return log10f( v / SQRT_50) * 20.0 + 30.0 ;
    break;
  case U_WATT:
    return log10f(v*1000.0)*10.0;
    break;
  }
//  case U_DBM:
    return v;  // raw data is in logmag*10 format

}


#ifdef __VNA_
/*
 * calculate phase[-2:2] of coefficient
 */ 
static float
phase(const float *v)
{
  return 2 * atan2f(v[1], v[0]) / VNA_PI * 90;
}

/*
 * calculate groupdelay
 */
static float
groupdelay(const float *v, const float *w, float deltaf)
{
#if 1
  // atan(w)-atan(v) = atan((w-v)/(1+wv))
  float r = w[0]*v[1] - w[1]*v[0];
  float i = w[0]*v[0] + w[1]*v[1];
  return atan2f(r, i) / (2 * VNA_PI * deltaf);
#else
  return (atan2f(w[0], w[1]) - atan2f(v[0], v[1])) / (2 * VNA_PI * deltaf);
#endif
}

/*
 * calculate abs(gamma)
 */
static float
linear(const float *v)
{
  return - sqrtf(v[0]*v[0] + v[1]*v[1]);
}

/*
 * calculate vswr; (1+gamma)/(1-gamma)
 */ 
static float
swr(const float *v)
{
  float x = sqrtf(v[0]*v[0] + v[1]*v[1]);
  if (x >= 1)
    return INFINITY;
  return (1 + x)/(1 - x);
}

static float
resitance(const float *v)
{
  float z0 = 50;
  float d = z0 / ((1-v[0])*(1-v[0])+v[1]*v[1]);
  float zr = ((1+v[0])*(1-v[0]) - v[1]*v[1]) * d;
  return zr;
}

static float
reactance(const float *v)
{
  float z0 = 50;
  float d = z0 / ((1-v[0])*(1-v[0])+v[1]*v[1]);
  float zi = 2*v[1] * d;
  return zi;
}

static void
cartesian_scale(float re, float im, int *xp, int *yp, float scale)
{
  //float scale = 4e-3;
  int x = float2int(re * P_RADIUS * scale);
  int y = float2int(im * P_RADIUS * scale);
  if      (x < -P_RADIUS) x = -P_RADIUS;
  else if (x >  P_RADIUS) x =  P_RADIUS;
  if      (y < -P_RADIUS) y = -P_RADIUS;
  else if (y >  P_RADIUS) y =  P_RADIUS;
  *xp = P_CENTER_X + x;
  *yp = P_CENTER_Y - y;
}

float
groupdelay_from_array(int i, float array[POINTS_COUNT][2])
{
  int bottom = (i ==   0) ?   0 : i - 1;
  int top    = (i == sweep_points-1) ? sweep_points-1 : i + 1;
  float deltaf = frequencies[top] - frequencies[bottom];
  return groupdelay(array[bottom], array[top], deltaf);
}

static float
gamma2resistance(const float v[2])
{
  float z0 = 50;
  float d = z0 / ((1-v[0])*(1-v[0])+v[1]*v[1]);
  return ((1+v[0])*(1-v[0]) - v[1]*v[1]) * d;
}

static float
gamma2reactance(const float v[2])
{
  float z0 = 50;
  float d = z0 / ((1-v[0])*(1-v[0])+v[1]*v[1]);
  return 2*v[1] * d;
}
#endif

static index_t
trace_into_index(int t, int i, float array[POINTS_COUNT])
{
  int y, x;
  float coeff = array[i];
  float refpos = get_trace_refpos(t);
  float v=0;
  float scale = get_trace_scale(t);

  switch (trace[t].type) {
  case TRC_LOGMAG:
    v = ( refpos - value(coeff) ) / scale;
    break;
#ifdef __VNA__
	case TRC_PHASE:
    v-= phase(coeff) * scale;
    break;
  case TRC_DELAY:
    v-= groupdelay_from_array(i, array) * scale;
    break;
  case TRC_LINEAR:
    v+= linear(coeff) * scale;
    break;
  case TRC_SWR:
    v+= (1 - swr(coeff)) * scale;
    break;
  case TRC_REAL:
    v-= coeff[0] * scale;
    break;
  case TRC_IMAG:
    v-= coeff[1] * scale;
    break;
  case TRC_R:
    v-= resitance(coeff) * scale;
    break;
  case TRC_X:
    v-= reactance(coeff) * scale;
    break;
  case TRC_SMITH:
  //case TRC_ADMIT:
  case TRC_POLAR:
    cartesian_scale(coeff[0], coeff[1], &x, &y, scale);
    goto set_index;
#endif
	}
  if (v <  0) v = 0;
  if (v > NGRIDY) v = NGRIDY;
  x = (i * (WIDTH) + (sweep_points-1)/2) / (sweep_points-1) + CELLOFFSETX;
  y = float2int(v * GRIDY);
// set_index:
  return INDEX(x, y);
}

#ifdef __VNA__
static void
format_smith_value(char *buf, int len, const float coeff[2], uint32_t frequency)
{
  // z = (gamma+1)/(gamma-1) * z0
  float z0 = 50;
  float d = z0 / ((1-coeff[0])*(1-coeff[0])+coeff[1]*coeff[1]);
  float zr = ((1+coeff[0])*(1-coeff[0]) - coeff[1]*coeff[1]) * d;
  float zi = 2*coeff[1] * d;
  char prefix;
  float value;
  switch (marker_smith_format) {
  case MS_LIN:
    plot_printf(buf, len, "%.2f %.1f" S_DEGREE, linear(coeff), phase(coeff));
    break;

  case MS_LOG: {
      float v = logmag(coeff);
      if (v == -INFINITY)
        plot_printf(buf, len, "-"S_INFINITY" dB");
      else
        plot_printf(buf, len, "%.1fdB %.1f" S_DEGREE, v, phase(coeff));
    }
    break;

  case MS_REIM:
    plot_printf(buf, len, "%F%+Fj", coeff[0], coeff[1]);
    break;

  case MS_RX:
    plot_printf(buf, len, "%F"S_OHM"%+Fj", zr, zi);
    break;

  case MS_RLC:
    if (zi < 0) {// Capacity
      prefix = 'F';
      value = -1 / (2 * VNA_PI * frequency * zi);
    } else {
      prefix = 'H';
      value = zi / (2 * VNA_PI * frequency);
    }
    plot_printf(buf, len, "%F"S_OHM" %F%c", zr, value, prefix);
    break;
  }
}
#endif

#ifdef __VNA__
static void
trace_get_value_string(int t, char *buf, int len, float array[POINTS_COUNT][2], int i)
{
  float *coeff = array[i];
  float v;
  char *format;
  switch (trace[t].type) {
  case TRC_LOGMAG:
    format = "%.2fdB";
    v = logmag(coeff);
    break;
  case TRC_PHASE:
    format = "%.1f"S_DEGREE;
    v = phase(coeff);
    break;
  case TRC_DELAY:
    format = "%.2Fs";
    v = groupdelay_from_array(i, array);
    break;
  case TRC_LINEAR:
    format = "%.4f";
    v = linear(coeff);
    break;
  case TRC_SWR:
    format = "%.4f";
    v = swr(coeff);
    break;
  case TRC_REAL:
    format = "%.4f";
    v = coeff[0];
    break;
  case TRC_IMAG:
    format = "%.4fj";
    v = coeff[1];
    break;
  case TRC_R:
    format = "%.2F"S_OHM;
    v = gamma2resistance(coeff);
    break;
  case TRC_X:
    format = "%.2F"S_OHM;
    v = gamma2reactance(coeff);
    break;
  case TRC_SMITH:
    format_smith_value(buf, len, coeff, frequencies[i]);
    return;
    //case TRC_ADMIT:
  case TRC_POLAR:
    plot_printf(buf, len, "%.2f%+.2fj", coeff[0], coeff[1]);
  default:
    return;
  }
  plot_printf(buf, len, format, v);
}

static void
trace_get_value_string_delta(int t, char *buf, int len, float array[POINTS_COUNT][2], int index, int index_ref)
{
  float *coeff = array[index];
  float *coeff_ref = array[index_ref];
  float v;
  char *format;
  switch (trace[t].type) {
  case TRC_LOGMAG:
    format = S_DELTA"%.2fdB";
    v = logmag(coeff) - logmag(coeff_ref);
    break;
  case TRC_PHASE:
    format = S_DELTA"%.2f"S_DEGREE;
    v = phase(coeff) - phase(coeff_ref);
    break;
  case TRC_DELAY:
    format = "%.2Fs";
    v = groupdelay_from_array(index, array) - groupdelay_from_array(index_ref, array);
    break;
  case TRC_LINEAR:
    format = S_DELTA"%.3f";
    v = linear(coeff) - linear(coeff_ref);
    break;
  case TRC_SWR:
    format = S_DELTA"%.3f";
    v = swr(coeff);
    if (v != INFINITY) v -= swr(coeff_ref);
    break;
  case TRC_SMITH:
    format_smith_value(buf, len, coeff, frequencies[index]);
    return;
  case TRC_REAL:
    format = S_DELTA"%.3f";
    v = coeff[0] - coeff_ref[0];
    break;
  case TRC_IMAG:
    format = S_DELTA"%.3fj";
    v = coeff[1] - coeff_ref[1];
    break;
  case TRC_R:
    format = "%.2F"S_OHM;
    v = gamma2resistance(coeff);
    break;
  case TRC_X:
    format = "%.2F"S_OHM;
    v = gamma2reactance(coeff);
    break;
  //case TRC_ADMIT:
  case TRC_POLAR:
    plot_printf(buf, len, "%.2f%+.2fj", coeff[0], coeff[1]);
    return;
  default:
    return;
  }
  plot_printf(buf, len, format, v);
}
#endif

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
    if (mtype & M_NOISE)
      v = v - 10*log10f(actual_rbw_x10*100.0);
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

#ifdef __VNA__
static int
trace_get_info(int t, char *buf, int len)
{
  const char *name = get_trace_typename(t);
  float scale = get_trace_scale(t);
  switch (trace[t].type) {
  case TRC_LOGMAG:
      return plot_printf(buf, len, "%s %ddB/", name, (int)scale);
  case TRC_PHASE:
      return plot_printf(buf, len, "%s %d" S_DEGREE "/", name, (int)scale);
  case TRC_SMITH:
  //case TRC_ADMIT:
  case TRC_POLAR:
    if (scale != 1.0)
      return plot_printf(buf, len, "%s %.1fFS", name, scale);
    else
      return plot_printf(buf, len, "%s ", name);
  default:
      return plot_printf(buf, len, "%s %F/", name, scale);
  }
  return 0;
}

static float time_of_index(int idx)
{
  return 1.0 / (float)(frequencies[1] - frequencies[0]) / (float)FFT_SIZE * idx;
}

static float distance_of_index(int idx)
{
  float distance = ((float)idx * (float)SPEED_OF_LIGHT) /
                   ((float)(frequencies[1] - frequencies[0]) * (float)FFT_SIZE * 2.0);
  return distance * velocity_factor;
}
#endif

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
  for (t = 0; t < TRACES_MAX; t++) {
    if (!trace[t].enabled)
      continue;
    index_t *index = trace_index[t];
    int m0 = CELL_X(index[0]) / CELLWIDTH;
    int n0 = CELL_Y(index[0]) / CELLHEIGHT;
    map[n0] |= 1 << m0;
    for (i = 1; i < sweep_points; i++) {
      int m1 = CELL_X(index[i]) / CELLWIDTH;
      int n1 = CELL_Y(index[i]) / CELLHEIGHT;
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
  index_t idx = trace_into_index(TRACE_ACTUAL, 0, &setting.trigger_level);
  return CELL_Y(idx);
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
  if (x1 < x0) { SWAP(x0, x1); SWAP(y0, y1); }
  int dx = x1 - x0;
  int dy = y1 - y0, sy = 1; if (dy < 0) { dy = -dy; sy = -1; }
  int err = (dx > dy ? dx : -dy) / 2;

  while (1) {
    if (y0 >= 0 && y0 < CELLHEIGHT && x0 >= 0 && x0 < CELLWIDTH)
      cell_buffer[y0 * CELLWIDTH + x0] |= c;
    if (x0 == x1 && y0 == y1)
      return;
    int e2 = err;
    if (e2 > -dx) { err -= dy; x0++;  }
    if (e2 <  dy) { err += dx; y0+=sy;}
  }
}

// Give a little speedup then draw rectangular plot (50 systick on all calls, all render req 700 systick)
// Write more difficult algoritm for seach indexes not give speedup
static int
search_index_range_x(int x1, int x2, index_t index[POINTS_COUNT], int *i0, int *i1)
{
  int i, j;
  int head = 0;
  int tail = sweep_points;
  int idx_x;

  // Search index point in cell
  while (1) {
    i = (head + tail) / 2;
    idx_x = CELL_X(index[i]);
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
  } while (j > 0 && x1 <= CELL_X(index[j]));
  *i0 = j;
  // Search index right from point
  do {
    i++;
  } while (i < sweep_points-1 && CELL_X(index[i]) < x2);
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
  int t;
  if (!markers[marker].enabled)
    return;
  for (t = TRACE_ACTUAL; t <= TRACE_ACTUAL; t++) {
    if (!trace[t].enabled)
      continue;
    index_t index = trace_index[t][markers[marker].index];
    int x = CELL_X(index) - X_MARKER_OFFSET;
    int y = CELL_Y(index) - Y_MARKER_OFFSET;
    invalidate_rect(x, y, x+MARKER_WIDTH-1, y+MARKER_HEIGHT-1);
  }
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
  index_t *index = trace_index[t];
  x-= CELL_X(index[idx]);
  y-= CELL_Y(index[idx]);
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
  int t, i;
//  START_PROFILE
  for (t = 0; t < TRACES_MAX; t++) {
    if (!trace[t].enabled)
      continue;
    int ch = trace[t].channel;
    index_t *index = trace_index[t];
    for (i = 0; i < sweep_points; i++)
      index[i] = trace_into_index(t, i, measured[ch]);
  }
//  STOP_PROFILE
#if 0
  for (t = 0; t < TRACES_MAX; t++)
    if (trace[t].enabled && trace[t].polar)
      quicksort(trace_index[t], 0, sweep_points);
#endif

  mark_cells_from_index();
  markmap_all_markers();
}

static void
draw_cell(int m, int n)
{
  int x0 = m * CELLWIDTH;
  int y0 = n * CELLHEIGHT;
  int w = CELLWIDTH;
  int h = CELLHEIGHT;
  int x, y;
  int i0, i1, i;
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
      trace_type |= (1 << trace[t].type);
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
#ifdef __VNA__
  // Smith greed line (1000 system ticks for all screen calls)
  if (trace_type & (1 << TRC_SMITH)) {
    for (y = 0; y < h; y++)
      for (x = 0; x < w; x++)
        if (smith_grid(x + x0, y + y0)) cell_buffer[y * CELLWIDTH + x] = c;
  }
  // Polar greed line (800 system ticks for all screen calls)
  else if (trace_type & (1 << TRC_POLAR)) {
    for (y = 0; y < h; y++)
      for (x = 0; x < w; x++)
        if (polar_grid(x + x0, y + y0)) cell_buffer[y * CELLWIDTH + x] = c;
  }
#if 0
  else if (trace_type & (1 << TRC_ADMIT)) {
    for (y = 0; y < h; y++)
      for (x = 0; x < w; x++)
        if (smith_grid3(x+x0, y+y0)
         // smith_grid2(x+x0, y+y0, 0.5))
           cell_buffer[y * CELLWIDTH + x] = c;
  }
#endif
#endif
//  PULSE;
#endif
// Draw trigger line
  if (setting.trigger != T_AUTO) {
    int tp = get_trigger_level() - y0;
    if (tp>=0 && tp < h)
      for (x = 0; x < w; x++)
        if (x + x0 >= CELLOFFSETX && x + x0 <= WIDTH + CELLOFFSETX)
          cell_buffer[tp * CELLWIDTH + x] = LCD_TRIGGER_COLOR;
  }

#if 1
  // Only right cells
  if (m >= (GRID_X_TEXT)/CELLWIDTH)
    cell_grid_line_info(x0, y0);
#endif

// Draw traces (50-600 system ticks for all screen calls, depend from lines
// count and size)
#if 1
  for (t = 0; t < TRACES_MAX; t++) {
    if (!trace[t].enabled)
      continue;
    c = GET_PALTETTE_COLOR(LCD_TRACE_1_COLOR + t);
    // draw polar plot (check all points)
    i0 = 0;
    i1 = 0;
    uint32_t trace_type = (1 << trace[t].type);
    if (trace_type & ((1 << TRC_SMITH) | (1 << TRC_POLAR)))
      i1 = sweep_points - 1;
    else  // draw rectangular plot (search index range in cell, save 50-70
          // system ticks for all screen calls)
      search_index_range_x(x0, x0 + w, trace_index[t], &i0, &i1);
    index_t *index = trace_index[t];
    for (i = i0; i < i1; i++) {
      int x1 = CELL_X(index[i]) - x0;
      int y1 = CELL_Y(index[i]) - y0;
      int x2 = CELL_X(index[i + 1]) - x0;
      int y2 = CELL_Y(index[i + 1]) - y0;
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
  for (i = 0; i < MARKERS_MAX; i++) {
    if (!markers[i].enabled)
      continue;
//    for (t = 0; t < TRACES_MAX; t++) {
//      if (!trace[t].enabled)
//        continue;
      t = TRACE_ACTUAL;
      index_t index = trace_index[t][markers[i].index];
      int x = CELL_X(index) - x0 - X_MARKER_OFFSET;
      int y = CELL_Y(index) - y0 - Y_MARKER_OFFSET;
      // Check marker icon on cell
      if ((uint32_t)(x+MARKER_WIDTH ) < (CELLWIDTH  + MARKER_WIDTH ) &&
          (uint32_t)(y+MARKER_HEIGHT) < (CELLHEIGHT + MARKER_HEIGHT)){
          // Draw marker plate
          ili9341_set_foreground(LCD_TRACE_1_COLOR + t);
          cell_blit_bitmap(x, y, MARKER_WIDTH, MARKER_HEIGHT, MARKER_BITMAP(0));
          // Draw marker number
          ili9341_set_foreground(LCD_BG_COLOR);
          cell_blit_bitmap(x, y, MARKER_WIDTH, MARKER_HEIGHT, MARKER_BITMAP(i+1));
      }
//    }
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
  ili9341_bulk(OFFSETX + x0, OFFSETY + y0, w, h);
}

extern float peakLevel;
extern float min_level;
int w_max = -130;
int w_min = 0;


static void
draw_all_cells(bool flush_markmap)
{
  int m, n;
//  START_PROFILE
  for (m = 0; m < (area_width+CELLWIDTH-1) / CELLWIDTH; m++)
    for (n = 0; n < (area_height+CELLHEIGHT-1) / CELLHEIGHT; n++) {
      if ((markmap[0][n] | markmap[1][n]) & (1 << m)) {
        draw_cell(m, n);
//        ili9341_fill(m*CELLWIDTH+10, n*CELLHEIGHT, 2, 2, RGB565(255,0,0));
      }
//      else
//        ili9341_fill(m*CELLWIDTH+10, n*CELLHEIGHT, 2, 2, RGB565(0,255,0));
    }
//  STOP_PROFILE
  if (flush_markmap) {
    // keep current map for update
    swap_markmap();
    // clear map for next plotting
    clear_markmap();
  }
}

void
draw_all(bool flush)
{
  if (redraw_request & REDRAW_AREA)
    force_set_markmap();
  if (redraw_request & REDRAW_MARKER)
    markmap_upperarea();
  if (redraw_request & REDRAW_TRIGGER)
    markmap_trigger_area();
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

#ifdef __VNA__
static void
cell_draw_marker_info(int x0, int y0)
{
  char buf[24];
  int t;
  if (active_marker < 0)
    return;
  int idx = markers[active_marker].index;
  int j = 0;
  if (previous_marker != -1 && uistat.current_trace != -1) {
    int t = uistat.current_trace;
    int mk;
    for (mk = 0; mk < MARKERS_MAX; mk++) {
      if (!markers[mk].enabled)
        continue;
      int xpos = 1 + (j%2)*(WIDTH/2) + CELLOFFSETX - x0;
      int ypos = 1 + (j/2)*(FONT_GET_HEIGHT+1) - y0;

      ili9341_set_foreground(GET_PALTETTE_COLOR(LCD_TRACE_1_COLOR + t));
      if (mk == active_marker)
        cell_drawstring(S_SARROW, xpos, ypos);
      xpos += 5;
      plot_printf(buf, sizeof buf, "M%d", mk+1);
      cell_drawstring(buf, xpos, ypos);
      xpos += 13;
      //trace_get_info(t, buf, sizeof buf);
      freq_t freq = frequencies[markers[mk].index];
      if (uistat.marker_delta && mk != active_marker) {
        freq_t freq1 = frequencies[markers[active_marker].index];
        freq_t delta = freq > freq1 ? freq - freq1 : freq1 - freq;
        plot_printf(buf, sizeof buf, S_DELTA"%.9qHz", delta);
      } else {
        plot_printf(buf, sizeof buf, "%.10qHz", freq);
      }
      cell_drawstring(buf, xpos, ypos);
      xpos += 67;
      if (uistat.marker_delta && mk != active_marker)
        trace_get_value_string_delta(t, buf, sizeof buf, measured[trace[t].channel], markers[mk].index, markers[active_marker].index);
      else
        trace_get_value_string(t, buf, sizeof buf, measured[trace[t].channel], markers[mk].index);
      ili9341_set_foreground(LCD_FG_COLOR);
      cell_drawstring(buf, xpos, ypos);
      j++;
    }

    // draw marker delta
    if (!uistat.marker_delta && previous_marker >= 0 && active_marker != previous_marker && markers[previous_marker].enabled) {
      int idx0 = markers[previous_marker].index;
      int xpos = (WIDTH/2+30) + CELLOFFSETX - x0;
      int ypos = 1 + (j/2)*(FONT_GET_HEIGHT+1) - y0;

      plot_printf(buf, sizeof buf, S_DELTA"%d-%d:", active_marker+1, previous_marker+1);
      ili9341_set_foreground(LCD_FG_COLOR);
      cell_drawstring(buf, xpos, ypos);
      xpos += 27;
      if ((domain_mode & DOMAIN_MODE) == DOMAIN_FREQ) {
        freq_t freq  = frequencies[idx];
        freq_t freq1 = frequencies[idx0];
        freq_t delta = freq > freq1 ? freq - freq1 : freq1 - freq;
        plot_printf(buf, sizeof buf, "%c%.13qHz", freq >= freq1 ? '+' : '-', delta);
      } else {
        plot_printf(buf, sizeof buf, "%Fs (%Fm)", time_of_index(idx) - time_of_index(idx0), distance_of_index(idx) - distance_of_index(idx0));
      }
      cell_drawstring(buf, xpos, ypos);
    }
  } else {
    for (t = 0; t < TRACES_MAX; t++) {
      if (!trace[t].enabled)
        continue;
      int xpos = 1 + (j%2)*(WIDTH/2) + CELLOFFSETX - x0;
      int ypos = 1 + (j/2)*(FONT_GET_HEIGHT+1) - y0;

      ili9341_set_foreground(GET_PALTETTE_COLOR(LCD_TRACE_1_COLOR + t));
      if (t == uistat.current_trace)
        cell_drawstring(S_SARROW, xpos, ypos);
      xpos += 5;
      plot_printf(buf, sizeof buf, "CH%d", trace[t].channel);
      cell_drawstring(buf, xpos, ypos);
      xpos += 19;

      int n = trace_get_info(t, buf, sizeof buf);
      cell_drawstring(buf, xpos, ypos);
      xpos += n * 5 + 2;
      //xpos += 60;
      trace_get_value_string(t, buf, sizeof buf, measured[trace[t].channel], idx);
      ili9341_set_foreground(LCD_FG_COLOR);
      cell_drawstring(buf, xpos, ypos);
      j++;
    }

    // draw marker frequency
    int xpos = (WIDTH/2+40) + CELLOFFSETX - x0;
    int ypos = 1 + (j/2)*(FONT_GET_HEIGHT+1) - y0;

    ili9341_set_foreground(LCD_FG_COLOR);
    if (uistat.lever_mode == LM_MARKER)
      cell_drawstring(S_SARROW, xpos, ypos);
    xpos += 5;
    plot_printf(buf, sizeof buf, "M%d:", active_marker+1);
    cell_drawstring(buf, xpos, ypos);
    xpos += 19;

    if ((domain_mode & DOMAIN_MODE) == DOMAIN_FREQ) {
      plot_printf(buf, sizeof buf, "%qHz", frequencies[idx]);
    } else {
      plot_printf(buf, sizeof buf, "%Fs (%Fm)", time_of_index(idx), distance_of_index(idx));
    }
    cell_drawstring(buf, xpos, ypos);
  }
  ili9341_set_foreground(LCD_FG_COLOR);
  if (electrical_delay != 0) {
    // draw electrical delay
    int xpos = 21 + CELLOFFSETX - x0;
    int ypos = 1 + ((j+1)/2)*(FONT_GET_HEIGHT+1) - y0;

    if (uistat.lever_mode == LM_EDELAY)
      cell_drawstring(S_SARROW, xpos, ypos);
    xpos += 5;

    float light_speed_ps = SPEED_OF_LIGHT*1e-12; //(m/ps)
    plot_printf(buf, sizeof buf, "Edelay %Fs %Fm", electrical_delay * 1e-12,
                                                   electrical_delay * light_speed_ps * velocity_factor);
    cell_drawstring(buf, xpos, ypos);
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
  float   ref = get_trace_refpos(TRACE_ACTUAL);
  float scale = get_trace_scale(TRACE_ACTUAL);;
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
        int depth =(int) (powf((float)10.0, 2.0 + (level + 6.02) /20.0));
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
          idx, measured[trace[t].channel], ridx, markers[i].mtype,markers[i].frequency, markers[ref_marker].frequency);
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
  char buf2[32]; buf2[0] = 0;
  if (MODE_OUTPUT(setting.mode))     // No frequencies during output
    return;
  if (current_menu_is_form() && !in_selftest)
    return;

#ifdef __VNA__
  if ((domain_mode & DOMAIN_MODE) == DOMAIN_FREQ) {
#endif
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
#ifdef __VNA__
  } else {
    plot_printf(buf1, sizeof(buf1), " START 0s");
    plot_printf(buf2, sizeof(buf2), "STOP %Fs (%Fm)", time_of_index(sweep_points-1), distance_of_index(sweep_points-1));
  }
#endif
  ili9341_set_foreground(LCD_FG_COLOR);
  ili9341_set_background(LCD_BG_COLOR);
  ili9341_fill(FREQUENCIES_XPOS1, FREQUENCIES_YPOS, LCD_WIDTH- FREQUENCIES_XPOS1, FONT_GET_HEIGHT);
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
#ifdef __VNA__
void
draw_cal_status(void)
{
  int x = 0;
  int y = 100;
  char c[3];
  ili9341_set_foreground(LCD_FG_COLOR);
  ili9341_set_background(LCD_BG_COLOR);
  ili9341_fill(0, y, OFFSETX, 6*(FONT_GET_HEIGHT+1), LCD_BG_COLOR);
  if (cal_status & CALSTAT_APPLY) {
    c[0] = cal_status & CALSTAT_INTERPOLATED ? 'c' : 'C';
    c[1] = active_props == &current_props ? '*' : '0' + lastsaveid;
    c[2] = 0;
    ili9341_drawstring(c, x, y);
    y +=FONT_GET_HEIGHT+1;
  }
  int i;
  static const struct {char text, zero, mask;} calibration_text[]={
    {'D', 0, CALSTAT_ED},
    {'R', 0, CALSTAT_ER},
    {'S', 0, CALSTAT_ES},
    {'T', 0, CALSTAT_ET},
    {'X', 0, CALSTAT_EX}
  };
  for (i = 0; i < 5; i++, y+=FONT_GET_HEIGHT+1)
    if (cal_status & calibration_text[i].mask)
      ili9341_drawstring(&calibration_text[i].text, x, y);
}
#endif

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
      ili9341_read_memory(0, h, LCD_WIDTH, 1, LCD_WIDTH, spi_buffer);
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
  // Waterfall only in 290 or 145 points
//  if (!(sweep_points == 290 || sweep_points == 145))
//    return;
  for (i = CHART_BOTTOM-1; i >=graph_bottom+1; i--) {		// Scroll down
    ili9341_read_memory(OFFSETX, i  , w_width, 1, w_width*1, spi_buffer);
           ili9341_bulk(OFFSETX, i+1, w_width, 1);
  }
  index_t *index;
  if (setting.average == AV_OFF)
    index = trace_index[TRACE_ACTUAL];
  else
    index = trace_index[TRACE_TEMP];
  int j = 0;
  for (i=0; i< sweep_points; i++) {			// Add new topline
    uint16_t color;
#ifdef _USE_WATERFALL_PALETTE
    uint16_t y = _PALETTE_ALIGN(CELL_Y(index[i])); // should be always in range 0 - graph_bottom
//    y = (uint8_t)i;  // for test
    color = waterfall_palette[y];
#elif 0
    uint16_t y = CELL_Y(index[i]); // should be always in range 0 - graph_bottom
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
    uint16_t y = SMALL_WATERFALL - CELL_Y(index[i])* (graph_bottom == BIG_WATERFALL ? 2 : 1); // should be always in range 0 - graph_bottom *2 depends on height of scroll
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
}

int get_waterfall(void)
{
  return(setting.waterfall);
}

void
toggle_waterfall(void)
{
  if (setting.waterfall == W_OFF) {
    w_min = (int)min_level;
    w_max = (int)peakLevel;
    if (w_max < w_min + 20)
      w_max = w_min + 20;
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
