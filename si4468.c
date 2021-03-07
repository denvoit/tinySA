/*
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
#include "nanovna.h"
#include <math.h>
#include "si4432.h"
#include "spi.h"

#pragma GCC push_options
#pragma GCC optimize ("Og")

// Define for use hardware SPI mode
#define USE_HARDWARE_SPI_MODE

// 10MHz clock
#define SI4432_10MHZ 10000000U
// !!!! FROM ili9341.c for disable it !!!!
//#define LCD_CS_HIGH    palSetPad(GPIOB, GPIOB_LCD_CS)
#define SI_CS_LOW      palClearLine(LINE_RX_SEL)
#define SI_CS_HIGH     palSetLine(LINE_RX_SEL)

#define SI_SDN_LOW      palClearLine(LINE_RX_SDN)
#define SI_SDN_HIGH     palSetLine(LINE_RX_SDN)


// Hardware or software SPI use
#ifdef USE_HARDWARE_SPI_MODE
#define SI4432_SPI         SPI1
//#define SI4432_SPI_SPEED   SPI_BR_DIV64
//#define SI4432_SPI_SPEED   SPI_BR_DIV32
#define SI4432_SPI_SPEED   SPI_BR_DIV4

//#define ADF_SPI_SPEED   SPI_BR_DIV64
//#define ADF_SPI_SPEED   SPI_BR_DIV32
#define ADF_SPI_SPEED   SPI_BR_DIV2

#define PE_SPI_SPEED   SPI_BR_DIV32

static uint32_t old_spi_settings;
#else
static uint32_t old_port_moder;
static uint32_t new_port_moder;
#endif

#define SPI1_CLK_HIGH   palSetPad(GPIOB, GPIOB_SPI_SCLK)
#define SPI1_CLK_LOW    palClearPad(GPIOB, GPIOB_SPI_SCLK)

#define SPI1_SDI_HIGH   palSetPad(GPIOB, GPIOB_SPI_MOSI)
#define SPI1_SDI_LOW    palClearPad(GPIOB, GPIOB_SPI_MOSI)
#define SPI1_RESET      palClearPort(GPIOB, (1<<GPIOB_SPI_SCLK)|(1<<GPIOB_SPI_MOSI))

#define SPI1_SDO       ((palReadPort(GPIOB)>>GPIOB_SPI_MISO)&1)
#define SPI1_portSDO   (palReadPort(GPIOB)&(1<<GPIOB_SPI_MISO))
#ifdef __PE4302__
#define CS_PE_HIGH      palSetLine(LINE_PE_SEL)
#define CS_PE_LOW      palClearLine(LINE_PE_SEL)
#endif

//#define MAXLOG 1024
//unsigned char SI4432_logging[MAXLOG];
//volatile int log_index = 0;

//#define SI4432_log(X)   { if (log_index < MAXLOG)  SI4432_logging[log_index++] = X; }
#define SI4432_log(X)

void start_SI4432_SPI_mode(void){
#ifdef USE_HARDWARE_SPI_MODE
  old_spi_settings = SI4432_SPI->CR1;
  SPI_BR_SET(SI4432_SPI, SI4432_SPI_SPEED);
#else
  // Init legs mode for software bitbang
  old_port_moder = GPIOB->MODER;
  new_port_moder = old_port_moder & ~(PIN_MODE_ANALOG(GPIOB_SPI_SCLK)|PIN_MODE_ANALOG(GPIOB_SPI_MISO)|PIN_MODE_ANALOG(GPIOB_SPI_MOSI));
  new_port_moder|= PIN_MODE_OUTPUT(GPIOB_SPI_SCLK)|PIN_MODE_INPUT(GPIOB_SPI_MISO)|PIN_MODE_OUTPUT(GPIOB_SPI_MOSI);
  GPIOB->MODER = new_port_moder;
  // Pull down SPI
  SPI1_SDI_LOW;
  SPI1_CLK_LOW;
#endif
}

void stop_SI4432_SPI_mode(void){
#ifdef USE_HARDWARE_SPI_MODE
  SI4432_SPI->CR1 = old_spi_settings;
#else
  // Restore hardware SPI
  GPIOB->MODER = old_port_moder;
#endif
}

static void shiftOut(uint8_t val)
{
#ifdef USE_HARDWARE_SPI_MODE
  SPI_WRITE_8BIT(SI4432_SPI, val);
  while (SPI_IS_BUSY(SI4432_SPI)) // drop rx and wait tx
    (void)SPI_READ_8BIT(SI4432_SPI);
#else
  SI4432_log(SI4432_Sel);
  SI4432_log(val);
  uint8_t i = 0;
  do {
    SPI1_SDI_HIGH;
    SPI1_CLK_HIGH;
    SPI1_RESET;
    val<<=1;
  }while((++i) & 0x07);
#endif
}

static uint8_t shiftIn(void)
{
#ifdef USE_HARDWARE_SPI_MODE
  SPI_WRITE_8BIT(SI4432_SPI, 0xFF);
  while (SPI_IS_BUSY(SI4432_SPI)) // drop rx and wait tx
  while (SPI_RX_IS_EMPTY(SI4432_SPI)); //wait rx data in buffer
  return SPI_READ_8BIT(SI4432_SPI);
#else
  uint32_t value = 0;
  uint8_t i = 0;
  do {
    value<<=1;
    SPI1_CLK_HIGH;
    value|=SPI1_portSDO;
    SPI1_CLK_LOW;
  }while((++i) & 0x07);
  return value>>GPIOB_SPI_MISO;
#endif
}

static inline void shiftInBuf(uint16_t sel, uint8_t addr, deviceRSSI_t *buf, uint16_t size, uint16_t delay) {
#ifdef USE_HARDWARE_SPI_MODE
  do{
    palClearPad(GPIOB, sel);
    SPI_WRITE_8BIT(SI4432_SPI, addr);
    while (SPI_IS_BUSY(SI4432_SPI))      // drop rx and wait tx
      (void)SPI_READ_8BIT(SI4432_SPI);

    SPI_WRITE_8BIT(SI4432_SPI, 0xFF);
    while (SPI_RX_IS_EMPTY(SI4432_SPI)); //wait rx data in buffer
    *buf++=SPI_READ_8BIT(SI4432_SPI);
    palSetPad(GPIOB, sel);
    if (delay)
      my_microsecond_delay(delay);
  }while(--size);
#else
  uint8_t i = 0;
  do{
    uint32_t value = addr;
    palClearPad(GPIOB, sel);
    do {
      if (value & 0x80)
        SPI1_SDI_HIGH;
      SPI1_CLK_HIGH;
      SPI1_RESET;
      value<<=1;
    }while((++i) & 0x07);
    value = 0;
    do {
      SPI1_CLK_HIGH;
      value<<=1;
      value|=SPI1_portSDO;
      SPI1_CLK_LOW;
    }while((++i) & 0x07);
    palSetPad(GPIOB, sel);
    *buf++=value>>GPIOB_SPI_MISO;
    if (delay)
      my_microsecond_delay(delay);
  }while(--size);
#endif
}
#if 0
static void shiftOutBuf(uint8_t *buf, uint16_t size) {
  uint8_t i = 0;
  do{
    uint8_t val = *buf++;
    do{
      if (val & 0x80)
        SPI1_SDI_HIGH;
      else
        SPI1_SDI_LOW;
      val<<=1;
      SPI1_CLK_HIGH;
      SPI1_CLK_LOW;
    }while((++i) & 0x07);
  }while(--size);
}
#endif

int SI4432_step_delay = 1500;
int SI4432_offset_delay = 1500;
#define MINIMUM_WAIT_FOR_RSSI   280

#ifdef __SI4432__
#define CS_SI0_HIGH     palSetPad(GPIOC, GPIO_RX_SEL)
#define CS_SI1_HIGH     palSetPad(GPIOC, GPIO_LO_SEL)

#define RF_POWER_HIGH   palSetPad(GPIOB, GPIO_RF_PWR)


#define CS_SI0_LOW     palClearPad(GPIOC, GPIO_RX_SEL)
#define CS_SI1_LOW     palClearPad(GPIOC, GPIO_LO_SEL)


const uint16_t SI_nSEL[MAX_SI4432+1] = { GPIO_RX_SEL, GPIO_LO_SEL, 0}; // #3 is dummy!!!!!!

volatile int SI4432_Sel = 0;         // currently selected SI4432
// volatile int SI4432_guard = 0;

#ifdef __SI4432_H__
#define SELECT_DELAY 10
void SI4432_Write_Byte(uint8_t ADR, uint8_t DATA )
{
  set_SPI_mode(SPI_MODE_SI);
//  if (SI4432_guard)
//    while(1) ;
//  SI4432_guard = 1;
//  SPI1_CLK_LOW;
  palClearPad(GPIOB, SI_nSEL[SI4432_Sel]);
//  my_microsecond_delay(SELECT_DELAY);
  ADR |= 0x80 ; // RW = 1
  shiftOut( ADR );
  shiftOut( DATA );
  palSetPad(GPIOB, SI_nSEL[SI4432_Sel]);
//  SI4432_guard = 0;
}

void SI4432_Write_2_Byte(uint8_t ADR, uint8_t DATA1, uint8_t DATA2)
{
//  if (SI4432_guard)
//    while(1) ;
//  SI4432_guard = 1;
//  SPI2_CLK_LOW;
  palClearPad(GPIOC, SI_nSEL[SI4432_Sel]);
//  chThdSleepMicroseconds(SELECT_DELAY);
  ADR |= 0x80 ; // RW = 1
  shiftOut( ADR );
  shiftOut( DATA1 );
  shiftOut( DATA2 );
  palSetPad(GPIOC, SI_nSEL[SI4432_Sel]);
//  SI4432_guard = 0;
}

void SI4432_Write_3_Byte(uint8_t ADR, uint8_t DATA1, uint8_t DATA2, uint8_t DATA3 )
{
  set_SPI_mode(SPI_MODE_SI);
//  if (SI4432_guard)
//    while(1) ;
//  SI4432_guard = 1;
//  SPI1_CLK_LOW;
  palClearPad(GPIOB, SI_nSEL[SI4432_Sel]);
//  my_microsecond_delay(SELECT_DELAY);
  ADR |= 0x80 ; // RW = 1
  shiftOut( ADR );
  shiftOut( DATA1 );
  shiftOut( DATA2 );
  shiftOut( DATA3 );
  palSetPad(GPIOB, SI_nSEL[SI4432_Sel]);
//  SI4432_guard = 0;
}

uint8_t SI4432_Read_Byte( uint8_t ADR )
{
  set_SPI_mode(SPI_MODE_SI);
  uint8_t DATA ;
//  if (SI4432_guard)
//    while(1) ;
//  SI4432_guard = 1;
//  SPI1_CLK_LOW;
  palClearPad(GPIOB, SI_nSEL[SI4432_Sel]);
  shiftOut( ADR );
  DATA = shiftIn();
  palSetPad(GPIOB, SI_nSEL[SI4432_Sel]);
//  SI4432_guard = 0;
  return DATA ;
}



void SI4432_Reset(void)
{
  int count = 0;
  SI4432_Read_Byte (SI4432_INT_STATUS1);    // Clear pending interrupts
  SI4432_Read_Byte (SI4432_INT_STATUS2);
  // always perform a system reset (don't send 0x87)
  SI4432_Write_Byte(SI4432_STATE, 0x80);
  chThdSleepMilliseconds(10);
  // wait for chiprdy bit
  while (count++ < 100 && ( SI4432_Read_Byte (SI4432_INT_STATUS2) & 0x02 ) == 0) {
    chThdSleepMilliseconds(10);
  }
}

void SI4432_Drive(int d)
{
  SI4432_Write_Byte(SI4432_TX_POWER, (uint8_t) (0x18+(d & 7)));
}

void SI4432_Transmit(int d)
{
  int count = 0;
  SI4432_Drive(d);
  if (( SI4432_Read_Byte(SI4432_DEV_STATUS) & 0x03 ) == 2)
    return; // Already in transmit mode
  chThdSleepMilliseconds(3);
  SI4432_Write_Byte(SI4432_STATE, 0x02);
  chThdSleepMilliseconds(3);
  SI4432_Write_Byte(SI4432_STATE, 0x0b);
  chThdSleepMilliseconds(10);
  while (count++ < 100 && ( SI4432_Read_Byte(SI4432_DEV_STATUS) & 0x03 ) != 2) {
    chThdSleepMilliseconds(10);
  }
}

void SI4432_Receive(void)
{
  int count = 0;
  if (( SI4432_Read_Byte (SI4432_DEV_STATUS) & 0x03 ) == 1)
    return; // Already in receive mode
  chThdSleepMilliseconds(3);
  SI4432_Write_Byte(SI4432_STATE, 0x02);
  chThdSleepMilliseconds(3);
  SI4432_Write_Byte(SI4432_STATE, 0x07);
  chThdSleepMilliseconds(10);
  while (count++ < 100 && ( SI4432_Read_Byte(SI4432_DEV_STATUS) & 0x03 ) != 1) {
    chThdSleepMilliseconds(5);
  }
}

// User asks for an RBW of WISH, go through table finding the last triple
// for which WISH is greater than the first entry, use those values,
// Return the first entry of the following triple for the RBW actually achieved
#define IF_BW(dwn3, ndec, filset) (((dwn3)<<7)|((ndec)<<4)|(filset))
typedef struct {
  uint8_t  reg;                   // IF_BW(dwn3, ndec, filset)
  int8_t   RSSI_correction_x_10;  // Correction * 10
  uint16_t RBWx10;                // RBW * 10 in kHz
}RBW_t; // sizeof(RBW_t) = 4 bytes
RBW_t RBW_choices[] = {
// BW register    corr  freq
//                              {IF_BW(0,5,1),0,26},
//                              {IF_BW(0,5,2),0,28},
                              {IF_BW(0,5,3),3,31},
                              {IF_BW(0,5,4),-3,32},
                              {IF_BW(0,5,5),6,37},
                              {IF_BW(0,5,6),5,42},
                              {IF_BW(0,5,7),5,45},
                              {IF_BW(0,4,1),0,49},
                              {IF_BW(0,4,2),0,54},
                              {IF_BW(0,4,3),0,59},
                              {IF_BW(0,4,4),0,61},
                              {IF_BW(0,4,5),5,72},
                              {IF_BW(0,4,6),5,82},
                              {IF_BW(0,4,7),5,88},
                              {IF_BW(0,3,1),0,95},
                              {IF_BW(0,3,2),0,106},
                              {IF_BW(0,3,3),2,115},
                              {IF_BW(0,3,4),0,121},
                              {IF_BW(0,3,5),5,142},
                              {IF_BW(0,3,6),5,162},
                              {IF_BW(0,3,7),5,175},
                              {IF_BW(0,2,1),0,189},
                              {IF_BW(0,2,2),0,210},
                              {IF_BW(0,2,3),3,227},
                              {IF_BW(0,2,4),0,240},
                              {IF_BW(0,2,5),5,282},
                              {IF_BW(0,2,6),5,322},
                              {IF_BW(0,2,7),5,347},
                              {IF_BW(0,1,1),0,377},
                              {IF_BW(0,1,2),0,417},
                              {IF_BW(0,1,3),1,452},
                              {IF_BW(0,1,4),0,479},
                              {IF_BW(0,1,5),5,562},
                              {IF_BW(0,1,6),5,641},
                              {IF_BW(0,1,7),5,692},
                              {IF_BW(0,0,1),0,752},
                              {IF_BW(0,0,2),0,832},
                              {IF_BW(0,0,3),0,900},
                              {IF_BW(0,0,4),-1,953},
                              {IF_BW(0,0,5),9,1121},
                              {IF_BW(0,0,6),2,1279},
                              {IF_BW(0,0,7),5,1379},
                              {IF_BW(1,1,4),20,1428},
                              {IF_BW(1,1,5),26,1678},
                              {IF_BW(1,1,9),-50,1811},
                              {IF_BW(1,0,15),-100,1915},
                              {IF_BW(1,0,1),20,2251},
                              {IF_BW(1,0,2),22,2488},
                              {IF_BW(1,0,3),21,2693},
                              {IF_BW(1,0,4),15,2849},
                              {IF_BW(1,0,8),-15,3355},
                              {IF_BW(1,0,9),-53,3618},
                              {IF_BW(1,0,10),-15,4202},
                              {IF_BW(1,0,11),-13,4684},
                              {IF_BW(1,0,12),-20,5188},
                              {IF_BW(1,0,13),-14,5770},
                              {IF_BW(1,0,14),-9,6207},


};

const int SI4432_RBW_count = ((int)(sizeof(RBW_choices)/sizeof(RBW_t)));

static pureRSSI_t SI4432_RSSI_correction = float_TO_PURE_RSSI(-120);

uint16_t force_rbw(int i)
{
  SI4432_Write_Byte(SI4432_IF_FILTER_BW, RBW_choices[i].reg);                     // Write RBW settings to Si4432
  SI4432_RSSI_correction = float_TO_PURE_RSSI(RBW_choices[i].RSSI_correction_x_10 - 1200)/10;  // Set RSSI correction
//  SI4432_RSSI_correction = float_TO_PURE_RSSI( - 1200)/10;  // Set RSSI correction
  return RBW_choices[i].RBWx10;                                                   // RBW achieved by Si4432 in kHz * 10
}

uint16_t set_rbw(uint16_t WISH)  {
  int i;
  for (i=0; i < SI4432_RBW_count - 1; i++)
    if (WISH <= RBW_choices[i].RBWx10) 
      break; 
  return force_rbw(i);
}


int SI4432_frequency_changed = false;
int SI4432_offset_changed = false;

// #define __CACHE_BAND__  // Is not reliable!!!!!!

#ifdef __CACHE_BAND__
static int old_freq_band[2] = {-1,-1};
static int written[2]= {0,0};
#endif

void SI4432_Set_Frequency ( freq_t Freq ) {
//  int mode = SI4432_Read_Byte(0x02) & 0x03;           // Disabled as unreliable
//  SI4432_Write_Byte(0x07, 0x02);    // Switch to tune mode

//  Freq = (Freq / 1000 ) * 1000; // force freq to 1000 grid

  uint8_t hbsel;
  if (0) shell_printf("%d: Freq %q\r\n", SI4432_Sel, Freq);
  if (Freq >= 480000000U) {
    hbsel = 1<<5;
    Freq>>=1;
  } else {
    hbsel = 0;
  }
  uint8_t sbsel = 1 << 6;
  uint32_t N = (Freq / config.setting_frequency_10mhz - 24)&0x1F;
  uint32_t K = Freq % config.setting_frequency_10mhz;
  uint32_t Carrier = (K<<2) / 625;
  uint8_t Freq_Band = N | hbsel | sbsel;
//  int count = 0;
//  my_microsecond_delay(200);
//  int s;
//  while (count++ < 100 && ( (s = SI4432_Read_Byte ( 0x02 )) & 0x03 ) != 0) {
//    my_microsecond_delay(100);
//  }

#ifdef __CACHE_BAND__
  if (old_freq_band[SI4432_Sel] == Freq_Band) {
    if (written[SI4432_Sel] < 4) {
      SI4432_Write_Byte ( 0x75, Freq_Band );
      written[SI4432_Sel]++;
    }
    SI4432_Write_Byte(SI4432_FREQCARRIER_H, (Carrier>>8) & 0xFF );
    SI4432_Write_Byte(SI4432_FREQCARRIER_L, Carrier & 0xFF  );
  } else {
#endif
#if 0       // Do not use multi byte write
    SI4432_Write_Byte(SI4432_FREQBAND, Freq_Band);                          // Freq band must be written first !!!!!!!!!!!!
    SI4432_Write_Byte(SI4432_FREQCARRIER_H, (Carrier>>8) & 0xFF );
    SI4432_Write_Byte(SI4432_FREQCARRIER_L, Carrier & 0xFF  );
#else
    SI4432_Write_3_Byte (SI4432_FREQBAND, Freq_Band, (Carrier>>8) & 0xFF, Carrier & 0xFF);
#endif
#ifdef __CACHE_BAND__
    old_freq_band[SI4432_Sel] = Freq_Band;
    written[SI4432_Sel] = 0;
  }
#endif
  SI4432_frequency_changed = true;
//  if (mode == 1)        // RX mode            Disabled as unreliable
//    SI4432_Write_Byte( 0x07, 0x07);
//  else
//    SI4432_Write_Byte( 0x07, 0x0B);
}

//extern int setting.repeat;

#ifdef __FAST_SWEEP__
extern deviceRSSI_t age[POINTS_COUNT];
static int buf_index = 0;
static int buf_end = 0;
static bool  buf_read = false;

#if 0
int SI4432_is_fast_mode(void)
{
  return buf_read;
}
#endif


//--------------------------- Trigger -------------------
// ************** trigger mode if need
// trigger on measure 4 point
#define T_POINTS            4
#define T_LEVEL_UNDEF       (1<<(16-T_POINTS)) // should drop after 4 shifts left
#define T_LEVEL_BELOW       1
#define T_LEVEL_ABOVE       0
// Trigger mask, should have width T_POINTS bit
#define T_DOWN_MASK         (0b0011)           // 2 from up 2 to bottom
#define T_UP_MASK           (0b1100)           // 2 from bottom 2 to up
#define T_LEVEL_CLEAN       ~(1<<T_POINTS)     // cleanup old trigger data

enum { ST_ARMING, ST_WAITING, ST_FILLING };

void SI4432_trigger_fill(int s, uint8_t trigger_lvl, int up_direction, int trigger_mode)
{
  SI4432_Sel = s;
  uint8_t rssi;
  uint16_t sel = SI_nSEL[SI4432_Sel];
  uint32_t t = setting.additional_step_delay_us;
  systime_t measure = chVTGetSystemTimeX();
  int waiting = ST_ARMING;
//  __disable_irq();
  SPI2_CLK_LOW;
  int i = 0;

  register uint16_t t_mode;
  uint16_t data_level = T_LEVEL_UNDEF;
  if (up_direction)
    t_mode = T_UP_MASK;
  else
    t_mode = T_DOWN_MASK;
  do {
    palClearPad(GPIOC, sel);
    shiftOut(SI4432_REG_RSSI);
    if (operation_requested)                        // allow aborting a wait for trigger
      return;                                                           // abort
    // Store data level bitfield (remember only last 2 states)
    // T_LEVEL_UNDEF mode bit drop after 2 shifts
    rssi = shiftIn();
    palSetPad(GPIOC, sel);
    age[i] = rssi;
    i++;
    if (i >= sweep_points)
      i = 0;
    switch (waiting) {
    case ST_ARMING:
      if (i == sweep_points-1) {
        waiting = ST_WAITING;
        setting.measure_sweep_time_us = (chVTGetSystemTimeX() - measure)*100;
      }
      break;
    case ST_WAITING:
#if 1
      if (rssi < trigger_lvl) {
        data_level = ((data_level<<1) | (T_LEVEL_BELOW))&(T_LEVEL_CLEAN);
      } else {
        data_level = ((data_level<<1) | (T_LEVEL_ABOVE))&(T_LEVEL_CLEAN);
      }
#else
      data_level = ((data_level<<1) | (rssi < trigger_lvl ? T_LEVEL_BELOW : T_LEVEL_ABOVE))&(T_LEVEL_CLEAN);
#endif
      if (data_level == t_mode) {  // wait trigger
 //     if (i == 128) {  // wait trigger
        waiting = ST_FILLING;
        switch (trigger_mode) {
        case T_PRE:                // Trigger at the begin of the scan
          buf_index = i;
          goto fill_rest;
          break;
        case T_POST:               // Trigger at the end of the scan
          buf_index = i;
          goto done;
          break;
        case T_MID:                // Trigger in the middle of the scan
          buf_index = i + sweep_points/2;
          if (buf_index >= sweep_points)
            buf_index -= sweep_points;
          break;
        }
      }
      break;
    case ST_FILLING:
      if (i == buf_index)
        goto done;
    }
fill_rest:
    if (t)
      my_microsecond_delay(t);
  }while(1);
done:
  buf_end = buf_index;
  buf_read = true;
}

void SI4432_Fill(int s, int start)
{
  set_SPI_mode(SPI_MODE_SI);
  SI4432_Sel = s;
  uint16_t sel = SI_nSEL[SI4432_Sel];
#if 0
  uint32_t t = calc_min_sweep_time_us(); // Time to delay in uS for all sweep
  if (t < setting.sweep_time_us){
    t = setting.sweep_time_us - t;
    t = t / (sweep_points - 1);          // Now in uS per point
  }
  else
    t = 0;
#endif
  uint32_t t = setting.additional_step_delay_us;
  systime_t measure = chVTGetSystemTimeX();
//  __disable_irq();
#if 1
  SPI2_CLK_LOW;
  int i = start;
  do {
    palClearPad(GPIOC, sel);
    shiftOut(SI4432_REG_RSSI);
    age[i]=(char)shiftIn();
    palSetPad(GPIOC, sel);
    if (++i >= sweep_points) break;
    if (t)
      my_microsecond_delay(t);
  } while(1);
#else
  shiftInBuf(sel, SI4432_REG_RSSI, &age[start], sweep_points - start, t);
#endif
//  __enable_irq();
  setting.measure_sweep_time_us = (chVTGetSystemTimeX() - measure)*100;
  buf_index = start; // Is used to skip 1st entry during level triggering
  buf_end = sweep_points - 1;
  buf_read = true;
}
#endif


pureRSSI_t getSI4432_RSSI_correction(void){
  return SI4432_RSSI_correction;
};

pureRSSI_t SI4432_RSSI(uint32_t i, int s)
{
  (void) i;
  int32_t RSSI_RAW;
  (void) i;
  // SEE DATASHEET PAGE 61
#ifdef USE_SI4463           // Not used!!!!!!!
  if (SI4432_Sel == 2) {
    RSSI_RAW = Si446x_getRSSI();
  } else
#endif
//START_PROFILE
#ifdef __FAST_SWEEP__
  if (buf_read) {
    pureRSSI_t val = DEVICE_TO_PURE_RSSI(age[buf_index++]);
    if (buf_index >= sweep_points)
      buf_index = 0;
    if (buf_index == buf_end)
      buf_read = false;
    return val;
  }
#endif
  SI4432_Sel = s;
  int stepdelay = SI4432_step_delay;
  if (SI4432_frequency_changed) {
    if (stepdelay < MINIMUM_WAIT_FOR_RSSI) {
      stepdelay = MINIMUM_WAIT_FOR_RSSI;
    }
    SI4432_frequency_changed = false;
  } else if (SI4432_offset_changed) {
//    stepdelay = MINIMUM_WAIT_FOR_RSSI + (stepdelay - MINIMUM_WAIT_FOR_RSSI)/8;
    stepdelay = SI4432_offset_delay;
    SI4432_offset_changed = false;
  }
  if (stepdelay)
    my_microsecond_delay(stepdelay);
    // my_microsecond_delay(SI4432_step_delay);
  i = setting.repeat;
  RSSI_RAW  = 0;
  do{
    RSSI_RAW += DEVICE_TO_PURE_RSSI((deviceRSSI_t)SI4432_Read_Byte(SI4432_REG_RSSI));
    if (--i == 0) break;
//    my_microsecond_delay(100);
  }while(1);

  if (setting.repeat > 1)
    RSSI_RAW = RSSI_RAW / setting.repeat;
 //   if (MODE_INPUT(setting.mode) && RSSI_RAW == 0)
 //     SI4432_Init();
#ifdef __SIMULATION__
#error "Fixme!!! add correct simulation in pureRSSI_t type"
  RSSI_RAW = Simulated_SI4432_RSSI(i,s);
#endif
//STOP_PROFILE
  // Serial.println(dBm,2);
  return RSSI_RAW;
}

static uint8_t SI4432_init_script[] =
{
  SI4432_INT_ENABLE1, 0x0,
  SI4432_INT_ENABLE2, 0x0,
  SI4432_CLOCK_RECOVERY_GEARSHIFT, 0x00,
  SI4432_AGC_OVERRIDE, 0x60,
  SI4432_AFC_LOOP_GEARSHIFT_OVERRIDE, 0x00,
  SI4432_AFC_TIMING_CONTROL, 0x02,
  SI4432_CLOCK_RECOVERY_GEARSHIFT, 0x03,
  SI4432_CLOCK_RECOVERY_OFFSET2, 0x01,
  SI4432_CLOCK_RECOVERY_OFFSET1, 0x11,
  SI4432_CLOCK_RECOVERY_OFFSET0, 0x11,
  SI4432_CLOCK_RECOVERY_TIMING_GAIN1, 0x01,
  SI4432_CLOCK_RECOVERY_TIMING_GAIN0, 0x13,
  SI4432_AFC_LIMITER, 0xFF,
  SI4432_DATAACCESS_CONTROL, 0x61, // Disable all packet handling
  SI4432_AGC_OVERRIDE, 0x60, // AGC, no LNA, fast gain increment
  SI4432_GPIO0_CONF, 0x12, // Normal
  SI4432_GPIO1_CONF, 0x15,
  SI4432_GPIO2_CONF, 0x1F
};


void SI4432_Sub_Init(void)
{
  SI4432_Reset();
  uint8_t *p = SI4432_init_script;
  while (*p) {
    uint8_t r = *p++;
    uint8_t v = *p++;
    SI4432_Write_Byte (r,v);
  }
  // IF Filter Bandwidth
//  set_rbw(100) ;

//  SI4432_Receive();

}

#define V0_XTAL_CAPACITANCE 0x64
#define V1_XTAL_CAPACITANCE 0x64



void SI4432_Init()
{
  return;
#if 1

  CS_SI0_LOW;                       // Drop CS so power can be removed
  CS_SI1_LOW;                       // Drop CS so power can be removed
  CS_PE_LOW;                        // low is the default safe state
  SPI1_CLK_LOW;                     // low is the default safe state
  SPI1_SDI_LOW;                     // will be set with any data out

  palClearPad(GPIOA, GPIOA_RF_PWR);  // Drop power
  chThdSleepMilliseconds(10);      // Wait
  palSetPad(GPIOA, GPIOA_RF_PWR);    // Restore power
  CS_SI0_HIGH;                      // And set chip select lines back to inactive
  CS_SI1_HIGH;
  chThdSleepMilliseconds(10);      // Wait
#endif
  SPI1_CLK_LOW;
  //DebugLine("IO set");
  SI4432_Sel = SI4432_RX;
  SI4432_Sub_Init();

  SI4432_Sel = SI4432_LO;
  SI4432_Sub_Init();
//DebugLine("1 init done");
}

void set_calibration_freq(int freq)
{
  SI4432_Sel = SI4432_LO;         //Select Lo module
  if (freq < 0 || freq > 7 ) {
    SI4432_Write_Byte(SI4432_GPIO2_CONF, 0x1F) ; // Set GPIO2 to GND
  } else {
    SI4432_Write_Byte(SI4432_GPIO2_CONF, 0xC0) ; // Set GPIO2 maximumdrive and clock output
    SI4432_Write_Byte(Si4432_UC_OUTPUT_CLOCK, freq & 0x07) ; // Set GPIO2 frequency
  }
}
#endif
#endif

//------------PE4302 -----------------------------------------------
#ifdef __PE4302__

// Comment out this define to use parallel mode PE4302

#define PE4302_en 10

void PE4302_init(void) {
  CS_PE_LOW;
}

#define PE4302_DELAY 100
#if 0
void PE4302_shiftOut(uint8_t val)
{
     uint8_t i;
     SI4432_log(SI4432_Sel);
     SI4432_log(val);
     for (i = 0; i < 8; i++)  {
           if (val & (1 << (7 - i)))
             SPI1_SDI_HIGH;
           else
             SPI1_SDI_LOW;
//           my_microsecond_delay(PE4302_DELAY);
           SPI1_CLK_HIGH;
//           my_microsecond_delay(PE4302_DELAY);
           SPI1_CLK_LOW;
//           my_microsecond_delay(PE4302_DELAY);
     }
}
#endif

static unsigned char old_attenuation = 255;
bool PE4302_Write_Byte(unsigned char DATA )
{
  if (old_attenuation == DATA)
    return false;
//  my_microsecond_delay(PE4302_DELAY);
//  SPI1_CLK_LOW;
//  my_microsecond_delay(PE4302_DELAY);
//  PE4302_shiftOut(DATA);

  set_SPI_mode(SPI_MODE_SI);
  SPI_BR_SET(SI4432_SPI, PE_SPI_SPEED);

  shiftOut(DATA);
//  my_microsecond_delay(PE4302_DELAY);
  CS_PE_HIGH;
//  my_microsecond_delay(PE4302_DELAY);
  CS_PE_LOW;
//  my_microsecond_delay(PE4302_DELAY);
  SPI_BR_SET(SI4432_SPI, SI4432_SPI_SPEED);
  return true;
}

#endif

#if 0
//-----------------SI4432 dummy------------------
void SI4432_Write_Byte(unsigned char ADR, unsigned char DATA ) {}
unsigned char SI4432_Read_Byte(unsigned char ADR) {return ADR;}
float set_rbw(float WISH) {return (WISH > 600.0?600: (WISH<3.0?3:WISH));}
void set_calibration_freq(int p) {}
void SI4432_Set_Frequency(long f) {}
void PE4302_Write_Byte(unsigned char DATA ) {}
void PE4302_init(void) {}
#endif

#ifdef __SIMULATION__
unsigned long seed = 123456789;
extern float actual_rbw;
float myfrand(void)
{
  seed = (unsigned int) (1103515245 * seed + 12345) ;
  return ((float) seed) / 1000000000.0;
}
#define NOISE  ((myfrand()-2) * 2)  // +/- 4 dBm noise
extern int settingAttenuate;

//#define LEVEL(i, f, v) (v * (1-(fabs(f - frequencies[i])/actual_rbw/1000)))

float LEVEL(uint32_t i, freq_t f, int v)
{
  float dv;
  float df = fabs((float)f - (float)i);
  if (df < actual_rbw*1000)
    dv = df/(actual_rbw*1000);
  else
    dv =  1 + 50*(df - actual_rbw*1000)/(actual_rbw*1000);
  return (v - dv - settingAttenuate);
}

float Simulated_SI4432_RSSI(uint32_t i, int s)
{
  SI4432_Sel = s;
  float v = -100 + log10(actual_rbw)*10 + NOISE;
  if(s == 0) {
    v = fmax(LEVEL(i,10000000,-20),v);
    v = fmax(LEVEL(i,20000000,-40),v);
    v = fmax(LEVEL(i,30000000,-30),v);
    v = fmax(LEVEL(i,40000000,-90),v);
  } else {
    v = fmax(LEVEL(i,320000000,-20),v);
    v = fmax(LEVEL(i,340000000,-40),v);
    v = fmax(LEVEL(i,360000000,-30),v);
    v = fmax(LEVEL(i,380000000,-90),v);
  }
  return(v);
}

#endif
//------------------------------- ADF4351 -------------------------------------

#ifdef __ADF4351__

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))

#define CS_ADF0_HIGH     palSetLine(LINE_LO_SEL)
#define CS_ADF1_HIGH     palSetLine(LINE_LO_SEL)

#define CS_ADF0_LOW     palClearLine(LINE_LO_SEL)
#define CS_ADF1_LOW     palClearLine(LINE_LO_SEL)

//uint32_t t_R[6] =  {0x320000, 0x8008011, 0x4E42, 0x4B3,0x8C803C , 0x580005} ;         //25 MHz ref
#ifdef TINYSA4_PROTO
uint32_t registers[6] =  {0xC80000, 0x8008011, 0x1800C642, 0x48963,0xA5003C , 0x580005} ;         //10 MHz ref
#else
uint32_t registers[6] =  {0xA00000, 0x8000011, 0x4E42, 0x4B3,0xDC003C , 0x580005} ;         //10 MHz ref
#endif
int debug = 0;
ioline_t ADF4351_LE[2] = { LINE_LO_SEL, LINE_LO_SEL};
//int ADF4351_Mux = 7;

int ADF4351_frequency_changed = false;

//#define DEBUG(X) // Serial.print( X )
//#define DEBUGLN(X) Serial.println( X )
//#define DEBUGFLN(X,Y) Serial.println( X,Y )
//#define DEBUGF(X,Y) Serial.print( X,Y )
#define DEBUG(X)
#define DEBUGLN(X)

#ifdef TINYSA4_PROTO
#define XTAL    29999960
#else
#define XTAL    26000000
#endif
//double RFout; //Output freq in MHz
uint64_t  PFDRFout[6] = {XTAL,XTAL,XTAL,10000000,10000000,10000000}; //Reference freq in MHz
//uint64_t  Chrystal[6] = {XTAL,XTAL,XTAL,10000000,10000000,10000000};
//double  FRACF; // Temp

volatile int64_t
  INTA,         // Temp
  ADF4350_modulo = 60,          // Linked to spur table!!!!!
  MOD,
  target_freq,
  FRAC; //Temp

uint8_t OutputDivider; // Temp
uint8_t lock=2; //Not used
static int old_R = 0;

// Lock = A4

void ADF4351_Setup(void)
{
//  palSetPadMode(GPIOA, 1, PAL_MODE_OUTPUT_PUSHPULL );
//  palSetPadMode(GPIOA, 2, PAL_MODE_OUTPUT_PUSHPULL );

//  SPI3_CLK_HIGH;
//  SPI3_SDI_HIGH;
  CS_ADF0_HIGH;
//  CS_ADF1_HIGH;

  //  bitSet (registers[2], 17); // R set to 8
//  bitClear (registers[2], 14); // R set to 8

//  while(1) {
//

  ADF4351_R_counter(1);

  ADF4351_CP(0);

  ADF4351_fastlock(1);      // Fastlock enabled
  ADF4351_csr(1);           //Cycle slip enabled

  ADF4351_set_frequency(0,200000000);

  ADF4351_mux(2);   // No led
//    ADF4351_mux(6);   // Show lock on led

//  ADF4351_set_frequency(1,150000000,0);
//  ADF4351_Set(0);
//  ADF4351_Set(1);
//  chThdSleepMilliseconds(1000);
//  }
//  bitSet (registers[2], 17); // R set to 8
//  bitClear (registers[2], 14); // R set to 8
//  for (int i=0; i<6; i++) pinMode(ADF4351_LE[i], OUTPUT);          // Setup pins
//  for (int i=0; i<6; i++) digitalWrite(ADF4351_LE[i], HIGH);
//  pinMode(ADF4351_Mux, INPUT);
//  SPI.begin();                          // Init SPI bus
//  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  //SPI.setDataMode(SPI_MODE0);           // CPHA = 0  Clock positive
  //SPI.setBitOrder(MSBFIRST);
}

void ADF4351_WriteRegister32(int channel, const uint32_t value)
{
  registers[value & 0x07] = value;
  for (int i = 3; i >= 0; i--) shiftOut((value >> (8 * i)) & 0xFF);
  palSetLine(ADF4351_LE[channel]);
  my_microsecond_delay(1);        // Must
  palClearLine(ADF4351_LE[channel]);
}

void ADF4351_Set(int channel)
{
  set_SPI_mode(SPI_MODE_SI);
  SPI_BR_SET(SI4432_SPI, ADF_SPI_SPEED);
//  my_microsecond_delay(1);
  palClearLine(ADF4351_LE[channel]);
//  my_microsecond_delay(1);

  for (int i = 5; i >= 0; i--) {
    ADF4351_WriteRegister32(channel, registers[i]);
  }
  SPI_BR_SET(SI4432_SPI, SI4432_SPI_SPEED);
}

#if 0
void ADF4351_disable_output(void)
{
    bitClear (registers[4], 5); // main output
    ADF4351_Set(0);
}

void ADF4351_enable_output(void)
{
    bitSet (registers[4], 5); // main output
    ADF4351_Set(0);
}
#endif

static freq_t prev_actual_freq = 0;

void ADF4351_force_refresh(void) {
  prev_actual_freq = 0;
}

void ADF4351_modulo(int m)
{
  ADF4350_modulo = m;
//  ADF4351_set_frequency(0, (uint64_t)prev_actual_freq);
}

uint64_t ADF4351_set_frequency(int channel, uint64_t freq)  // freq / 10Hz
{
//  freq -= 71000;
//  SI4463_set_gpio(3,GPIO_HIGH);

//  uint32_t offs = ((freq / 1000)* ( 0) )/ 1000;
  uint32_t offs = 0;
  uint64_t actual_freq = ADF4351_prepare_frequency(channel,freq + offs);
//  SI4463_set_gpio(3,GPIO_LOW);
  if (actual_freq != prev_actual_freq) {
//START_PROFILE;
    ADF4351_frequency_changed = true;
    ADF4351_Set(channel);
    prev_actual_freq = actual_freq;
  }
//STOP_PROFILE;
  return actual_freq;
}

void ADF4351_spur_mode(int S)
{
    if (S & 1) {
      bitSet (registers[2], 29); // R set to 8
    } else {
      bitClear (registers[2], 29); // R set to 8
    }
    if (S & 2)
      bitSet (registers[2], 30); // R set to 8
    else
      bitClear (registers[2], 30); // R set to 8
    ADF4351_Set(0);
}

void ADF4351_R_counter(int R)
{
  if (R == old_R)
    return;
  old_R = R;
  int dbl = false;
      if (R < 0) {
        dbl = true;
        R = -R;
      }
      if (R<1)
        return;
      if (dbl) {
        bitSet (registers[2], 25); // Reference doubler
      } else {
        bitClear (registers[2], 25); // Reference doubler
      }
      for (int channel=0; channel < 6; channel++) {
        PFDRFout[channel] = (config.setting_frequency_30mhz * (dbl?2:1)) / R;
      }
      clear_frequency_cache();                              // When R changes the possible frequencies will change
      registers[2] &= ~ (((unsigned long)0x3FF) << 14);
      registers[2] |= (((unsigned long)R) << 14);
      ADF4351_Set(0);
}

void ADF4351_mux(int R)
{
      registers[2] &= ~ (((unsigned long)0x7) << 26);
      registers[2] |= (((unsigned long)R & (unsigned long)0x07) << 26);
      ADF4351_Set(0);
}

void ADF4351_csr(int c)
{
      registers[3] &= ~ (((unsigned long)0x1) << 18);
      registers[3] |= (((unsigned long)c & (unsigned long)0x01) << 18);
      ADF4351_Set(0);
}

void ADF4351_fastlock(int c)
{
      registers[3] &= ~ (((unsigned long)0x3) << 15);
      registers[3] |= (((unsigned long)c & (unsigned long)0x03) << 15);
      ADF4351_Set(0);
}

void ADF4351_CP(int p)
{
      registers[2] &= ~ (((unsigned long)0xF) << 9);
      registers[2] |= (((unsigned long)p) << 9);
      ADF4351_Set(0);
}

void ADF4351_drive(int p)
{
      registers[4] &= ~ (((unsigned long)0x3) << 3);
      registers[4] |= (((unsigned long)p) << 3);
      ADF4351_Set(0);
}

void ADF4351_aux_drive(int p)
{
      registers[4] &= ~ (((unsigned long)0x3) << 6);
      registers[4] |= (((unsigned long)p) << 6);
      ADF4351_Set(0);
}
#if 0
static uint32_t gcd(uint32_t x, uint32_t y)
{
  uint32_t z;
  while (y != 0) {
    z = x % y;
    x = y;
    y = z;
  }
  return x;
}
#endif

#if 0
#endif

uint64_t ADF4351_prepare_frequency(int channel, uint64_t freq)  // freq / 10Hz
{
  target_freq = freq;
  if (freq >= 2200000000) {
      OutputDivider = 1;
      bitWrite (registers[4], 22, 0);
      bitWrite (registers[4], 21, 0);
      bitWrite (registers[4], 20, 0);
    } else if (freq >= 1100000000) {
      OutputDivider = 2;
      bitWrite (registers[4], 22, 0);
      bitWrite (registers[4], 21, 0);
      bitWrite (registers[4], 20, 1);
    } else if (freq >= 550000000) {
      OutputDivider = 4;
      bitWrite (registers[4], 22, 0);
      bitWrite (registers[4], 21, 1);
      bitWrite (registers[4], 20, 0);
    } else if (freq >= 275000000)  {
      OutputDivider = 8;
      bitWrite (registers[4], 22, 0);
      bitWrite (registers[4], 21, 1);
      bitWrite (registers[4], 20, 1);
    } else {                        // > 137500000
      OutputDivider = 16;
      bitWrite (registers[4], 22, 1);
      bitWrite (registers[4], 21, 0);
      bitWrite (registers[4], 20, 0);
    }

    volatile uint64_t PFDR = PFDRFout[channel];
    INTA = (((uint64_t)freq) * OutputDivider) / PFDR;
    MOD = ADF4350_modulo;
    uint64_t f_int = INTA *(uint64_t) MOD;
    uint64_t f_target = ((((uint64_t)freq) * OutputDivider) * (uint64_t) MOD) / PFDR;
    FRAC = f_target - f_int;
    if (FRAC >= MOD) {
      FRAC -= MOD;
      INTA++;
    }
#if 0
    while (FRAC > 4095 || MOD > 4095) {
      FRAC = FRAC >> 1;
      MOD = MOD >> 1;
 //   Serial.println( "MOD/FRAC reduced");
    }
#endif
#if 0
    uint32_t reduce = gcd(MOD, FRAC);
    if (reduce>1) {
      FRAC /= reduce;
      MOD /= reduce;
      if (MOD == 1)
        MOD=2;
    }
#endif
    uint64_t actual_freq = (PFDR *(INTA * MOD +FRAC))/OutputDivider / MOD;
#if 0
    volatile int max_delta =  PFDRFout[channel]/OutputDivider/MOD/100;
    if (actual_freq < freq - max_delta || actual_freq > freq + max_delta ){
       while(1)
         my_microsecond_delay(10);
    }
    max_delta = freq - actual_freq;
    if (max_delta > 200000 || max_delta < -200000 || freq == 0) {
      while(1)
        my_microsecond_delay(10);
    }
#endif
    if (FRAC >= MOD ){
       while(1)
         my_microsecond_delay(10);
    }

    registers[0] = 0;
    registers[0] = INTA << 15; // OK
    registers[0] = registers[0] + (FRAC << 3);
    if (MOD == 1) MOD = 2;
    registers[1] = 0;
    registers[1] = MOD << 3;
    registers[1] = registers[1] + 1 ; // restore address "001"
    bitSet (registers[1], 27); // Prescaler at 8/9
    return actual_freq;
}

#endif

void ADF4351_enable(int s)
{
  if (s)
    bitClear(registers[4], 11);     // Inverse logic!!!!!
  else
    bitSet(registers[4], 11);
  ADF4351_Set(0);
}

void ADF4351_enable_aux_out(int s)
{
  if (s)
    bitSet(registers[4], 8);
  else
    bitClear(registers[4], 8);
  ADF4351_Set(0);
}

void ADF4351_enable_out(int s)
{
  if (s) {
    bitClear(registers[2], 11);     // Disable VCO power down
    bitClear(registers[2], 5);      // Disable power down
    bitSet(registers[4], 5);        // Enable output
  } else {
    bitClear(registers[4], 5);      // Disable output
    bitSet(registers[2], 5);        // Enable power down
    bitSet(registers[2], 11);        // Enable VCO power down
  }
  ADF4351_Set(0);
}


// ------------------------------ SI4463 -------------------------------------


int SI4463_frequency_changed = false;
int SI4463_offset_changed = false;
int SI4463_offset_value = 0;

static int SI4463_band = -1;
static int64_t SI4463_outdiv = -1;
//static freq_t SI4463_prev_freq = 0;
//static float SI4463_step_size = 100;        // Will be recalculated once used
static uint8_t SI4463_channel = 0;
static uint8_t SI4463_in_tx_mode = false;
int SI4463_R = 5;
static int SI4463_output_level = 0x20;

static si446x_state_t SI4463_get_state(void);
static void SI4463_set_state(si446x_state_t);


#define MIN_DELAY   2
#define my_deleted_delay(T)

#include <string.h>

#define SI4463_READ_CTS       (palReadLine(LINE_RX_CTS))

static int SI4463_wait_for_cts(void)
{
//  set_SPI_mode(SPI_MODE_SI);
  while (!SI4463_READ_CTS) {
//    chThdSleepMicroseconds(100);
    my_microsecond_delay(1);
  }
  return 1;
}

#if 0   // not used
static void SI4463_write_byte(uint8_t ADR, uint8_t DATA)
{
  set_SPI_mode(SPI_MODE_SI);
  SI_CS_LOW;
  ADR |= 0x80 ; // RW = 1
  shiftOut( ADR );
  shiftOut( DATA );
  SI_CS_HIGH;
}

static void SI4463_write_buffer(uint8_t ADR, uint8_t *DATA, int len)
{
  set_SPI_mode(SPI_MODE_SI);
  SI_CS_LOW;
  ADR |= 0x80 ; // RW = 1
  shiftOut( ADR );
  while (len-- > 0)
    shiftOut( *(DATA++) );
  SI_CS_HIGH;
}
#endif

static uint8_t SI4463_read_byte( uint8_t ADR )
{
  uint8_t DATA ;
  set_SPI_mode(SPI_MODE_SI);
  SPI_BR_SET(SI4432_SPI, SI4432_SPI_SPEED);

  SI_CS_LOW;
  shiftOut( ADR );
  DATA = shiftIn();
  SI_CS_HIGH;
  return DATA ;
}

static uint8_t SI4463_get_response(void* buff, uint8_t len)
{
    uint8_t cts = 0;
//    set_SPI_mode(SPI_MODE_SI);
    cts = SI4463_READ_CTS;
    if (!cts) {
      return false;
    }
//    __disable_irq();
    SI_CS_LOW;
    shiftOut( SI446X_CMD_READ_CMD_BUFF );
    cts = (shiftIn() == 0xFF);
    if (cts)
    {
        // Get response data
        for(uint8_t i=0;i<len;i++) {
            ((uint8_t*)buff)[i] = shiftIn();
        }
    }
    SI_CS_HIGH;
//    __enable_irq();
    return cts;
}

static uint8_t SI4463_wait_response(void* buff, uint8_t len, uint8_t use_timeout)
{
  uint16_t timeout = 40000;
  while(!SI4463_get_response(buff, len))
  {
    my_microsecond_delay(1);
    if(use_timeout && !--timeout)
    {
      ((char *)buff)[0] = 1;
      return 0;
    }
  }
  return 1;
}

void SI4463_do_api(void* data, uint8_t len, void* out, uint8_t outLen)
{
  set_SPI_mode(SPI_MODE_SI);
  SPI_BR_SET(SI4432_SPI, SI4432_SPI_SPEED);
#if 0
  if(SI4463_wait_response(NULL, 0, true)) // Make sure it's ok to send a command
#else
  if (SI4463_wait_for_cts())
#endif
    {
//   SPI_BR_SET(SI4432_SPI, SPI_BR_DIV8);

//    __disable_irq();
    SI_CS_LOW;
    for(uint8_t i=0;i<len;i++) {
      shiftOut(((uint8_t*)data)[i]); // (pgm_read_byte(&((uint8_t*)data)[i]));
    }
//    SPI_BR_SET(SI4432_SPI, SPI_BR_DIV8);
    SI_CS_HIGH;
//    __enable_irq();
#if 0
    if(((uint8_t*)data)[0] == SI446X_CMD_IRCAL) // If we're doing an IRCAL then wait for its completion without a timeout since it can sometimes take a few seconds
#if 0
      SI4463_wait_response(NULL, 0, false);
#else
      SI4463_wait_for_cts();
#endif
    else
#endif
#if 0
     if (((uint8_t*)data)[0] == SI446X_CMD_GET_MODEM_STATUS)
      chThdSleepMicroseconds(2000);
#endif
#if 1
    SI4463_wait_for_cts();
#endif
    if(out != NULL) { // If we have an output buffer then read command response into it
//      if (((uint8_t*)data)[0] == SI446X_CMD_GET_MODEM_STATUS)
//        my_microsecond_delay(18);   // Prevent extra wait cycles
      SI4463_wait_response(out, outLen, true);
    }
  }
}

#ifdef notused
static void SI4463_set_properties(uint16_t prop, void* values, uint8_t len)
{
    // len must not be greater than 12

    uint8_t data[16] = {
        SI446X_CMD_SET_PROPERTY,
        (uint8_t)(prop>>8),
        len,
        (uint8_t)prop
    };

    // Copy values into data, starting at index 4
    memcpy(data + 4, values, len);

    SI4463_do_api(data, len + 4, NULL, 0);
}
#endif

#include "SI446x_cmd.h"

#if 0
#include "SI4463_radio_config.h"
static const uint8_t SI4463_config[] = RADIO_CONFIGURATION_DATA_ARRAY;
#endif

#ifdef __SI4468__
#include "radio_config_Si4468_undef.h"
#undef RADIO_CONFIGURATION_DATA_ARRAY
#include "radio_config_Si4468_default.h"

#define GLOBAL_GPIO_PIN_CFG 0x13, 0x07, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00
#define GLOBAL_CLK_CFG 0x11, 0x00, 0x01, 0x01, 0x00

//#undef RF_MODEM_RAW_CONTROL_10                      // Override RSSI averaging
//#define RF_MODEM_RAW_CONTROL_10 0x11, 0x20, 0x0A, 0x45, 0x03, 0x00, 0x00, 0x01, 0x00, 0xFF, 0x06, 0x18, 0x10, 0x40

//#undef RF_MODEM_AGC_CONTROL_1
//#define RF_MODEM_AGC_CONTROL_1 0x11, 0x20, 0x01, 0x35, 0x92             // Override AGC gain increase
#undef RF_MODEM_AGC_CONTROL_1
#define RF_MODEM_AGC_CONTROL_1 0x11, 0x20, 0x01, 0x35, 0xE0 + 0x10 + 0x08 // slow AGC
//#undef RF_MODEM_RSSI_JUMP_THRESH_4
//#define RF_MODEM_RSSI_JUMP_THRESH_4 0x11, 0x20, 0x04, 0x4B, 0x06, 0x09, 0x10, 0x45  // Increase RSSI reported value with 2.5dB

#undef RF_GPIO_PIN_CFG
#define RF_GPIO_PIN_CFG GLOBAL_GPIO_PIN_CFG
#undef RF_GLOBAL_CLK_CFG_1
#define RF_GLOBAL_CLK_CFG_1 GLOBAL_CLK_CFG

// Remember to change RF_MODEM_AFC_LIMITER_1_3_1 !!!!!!!!!

static const uint8_t SI4468_config[] = RADIO_CONFIGURATION_DATA_ARRAY;
#endif

// Set new state
static void SI4463_set_state(si446x_state_t newState)
{
  uint8_t data[] = {
        SI446X_CMD_CHANGE_STATE,
        newState
  };
  SI4463_do_api(data, sizeof(data), NULL, 0);
}

static uint8_t gpio_state[4] = { 7,8,0,0 };

void SI4463_refresh_gpio(void)
{
#ifndef TINYSA4_PROTO               // Force clock to max frequency for ADF
  uint8_t data[] =
  {
    0x11, 0x00, 0x01, 0x01, 0x40 // GLOBAL_CLK_CFG     Enable divided clock
  };
  SI4463_do_api(data, sizeof(data), NULL, 0);
#endif
  uint8_t data2[] =
  {
    0x13, gpio_state[0], gpio_state[1], gpio_state[2], gpio_state[3], 0, 0, 0
  };
  SI4463_do_api(data2, sizeof(data2), NULL, 0);
}

void SI4463_set_gpio(int i, int s)
{
  gpio_state[i] = s;
#if 0   // debug gpio
  gpio_state[2] = 3;
  gpio_state[3] = 2;
#endif
  SI4463_refresh_gpio();
}

#if 0
static void SI4463_clear_FIFO(void)
{
    // 'static const' saves 20 bytes of flash here, but uses 2 bytes of RAM
  static const uint8_t clearFifo[] = {
        SI446X_CMD_FIFO_INFO,
        SI446X_FIFO_CLEAR_RX | SI446X_FIFO_CLEAR_TX
  };
  SI4463_do_api((uint8_t*)clearFifo, sizeof(clearFifo), NULL, 0);
}
#endif

void SI4463_set_output_level(int t)
{
  SI4463_output_level = t;
  if (SI4463_in_tx_mode)
    SI4463_start_tx(0);         // Refresh output level
}
void SI4463_start_tx(uint8_t CHANNEL)
{
//  volatile si446x_state_t s;
#if 0
  s = SI4463_get_state();
  if (s == SI446X_STATE_RX){
    SI4463_set_state(SI446X_STATE_READY);
    my_microsecond_delay(200);
    s = SI4463_get_state();
    if (s != SI446X_STATE_READY){
      my_microsecond_delay(1000);
    }
  }
#endif
#if 1
  {
    uint8_t data[] =
    {
     0x11, 0x20, 0x01, 0x00,
     0x00,  // CW mode
    };
    SI4463_do_api(data, sizeof(data), NULL, 0);
  }
#endif
#if 1
  {
    uint8_t data[] =
    {
     0x11, 0x22, 0x04, 0x00,        // PA_MODE
     0x08,  // Coarse PA mode and class E PA
     (uint8_t)SI4463_output_level,  // Level
     0x00,  // Duty
     0x00   // Ramp
    };
    SI4463_do_api(data, sizeof(data), NULL, 0);
  }
#endif
//  retry:
  {
    uint8_t data[] =
    {
     SI446X_CMD_ID_START_TX,
     CHANNEL,
     0, // Stay in TX state
     0,  // TX len
     0,  // TX len
     0,// TX delay
     0// Num repeat
    };
    SI4463_do_api(data, sizeof(data), NULL, 0);
  }
  SI4463_in_tx_mode = true;
  my_microsecond_delay(1000);
#if 0
  s = SI4463_get_state();
  if (s != SI446X_STATE_TX){
    my_microsecond_delay(1000);
    goto retry;
  }
#endif

}


void SI4463_start_rx(uint8_t CHANNEL)
{
  volatile si446x_state_t s = SI4463_get_state();
  if (s == SI446X_STATE_TX){
    SI4463_set_state(SI446X_STATE_READY);
  }
  SI4463_refresh_gpio();
#if 0
  {
    uint8_t data[] =
    {
     0x11, 0x20, 0x01, 0x00,
     0x0A,  // Restore 2FSK mode
    };
    SI4463_do_api(data, sizeof(data), NULL, 0);
  }
#endif
  uint8_t data[] = {
    SI446X_CMD_ID_START_RX,
    CHANNEL,
    0,
    0,
    0,
    0,// 8,
    0,// SI446X_CMD_START_RX_ARG_NEXT_STATE2_RXVALID_STATE_ENUM_RX,
    0, //SI446X_CMD_START_RX_ARG_NEXT_STATE3_RXINVALID_STATE_ENUM_RX
  };
//retry:
  SI4463_do_api(data, sizeof(data), NULL, 0);

#if 0
  si446x_state_t s = SI4463_get_state();
  if (s != SI446X_STATE_RX) {
    my_microsecond_delay(1000);
    goto retry;
  }
#endif
  SI4463_in_tx_mode = false;
}



void SI4463_short_start_rx(void)
{
  uint8_t data[] = {
    SI446X_CMD_ID_START_RX,
  };
  SI4463_do_api(data, sizeof(data), NULL, 0);
  SI4463_in_tx_mode = false;
}

void SI4463_clear_int_status(void)
{
  uint8_t data[9] = {
    SI446X_CMD_ID_GET_INT_STATUS
  };
  SI4463_do_api(data, 1, data, SI446X_CMD_REPLY_COUNT_GET_INT_STATUS);

}

void set_calibration_freq(int ref)
{
#ifndef TINYSA4_PROTO
  ref = 0;   // <--------------------- DISABLED FOR PROTOTYPE!!!!!!!!!!!!!!!!!!!!!!!!!
#endif

    if (ref >= 0) {
      SI4463_set_gpio(0, 7);                            // GPIO 0 dic clock out

      uint8_t data2[5] = {
                        0x11, 0x00, 0x01, 0x01, 0x40                      // GLOBAL_CLK_CFG Clock config
      };
      data2[4] |= ref<<3;
      SI4463_do_api(data2, 5, NULL, 0);
    } else {
      SI4463_set_gpio(0, 1);                            // stop clock out
    }
}

si446x_info_t SI4463_info;

void Si446x_getInfo(si446x_info_t* info)
{
    uint8_t data[8] = {
        SI446X_CMD_PART_INFO
    };
    SI4463_do_api(data, 1, data, 8);

    info->chipRev   = data[0];
    info->part      = (data[1]<<8) | data[2];
    info->partBuild = data[3];
    info->id        = (data[4]<<8) | data[5];
    info->customer  = data[6];
    info->romId     = data[7];


    data[0] = SI446X_CMD_FUNC_INFO;
    SI4463_do_api(data, 1, data, 6);

    info->revExternal   = data[0];
    info->revBranch     = data[1];
    info->revInternal   = data[2];
    info->patch         = (data[3]<<8) | data[4];
    info->func          = data[5];
}
#ifdef notused
static uint8_t SI4463_get_device_status(void)
{
  uint8_t data[2] =
  {
    SI446X_CMD_ID_REQUEST_DEVICE_STATE, 0
  };
  SI4463_do_api(data, 1, data, SI446X_CMD_REPLY_COUNT_REQUEST_DEVICE_STATE);
  return(data[0]);
}
#endif


// Read a fast response register
uint8_t getFRR(uint8_t reg)
{
  set_SPI_mode(SPI_MODE_SI);
  SPI_BR_SET(SI4432_SPI, SI4432_SPI_SPEED);

  return SI4463_read_byte(reg);
}

// Get current radio state
static si446x_state_t SI4463_get_state(void)
{
#if 0
#if 0
   uint8_t data[2] = {
       SI446X_CMD_REQUEST_DEVICE_STATE
   };
   SI4463_do_api(data, 1, data, 2);
   uint8_t state = data[0] & 0x0F;
#endif
   uint8_t state = SI4463_get_device_status();
#else
again:
   SI4463_wait_for_cts();
   uint8_t state = getFRR(SI446X_CMD_READ_FRR_B);
#endif
   if (state == 255) {
     my_microsecond_delay(100);
     goto again;
   }
   if(state == SI446X_STATE_TX_TUNE)
        state = SI446X_STATE_TX;
    else if(state == SI446X_STATE_RX_TUNE)
        state = SI446X_STATE_RX;
    else if(state == SI446X_STATE_READY2)
        state = SI446X_STATE_READY;
    else
      state = state;
    return (si446x_state_t)state;
}


void set_RSSI_comp(void)
{
  // Set properties:           RF_MODEM_RSSI_COMP_1
  // Number of properties:     1
  // Group ID:                 0x20
  // Start ID:                 0x4E
  // Default values:           0x40,
  // Descriptions:
  //   MODEM_RSSI_JUMP_THRESH - Configures the RSSI Jump Detection threshold.
  //   MODEM_RSSI_CONTROL - Control of the averaging modes and latching time for reporting RSSI value(s).
  //   MODEM_RSSI_CONTROL2 - RSSI Jump Detection control.
  //   MODEM_RSSI_COMP - RSSI compensation value.
  //
  // #define RF_MODEM_RSSI_COMP_1 0x11, 0x20, 0x01, 0x4E, 0x40

  uint8_t data[5] = {
      0x11,
      0x20,
      0x01,
      0x4E,
      0x40
  };
  SI4463_do_api(data, sizeof(data), NULL, 0);

}

int SI4463_offset_active = false;

void si_set_offset(int16_t offset)
{
  // Set properties:           MODEM_FREQ_OFFSET
  // Number of properties:     2
  // Group ID:                 0x20
  // Start ID:                 0x0d
  // Default values:           0x00, 0x00
  // Descriptions:
  //   MODEM_FREQ_OFFSET1 - High byte of the offset
  //   MODEM_FREQ_OFFSET2 - Low byte of the offset
  //
  // #define RF_MODEM_RSSI_COMP_1 0x11, 0x20, 0x01, 0x4E, 0x40
  SI4463_offset_value = offset;
  uint8_t data[] = {
      0x11,
      0x20,
      0x02,
      0x0d,
      (uint8_t) ((offset>>8) & 0xff),
      (uint8_t) ((offset) & 0xff)
  };
  SI4463_do_api(data, sizeof(data), NULL, 0);
  SI4463_offset_changed = true;
  SI4463_offset_active = (offset != 0);
}

void si_fm_offset(int16_t offset)
{
  // Set properties:           MODEM_FREQ_OFFSET
  // Number of properties:     2
  // Group ID:                 0x20
  // Start ID:                 0x0d
  // Default values:           0x00, 0x00
  // Descriptions:
  //   MODEM_FREQ_OFFSET1 - High byte of the offset
  //   MODEM_FREQ_OFFSET2 - Low byte of the offset
  //
  // #define RF_MODEM_RSSI_COMP_1 0x11, 0x20, 0x01, 0x4E, 0x40
  offset = SI4463_offset_value + offset;
  uint8_t data[] = {
      0x11,
      0x20,
      0x02,
      0x0d,
      (uint8_t) ((offset>>8) & 0xff),
      (uint8_t) ((offset) & 0xff)
  };
  SI4463_do_api(data, sizeof(data), NULL, 0);
  SI4463_offset_changed = true;
  SI4463_offset_active = (offset != 0);
}



#ifdef __FAST_SWEEP__
extern deviceRSSI_t age[POINTS_COUNT];
static int buf_index = 0;
static bool  buf_read = false;

void SI446x_Fill(int s, int start)
{
  (void)s;
#if 0
  set_SPI_mode(SPI_MODE_SI);
  SI4432_Sel = s;
  uint16_t sel = SI_nSEL[SI4432_Sel];
#endif

  uint32_t t = setting.additional_step_delay_us;
  systime_t measure = chVTGetSystemTimeX();
  __disable_irq();

#if 1
    int i = start;
  uint8_t data[3];
  do {
again:
    data[0] = SI446X_CMD_GET_MODEM_STATUS;
    data[1] = 0xFF;
    SI4463_do_api(data, 1, data, 3);            // TODO no clear of interrups
    if (data[2] == 0) goto again;
    if (data[2] == 255) goto again;
    age[i]=(char)data[2];
    if (++i >= sweep_points) break;
    if (t)
      my_microsecond_delay(t);
  } while(1);
#else
  shiftInBuf(sel, SI4432_REG_RSSI, &age[start], sweep_points - start, t);
#endif
  __enable_irq();

  setting.measure_sweep_time_us = (chVTGetSystemTimeX() - measure)*100;
  buf_index = start; // Is used to skip 1st entry during level triggering
  buf_read = true;
}
#endif



int16_t Si446x_RSSI(void)
{
#ifdef __FAST_SWEEP__
  if (buf_read) {
    if (buf_index == sweep_points-1)
      buf_read = false;
    return DEVICE_TO_PURE_RSSI(age[buf_index++]);
  }
#endif

  int i = setting.repeat;
  int32_t RSSI_RAW  = 0;
  do{
    //   if (MODE_INPUT(setting.mode) && RSSI_R
    uint8_t data[3] = {
                       SI446X_CMD_GET_MODEM_STATUS,
                       0xFF
    };
    if (SI4432_step_delay && (ADF4351_frequency_changed || SI4463_frequency_changed)) {
      my_microsecond_delay(SI4432_step_delay * ((setting.R == 0 && old_R > 5 ) ? 8 : 1));
      ADF4351_frequency_changed = false;
      SI4463_frequency_changed = false;
      SI4463_offset_changed = false;
    } else if (SI4432_offset_delay && SI4463_offset_changed) {
      my_microsecond_delay(SI4432_offset_delay);
      SI4463_offset_changed = false;
    }
#define SAMPLE_COUNT 1
    int j = SAMPLE_COUNT; //setting.repeat;
    int RSSI_RAW_ARRAY[3];
    do{
    again:
      __disable_irq();
      data[0] = SI446X_CMD_GET_MODEM_STATUS;
      data[1] = 0xFF;
      SI4463_do_api(data, 2, data, 3);          // TODO no clear of interrupts
      __enable_irq();

      if (data[2] == 255) {
        my_microsecond_delay(10);
        goto again;
      }
#if 0
      if (data[2] == 0) {
//        my_microsecond_delay(10);
        goto again;
      }
#endif
      RSSI_RAW_ARRAY[--j] = data[2];
      if (j == 0) break;
//      my_microsecond_delay(20);
    }while(1);
#if SAMPLE_COUNT == 3
    int t;
    if (RSSI_RAW_ARRAY[0] > RSSI_RAW_ARRAY[1]) {
      t = RSSI_RAW_ARRAY[1];
      RSSI_RAW_ARRAY[1] = RSSI_RAW_ARRAY[0];
      RSSI_RAW_ARRAY[0] = t;
    }
    if (RSSI_RAW_ARRAY[1] > RSSI_RAW_ARRAY[2]) {
      t = RSSI_RAW_ARRAY[2];
      RSSI_RAW_ARRAY[2] = RSSI_RAW_ARRAY[1];
      RSSI_RAW_ARRAY[1] = t;
    }
    if (RSSI_RAW_ARRAY[0] > RSSI_RAW_ARRAY[1]) {
      t = RSSI_RAW_ARRAY[1];
      RSSI_RAW_ARRAY[1] = RSSI_RAW_ARRAY[0];
      RSSI_RAW_ARRAY[0] = t;
    }

    RSSI_RAW += DEVICE_TO_PURE_RSSI(RSSI_RAW_ARRAY[1]);
#else
    RSSI_RAW += DEVICE_TO_PURE_RSSI(RSSI_RAW_ARRAY[0]);
#endif
    if (--i <= 0) break;
//    my_microsecond_delay(100);
  }while(1);

  if (setting.repeat > 1)
    RSSI_RAW = RSSI_RAW / setting.repeat;

  return RSSI_RAW;
}



void SI446x_set_AGC_LNA(uint8_t v)
{

  uint8_t data[2] = {
      0xd0,                 // AGC_OVERRIDE
      v
  };
  SI4463_do_api(data, sizeof(data), NULL, 0);
#if 0
  if (v == 0) {
    data[0] = 0xd1;     // Read AGC?????? NO
    SI4463_do_api(data, 1, data, 1);
  }
#endif
}


#ifdef notused
// Do an ADC conversion
static uint16_t getADC(uint8_t adc_en, uint8_t adc_cfg, uint8_t part)
{
    uint8_t data[6] = {
        SI446X_CMD_GET_ADC_READING,
        adc_en,
        adc_cfg
    };
    SI4463_do_api(data, 3, data, 6);
    return (data[part]<<8 | data[part + 1]);
}
#endif

#ifndef __SI4468__
// -------------- 0.2 kHz ----------------------------

#undef RF_MODEM_TX_RAMP_DELAY_8_1
#undef RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12_1
#undef RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12_1
#undef RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12_1

#define RF_MODEM_TX_RAMP_DELAY_12_1 0x11, 0x20, 0x0C, 0x18, 0x01, 0x00, 0x08, 0x03, 0x80, 0x00, 0xF0, 0x10, 0x74, 0xE8, 0x00, 0xA9
// #define RF_MODEM_TX_RAMP_DELAY_8_1 0x11, 0x20, 0x08, 0x18, 0x01, 0x00, 0x08, 0x03, 0x80, 0x00, 0xF0, 0x11
#define RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12_1 0x11, 0x21, 0x0C, 0x00, 0x0C, 0x01, 0xE4, 0xB9, 0x86, 0x55, 0x2B, 0x0B, 0xF8, 0xEF, 0xEF, 0xF2
//#define RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12_1 0x11, 0x21, 0x0C, 0x00, 0xC6, 0xC1, 0xB2, 0x9C, 0x80, 0x63, 0x47, 0x2F, 0x1B, 0x0E, 0x05, 0x00
#define RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12_1 0x11, 0x21, 0x0C, 0x0C, 0xF8, 0xFC, 0x05, 0x00, 0xFF, 0x0F, 0x0C, 0x01, 0xE4, 0xB9, 0x86, 0x55
//#define RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12_1 0x11, 0x21, 0x0C, 0x0C, 0xFF, 0xFE, 0x00, 0x00, 0x00, 0x0F, 0xC6, 0xC1, 0xB2, 0x9C, 0x80, 0x63
#define RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12_1 0x11, 0x21, 0x0C, 0x18, 0x2B, 0x0B, 0xF8, 0xEF, 0xEF, 0xF2, 0xF8, 0xFC, 0x05, 0x00, 0xFF, 0x0F
//#define RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12_1 0x11, 0x21, 0x0C, 0x18, 0x47, 0x2F, 0x1B, 0x0E, 0x05, 0x00, 0xFF, 0xFE, 0x00, 0x00, 0x00, 0x0F

#define RF_MODEM_RAW_SEARCH2_2_1 0x11, 0x20, 0x02, 0x50, 0x94, 0x0A

#define RF_GLOBAL_CONFIG_1_1 0x11, 0x00, 0x01, 0x03, 0x20

uint8_t SI4463_RBW_02kHz[] =
{
 6, RF_MODEM_RAW_SEARCH2_2_1, \
 5, RF_GLOBAL_CONFIG_1_1, \
 0x0C, RF_MODEM_TX_RAMP_DELAY_12_1, \
 0x10, RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12_1, \
 0x10, RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12_1, \
 0x10, RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12_1, \
 0x00
};
// -------------- 1kHz ----------------------------

#undef RF_MODEM_TX_RAMP_DELAY_8_1
#undef RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12_1
#undef RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12_1
#undef RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12_1

#define RF_MODEM_TX_RAMP_DELAY_8_1 0x11, 0x20, 0x08, 0x18, 0x01, 0x00, 0x08, 0x03, 0x80, 0x00, 0xF0, 0x11
#define RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12_1 0x11, 0x21, 0x0C, 0x00, 0xC6, 0xC1, 0xB2, 0x9C, 0x80, 0x63, 0x47, 0x2F, 0x1B, 0x0E, 0x05, 0x00
#define RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12_1 0x11, 0x21, 0x0C, 0x0C, 0xFF, 0xFE, 0x00, 0x00, 0x00, 0x0F, 0xC6, 0xC1, 0xB2, 0x9C, 0x80, 0x63
#define RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12_1 0x11, 0x21, 0x0C, 0x18, 0x47, 0x2F, 0x1B, 0x0E, 0x05, 0x00, 0xFF, 0xFE, 0x00, 0x00, 0x00, 0x0F

uint8_t SI4463_RBW_1kHz[] =
{
 0x0C, RF_MODEM_TX_RAMP_DELAY_8_1, \
 0x10, RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12_1, \
 0x10, RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12_1, \
 0x10, RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12_1, \
 0x00
};

// -------------- 3 kHz ----------------------------

#undef RF_MODEM_TX_RAMP_DELAY_8_1
#undef RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12_1
#undef RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12_1
#undef RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12_1

#define RF_MODEM_TX_RAMP_DELAY_8_1 0x11, 0x20, 0x08, 0x18, 0x01, 0x80, 0x08, 0x03, 0x80, 0x00, 0xF0, 0x11
#define RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12_1 0x11, 0x21, 0x0C, 0x00, 0xCC, 0xA1, 0x30, 0xA0, 0x21, 0xD1, 0xB9, 0xC9, 0xEA, 0x05, 0x12, 0x11
#define RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12_1 0x11, 0x21, 0x0C, 0x0C, 0x0A, 0x04, 0x15, 0xFC, 0x03, 0x00, 0xCC, 0xA1, 0x30, 0xA0, 0x21, 0xD1
#define RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12_1 0x11, 0x21, 0x0C, 0x18, 0xB9, 0xC9, 0xEA, 0x05, 0x12, 0x11, 0x0A, 0x04, 0x15, 0xFC, 0x03, 0x00

uint8_t SI4463_RBW_3kHz[] =
{
 0x0C, RF_MODEM_TX_RAMP_DELAY_8_1, \
 0x10, RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12_1, \
 0x10, RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12_1, \
 0x10, RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12_1, \
 0x00
};

// -------------- 10 kHz ----------------------------

#undef RF_MODEM_TX_RAMP_DELAY_8_1
#undef RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12_1
#undef RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12_1
#undef RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12_1

#define RF_MODEM_TX_RAMP_DELAY_8_1 0x11, 0x20, 0x08, 0x18, 0x01, 0x80, 0x08, 0x03, 0x80, 0x00, 0xB0, 0x20
#define RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12_1 0x11, 0x21, 0x0C, 0x00, 0xCC, 0xA1, 0x30, 0xA0, 0x21, 0xD1, 0xB9, 0xC9, 0xEA, 0x05, 0x12, 0x11
#define RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12_1 0x11, 0x21, 0x0C, 0x0C, 0x0A, 0x04, 0x15, 0xFC, 0x03, 0x00, 0xCC, 0xA1, 0x30, 0xA0, 0x21, 0xD1
#define RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12_1 0x11, 0x21, 0x0C, 0x18, 0xB9, 0xC9, 0xEA, 0x05, 0x12, 0x11, 0x0A, 0x04, 0x15, 0xFC, 0x03, 0x00


uint8_t SI4463_RBW_10kHz[] =
{
 0x0C, RF_MODEM_TX_RAMP_DELAY_8_1, \
 0x10, RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12_1, \
 0x10, RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12_1, \
 0x10, RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12_1, \
 0x00
};

// -------------- 30 kHz ----------------------------

#undef RF_MODEM_TX_RAMP_DELAY_8_1
#undef RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12_1
#undef RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12_1
#undef RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12_1

#define RF_MODEM_TX_RAMP_DELAY_8_1 0x11, 0x20, 0x08, 0x18, 0x01, 0x80, 0x08, 0x03, 0x80, 0x00, 0x30, 0x10
#define RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12_1 0x11, 0x21, 0x0C, 0x00, 0xFF, 0xBA, 0x0F, 0x51, 0xCF, 0xA9, 0xC9, 0xFC, 0x1B, 0x1E, 0x0F, 0x01
#define RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12_1 0x11, 0x21, 0x0C, 0x0C, 0xFC, 0xFD, 0x15, 0xFF, 0x00, 0x0F, 0xFF, 0xBA, 0x0F, 0x51, 0xCF, 0xA9
#define RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12_1 0x11, 0x21, 0x0C, 0x18, 0xC9, 0xFC, 0x1B, 0x1E, 0x0F, 0x01, 0xFC, 0xFD, 0x15, 0xFF, 0x00, 0x0F

uint8_t SI4463_RBW_30kHz[] =
{
 0x0C, RF_MODEM_TX_RAMP_DELAY_8_1, \
 0x10, RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12_1, \
 0x10, RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12_1, \
 0x10, RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12_1, \
 0x00
};

// -------------- 100kHz ----------------------------

#undef RF_MODEM_TX_RAMP_DELAY_8_1
#undef RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12_1
#undef RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12_1
#undef RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12_1

#define RF_MODEM_TX_RAMP_DELAY_8_1 0x11, 0x20, 0x08, 0x18, 0x01, 0x80, 0x08, 0x03, 0x80, 0x00, 0x20, 0x20
#define RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12_1 0x11, 0x21, 0x0C, 0x00, 0xFF, 0xBA, 0x0F, 0x51, 0xCF, 0xA9, 0xC9, 0xFC, 0x1B, 0x1E, 0x0F, 0x01
#define RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12_1 0x11, 0x21, 0x0C, 0x0C, 0xFC, 0xFD, 0x15, 0xFF, 0x00, 0x0F, 0xFF, 0xBA, 0x0F, 0x51, 0xCF, 0xA9
#define RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12_1 0x11, 0x21, 0x0C, 0x18, 0xC9, 0xFC, 0x1B, 0x1E, 0x0F, 0x01, 0xFC, 0xFD, 0x15, 0xFF, 0x00, 0x0F

uint8_t SI4463_RBW_100kHz[] =
{
 0x0C, RF_MODEM_TX_RAMP_DELAY_8_1, \
 0x10, RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12_1, \
 0x10, RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12_1, \
 0x10, RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12_1, \
 0x00
};

// -------------- 300kHz ----------------------------

#undef RF_MODEM_TX_RAMP_DELAY_8_1
#undef RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12_1
#undef RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12_1
#undef RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12_1

#define RF_MODEM_TX_RAMP_DELAY_8_1 0x11, 0x20, 0x08, 0x18, 0x01, 0x80, 0x08, 0x03, 0x80, 0x00, 0x00, 0x20
#define RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12_1 0x11, 0x21, 0x0C, 0x00, 0xCC, 0xA1, 0x30, 0xA0, 0x21, 0xD1, 0xB9, 0xC9, 0xEA, 0x05, 0x12, 0x11
#define RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12_1 0x11, 0x21, 0x0C, 0x0C, 0x0A, 0x04, 0x15, 0xFC, 0x03, 0x00, 0xCC, 0xA1, 0x30, 0xA0, 0x21, 0xD1
#define RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12_1 0x11, 0x21, 0x0C, 0x18, 0xB9, 0xC9, 0xEA, 0x05, 0x12, 0x11, 0x0A, 0x04, 0x15, 0xFC, 0x03, 0x00

uint8_t SI4463_RBW_300kHz[] =
{
 0x0C, RF_MODEM_TX_RAMP_DELAY_8_1, \
 0x10, RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12_1, \
 0x10, RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12_1, \
 0x10, RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12_1, \
 0x00
};



// -------------- 850kHz ----------------------------

#undef RF_MODEM_TX_RAMP_DELAY_8_1
#undef RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12_1
#undef RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12_1
#undef RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12_1

#define RF_MODEM_TX_RAMP_DELAY_8_1 0x11, 0x20, 0x08, 0x18, 0x01, 0x00, 0x08, 0x03, 0x80, 0x00, 0x00, 0x30
#define RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12_1 0x11, 0x21, 0x0C, 0x00, 0xFF, 0xBA, 0x0F, 0x51, 0xCF, 0xA9, 0xC9, 0xFC, 0x1B, 0x1E, 0x0F, 0x01
#define RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12_1 0x11, 0x21, 0x0C, 0x0C, 0xFC, 0xFD, 0x15, 0xFF, 0x00, 0x0F, 0xFF, 0xBA, 0x0F, 0x51, 0xCF, 0xA9
#define RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12_1 0x11, 0x21, 0x0C, 0x18, 0xC9, 0xFC, 0x1B, 0x1E, 0x0F, 0x01, 0xFC, 0xFD, 0x15, 0xFF, 0x00, 0x0F

uint8_t SI4463_RBW_850kHz[] =
{
 0x0C, RF_MODEM_TX_RAMP_DELAY_8_1, \
 0x10, RF_MODEM_CHFLT_RX1_CHFLT_COE13_7_0_12_1, \
 0x10, RF_MODEM_CHFLT_RX1_CHFLT_COE1_7_0_12_1, \
 0x10, RF_MODEM_CHFLT_RX2_CHFLT_COE7_7_0_12_1, \
 0x00
};
#else
// -------------- 0.2 kHz ----------------------------

#include "radio_config_Si4468_undef.h"
#include "radio_config_Si4468_200Hz.h"
#include "radio_config_Si4468_short.h"

static const uint8_t SI4463_RBW_02kHz[] =
    RADIO_CONFIGURATION_DATA_ARRAY;

// -------------- 1kHz ----------------------------

#include "radio_config_Si4468_undef.h"
#include "radio_config_Si4468_1kHz.h"
#include "radio_config_Si4468_short.h"

static const uint8_t SI4463_RBW_1kHz[] =
    RADIO_CONFIGURATION_DATA_ARRAY;

// -------------- 3 kHz ----------------------------
#include "radio_config_Si4468_undef.h"
#include "radio_config_Si4468_3kHz.h"
#include "radio_config_Si4468_short.h"

static const uint8_t SI4463_RBW_3kHz[] =
    RADIO_CONFIGURATION_DATA_ARRAY;

// -------------- 10 kHz ----------------------------
#include "radio_config_Si4468_undef.h"
#include "radio_config_Si4468_10kHz.h"
#include "radio_config_Si4468_short.h"

static const uint8_t SI4463_RBW_10kHz[] =
    RADIO_CONFIGURATION_DATA_ARRAY;

// -------------- 30 kHz ----------------------------
#include "radio_config_Si4468_undef.h"
#include "radio_config_Si4468_30kHz.h"
#include "radio_config_Si4468_short.h"

static const uint8_t SI4463_RBW_30kHz[] =
    RADIO_CONFIGURATION_DATA_ARRAY;

// -------------- 100kHz ----------------------------
#include "radio_config_Si4468_undef.h"
#include "radio_config_Si4468_100kHz.h"
#include "radio_config_Si4468_short.h"

static const uint8_t SI4463_RBW_100kHz[] =
    RADIO_CONFIGURATION_DATA_ARRAY;

// -------------- 300kHz ----------------------------

#include "radio_config_Si4468_undef.h"
#include "radio_config_Si4468_300kHz.h"
#include "radio_config_Si4468_short.h"

static const uint8_t SI4463_RBW_300kHz[] =
    RADIO_CONFIGURATION_DATA_ARRAY;

// -------------- 850kHz ----------------------------

#include "radio_config_Si4468_undef.h"
#include "radio_config_Si4468_850kHz.h"
#include "radio_config_Si4468_short.h"

static const uint8_t SI4463_RBW_850kHz[] =
    RADIO_CONFIGURATION_DATA_ARRAY;

#endif
// User asks for an RBW of WISH, go through table finding the last triple
// for which WISH is greater than the first entry, use those values,
// Return the first entry of the following triple for the RBW actually achieved
#define IF_BW(dwn3, ndec, filset) (((dwn3)<<7)|((ndec)<<4)|(filset))
typedef struct {
  const uint8_t  *reg;                   // IF_BW(dwn3, ndec, filset)
  int16_t   RSSI_correction_x_10;  // Correction * 10
  int16_t RBWx10;                // RBW in kHz
}RBW_t; // sizeof(RBW_t) = 8 bytes

static const RBW_t RBW_choices[] =
{
// BW register    corr  freq
 {SI4463_RBW_02kHz, 0,3},
 {SI4463_RBW_1kHz,  0,10},
 {SI4463_RBW_3kHz,  0,30},
 {SI4463_RBW_10kHz, 0,100},
 {SI4463_RBW_30kHz, 0,300},
 {SI4463_RBW_100kHz,0,1000},
 {SI4463_RBW_300kHz,0,3000},
 {SI4463_RBW_850kHz,0,6000},
};

const int SI4432_RBW_count = ((int)(sizeof(RBW_choices)/sizeof(RBW_t)));

static pureRSSI_t SI4463_RSSI_correction = float_TO_PURE_RSSI(-120);
static int prev_band = -1;

pureRSSI_t getSI4463_RSSI_correction(void){
  return SI4463_RSSI_correction;
};


uint16_t force_rbw(int f)
{
  if (SI4463_in_tx_mode)
    return(0);
  SI4463_set_state(SI446X_STATE_READY);
  const uint8_t *config = RBW_choices[f].reg;
  uint16_t i=0;
  while(config[i] != 0)
  {
    SI4463_do_api((void *)&config[i+1], config[i], NULL, 0);
    i += config[i]+1;
  }
  SI4463_clear_int_status();
  SI4463_short_start_rx();           // This can cause recalibration
  SI4463_wait_for_cts();
  set_RSSI_comp();
//  prev_band = -1;
  SI4463_RSSI_correction = float_TO_PURE_RSSI(RBW_choices[f].RSSI_correction_x_10 - 1200)/10;  // Set RSSI correction
  return RBW_choices[f].RBWx10;                                                   // RBW achieved by SI4463 in kHz * 10
}

uint16_t set_rbw(uint16_t WISH)  {
  int i;
  for (i=0; i < (int)(sizeof(RBW_choices)/sizeof(RBW_t)) - 1; i++)
    if (WISH <= RBW_choices[i].RBWx10)
      break;
  return force_rbw(i);
}


#define Npresc 1    // 0=low / 1=High performance mode

static int refresh_count = 0;

freq_t SI4463_set_freq(freq_t freq)
{
//  SI4463_set_gpio(3,GPIO_HIGH);
  int S = 4 ;               // Approx 100 Hz channels
  SI4463_channel = 0;
  if (freq >= 822000000 && freq <= 1130000000)         {       // 822 to 1130MHz
    SI4463_band = 0;
    SI4463_outdiv = 4;
  } else if (freq >= 411000000 && freq <= 566000000) {    // 411 to  568MHz
    SI4463_band = 2;
    SI4463_outdiv = 8;
  } else if (freq >= 329000000 && freq <= 454000000) {    // 329 to 454MHz
    SI4463_band = 1;
    SI4463_outdiv = 10;
  } else if (freq >= 274000000 && freq <= 378000000) {    // 274 to 378
    SI4463_band = 3;
    SI4463_outdiv = 12;
  } else if (freq >= 137000000 && freq <= 189000000){ // 137 to 189
    SI4463_band = 5;
    SI4463_outdiv = 24;
#if 0                           // Band 4, 6 and 7 do not function
  } else if (freq >= 137000000 && freq <= 189000000){ // 220 to 266
    SI4463_band = 4;
    SI4463_outdiv = 12;
#endif
  } else
    return 0;
  if (SI4463_offset_active) {
    si_set_offset(0);
    SI4463_offset_active = false;
  }
  int32_t R = (freq * SI4463_outdiv) / (Npresc ? 2*config.setting_frequency_30mhz : 4*config.setting_frequency_30mhz) - 1;        // R between 0x00 and 0x7f (127)
  int64_t MOD = 524288; // = 2^19
  int32_t  F = ((freq * SI4463_outdiv*MOD) / (Npresc ? 2*config.setting_frequency_30mhz : 4*config.setting_frequency_30mhz)) - R*MOD;
  freq_t actual_freq = (R*MOD + F) * (Npresc ? 2*config.setting_frequency_30mhz : 4*config.setting_frequency_30mhz)/ SI4463_outdiv/MOD;
  int delta = freq - actual_freq;
  if (delta < -100 || delta > 100 ){
    while(1)
      my_microsecond_delay(10);
  }
  if (F < MOD || F >= MOD*2){
    while(1)
      my_microsecond_delay(10);
  }
  if (false && (SI4463_band == prev_band)) {
    int vco = 2091 + ((((freq / 4 ) * SI4463_outdiv - 850000000)/1000) * 492) / 200000;

    if (SI4463_in_tx_mode) {
      uint8_t data[] = {
                      0x37,
                      (uint8_t) R,                   //  R data[4]
                      (uint8_t) ((F>>16) & 255),     //  F2,F1,F0 data[5] .. data[7]
                      (uint8_t) ((F>> 8) & 255),     //  F2,F1,F0 data[5] .. data[7]
                      (uint8_t) ((F    ) & 255),     //  F2,F1,F0 data[5] .. data[7]
                      (vco>>8) & 0xff,
                      vco & 0xff,
                      0x00,
                      0x32
      };
      SI4463_do_api(data, sizeof(data), NULL, 0);
    } else {

      uint8_t data[] = {
                      0x36,
                      (uint8_t) R,                   //  R data[4]
                      (uint8_t) ((F>>16) & 255),     //  F2,F1,F0 data[5] .. data[7]
                      (uint8_t) ((F>> 8) & 255),     //  F2,F1,F0 data[5] .. data[7]
                      (uint8_t) ((F    ) & 255),     //  F2,F1,F0 data[5] .. data[7]
                      (vco>>8) & 0xff,
                      vco & 0xff
      };
      SI4463_do_api(data, sizeof(data), NULL, 0);
    }
    SI4463_frequency_changed = true;
//    SI4463_set_gpio(3,GPIO_LOW);
    return actual_freq;
  }
#if 0
  static int old_R = -1;    // What about TX/RX switching?
  static int old_F = -1;
  if (old_R == R || old_F == F)
    return;
  old_R = R;
  old_F = f;
#endif
  refresh_count=0;
  SI4463_set_state(SI446X_STATE_READY);
  my_deleted_delay(100);

  /*
  // Set properties:           RF_FREQ_CONTROL_INTE_8
  // Number of properties:     8
  // Group ID:                 0x40
  // Start ID:                 0x00
  // Default values:           0x3C, 0x08, 0x00, 0x00, 0x00, 0x00, 0x20, 0xFF,
  // Descriptions:
  //   FREQ_CONTROL_INTE - Frac-N PLL Synthesizer integer divide number.
  //   FREQ_CONTROL_FRAC_2 - Frac-N PLL fraction number.
  //   FREQ_CONTROL_FRAC_1 - Frac-N PLL fraction number.
  //   FREQ_CONTROL_FRAC_0 - Frac-N PLL fraction number.
  //   FREQ_CONTROL_CHANNEL_STEP_SIZE_1 - EZ Frequency Programming channel step size.
  //   FREQ_CONTROL_CHANNEL_STEP_SIZE_0 - EZ Frequency Programming channel step size.
  //   FREQ_CONTROL_W_SIZE - Set window gating period (in number of crystal reference clock cycles) for counting VCO frequency during calibration.
  //   FREQ_CONTROL_VCOCNT_RX_ADJ - Adjust target count for VCO calibration in RX mode.
   */
  // #define RF_FREQ_CONTROL_INTE_8_1 0x11, 0x40, 0x08, 0x00, 0x41, 0x0D, 0xA9, 0x5A, 0x4E, 0xC5, 0x20, 0xFE
  uint8_t data[] = {
                    0x11, 0x40, 0x08, 0x00,
                    (uint8_t) R,                   //  R data[4]
                    (uint8_t) ((F>>16) & 255),     //  F2,F1,F0 data[5] .. data[7]
                    (uint8_t) ((F>> 8) & 255),     //  F2,F1,F0 data[5] .. data[7]
                    (uint8_t) ((F    ) & 255),     //  F2,F1,F0 data[5] .. data[7]
                    (uint8_t) ((S>> 8) & 255),     //  Step size data[8] .. data[9]
                    (uint8_t) ((S    ) & 255),     //  Step size data[8] .. data[9]
                    0x20,                   // Window gate
                    0xFF                    // Adj count
  };
  SI4463_do_api(data, sizeof(data), NULL, 0);

  if (SI4463_band != prev_band) {
    /*
  // Set properties:           RF_MODEM_CLKGEN_BAND_1
  // Number of properties:     1
  // Group ID:                 0x20
  // Start ID:                 0x51
  // Default values:           0x08,
  // Descriptions:
  //   MODEM_CLKGEN_BAND - Select PLL Synthesizer output divider ratio as a function of frequency band.
     */
    // #define RF_MODEM_CLKGEN_BAND_1 0x11, 0x20, 0x01, 0x51, 0x0A
    uint8_t data2[] = {
                       0x11, 0x20, 0x01, 0x51,
                       0x10 + (uint8_t)(SI4463_band + (Npresc ? 0x08 : 0))           // 0x08 for high performance mode, 0x10 to skip recal
    };
    SI4463_do_api(data2, sizeof(data2), NULL, 0);
//    my_microsecond_delay(30000);
  }




  //  SI4463_clear_int_status();
#if 0
  retry:
#endif
  if (SI4463_in_tx_mode)
    SI4463_start_tx(0);
  else {
    SI4463_start_rx(SI4463_channel);
#if 0
    si446x_state_t s = SI4463_get_state();
    if (s != SI446X_STATE_RX) {
      SI4463_start_rx(SI4463_channel);
      my_microsecond_delay(1000);
#if 1
      si446x_state_t s = SI4463_get_state();
      if (s != SI446X_STATE_RX) {
        my_microsecond_delay(3000);
        goto retry;
      }
#endif
    }
#endif
  }
  SI4463_wait_for_cts();
//  SI4463_set_gpio(3,GPIO_LOW);
  SI4463_frequency_changed = true;
  prev_band = SI4463_band;
  return actual_freq;
}

void SI4463_init_rx(void)
{
// reset:
  SI_SDN_LOW;
  my_microsecond_delay(100);
  SI_SDN_HIGH;
  my_microsecond_delay(1000);
  SI_SDN_LOW;
  my_microsecond_delay(1000);
#ifdef __SI4468__
  for(uint16_t i=0;i<sizeof(SI4468_config);i++)
  {
    SI4463_do_api((void *)&SI4468_config[i+1], SI4468_config[i], NULL, 0);
    i += SI4468_config[i];
  }
#endif
  clear_frequency_cache();
  SI4463_start_rx(SI4463_channel);
#if 0
volatile si446x_state_t s ;

again:
  Si446x_getInfo(&SI4463_info);
// s = SI4463_get_state();
//  SI4463_clear_int_status();
  my_microsecond_delay(15000);
  s = SI4463_get_state();
  if (s != SI446X_STATE_RX) {
    ili9341_drawstring_7x13("Waiting for RX", 50, 200);
    osalThreadSleepMilliseconds(3000);
    goto reset;
  }
  ili9341_drawstring_7x13("Waiting ready     ", 50, 200);
#endif
  prev_band = -1; // 433MHz
}



#if 0
#include "radio_config_Si4468_undef.h"
#include "radio_config_Si4468_tx.h"

static const uint8_t SI4463_config_tx[] =
    RADIO_CONFIGURATION_DATA_ARRAY;

#endif

void SI4463_init_tx(void)
{
#if 0
reset:
  SI_SDN_LOW;
  my_microsecond_delay(100);
  SI_SDN_HIGH;
  my_microsecond_delay(1000);
  SI_SDN_LOW;
  my_microsecond_delay(1000);
#ifdef __SI4468__
  for(uint16_t i=0;i<sizeof(SI4463_config_tx);i++)
  {
    SI4463_do_api((void *)&SI4463_config_tx[i+1], SI4463_config_tx[i], NULL, 0);
    i += SI4463_config_tx[i];
  }
#endif
#endif
  SI4463_start_tx(0);
#if 0
volatile si446x_state_t s ;

again:
  Si446x_getInfo(&SI4463_info);
// s = SI4463_get_state();
//  SI4463_clear_int_status();
  my_microsecond_delay(15000);
  s = SI4463_get_state();
  if (s != SI446X_STATE_RX) {
    ili9341_drawstring_7x13("Waiting for RX", 50, 200);
    osalThreadSleepMilliseconds(3000);
    goto reset;
  }
  ili9341_drawstring_7x13("Waiting ready     ", 50, 200);
#endif
  prev_band = -1; // 433MHz
}

void enable_extra_lna(int s)
{
#ifdef TINYSA4_PROTO
  static int old_extra_lna = -1;
  if (s != old_extra_lna) {
    if (s)
      palSetLine(LINE_LNA);
    else
      palClearLine(LINE_LNA);
    old_extra_lna = s;
  }
#else
  (void)s;
#endif
}

void enable_ultra(int s)
{
#ifdef TINYSA4_PROTO
static int old_ultra = -1;
  if (s != old_ultra) {
    if (s)
      palClearLine(LINE_ULTRA);
    else
      palSetLine(LINE_ULTRA);
    old_ultra = s;
  }
#else
  (void)s;
#endif
}

void enable_rx_output(int s)
{
  if (s)
    SI4463_set_gpio(3,SI446X_GPIO_MODE_DRIVE1);
  else
    SI4463_set_gpio(3,SI446X_GPIO_MODE_DRIVE0);
}

void enable_high(int s)
{
  if (s)
    SI4463_set_gpio(2,SI446X_GPIO_MODE_DRIVE1);
  else
    SI4463_set_gpio(2,SI446X_GPIO_MODE_DRIVE0);
}


#pragma GCC pop_options
