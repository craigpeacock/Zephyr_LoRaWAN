# Zephyr LoRaWAN & LoRa Examples

An example LoRa Point to Point and LoRaWAN Network Application for the Zephyr RTOS.

Tested with the following targets:

* Lemon IoT BLE nRF52832 + RFM95W (SX1276) 
* Nordic nRF52840 Dongle + RFM95W (SX1276) 
* Lemon IoT LTE nRF9160 + RFM95W (SX1276) 
* Lemon IoT LoRa RAK3172 (based on the STM32WL5ECC)

# LoRaWAN

The LoRaWAN folder contains example code to connect to [The Things Network](https://www.thethingsnetwork.org) and transmit temperature and humidity values from a Sensirion SHTC3.

LoRaWAN Device EUI, Join EUI and Application Key should be entered into the lorawan.h file prior to compiling. 

The prj.conf file includes statements to enable your region (Frequency):

```
#LORAMAC_REGION_AS923=y
LORAMAC_REGION_AU915=y
#LORAMAC_REGION_CN470=y
#LORAMAC_REGION_CN779=y
#LORAMAC_REGION_EU433=y
#LORAMAC_REGION_EU868=y
#LORAMAC_REGION_KR920=y
#LORAMAC_REGION_IN865=y
#LORAMAC_REGION_US915=y
#LORAMAC_REGION_RU864=y
```

The I2C SHTC3 sensor can be connected to the I2C pins allocated in the relevent [board](https://github.com/craigpeacock/Zephyr_LoRaWAN/tree/main/LoRaWAN/boards) file for your target. 

The example stores the DevNonce in NVS (Non-volatile Storage) as per LoRaWAN 1.0.4 Specifications.

## Work in progress

The STM32WL5E has an IEEE 64-bit EUI stored at 0x1FFF7580. We can read this and use it as the Device EUI. Currently the LoRaWAN Device EUI is hard-coded.

# LoRa

The LoRa folder contains example code to allow testing of LoRa radios (point to point communications). This is useful for validating your LoRa radio is working correctly before trying to connect to LoRaWAN networks.

When started, the app will listen for packets on the selected frequency/channel. When SW1 is pressed, will transmit a packet ('Hello') in ASCII. 

Please check the frequency/channel configuration prior to use and ensure you are transmitting on a permitted band for your country. 

```
*** Booting Zephyr OS build zephyr-v3.2.0-3920-g5787c69b9ce5 ***
LoRa Point to Point Communications Example
LoRa Device Configured
XMIT 6 bytes: 0x48 0x65 0x6c 0x6c 0x6f 0x00
XMIT 6 bytes: 0x48 0x65 0x6c 0x6c 0x6f 0x00
XMIT 6 bytes: 0x48 0x65 0x6c 0x6c 0x6f 0x00
XMIT 6 bytes: 0x48 0x65 0x6c 0x6c 0x6f 0x00
XMIT 6 bytes: 0x48 0x65 0x6c 0x6c 0x6f 0x00
RECV 6 bytes: 0x48 0x65 0x6c 0x6c 0x6f 0x00 RSSI = -75dBm, SNR = 8dBm
RECV 6 bytes: 0x48 0x65 0x6c 0x6c 0x6f 0x00 RSSI = -75dBm, SNR = 8dBm
RECV 6 bytes: 0x48 0x65 0x6c 0x6c 0x6f 0x00 RSSI = -75dBm, SNR = 9dBm
RECV 6 bytes: 0x48 0x65 0x6c 0x6c 0x6f 0x00 RSSI = -74dBm, SNR = 9dBm
RECV 6 bytes: 0x48 0x65 0x6c 0x6c 0x6f 0x00 RSSI = -74dBm, SNR = 9dBm
RECV 6 bytes: 0x48 0x65 0x6c 0x6c 0x6f 0x00 RSSI = -74dBm, SNR = 8dBm
```
