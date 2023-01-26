
/*
 * Class A LoRaWAN sample application
 *
 * Copyright (c) 2023 Craig Peacock
 * Copyright (c) 2020 Manivannan Sadhasivam <mani@kernel.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/lorawan/lorawan.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/random/rand32.h>

#include "shtc3.h"
#include "lorawan.h"

#define DELAY K_MINUTES(10)

#define LOG_LEVEL CONFIG_LOG_DBG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lorawan_class_a);

static void dl_callback(uint8_t port, bool data_pending, int16_t rssi, int8_t snr, uint8_t len, const uint8_t *data)
{
	LOG_INF("Port %d, Pending %d, RSSI %ddB, SNR %ddBm", port, data_pending, rssi, snr);
	if (data) {
		LOG_HEXDUMP_INF(data, len, "Payload: ");
	}
}

static void lorwan_datarate_changed(enum lorawan_datarate dr)
{
	uint8_t unused, max_size;

	lorawan_get_payload_sizes(&unused, &max_size);
	LOG_INF("New Datarate: DR_%d, Max Payload %d", dr, max_size);
}

void main(void)
{
	const struct device *lora_dev;
	const struct device *i2c_dev;
	struct lorawan_join_config join_cfg;

	uint8_t dev_eui[] = LORAWAN_DEV_EUI;
	uint8_t join_eui[] = LORAWAN_JOIN_EUI;
	uint8_t app_key[] = LORAWAN_APP_KEY;

	uint16_t payload[2];

	int ret;

	printk("Zephyr LoRaWAN Node Example\n");

  	printk("Opening I2C...\n");
	i2c_dev = DEVICE_DT_GET(DT_ALIAS(sensorbus));
  	if (!i2c_dev) {
		printk("I2C: Device driver not found.\n");
		return;
  	} else {
        //i2c_configure(i2c_dev, I2C_SPEED_SET(I2C_SPEED_STANDARD));
  	}

	lora_dev = DEVICE_DT_GET(DT_ALIAS(lora0));
	if (!device_is_ready(lora_dev)) {
		printk("%s: device not ready.", lora_dev->name);
		return;
	}

    printk("Starting LoRaWAN stack...\n");
	ret = lorawan_start();
	if (ret < 0) {
		printk("lorawan_start failed: %d\n\n", ret);
		return;
	}

    // Enable callbacks
    struct lorawan_downlink_cb downlink_cb = {
        .port = LW_RECV_PORT_ANY,
        .cb = dl_callback
    };

	lorawan_register_downlink_callback(&downlink_cb);
	lorawan_register_dr_changed_callback(lorwan_datarate_changed);

	uint32_t random = sys_rand32_get();
    uint16_t dev_nonce = random & 0x0000FFFF;

	join_cfg.mode = LORAWAN_ACT_OTAA;
	join_cfg.dev_eui = dev_eui;
	join_cfg.otaa.join_eui = join_eui;
	join_cfg.otaa.app_key = app_key;
	join_cfg.otaa.nwk_key = app_key;
	join_cfg.otaa.dev_nonce = dev_nonce;

	int i = 0;

	do {
		printk("Joining network over OTAA (%d)\n",i++);
		ret = lorawan_join(&join_cfg);
		if (ret < 0) {
			printk("Join failed (%d)\n", ret);

			random = sys_rand32_get();
    		join_cfg.otaa.dev_nonce = random & 0x0000FFFF;

			k_sleep(K_MSEC(5000));
		} else {
			printk("Join successful (%d)\n",ret);
		}
		
	} while (ret != 0);

	while (1) {

		shtc3_wakeup(i2c_dev);
		k_msleep(1);
		shtc3_GetTempAndHumidity(i2c_dev, &payload[0], &payload[1]);
		shtc3_sleep(i2c_dev);
		printk("Temp %.02f RH %.01f\r\n", shtc3_convert_temp(payload[0]), shtc3_convert_humd(payload[1])); 
	
		ret = lorawan_send(2, (uint8_t *)&payload, sizeof(payload), LORAWAN_MSG_UNCONFIRMED);
		if (ret == -EAGAIN) {
			LOG_ERR("lorawan_send failed: %d. Continuing...", ret);
			k_sleep(DELAY);
			continue;
		} else if (ret < 0) {
			LOG_ERR("lorawan_send failed: %d", ret);
			return;
		}

		LOG_INF("Data sent!");
		k_sleep(DELAY);
	}
}
