
/*
 * Copyright (c) 2023 Craig Peacock
 * Copyright (c) 2019 Manivannan Sadhasivam
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/drivers/gpio.h>

#define LOG_LEVEL CONFIG_LOG_DBG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lora);

#define TRANSMIT 1
#define RECEIVE 0

char data_tx[] = {"Hello"};

static struct gpio_callback button_callback_data;
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios, {0});
K_SEM_DEFINE(pb_pushed, 0, 1);

void button_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	k_sem_give(&pb_pushed);
}

void lora_recv_callback(const struct device *dev, uint8_t *data, uint16_t size, int16_t rssi, int8_t snr)
{
	// When lora_recv_async is cancelled, may be called with 0 bytes.
	if (size != 0) {
		printk("RECV %d bytes: ",size);
		for (uint16_t i = 0; i < size; i++)
			printk("0x%02x ",data[i]);
		printk("RSSI = %ddBm, SNR = %ddBm\n", rssi, snr);
	} 
}

int lora_configure(const struct device *dev, bool transmit)
{
	int ret;
	struct lora_modem_config config;

	config.frequency = 916800000;
	config.bandwidth = BW_125_KHZ;
	config.datarate = SF_10;
	config.preamble_len = 8;
	config.coding_rate = CR_4_5;
	config.tx_power = 4;
	config.iq_inverted = false;
	config.public_network = false;
	config.tx = transmit;

	ret = lora_config(dev, &config);
	if (ret < 0) {
		LOG_ERR("LoRa device configuration failed");
		return false;
	}

	return(true);
}

void main(void)
{
	const struct device *dev_lora;
	int ret, bytes;

	printk("LoRa Point to Point Communications Example\n");

	// Setup LoRa Radio Device:
	dev_lora = DEVICE_DT_GET(DT_ALIAS(lora0));
	if (!device_is_ready(dev_lora)) {
		printk("%s: device not ready", dev_lora->name);
		return;
	}

	if (lora_configure(dev_lora, RECEIVE)) {
		printk("LoRa Device Configured\n");
	} else {
		return;
	}

	// Setup SW1 Momentary Push Button:
	if (!device_is_ready(button.port)) {
		printk("Error: button device %s is not ready\n", button.port->name);
		return;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n", ret, button.port->name, button.pin);
		return;
	}

	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n", ret, button.port->name, button.pin);
		return;
	}

	gpio_init_callback(&button_callback_data, button_callback, BIT(button.pin));
	gpio_add_callback(button.port, &button_callback_data);

	// Start LoRa radio listening
	ret = lora_recv_async(dev_lora, lora_recv_callback);
	if (ret < 0) {
		LOG_ERR("LoRa recv_async failed %d\n", ret);
	} 

	while (1) {

		// Wait for SW1 to be pressed. Onced pressed, transmit some data

		if (k_sem_take(&pb_pushed, K_FOREVER) != 0) {
			printk("Error taking sem\n");
		} 

		// Cancel reception
		ret = lora_recv_async(dev_lora, NULL);
		if (ret < 0) {
			LOG_ERR("LoRa recv_async failed %d\n", ret);
		} 

		// Reconfigure radio for transmit
		lora_configure(dev_lora, TRANSMIT);	

		// Transmit data
		ret = lora_send(dev_lora, data_tx, sizeof(data_tx));
		if (ret < 0) {
			LOG_ERR("LoRa send failed");
		} else {
			bytes = sizeof(data_tx);
			printk("XMIT %d bytes: ", bytes);
			for (uint16_t i = 0; i < bytes; i++)
				printk("0x%02x ",data_tx[i]);
			printk("\n");
		}

		// Restart reception
		lora_configure(dev_lora, RECEIVE);	
		ret = lora_recv_async(dev_lora, lora_recv_callback);
		if (ret < 0) {
			LOG_ERR("LoRa recv_async failed %d\n", ret);
		} 
	}
}
