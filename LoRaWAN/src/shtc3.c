/*
 * SHTC3 - Sensirion Humidity and Temperature Sensor Code
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <stdint.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>
#include "shtc3.h"

int8_t i2c_write_short(const struct device *i2c_dev, uint8_t address, uint8_t command, uint16_t byte)
{
    int ret;
    uint8_t data[3];

    data[0] = command;
    data[1] = (byte & 0xFF00) >> 8;
    data[2] = byte & 0xFF;

    ret = i2c_write(i2c_dev, (uint8_t *)&data, 3, address);
    if (ret) {
        printk("Error writing to I2C error code (%d)\n", ret);
        return (-1);
    }
    return(0);
}

int16_t i2c_read_short(const struct device *i2c_dev, uint8_t address, uint8_t command)
{
    int ret;
    uint16_t data;
   
    ret = i2c_write_read(i2c_dev, address, &command, 1, &data, 2);
    if (ret) {
        printk("Error reading from I2C error code (%d)\n", ret);
        return (-1);
    }

    return(__bswap_16(data));
}

uint8_t shtc3_GetTempAndHumidity(const struct device *dev, uint16_t * temperature, uint16_t * humidity)
{
    union DATA data;
    uint8_t error = 0; 

    uint16_t cmd = __bswap_16(SHTC3_READ_T_FIRST);
    int ret = i2c_write(dev, (uint8_t *)&cmd, 2, SHTC3_ADDR);

    do {    
        ret = i2c_read(dev, (uint8_t *)&data, 6, SHTC3_ADDR);
        error++;
    } while ((ret != 0) && (error < SHTC3_NO_RETRIES));
    if (ret != 0) {
        printk("SHTC3: Get Temp & Humd Error\n");
        return SHTC3_I2C_ERROR;
    }

    error = shtc3_checkcrc((uint8_t *)&data.meas.temperature, 2, data.meas.temperature_crc);
    if (error != SHTC3_NO_ERROR) {
        printk("Temperature Checksum Error\r\n");
    } else {
        *temperature = __bswap_16(data.meas.temperature);
    }
    
    error = shtc3_checkcrc((uint8_t *)&data.meas.humidity, 2, data.meas.humidity_crc);
    if (error != SHTC3_NO_ERROR) {
        printk("Humidity Checksum Error\r\n");
    } else {
        *humidity = __bswap_16(data.meas.humidity);
    }

    return SHTC3_NO_ERROR;
}

uint8_t shtc3_checkcrc(uint8_t data[], uint8_t nbrOfBytes, uint8_t checksum)
{
  uint8_t bitmask;      // Bit mask
  uint8_t crc = 0xFF;   // Calculated checksum
  uint8_t byteCtr;      // Byte counter
 
  // Calculates 8-Bit checksum with given polynomial
  for(byteCtr = 0; byteCtr < nbrOfBytes; byteCtr++) {
    crc ^= (data[byteCtr]);
    for(bitmask = 8; bitmask > 0; --bitmask) {
      if(crc & 0x80) {
        crc = (crc << 1) ^ CRC_POLYNOMIAL;
      } else {
        crc = (crc << 1);
      }
    }
  }

  // Verify checksum
  if(crc != checksum) {
    return SHTC3_CHECKSUM_ERROR;
  } else {
    return SHTC3_NO_ERROR;
  }
}

float shtc3_convert_humd(uint16_t raw_humd)
{
    float humidity = 100.0 * ((float)raw_humd / 65536);
    //printk("RH %.01f%%\r\n", humidity);
    return(humidity);
}

float shtc3_convert_temp(uint16_t raw_temp)
{
    float temperature = 175.0 * ((float)raw_temp / 65536.0) - 45.0;
    //printk("Temp %.02f degC\r\n", temperature);
    return (temperature);
}

uint8_t shtc3_wakeup(const struct device *dev)
{
    uint16_t cmd = __bswap_16(SHTC3_WAKEUP);
    int ret = i2c_write(dev, (uint8_t *)&cmd, 2, SHTC3_ADDR);
    if (ret != 0) {
        printk("SHTC3: Wakeup Error\n");
        return SHTC3_I2C_ERROR;
    }
    return SHTC3_NO_ERROR;
}

uint8_t shtc3_sleep(const struct device *dev)
{
    uint16_t cmd = __bswap_16(SHTC3_SLEEP);
    int ret = i2c_write(dev, (uint8_t *)&cmd, 2, SHTC3_ADDR);
    if (ret != 0) {
        printk("SHTC3: Sleep Error\n");
        return SHTC3_I2C_ERROR;
    }
    return SHTC3_NO_ERROR;
}

uint8_t shtc3_software_reset(const struct device *dev)
{
    uint16_t cmd = __bswap_16(SHTC3_SWRESET);
    int ret = i2c_write(dev, (uint8_t *)&cmd, 2, SHTC3_ADDR);
    if (ret != 0) {
        printk("SHTC3: Sleep Error\n");
        return SHTC3_I2C_ERROR;
    }
    return SHTC3_NO_ERROR;
}

uint8_t shtc3_readid(const struct device *dev)
{
    uint16_t cmd = __bswap_16(SHTC3_IDREG);
    uint8_t buffer[3];
    
    int ret = i2c_write_read(dev, SHTC3_ADDR, (uint8_t *)&cmd, 2, buffer, 3);
    if (ret != 0) {
        printk("SHTC3: Read ID Error\n");
        return SHTC3_I2C_ERROR;
    }
    
    printk("ID Reg = 0x%02X%02X, Checksum 0x%02X\r\n", buffer[0], buffer[1], buffer[2]);

    ret = shtc3_checkcrc((uint8_t *)&buffer[0], 2, buffer[2]);
    if (ret != SHTC3_NO_ERROR) {
        printk("Checksum Error\n");
        return SHTC3_CHECKSUM_ERROR;
    }

    return SHTC3_NO_ERROR;
}