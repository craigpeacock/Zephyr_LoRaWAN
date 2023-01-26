/*
 * SHTC3 - Sensirion Humidity and Temperature Sensor Code
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SHTC3_ADDR              0x70

#define SHTC3_SLEEP             0xB098
#define SHTC3_WAKEUP            0x3517
#define SHTC3_SWRESET           0x805D     
#define SHTC3_IDREG             0xEFC8
#define SHTC3_READ_T_FIRST      0x7866
#define SHTC3_READ_RH_FIRST     0x58E0

#define CRC_POLYNOMIAL          0x131 // P(x) = x^8 + x^5 + x^4 + 1 = 100110001
#define SHTC3_NO_ERROR          0
#define SHTC3_CHECKSUM_ERROR    -1
#define SHTC3_I2C_ERROR         -2

#define SHTC3_NO_RETRIES        100

struct VALUES {
    uint16_t    temperature;
    uint8_t     temperature_crc;
    uint16_t    humidity;
    uint8_t     humidity_crc;
} __attribute__((packed));

union DATA {
    struct VALUES meas;
    uint8_t buffer[6];
};

int8_t i2c_write_short(const struct device *i2c_dev, uint8_t address, uint8_t command, uint16_t data);
int16_t i2c_read_short(const struct device *i2c_dev, uint8_t address, uint8_t command);
uint8_t shtc3_sleep(const struct device *dev);
uint8_t shtc3_wakeup(const struct device *dev);
uint8_t shtc3_readid(const struct device *dev);
uint8_t shtc3_software_reset(const struct device *dev);
uint8_t shtc3_GetTempAndHumidity(const struct device *dev, uint16_t * temperature, uint16_t * humidity);
uint8_t shtc3_checkcrc(uint8_t data[], uint8_t nbrOfBytes, uint8_t checksum);

float shtc3_convert_humd(uint16_t raw_humd);
float shtc3_convert_temp(uint16_t raw_temp);