/*
 * Copyright (c) 2014-2015, TAKAHASHI Tomohiro (TTRFTECH) edy555@gmail.com
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
#include <string.h>

static void flash_wait_for_last_operation(void)
{
  while (FLASH->SR == FLASH_SR_BSY) {
    //WWDG->CR = WWDG_CR_T;
  }
//  return FLASH->SR;
}

static inline void flash_unlock(void)
{
  // unlock sequence
  FLASH->KEYR = 0x45670123;
  FLASH->KEYR = 0xCDEF89AB;
}

static inline void flash_erase_page0(uint32_t page_address)
{
  flash_wait_for_last_operation();
  FLASH->CR |= FLASH_CR_PER;
  FLASH->AR = page_address;
  FLASH->CR |= FLASH_CR_STRT;
  flash_wait_for_last_operation();
  FLASH->CR &= ~FLASH_CR_PER;
}

static void flash_erase_pages(uint32_t page_address, uint32_t size)
{
  // Unlock for erase
  flash_unlock();

  chSysLock();
  // erase flash pages
  size+=page_address;
  for (; page_address < size; page_address+=FLASH_PAGESIZE)
    flash_erase_page0(page_address);
  chSysUnlock();
}

static void flash_program_half_word_buffer(uint16_t* dst, uint16_t *data, uint16_t size)
{
  uint32_t i;
  // unlock, and erase flash pages for buffer (aligned to FLASH_PAGESIZE)
  flash_erase_pages((uint32_t)dst, size);
  // Save buffer
  __IO uint16_t* p = dst;
  for (i = 0; i < size/sizeof(uint16_t); i++){
    flash_wait_for_last_operation();
    FLASH->CR |= FLASH_CR_PG;
    p[i] = data[i];
    flash_wait_for_last_operation();
    FLASH->CR &= ~FLASH_CR_PG;
  }
}

static uint32_t
checksum(const void *start, size_t len)
{
  uint32_t *p = (uint32_t*)start;
  uint32_t *tail = (uint32_t*)(start + len);
  uint32_t value = 0;
  while (p < tail)
    value = __ROR(value, 31) + *p++;
  return value;
}

int
config_save(void)
{
  config.magic = CONFIG_MAGIC;
  config.checksum = checksum(&config, sizeof config - sizeof config.checksum);

  // write to flash
  flash_program_half_word_buffer((uint16_t*)SAVE_CONFIG_ADDR, (uint16_t*)&config, sizeof(config_t));
  return 0;
}

int
config_recall(void)
{
  const config_t *src = (const config_t*)SAVE_CONFIG_ADDR;
  void *dst = &config;

  if (src->magic != CONFIG_MAGIC || checksum(src, sizeof *src - sizeof src->checksum) != src->checksum)
    return -1;

  // duplicated saved data onto sram to be able to modify marker/trace
  memcpy(dst, src, sizeof(config_t));
  return 0;
}

int
caldata_save(uint16_t id)
{
  if (id >= SAVEAREA_MAX)
    return -1;

  // Apply magic word and calculate checksum
  setting.magic = CONFIG_MAGIC;
  setting.checksum = checksum(&setting, sizeof setting - sizeof setting.checksum);

  uint16_t *dst = (uint16_t*)(SAVE_PROP_CONFIG_ADDR + id * SAVE_PROP_CONFIG_SIZE);
  flash_program_half_word_buffer(dst, (uint16_t*)&setting, sizeof(setting));

  return 0;
}

int
caldata_recall(uint16_t id)
{
  setting_t *src;
  void *dst = &setting;

  if (id >= SAVEAREA_MAX)
    return -1;

  // point to saved area on the flash memory
  src = (setting_t*)(SAVE_PROP_CONFIG_ADDR + id * SAVE_PROP_CONFIG_SIZE);

  if (src->magic != CONFIG_MAGIC || checksum(src, sizeof setting - sizeof src->checksum) != src->checksum)
    return -1;

  // duplicated saved data onto sram to be able to modify marker/trace
  memcpy(dst, src, sizeof(setting_t));
  // Restore stored trace
  memcpy(stored_t, &src[1], sizeof(stored_t));
  update_min_max_freq();
  update_frequencies();
  set_scale(setting.scale);
  set_reflevel(setting.reflevel);
  return 0;
}

void
clear_all_config_prop_data(void)
{
  // unlock and erase flash pages
  flash_erase_pages(SAVE_CONFIG_ADDR, SAVE_CONFIG_AREA_SIZE);
}

