/*
 * Copyright (c) 2019-2020, written by DiSlord dislordlive@gmail.com
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
#include "nanovna.h"
#ifdef TINYSA4
#include "si4432.h"
#endif

#include "spi.h"

// Allow enable DMA for read display data
#ifdef TINYSA4
#define __USE_DISPLAY_DMA_RX__
#endif

// Pin macros for LCD
#ifdef TINYSA4
#define LCD_CS_LOW        palClearPad(GPIO_LCD_CS_PORT, GPIO_LCD_CS)
#define LCD_CS_HIGH       palSetPad(GPIO_LCD_CS_PORT, GPIO_LCD_CS)
#define LCD_RESET_ASSERT  palClearPad(GPIO_LCD_RESET_PORT, GPIO_LCD_RESET)
#define LCD_RESET_NEGATE  palSetPad(GPIO_LCD_RESET_PORT, GPIO_LCD_RESET)
#define LCD_DC_CMD        palClearPad(GPIO_LCD_CD_PORT, GPIO_LCD_CD)
#define LCD_DC_DATA       palSetPad(GPIO_LCD_CD_PORT, GPIO_LCD_CD)
#else
#define LCD_CS_LOW        palClearPad(GPIOB, GPIOB_LCD_CS)
#define LCD_CS_HIGH       palSetPad(GPIOB, GPIOB_LCD_CS)
#define LCD_RESET_ASSERT  palClearPad(GPIOA, GPIOA_LCD_RESET)
#define LCD_RESET_NEGATE  palSetPad(GPIOA, GPIOA_LCD_RESET)
#define LCD_DC_CMD        palClearPad(GPIOB, GPIOB_LCD_CD)
#define LCD_DC_DATA       palSetPad(GPIOB, GPIOB_LCD_CD)
#endif

#define LCD_SPI           SPI1
// Set SPI bus speed for LCD
#define LCD_SPI_SPEED    SPI_BR_DIV2
//Not define if need use some as Tx speed
#ifdef TINYSA4
#define LCD_SPI_RX_SPEED SPI_BR_DIV4
#endif

pixel_t spi_buffer[SPI_BUFFER_SIZE];
// Default foreground & background colors
pixel_t foreground_color = 0;
pixel_t background_color = 0;

// Display width and height definition
#define ILI9341_WIDTH     LCD_WIDTH
#define ILI9341_HEIGHT    LCD_HEIGHT

// Display commands list
#define ILI9341_NOP                        0x00
#define ILI9341_SOFTWARE_RESET             0x01
#define ILI9341_READ_IDENTIFICATION        0x04
#define ILI9341_READ_STATUS                0x09
#define ILI9341_READ_POWER_MODE            0x0A
#define ILI9341_READ_MADCTL                0x0B
#define ILI9341_READ_PIXEL_FORMAT          0x0C
#define ILI9341_READ_IMAGE_FORMAT          0x0D
#define ILI9341_READ_SIGNAL_MODE           0x0E
#define ILI9341_READ_SELF_DIAGNOSTIC       0x0F
#define ILI9341_SLEEP_IN                   0x10
#define ILI9341_SLEEP_OUT                  0x11
#define ILI9341_PARTIAL_MODE_ON            0x12
#define ILI9341_NORMAL_DISPLAY_MODE_ON     0x13
#define ILI9341_INVERSION_OFF              0x20
#define ILI9341_INVERSION_ON               0x21
#define ILI9341_GAMMA_SET                  0x26
#define ILI9341_DISPLAY_OFF                0x28
#define ILI9341_DISPLAY_ON                 0x29
#define ILI9341_COLUMN_ADDRESS_SET         0x2A
#define ILI9341_PAGE_ADDRESS_SET           0x2B
#define ILI9341_MEMORY_WRITE               0x2C
#define ILI9341_COLOR_SET                  0x2D
#define ILI9341_MEMORY_READ                0x2E
#define ILI9341_PARTIAL_AREA               0x30
#define ILI9341_VERTICAL_SCROLLING_DEF     0x33
#define ILI9341_TEARING_LINE_OFF           0x34
#define ILI9341_TEARING_LINE_ON            0x35
#define ILI9341_MEMORY_ACCESS_CONTROL      0x36
#define ILI9341_VERTICAL_SCROLLING         0x37
#define ILI9341_IDLE_MODE_OFF              0x38
#define ILI9341_IDLE_MODE_ON               0x39
#define ILI9341_PIXEL_FORMAT_SET           0x3A
#define ILI9341_WRITE_MEMORY_CONTINUE      0x3C
#define ILI9341_READ_MEMORY_CONTINUE       0x3E
#define ILI9341_SET_TEAR_SCANLINE          0x44
#define ILI9341_GET_SCANLINE               0x45
#define ILI9341_WRITE_BRIGHTNESS           0x51
#define ILI9341_READ_BRIGHTNESS            0x52
#define ILI9341_WRITE_CTRL_DISPLAY         0x53
#define ILI9341_READ_CTRL_DISPLAY          0x54
#define ILI9341_WRITE_CA_BRIGHTNESS        0x55
#define ILI9341_READ_CA_BRIGHTNESS         0x56
#define ILI9341_WRITE_CA_MIN_BRIGHTNESS    0x5E
#define ILI9341_READ_CA_MIN_BRIGHTNESS     0x5F
#define ILI9341_READ_ID1                   0xDA
#define ILI9341_READ_ID2                   0xDB
#define ILI9341_READ_ID3                   0xDC
#define ILI9341_RGB_INTERFACE_CONTROL      0xB0
#define ILI9341_FRAME_RATE_CONTROL_1       0xB1
#define ILI9341_FRAME_RATE_CONTROL_2       0xB2
#define ILI9341_FRAME_RATE_CONTROL_3       0xB3
#define ILI9341_DISPLAY_INVERSION_CONTROL  0xB4
#define ILI9341_BLANKING_PORCH_CONTROL     0xB5
#define ILI9341_DISPLAY_FUNCTION_CONTROL   0xB6
#define ILI9341_ENTRY_MODE_SET             0xB7
#define ILI9341_BACKLIGHT_CONTROL_1        0xB8
#define ILI9341_BACKLIGHT_CONTROL_2        0xB9
#define ILI9341_BACKLIGHT_CONTROL_3        0xBA
#define ILI9341_BACKLIGHT_CONTROL_4        0xBB
#define ILI9341_BACKLIGHT_CONTROL_5        0xBC
#define ILI9341_BACKLIGHT_CONTROL_7        0xBE
#define ILI9341_BACKLIGHT_CONTROL_8        0xBF
#define ILI9341_POWER_CONTROL_1            0xC0
#define ILI9341_POWER_CONTROL_2            0xC1
#define ILI9341_VCOM_CONTROL_1             0xC5
#define ILI9341_VCOM_CONTROL_2             0xC7
#define ILI9341_POWERA                     0xCB
#define ILI9341_POWERB                     0xCF
#define ILI9341_NV_MEMORY_WRITE            0xD0
#define ILI9341_NV_PROTECTION_KEY          0xD1
#define ILI9341_NV_STATUS_READ             0xD2
#define ILI9341_READ_ID4                   0xD3
#define ILI9341_POSITIVE_GAMMA_CORRECTION  0xE0
#define ILI9341_NEGATIVE_GAMMA_CORRECTION  0xE1
#define ILI9341_DIGITAL_GAMMA_CONTROL_1    0xE2
#define ILI9341_DIGITAL_GAMMA_CONTROL_2    0xE3
#define ILI9341_DTCA                       0xE8
#define ILI9341_DTCB                       0xEA
#define ILI9341_POWER_SEQ                  0xED
#define ILI9341_3GAMMA_EN                  0xF2
#define ILI9341_INTERFACE_CONTROL          0xF6
#define ILI9341_PUMP_RATIO_CONTROL         0xF7

//
// ILI9341_MEMORY_ACCESS_CONTROL registers
//
#define ILI9341_MADCTL_MY  0x80
#define ILI9341_MADCTL_MX  0x40
#define ILI9341_MADCTL_MV  0x20
#define ILI9341_MADCTL_ML  0x10
#define ILI9341_MADCTL_BGR 0x08
#define ILI9341_MADCTL_MH  0x04
#define ILI9341_MADCTL_RGB 0x00

#define DISPLAY_ROTATION_270   (ILI9341_MADCTL_MX | ILI9341_MADCTL_BGR)
#define DISPLAY_ROTATION_90    (ILI9341_MADCTL_MY | ILI9341_MADCTL_BGR)
#define DISPLAY_ROTATION_0     (ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR)
#define DISPLAY_ROTATION_180   (ILI9341_MADCTL_MX | ILI9341_MADCTL_MY  \
                              | ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR)

//*****************************************************
// SPI DMA settings and data
//*****************************************************
#ifdef __USE_DISPLAY_DMA__
static const stm32_dma_stream_t *dmatx =
    STM32_DMA_STREAM(STM32_SPI_SPI1_TX_DMA_STREAM);
static const uint32_t txdmamode =
    STM32_DMA_CR_CHSEL(SPI1_TX_DMA_CHANNEL)         // Select SPI1 Tx DMA
    | STM32_DMA_CR_PL(STM32_SPI_SPI1_DMA_PRIORITY)  // Set priority
    | STM32_DMA_CR_DIR_M2P;                         // Memory to Spi

// Not handle interrupt
#if 0
static void spi_lld_serve_tx_interrupt(SPIDriver *spip, uint32_t flags)
{
  (void)spip;
  (void)flags;
}
#endif

#ifdef __USE_DISPLAY_DMA_RX__
static const stm32_dma_stream_t  *dmarx = STM32_DMA_STREAM(STM32_SPI_SPI1_RX_DMA_STREAM);
static const uint32_t rxdmamode =
    STM32_DMA_CR_CHSEL(SPI1_RX_DMA_CHANNEL)         // Select SPI1 Rx DMA
    | STM32_DMA_CR_PL(STM32_SPI_SPI1_DMA_PRIORITY)  // Set priority
    | STM32_DMA_CR_DIR_P2M;                         // SPI to Memory

// Not handle interrupt
#if 0
static void spi_lld_serve_rx_interrupt(SPIDriver *spip, uint32_t flags)
{
  (void)spip;
  (void)flags;
}
#endif
#endif

// Send prepared DMA data, and wait completion
static void dmaStreamFlush(uint32_t len)
{
  while (len) {
    // DMA data transfer limited by 65535
    uint16_t tx_size = len > 65535 ? 65535 : len;
    dmaStreamSetTransactionSize(dmatx, tx_size);
    dmaStreamEnable(dmatx);
    len -= tx_size;
    dmaWaitCompletion(dmatx);
  }
}
#endif

// SPI transmit byte to SPI (no wait complete transmit)
void spi_TxByte(uint8_t data) {
  SPI_WRITE_8BIT(LCD_SPI, data);
}

// Transmit word to SPI bus (if SPI in 8 bit mode LSB send first!!!!!)
void spi_TxWord(uint16_t data) {
  SPI_WRITE_16BIT(LCD_SPI, data);
}

// Transmit buffer to SPI bus  (len should be > 0)
void spi_TxBuffer(uint8_t *buffer, uint16_t len) {
  do {
    while (SPI_TX_IS_NOT_EMPTY(LCD_SPI));
    SPI_WRITE_8BIT(LCD_SPI, *buffer++);
  }while(--len);
}

// Receive byte from SPI bus
uint8_t spi_RxByte(void) {
  // Start RX clock (by sending data)
  SPI_WRITE_8BIT(LCD_SPI, 0xFF);
  while (SPI_RX_IS_EMPTY(LCD_SPI)||SPI_IS_BUSY(LCD_SPI));
  return SPI_READ_8BIT(LCD_SPI);
}

// Receive buffer from SPI bus (len should be > 0)
void spi_RxBuffer(uint8_t *buffer, uint16_t len) {
  do{
    SPI_WRITE_8BIT(LCD_SPI, 0xFF);
    while (SPI_RX_IS_EMPTY(LCD_SPI));
    *buffer++ = SPI_READ_8BIT(LCD_SPI);
  }while(--len);
}

void spi_DropRx(void){
  // Drop Rx buffer after tx and wait tx complete
  while (SPI_RX_IS_NOT_EMPTY(LCD_SPI)||SPI_IS_BUSY(LCD_SPI))
    (void)SPI_READ_8BIT(LCD_SPI);
}

#ifdef __USE_DISPLAY_DMA__
// SPI receive byte buffer use DMA
void spi_DMATxBuffer(uint8_t *buffer, uint16_t len) {
  dmaStreamSetMemory0(dmatx, buffer);
  dmaStreamSetMode(dmatx, txdmamode | STM32_DMA_CR_PSIZE_BYTE | STM32_DMA_CR_MSIZE_BYTE | STM32_DMA_CR_MINC);
  dmaStreamFlush(len);
}
#ifdef __USE_DISPLAY_DMA_RX__
#if 0 // Not used
// SPI transmit byte buffer use DMA
static void spi_DMARxBuffer(uint8_t *buffer, uint16_t len) {
  uint8_t dummy_tx = 0xFF;
  // Init Rx DMA buffer, size, mode (spi and mem data size is 8 bit)
  dmaStreamSetMemory0(dmarx, buffer);
  dmaStreamSetTransactionSize(dmarx, len);
  dmaStreamSetMode(dmarx, rxdmamode | STM32_DMA_CR_PSIZE_BYTE | STM32_DMA_CR_MSIZE_BYTE | STM32_DMA_CR_MINC);
  // Init dummy Tx DMA (for rx clock), size, mode (spi and mem data size is 8 bit)
  dmaStreamSetMemory0(dmatx, &dummy_tx);
  dmaStreamSetTransactionSize(dmatx, len);
  dmaStreamSetMode(dmatx, txdmamode | STM32_DMA_CR_PSIZE_BYTE | STM32_DMA_CR_MSIZE_BYTE);
  // Skip SPI rx buffer
  spi_DropRx();
  // Start DMA exchange
  dmaStreamEnable(dmarx);
  dmaStreamEnable(dmatx);
  // Wait DMA completion
  dmaWaitCompletion(dmatx);
  dmaWaitCompletion(dmarx);
}
#endif  // 0
#endif  // __USE_DISPLAY_DMA_RX__
#endif  // __USE_DISPLAY_DMA__

static void spi_init(void)
{
  rccEnableSPI1(FALSE);
  LCD_SPI->CR1 = 0;
  LCD_SPI->CR1 = SPI_CR1_MSTR      // SPI is MASTER
               | SPI_CR1_SSM       // Software slave management (The external NSS pin is free for other application uses)
               | SPI_CR1_SSI       // Internal slave select (This bit has an effect only when the SSM bit is set. Allow use NSS pin as I/O)
               | LCD_SPI_SPEED     // Baud rate control
//             | SPI_CR1_CPHA      // Clock Phase
//             | SPI_CR1_CPOL      // Clock Polarity
                 ;

  LCD_SPI->CR2 = SPI_CR2_8BIT      // SPI data size, set to 8 bit
               | SPI_CR2_FRXTH;    // SPI_SR_RXNE generated every 8 bit data
//             | SPI_CR2_SSOE;     //

#ifdef __USE_DISPLAY_DMA__
  // Tx DMA init
  dmaStreamAllocate(dmatx, STM32_SPI_SPI1_IRQ_PRIORITY, NULL, NULL);
  dmaStreamSetPeripheral(dmatx, &LCD_SPI->DR);
  LCD_SPI->CR2|= SPI_CR2_TXDMAEN;    // Tx DMA enable
#ifdef __USE_DISPLAY_DMA_RX__
  // Rx DMA init
  dmaStreamAllocate(dmarx, STM32_SPI_SPI1_IRQ_PRIORITY, NULL, NULL);
  dmaStreamSetPeripheral(dmarx, &LCD_SPI->DR);
  // Enable DMA on SPI
  LCD_SPI->CR2|= SPI_CR2_RXDMAEN;   // Rx DMA enable
#endif
#endif
  LCD_SPI->CR1|= SPI_CR1_SPE;       //SPI enable
}

#ifdef TINYSA4
static uint16_t current_spi_mode;
void set_SPI_mode(uint16_t mode){
  if (current_spi_mode == mode) return;
  switch(current_spi_mode){
    case SPI_MODE_LCD:
    break;
    case SPI_MODE_SD_CARD:
    break;
    case SPI_MODE_SI:
      stop_SI4432_SPI_mode();
    break;
  }
  switch(mode){
    case SPI_MODE_LCD:
    break;
    case SPI_MODE_SD_CARD:
    break;
    case SPI_MODE_SI:
      LCD_CS_HIGH;
      start_SI4432_SPI_mode();
    break;
  }
  current_spi_mode = mode;
}
#endif

// Disable inline for this function
static void send_command(uint8_t cmd, uint8_t len, const uint8_t *data)
{
// Uncomment on low speed SPI (possible get here before previous tx complete)
//  while (SPI_IN_TX_RX(LCD_SPI));
#ifdef TINYSA4
  set_SPI_mode(SPI_MODE_LCD);
#endif
  LCD_CS_LOW;
  LCD_DC_CMD;
  SPI_WRITE_8BIT(LCD_SPI, cmd);
  // Need wait transfer complete and set data bit
  while (SPI_IN_TX_RX(LCD_SPI))
    ;
  // Send command data (if need)
  LCD_DC_DATA;
  while (len-- > 0) {
    while (SPI_TX_IS_NOT_EMPTY(LCD_SPI))
      ;
    SPI_WRITE_8BIT(LCD_SPI, *data++);
  }
  //LCD_CS_HIGH;
}

#ifdef TINYSA4
static const uint8_t ST7796S_init_seq[] = {
  // SW reset
  ILI9341_SOFTWARE_RESET, 0,
  // display off
  ILI9341_DISPLAY_OFF, 0,
  // Interface Mode Control
  ILI9341_RGB_INTERFACE_CONTROL, 1, 0x00,
  // Frame Rate
  ILI9341_FRAME_RATE_CONTROL_1, 1, 0xA,
  // Display Inversion Control , 2 Dot
  ILI9341_DISPLAY_INVERSION_CONTROL, 1, 0x02,
  // RGB/MCU Interface Control
  ILI9341_DISPLAY_FUNCTION_CONTROL, 3, 0x02, 0x02, 0x3B,
  // EntryMode
  ILI9341_ENTRY_MODE_SET, 1, 0xC6,
  // Power Control 1
  ILI9341_POWER_CONTROL_1, 2, 0x17, 0x15,
  // Power Control 2
  ILI9341_POWER_CONTROL_2, 1, 0x41,
  // VCOM Control
//ILI9341_VCOM_CONTROL_1, 3, 0x00, 0x4D, 0x90,
  ILI9341_VCOM_CONTROL_1, 3, 0x00, 0x12, 0x80,
  // Memory Access
  ILI9341_MEMORY_ACCESS_CONTROL, 1, 0x28,  // landscape, BGR
//  ILI9341_MEMORY_ACCESS_CONTROL, 1, 0x20,  // landscape, RGB
  // Interface Pixel Format,	16bpp DPI and DBI and
  ILI9341_PIXEL_FORMAT_SET, 1, 0x55,
  // P-Gamma
//  ILI9341_POSITIVE_GAMMA_CORRECTION, 15, 0x00, 0x03, 0x09, 0x08, 0x16, 0x0A, 0x3F, 0x78, 0x4C, 0x09, 0x0A, 0x08, 0x16, 0x1A, 0x0F,
  // N-Gamma
//  ILI9341_NEGATIVE_GAMMA_CORRECTION, 15, 0x00, 0X16, 0X19, 0x03, 0x0F, 0x05, 0x32, 0x45, 0x46, 0x04, 0x0E, 0x0D, 0x35, 0x37, 0x0F,
  //Set Image Func
//  0xE9, 1, 0x00,
  // Set Brightness to Max
  ILI9341_WRITE_BRIGHTNESS, 1, 0xFF,
  // Adjust Control
  ILI9341_PUMP_RATIO_CONTROL, 4, 0xA9, 0x51, 0x2C, 0x82,
  //Exit Sleep
  ILI9341_SLEEP_OUT, 0x00,
  // display on
  ILI9341_DISPLAY_ON, 0,
  0 // sentinel
};
#else
static const uint8_t ili9341_init_seq[] = {
  // cmd, len, data...,
  // SW reset
  ILI9341_SOFTWARE_RESET, 0,
  // display off
  ILI9341_DISPLAY_OFF, 0,
  // Power control B
  ILI9341_POWERB, 3, 0x00, 0xC1, 0x30,
  // Power on sequence control
  ILI9341_POWER_SEQ, 4, 0x64, 0x03, 0x12, 0x81,
  // Driver timing control A
  ILI9341_DTCA, 3, 0x85, 0x00, 0x78,
  // Power control A
  ILI9341_POWERA, 5, 0x39, 0x2C, 0x00, 0x34, 0x02,
  // Pump ratio control
  ILI9341_PUMP_RATIO_CONTROL, 1, 0x20,
  // Driver timing control B
  ILI9341_DTCB, 2, 0x00, 0x00,
  // POWER_CONTROL_1
  ILI9341_POWER_CONTROL_1, 1, 0x23,
  // POWER_CONTROL_2
  ILI9341_POWER_CONTROL_2, 1, 0x10,
  // VCOM_CONTROL_1
  ILI9341_VCOM_CONTROL_1, 2, 0x3e, 0x28,
  // VCOM_CONTROL_2
  ILI9341_VCOM_CONTROL_2, 1, 0xBE,
  // MEMORY_ACCESS_CONTROL
  //ILI9341_MEMORY_ACCESS_CONTROL, 1, 0x48, // portlait
  ILI9341_MEMORY_ACCESS_CONTROL, 1, DISPLAY_ROTATION_0, // landscape
  // COLMOD_PIXEL_FORMAT_SET : 16 bit pixel
  ILI9341_PIXEL_FORMAT_SET, 1, 0x55,
  // Frame Rate
  ILI9341_FRAME_RATE_CONTROL_1, 2, 0x00, 0x18,
  // Gamma Function Disable
  ILI9341_3GAMMA_EN, 1, 0x00,
  // gamma set for curve 01/2/04/08
  ILI9341_GAMMA_SET, 1, 0x01,
  // positive gamma correction
  ILI9341_POSITIVE_GAMMA_CORRECTION, 15, 0x0F,  0x31,  0x2B,  0x0C,  0x0E,  0x08,  0x4E,  0xF1,  0x37,  0x07,  0x10,  0x03,  0x0E, 0x09,  0x00,
  // negativ gamma correction
  ILI9341_NEGATIVE_GAMMA_CORRECTION, 15, 0x00,  0x0E,  0x14,  0x03,  0x11,  0x07,  0x31,  0xC1,  0x48,  0x08,  0x0F,  0x0C,  0x31, 0x36,  0x0F,
  // Column Address Set
//ILI9341_COLUMN_ADDRESS_SET, 4, 0x00, 0x00, 0x01, 0x3f, // width 320
  // Page Address Set
//ILI9341_PAGE_ADDRESS_SET, 4, 0x00, 0x00, 0x00, 0xef,   // height 240
  // entry mode
  ILI9341_ENTRY_MODE_SET, 1, 0x06,
  // display function control
  ILI9341_DISPLAY_FUNCTION_CONTROL, 3, 0x08, 0x82, 0x27,
  // Interface Control (set WEMODE=0)
  ILI9341_INTERFACE_CONTROL, 3, 0x00, 0x00, 0x00,
  // sleep out
  ILI9341_SLEEP_OUT, 0,
  // display on
  ILI9341_DISPLAY_ON, 0,
  0 // sentinel
};

#endif

void ili9341_init(void)
{
  spi_init();
  LCD_DC_DATA;
  LCD_RESET_ASSERT;
  chThdSleepMilliseconds(10);
  LCD_RESET_NEGATE;
  const uint8_t *p;
#ifdef TINYSA4
  p = ST7796S_init_seq;
#else
  p = ili9341_init_seq;
#endif
  while (*p) {
    send_command(p[0], p[1], &p[2]);
    p += 2 + p[1];
    chThdSleepMilliseconds(5);
  }
#ifdef TINYSA4
  LCD_CS_HIGH;
#endif
}

static void ili9341_setWindow(int x, int y, int w, int h){
//uint8_t xx[4] = { x >> 8, x, (x+w-1) >> 8, (x+w-1) };
//uint8_t yy[4] = { y >> 8, y, (y+h-1) >> 8, (y+h-1) };
  uint32_t xx = __REV16(x | ((x + w - 1) << 16));
  uint32_t yy = __REV16(y | ((y + h - 1) << 16));
  send_command(ILI9341_COLUMN_ADDRESS_SET, 4, (uint8_t *)&xx);
  send_command(ILI9341_PAGE_ADDRESS_SET, 4, (uint8_t *)&yy);
}

#if 0
// Test code for palette mode
void ili9341_bulk_8bit(int x, int y, int w, int h, pixel_t *palette)
{
  ili9341_setWindow(x, y ,w, h);
  send_command(ILI9341_MEMORY_WRITE, 0, NULL);

  uint8_t *buf = (uint8_t *)spi_buffer;
  int32_t len = w * h;
  while (len-- > 0)
    spi_TxWord(palette[*buf++]);
//  LCD_CS_HIGH;
}
#endif

#if DISPLAY_CELL_BUFFER_COUNT != 1
#define LCD_BUFFER_1    0x01
#define LCD_DMA_RUN     0x02
static uint8_t LCD_dma_status = 0;
#endif

pixel_t *ili9341_get_cell_buffer(void){
#if DISPLAY_CELL_BUFFER_COUNT == 1
  return spi_buffer;
#else
  return &spi_buffer[(LCD_dma_status&LCD_BUFFER_1) ? SPI_BUFFER_SIZE/2 : 0];
#endif
}

#ifndef __USE_DISPLAY_DMA__
void ili9341_fill(int x, int y, int w, int h, pixel_t color)
{
  ili9341_setWindow(x, y ,w, h);
  send_command(ILI9341_MEMORY_WRITE, 0, NULL);
  uint32_t len = w * h;
  do {
    while (SPI_TX_IS_NOT_EMPTY(LCD_SPI))
      ;
    SPI_WRITE_16BIT(LCD_SPI, color);
  }while(--len);
#ifdef __REMOTE_DESKTOP__
  if (auto_capture) {
     send_region("fill", x,y,w,h);
     send_buffer((uint8_t *)&background_color, 2);
  }
#endif
}

void ili9341_bulk(int x, int y, int w, int h)
{
  ili9341_setWindow(x, y ,w, h);
  send_command(ILI9341_MEMORY_WRITE, 0, NULL);
  spi_TxBuffer((uint8_t *)spi_buffer, w * h * sizeof(pixel_t));
#ifdef __REMOTE_DESKTOP__
  if (auto_capture) {
     send_region("bulk", x,y,w,h);
     send_buffer((uint8_t *)buffer, w *h * sizeof(pixel_t));
  }
#endif
}

void ili9341_bulk_continue(int x, int y, int w, int h){
  ili9341_bulk(x, y, w, h);
}

void ili9341_bulk_finish(void){
  while (SPI_IS_BUSY(LCD_SPI));      // Wait tx
}

#else
//
// Use DMA for send data
//
// Fill region by some color
void ili9341_fill(int x, int y, int w, int h)
{
  ili9341_setWindow(x, y ,w, h);
  send_command(ILI9341_MEMORY_WRITE, 0, NULL);

  dmaStreamSetMemory0(dmatx, &background_color);
  dmaStreamSetMode(dmatx, txdmamode | STM32_DMA_CR_PSIZE_HWORD | STM32_DMA_CR_MSIZE_HWORD);
#ifdef __REMOTE_DESKTOP__
  if (auto_capture) {
     send_region("fill", x, y, w, h);
     send_buffer((uint8_t *)&background_color, sizeof(pixel_t));
  }
#endif
  dmaStreamFlush(w * h);
}

void ili9341_bulk_finish(void){
  dmaWaitCompletion(dmatx);        // Wait DMA
  while (SPI_IN_TX_RX(LCD_SPI));   // Wait tx
}

static void ili9341_DMA_bulk(int x, int y, int w, int h, pixel_t *buffer){
  ili9341_setWindow(x, y ,w, h);
  send_command(ILI9341_MEMORY_WRITE, 0, NULL);

  dmaStreamSetMemory0(dmatx, buffer);
  dmaStreamSetMode(dmatx, txdmamode | STM32_DMA_CR_PSIZE_HWORD | STM32_DMA_CR_MSIZE_HWORD | STM32_DMA_CR_MINC);
  dmaStreamSetTransactionSize(dmatx, w * h);
  dmaStreamEnable(dmatx);
#ifdef __REMOTE_DESKTOP__
  if (auto_capture) {
     send_region("bulk", x, y, w, h);
     send_buffer((uint8_t *)buffer, w *h * sizeof(pixel_t));
  }
#endif
}

// Copy spi_buffer to region
void ili9341_bulk(int x, int y, int w, int h)
{
  ili9341_DMA_bulk(x, y ,w, h, spi_buffer);  // Send data
  ili9341_bulk_finish();                     // Wait
}
#endif

// Copy part of spi_buffer to region, no wait completion after if buffer count !=1
void ili9341_bulk_continue(int x, int y, int w, int h)
{
#if DISPLAY_CELL_BUFFER_COUNT == 1
  ili9341_bulk(x, y, w, h);
#else
  ili9341_bulk_finish();                                    // Wait DMA
  ili9341_DMA_bulk(x, y , w, h, ili9341_get_cell_buffer()); // Send new cell data
  LCD_dma_status^=LCD_BUFFER_1;                             // Switch buffer
#endif
}

#ifndef __USE_DISPLAY_DMA_RX__

void ili9341_read_memory(int x, int y, int w, int h, int len, pixel_t *out)
{
  ili9341_setWindow(x, y ,w, h);
  send_command(ILI9341_MEMORY_READ, 0, NULL);
  // Skip data from rx buffer
  spi_DropRx();
  // Set read speed (if need different)
#ifdef LCD_SPI_RX_SPEED
  SPI_BR_SET(LCD_SPI, LCD_SPI_RX_SPEED);
#endif
  // require 8bit dummy clock
  spi_RxByte();
#ifdef TINYSA4
  // receive pixel data to buffer
  spi_RxBuffer((uint8_t *)out, len * 2);
#else
  while (len-- > 0) {
    uint8_t r, g, b;
    // read data is always 18bit
    r = spi_RxByte();
    g = spi_RxByte();
    b = spi_RxByte();
    *out++ = RGB565(r, g, b);
  }
#endif
  // restore speed if need
#ifdef LCD_SPI_RX_SPEED
  SPI_BR_SET(LCD_SPI, LCD_SPI_SPEED);
#endif
  LCD_CS_HIGH;
}

#else
// Copy screen data to buffer
// Warning!!! buffer size must be greater then 3*len + 1 bytes
void ili9341_read_memory(int x, int y, int w, int h, int len, pixel_t *out)
{
  uint16_t dummy_tx = 0;
  uint8_t *rgbbuf = (uint8_t *)out;
#ifdef TINYSA4
  uint16_t data_size = len * 2;
  //uint8_t xx[4] = { x >> 8, x, (x+w-1) >> 8, (x+w-1) };
  //uint8_t yy[4] = { y >> 8, y, (y+h-1) >> 8, (y+h-1) };
  uint32_t xx = __REV16(x | ((x + w - 1) << 16));
  uint32_t yy = __REV16(y | ((y + h - 1) << 16));
  send_command(ILI9341_COLUMN_ADDRESS_SET, 4, (uint8_t *)&xx);
  send_command(ILI9341_PAGE_ADDRESS_SET, 4, (uint8_t *)&yy);
#else
  uint16_t data_size = len * 3;

  ili9341_setWindow(x, y ,w, h);
#endif
  send_command(ILI9341_MEMORY_READ, 0, NULL);
  
  // Init Rx DMA buffer, size, mode (spi and mem data size is 8 bit)
  dmaStreamSetMemory0(dmarx, rgbbuf);
  dmaStreamSetTransactionSize(dmarx, data_size);
  dmaStreamSetMode(dmarx, rxdmamode | STM32_DMA_CR_PSIZE_BYTE | STM32_DMA_CR_MSIZE_BYTE | STM32_DMA_CR_MINC);
  // Init dummy Tx DMA (for rx clock), size, mode (spi and mem data size is 8 bit)
  dmaStreamSetMemory0(dmatx, &dummy_tx);
  dmaStreamSetTransactionSize(dmatx, data_size);
  dmaStreamSetMode(dmatx, txdmamode | STM32_DMA_CR_PSIZE_BYTE | STM32_DMA_CR_MSIZE_BYTE);
  // Skip SPI rx buffer
  spi_DropRx();
  // Set read speed (if need different)
#ifdef LCD_SPI_RX_SPEED
  SPI_BR_SET(LCD_SPI, LCD_SPI_RX_SPEED);
	#endif
  // require 8bit dummy clock
  spi_RxByte();
  // Start DMA exchange
  dmaStreamEnable(dmarx);
  dmaStreamEnable(dmatx);
  // Wait DMA completion
  dmaWaitCompletion(dmatx);
  dmaWaitCompletion(dmarx);
  // restore speed if need
#ifdef LCD_SPI_RX_SPEED
  SPI_BR_SET(LCD_SPI, LCD_SPI_SPEED);
#endif
  LCD_CS_HIGH;
#ifndef TINYSA4
  // Parce recived data
  while (len-- > 0) {
    uint8_t r, g, b;
    // read data is always 18bit
    r = rgbbuf[0];
    g = rgbbuf[1];
    b = rgbbuf[2];
    *out++ = RGB565(r, g, b);
    rgbbuf += 3;
  }
#endif
}
#endif

void ili9341_clear_screen(void)
{
  ili9341_fill(0, 0, ILI9341_WIDTH, ILI9341_HEIGHT);
}

#ifndef ili9341_set_foreground
void ili9341_set_foreground(uint16_t fg_idx)
{
  if (fg_idx >= 32)
    foreground_color = fg_idx;
  else
    foreground_color = GET_PALTETTE_COLOR(fg_idx);
}
#endif

#ifndef ili9341_set_background
void ili9341_set_background(uint16_t bg_idx)
{
//  if (bg_idx >= 32) bg_idx = 0;
  background_color = GET_PALTETTE_COLOR(bg_idx);
}
#endif

void ili9341_set_rotation(uint8_t r)
{
  //  static const uint8_t rotation_const[]={DISPLAY_ROTATION_0, DISPLAY_ROTATION_90,
  //  DISPLAY_ROTATION_180, DISPLAY_ROTATION_270};
  send_command(ILI9341_MEMORY_ACCESS_CONTROL, 1, &r);
}

static uint8_t bit_align = 0;
void ili9341_blitBitmap(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                         const uint8_t *b)
{
  uint16_t *buf = spi_buffer;
  uint8_t bits = 0;
  for (uint16_t c = 0; c < height; c++) {
    for (uint16_t r = 0; r < width; r++) {
      if ((r&7) == 0) bits = *b++;
      *buf++ = (0x80 & bits) ? foreground_color : background_color;
      bits <<= 1;
    }
    if (bit_align) b+=bit_align;
  }
  ili9341_bulk(x, y, width, height);
}

void ili9341_drawchar(uint8_t ch, int x, int y)
{
  ili9341_blitBitmap(x, y, FONT_GET_WIDTH(ch), FONT_GET_HEIGHT, FONT_GET_DATA(ch));
}

void ili9341_drawstring(const char *str, int x, int y)
{
  int x_pos = x;
  while (*str) {
    uint8_t ch = *str++;
    if (ch == '\n') {x = x_pos; y+=FONT_STR_HEIGHT; continue;}
    const uint8_t *char_buf = FONT_GET_DATA(ch);
    uint16_t w = FONT_GET_WIDTH(ch);
    ili9341_blitBitmap(x, y, w, FONT_GET_HEIGHT, char_buf);
    x += w;
  }
}

void ili9341_drawstring_7x13(const char *str, int x, int y)
{
  int x_pos = x;
  while (*str) {
    uint8_t ch = *str++;
    if (ch == '\n') {x = x_pos; y+=bFONT_STR_HEIGHT; continue;}
    const uint8_t *char_buf = bFONT_GET_DATA(ch);
    uint16_t w = bFONT_GET_WIDTH(ch);
    ili9341_blitBitmap(x, y, w, bFONT_GET_HEIGHT, char_buf);
    x += w;
  }
}

void ili9341_drawstring_10x14(const char *str, int x, int y)
{
#ifdef wFONT_GET_DATA
  int x_pos = x;
  while (*str) {
    uint8_t ch = *str++;
    if (ch == '\n') {x = x_pos; y+=wFONT_STR_HEIGHT; continue;}
    const uint8_t *char_buf = wFONT_GET_DATA(ch);
    uint16_t w = wFONT_GET_WIDTH(ch);
    bit_align = (w<=8) ? 1 : 0;
    ili9341_blitBitmap(x, y, w, wFONT_GET_HEIGHT, char_buf);
    x += w;
  }
  bit_align = 0;
#else
  ili9341_drawstring_size(str, x, y, 2);
#endif
}

void ili9341_drawstringV(const char *str, int x, int y)
{
  ili9341_set_rotation(DISPLAY_ROTATION_270);
  ili9341_drawstring(str, ILI9341_HEIGHT-y, x);
  ili9341_set_rotation(DISPLAY_ROTATION_0);
}
#ifndef wFONT_GET_DATA
int ili9341_drawchar_size(uint8_t ch, int x, int y, uint8_t size)
{
  uint16_t *buf = spi_buffer;
  const uint8_t *char_buf = FONT_GET_DATA(ch);
  uint16_t w = FONT_GET_WIDTH(ch);
  for (int c = 0; c < FONT_GET_HEIGHT; c++, char_buf++) {
    for (int i = 0; i < size; i++) {
      uint8_t bits = *char_buf;
      for (int r = 0; r < w; r++, bits <<= 1)
        for (int j = 0; j < size; j++)
          *buf++ = (0x80 & bits) ? foreground_color : background_color;
    }
  }
  ili9341_bulk(x, y, w * size, FONT_GET_HEIGHT * size);
  return w*size;
}

void ili9341_drawstring_size(const char *str, int x, int y, uint8_t size)
{
  while (*str)
    x += ili9341_drawchar_size(*str++, x, y, size);
}
#endif

void ili9341_drawfont(uint8_t ch, int x, int y)
{
  ili9341_blitBitmap(x, y, NUM_FONT_GET_WIDTH, NUM_FONT_GET_HEIGHT,
                       NUM_FONT_GET_DATA(ch));
}

#if 0
static void ili9341_pixel(int x, int y, uint16_t color)
{
  uint32_t xx = __REV16(x|((x)<<16));
  uint32_t yy = __REV16(y|((y)<<16));
  send_command(ILI9341_COLUMN_ADDRESS_SET, 4, (uint8_t*)&xx);
  send_command(ILI9341_PAGE_ADDRESS_SET, 4, (uint8_t*)&yy);
  send_command(ILI9341_MEMORY_WRITE, 2, &color);
}
#endif

#define SWAP(x, y) { int z = x; x = y; y = z; }

void ili9341_line(int x0, int y0, int x1, int y1)
{
  SWAP(foreground_color, background_color);
#if 0
  // modified Bresenham's line algorithm, see https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
  int dx = x1 - x0, sx = 1; if (dx < 0) {dx = -dx; sx = -1;}
  int dy = y1 - y0, sy = 1; if (dy < 0) {dy = -dy; sy = -1;}
  int err = (dx > dy ? dx : -dy) / 2;
  while (1) {
    ili9341_fill(x0, y0, 1, 1);
    if (x0 == x1 && y0 == y1)
      break;
    int e2 = err;
    if (e2 > -dx) { err -= dy; x0 += sx; }
    if (e2 <  dy) { err += dx; y0 += sy; }
  }
#else
  if (x0 > x1) {
    SWAP(x0, x1);
    SWAP(y0, y1);
  }

  while (x0 <= x1) {
    int dx = x1 - x0 + 1;
    int dy = y1 - y0;
    if (dy >= 0) {
      dy++;
      if (dy > dx) {
        dy /= dx; dx = 1;
      } else {
        dx /= dy; dy = 1;
      }
    } else {
      dy--;
      if (-dy > dx) {
        dy /= dx; dx = 1;
      } else {
        dx /= -dy;dy = -1;
      }
    }
    if (dy > 0)
      ili9341_fill(x0, y0, dx, dy);
    else
      ili9341_fill(x0, y0+dy, dx, -dy);
    x0 += dx;
    y0 += dy;
  }
#endif
  SWAP(foreground_color, background_color);
}

#if 0
static const uint16_t colormap[] = {
  RGBHEX(0x00ff00), RGBHEX(0x0000ff), RGBHEX(0xff0000),
  RGBHEX(0x00ffff), RGBHEX(0xff00ff), RGBHEX(0xffff00)
};

void ili9341_test(int mode)
{
  int x, y;
  int i;
  switch (mode) {
    default:
#if 1
    ili9341_fill(0, 0, LCD_WIDTH, LCD_HEIGHT, 0);
    for (y = 0; y < LCD_HEIGHT; y++) {
      ili9341_fill(0, y, LCD_WIDTH, 1, RGB(LCD_HEIGHT-y, y, (y + 120) % 256));
    }
    break;
    case 1:
      ili9341_fill(0, 0, LCD_WIDTH, LCD_HEIGHT, 0);
      for (y = 0; y < LCD_HEIGHT; y++) {
        for (x = 0; x < LCD_WIDTH; x++) {
          ili9341_pixel(x, y, (y<<8)|x);
        }
      }
      break;
    case 2:
      //send_command16(0x55, 0xff00);
      ili9341_pixel(64, 64, 0xaa55);
    break;
#endif
#if 1
    case 3:
      for (i = 0; i < 10; i++)
        ili9341_drawfont(i, i*20, 120);
    break;
#endif
#if 0
    case 4:
      draw_grid(10, 8, 29, 29, 15, 0, 0xffff, 0);
    break;
#endif
    case 4:
      ili9341_line(0, 0, 15, 100);
      ili9341_line(0, 0, 100, 100);
      ili9341_line(0, 15, 100, 0);
      ili9341_line(0, 100, 100, 0);
    break;
  }
}
#endif
