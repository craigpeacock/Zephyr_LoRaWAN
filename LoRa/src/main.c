

#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/lora.h>

#define LOG_LEVEL CONFIG_LOG_DBG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lora_transmit);

//#define TRANSMIT

char data_tx[] = {'h', 'e', 'l', 'l', 'o'};
char data_rx[100];

void main(void)
{
	const struct device *lora_dev;
	struct lora_modem_config config;
	int16_t rssi;
	int8_t snr;
	int ret;
	int i;

	printk("LoRa Example\n");

	lora_dev = DEVICE_DT_GET(DT_ALIAS(lora0));
	if (!device_is_ready(lora_dev)) {
		printk("%s: device not ready.", lora_dev->name);
		return;
	}

	config.frequency = 916800000;
	config.bandwidth = BW_125_KHZ;
	config.datarate = SF_10;
	config.preamble_len = 8;
	config.coding_rate = CR_4_5;
	config.tx_power = 4;
	config.iq_inverted = false;

#ifdef TRANSMIT
	config.tx = true;
#else
	config.tx = false;
#endif

	ret = lora_config(lora_dev, &config);
	if (ret < 0) {
		LOG_ERR("LoRa config failed");
		return;
	}

	printk("LoRa Device Configured.\n");

#ifdef TRANSMIT

	while (1) {
        
		ret = lora_send(lora_dev, data_tx, sizeof(data_tx));
		if (ret < 0) {
			LOG_ERR("LoRa send failed");
			return;
		}
		LOG_INF("Data sent!");
		k_sleep(K_MSEC(5000));

	}

#else 

	while (1) {
		ret = lora_recv(lora_dev, &data_rx, sizeof(data_rx), K_FOREVER, &rssi, &snr);
		if (ret < 0) {
			LOG_ERR("LoRa recv failed %d\n", ret);
			//return;
		} else {
			printk("Received %d bytes, ",ret);
			for (i = 0; i < ret; i++)
				printk("[0x%02x]",data_rx[i]);
			printk(", RSSI = %d, SNR = %d\n", rssi, snr);
		}
	}

#endif

}
