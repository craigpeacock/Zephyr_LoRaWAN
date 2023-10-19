
/*
 * LoRaWAN Network Time Example
 *
 * Copyright (c) 2023 Craig Peacock
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/lorawan/lorawan.h>
#include <zephyr/drivers/i2c.h>
#include <time.h>
#include "lorawan.h"

#define LOG_LEVEL CONFIG_LOG_DBG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

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

int main(void)
{
	const struct device *lora_dev;

	struct lorawan_join_config join_cfg;
	uint16_t dev_nonce = 0;

	uint8_t dev_eui[] = LORAWAN_DEV_EUI;
	uint8_t join_eui[] = LORAWAN_JOIN_EUI;
	uint8_t app_key[] = LORAWAN_APP_KEY;

	uint32_t gps_time;
	time_t unix_time;
	struct tm timeinfo;
	char buf[32];
	
	int ret;

	LOG_INF("Zephyr LoRaWAN Network Time Server Example, Board: %s", CONFIG_BOARD);

	lora_dev = DEVICE_DT_GET(DT_ALIAS(lora0));
	if (!device_is_ready(lora_dev)) {
		LOG_ERR("%s: device not ready.", lora_dev->name);
		return(-1);
	}

	LOG_INF("Starting LoRaWAN stack.");
	ret = lorawan_start();
	if (ret < 0) {
		LOG_ERR("lorawan_start failed: %d", ret);
		return(-1);
	}

	// Enable callbacks
	struct lorawan_downlink_cb downlink_cb = {
		.port = LW_RECV_PORT_ANY,
		.cb = dl_callback
	};

	lorawan_register_downlink_callback(&downlink_cb);
	lorawan_register_dr_changed_callback(lorwan_datarate_changed);

	join_cfg.mode = LORAWAN_ACT_OTAA;
	join_cfg.dev_eui = dev_eui;
	join_cfg.otaa.join_eui = join_eui;
	join_cfg.otaa.app_key = app_key;
	join_cfg.otaa.nwk_key = app_key;
	join_cfg.otaa.dev_nonce = dev_nonce;

	int i = 1;

	do {
		LOG_INF("Joining network using OTAA, dev nonce %d, attempt %d", join_cfg.otaa.dev_nonce, i++);
		ret = lorawan_join(&join_cfg);
		if (ret < 0) {
			if ((ret =-ETIMEDOUT)) {
				LOG_WRN("Timed-out waiting for response.");
			} else {
				LOG_ERR("Join failed (%d)", ret);
			}
		} else {
			LOG_INF("Join successful.");
		}

		// Increment DevNonce as per LoRaWAN 1.0.4 Spec.
		dev_nonce++;
		join_cfg.otaa.dev_nonce = dev_nonce;

		if (ret < 0) {
			// If failed, wait before re-trying.
			k_sleep(K_MSEC(5000));
		}

	} while (ret != 0);

#ifdef CONFIG_LORAWAN_APP_CLOCK_SYNC

	/*
	 * lorawan_clock_sync_run() registers a callback on port 202 (LoRaWAN Clock Sync Port) 
	 * and starts a delayable work function to request the time periodically. By default,
	 * this is every 86400 seconds (24 hours), but can be changed using 
	 * CONFIG_LORAWAN_APP_CLOCK_SYNC_PERIODICITY
	 *
	 * If successful, you should recieve a response back from the server similar to that 
	 * below with the time correction (in GPS Time).
	 * 
	 * <inf> main: Port 202, Pending 0, RSSI -66dB, SNR 8dBm
	 * <inf> main: Payload:
	 *			   01 77 7b 5b 52 00                                |.w{[R.
	 * <dbg> lorawan_clock_sync: clock_sync_package_callback: AppTimeAns time_correction 1381727095 (token 0)
	 */

	lorawan_clock_sync_run();

#endif

	while (1) {

		/*
		 * Once time synchronisation has occurred, lorawan_clock_sync_get() can 
		 * be called to populate an uint32_t variable with GPS Time. This is the 
		 * number of seconds since Jan 6th 1980 ignoring leap seconds.
		 */

		ret = lorawan_clock_sync_get(&gps_time);
		if (ret != 0) { 
			LOG_ERR("lorawan_clock_sync_get returned %d", ret);
		} else {
			/* 
			 * The difference in time between UNIX (epoch Jan 1st 1970) and
			 * GPS (epoch Jan 6th 1980) is 315964800 seconds. This is a bit
			 * of a fudge as it doesn't take into account leap seconds and 
			 * hence is out by roughly 18 seconds. 
			 *
			 */
			unix_time = gps_time - 315964800;
			localtime_r(&unix_time, &timeinfo);
			strftime(buf, sizeof(buf), "%A %B %d %Y %I:%M:%S %p %Z", &timeinfo);
			LOG_INF("GPS Time (Seconds since Jan 6th 1980) = %"PRIu32", UTC Time: %s", gps_time, buf);
		}

		k_sleep(K_SECONDS(5));
	}
}
