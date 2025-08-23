/****************************************************************************
 * boards/arm/stm32h7/weact-stm32h750/src/stm32_w25q_spi.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include <errno.h>
#include <debug.h>
#include <string.h>
#include <stdlib.h>

#include <nuttx/arch.h>
#include <nuttx/board.h>
#include <nuttx/fs/fs.h>
#include <nuttx/spi/spi.h>

#include <arch/board/board.h>

#include <nuttx/mtd/mtd.h>
#include <nuttx/drivers/drivers.h>
#include <nuttx/drivers/ramdisk.h>

#ifdef CONFIG_FS_NXFFS
#include <nuttx/fs/nxffs.h>
#endif

#ifdef CONFIG_FS_SMARTFS
#include <nuttx/fs/smart.h>
#endif

#ifdef CONFIG_FS_LITTLEFS
// #include <nuttx/fs/littlefs/littlefs/lfs.h>
#endif

#include "weact-stm32h750.h"

#include "stm32_spi.h"

#ifdef CONFIG_MTD_W25QXXXJV

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32_w25qspi_setup
 *
 * Description:
 *   This function is called by board-bringup logic to configure the
 *   SPI flash device.
 *
 * Returned Value:
 *   Zero is returned on success.  Otherwise, a negated errno value is
 *   returned to indicate the nature of the failure.
 *
 ****************************************************************************/

int stm32_w25qspi_setup(void)
{
  struct spi_dev_s *spi_dev;
  struct mtd_dev_s *mtd_dev;
  int ret = -1;

  /* Get the SPI1 device */

  spi_dev = stm32_spibus_initialize(1);
  if (!spi_dev)
    {
      _err("ERROR: Failed to initialize SPI1\n");
      return -1;
    }

  /* Set SPI frequency to 24MHz for W25Q64 compatibility
   * W25Q64 requires more dummy clocks above 26MHz
   * SPI1 on STM32H7 uses PCLK2 (120MHz), so we need to limit frequency
   */

  SPI_SETFREQUENCY(spi_dev, 24000000);

  /* Wake up the W25Q flash from power-down mode */
  /* Send Release from Power-down (0xAB) command to ensure flash is awake */

  SPI_LOCK(spi_dev, true);
  SPI_SELECT(spi_dev, SPIDEV_FLASH(0), true);
  SPI_SEND(spi_dev, 0xAB);  /* Release from power-down command */
  /* Wait for the flash to wake up (typical 3us, max 30us according to datasheet) */
  up_udelay(50);  /* 50us delay to be safe */
  SPI_SELECT(spi_dev, SPIDEV_FLASH(0), false);
  SPI_LOCK(spi_dev, false);

  /* Additional delay to ensure flash is fully ready */
  up_mdelay(1);

  /* Test basic SPI communication before initializing driver */
  uint8_t manufacturer_id;
  SPI_LOCK(spi_dev, true);
  SPI_SELECT(spi_dev, SPIDEV_FLASH(0), true);
  SPI_SEND(spi_dev, 0x90);  /* Read Manufacturer/Device ID command */
  SPI_SEND(spi_dev, 0x00);  /* Dummy address bytes */
  SPI_SEND(spi_dev, 0x00);
  SPI_SEND(spi_dev, 0x00);
  manufacturer_id = SPI_SEND(spi_dev, 0xFF);  /* Read manufacturer ID */
  SPI_SELECT(spi_dev, SPIDEV_FLASH(0), false);
  SPI_LOCK(spi_dev, false);

  syslog(LOG_INFO, "Flash Manufacturer ID: 0x%02x (expected 0xEF for Winbond)\n", manufacturer_id);
  
  /* If we can't read a valid manufacturer ID, don't try to initialize the driver */
  if (manufacturer_id != 0xEF)
    {
      if (manufacturer_id == 0xFF || manufacturer_id == 0x00)
        {
          syslog(LOG_ERR, "ERROR: Cannot communicate with SPI flash (ID: 0x%02x), skipping initialization\n", manufacturer_id);
        }
      else
        {
          syslog(LOG_WARNING, "WARNING: Unexpected manufacturer ID 0x%02x, skipping flash driver initialization\n", manufacturer_id);
        }
      return -ENODEV;
    }

  syslog(LOG_INFO, "Valid W25Q flash detected, proceeding with MTD driver initialization\n");

  /* Read full JEDEC ID to see exactly what the flash reports */
  uint8_t jedec_id[3];
  SPI_LOCK(spi_dev, true);
  SPI_SELECT(spi_dev, SPIDEV_FLASH(0), true);
  SPI_SEND(spi_dev, 0x9F);  /* Read JEDEC ID command */
  jedec_id[0] = SPI_SEND(spi_dev, 0xFF);  /* Manufacturer ID */
  jedec_id[1] = SPI_SEND(spi_dev, 0xFF);  /* Memory Type */
  jedec_id[2] = SPI_SEND(spi_dev, 0xFF);  /* Capacity */
  SPI_SELECT(spi_dev, SPIDEV_FLASH(0), false);
  SPI_LOCK(spi_dev, false);
  
  syslog(LOG_INFO, "Full JEDEC ID: %02X%02X%02X (Mfg: 0x%02X, MemType: 0x%02X, Capacity: 0x%02X)\n", 
         jedec_id[0], jedec_id[1], jedec_id[2], jedec_id[0], jedec_id[1], jedec_id[2]);
  
  /* Check against expected W25 driver values:
   * Manufacturer: 0xEF (W25_JEDEC_WINBOND)
   * Memory Type: 0x30, 0x40, 0x50, 0x60, 0x70 (various W25Q types)
   * Capacity: 0x17 (W25_JEDEC_CAPACITY_64MBIT)
   */

  /* Initialize W25Q flash driver */

  syslog(LOG_INFO, "Calling w25_initialize() with SPI device...\n");
  mtd_dev = w25_initialize(spi_dev);
  if (!mtd_dev)
    {
      syslog(LOG_ERR, "ERROR: w25_initialize() failed for SPI1!\n");
      syslog(LOG_ERR, "This could be due to SPI communication issues during driver initialization\n");
      return -ENODEV;
    }

  syslog(LOG_INFO, "W25Q flash driver initialized successfully!\n");

  ret = register_mtddriver("/dev/mtdblock0", mtd_dev, 0755, NULL);
  if (ret < 0)
    {
      _err("ERROR: Failed to register MTD driver: %d\n", ret);
      return ret;
    }

  return 0;
}

#endif
