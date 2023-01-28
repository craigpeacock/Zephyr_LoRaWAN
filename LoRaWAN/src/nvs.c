
/*
 * Copyright (c) 2023 Craig Peacock
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/random/rand32.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/nvs.h>

#include "nvs.h"

// Set NVS_CLEAR to wipe NVS partition on boot.
//#define NVS_CLEAR

const int nVars = 4;
const char *nvs_name[] = {"DevNonce", "DevEUI  ", "JoinEUI ", "AppKey  "};
int nvs_len[] = {2, 8, 8, 16};

void stm32wl_ieee_64uid(uint8_t dev_eui[])
{
	//uint32_t *UID;
	//UID = (void *)0x1FFF7580;
	//printk("IEEE 64-bit UID = 0x%08X:%08X\n",*(UID+1),*UID);
	
	char dev_eui_str[8];
	uint64_t *uid;
	uid = ((void *)0x1FFF7580);
	strncpy(dev_eui_str, (char *)uid, 8);
	for (int i = 0; i < 8; i++)
		dev_eui[i] = dev_eui_str[7-i];
}

void nvs_init(struct nvs_fs *fs)
{
	struct flash_pages_info info;
	int ret;

	fs->flash_device = NVS_PARTITION_DEVICE;
	if (!device_is_ready(fs->flash_device)) {
		printk("Flash device %s is not ready\n", fs->flash_device->name);
		return;
	}
	fs->offset = NVS_PARTITION_OFFSET;
	ret = flash_get_page_info_by_offs(fs->flash_device, fs->offset, &info);
	if (ret) {
		printk("Unable to get page info\n");
		return;
	}
	fs->sector_size = info.size;
	fs->sector_count = 3U;

	ret = nvs_mount(fs);
	if (ret) {
		printk("Flash Init failed\n");
		return;
	}

#ifdef NVS_CLEAR
	ret = nvs_clear(fs);
	if (ret) {
		printk("Flash Clear failed\n");
		return;
	} else {
		printk("Cleared NVS from flash\n");
	}
    while(1);
#endif
}

void nvs_read_init_parameter(struct nvs_fs *fs, uint16_t id, void *data)
{
	int ret;
	ssize_t bytes_written;

	char *array = (void *)data;
	int *devnonce = (void *)data;

	printk("NVS: ID %d %s ", id, nvs_name[id]);
	ret = nvs_read(fs, id, data, nvs_len[id]);
	if (ret > 0) { 
		// Item found, print output:
		switch (id) {
			case NVS_DEVNONCE_ID:
				printk("%d\n",*devnonce);
				break;
			case NVS_LORAWAN_DEV_EUI_ID:
			case NVS_LORAWAN_JOIN_EUI_ID:
			case NVS_LORAWAN_APP_KEY_ID:
				for (int i = 0; i < nvs_len[id]; i++)
					printk("%02X ",array[i]);
				printk("\n");
				break;
			default:
				break;
		}
	} else {
		// Item not found
		printk("not found. Initialised to ");

		switch (id) {
			case NVS_DEVNONCE_ID:
				*devnonce = 0;
				printk("%d ",*devnonce);
				break;

			case NVS_LORAWAN_DEV_EUI_ID:

#ifdef CONFIG_SOC_STM32WLE5XX
				// Get IEEE 64-bit UID from STM
				stm32wl_ieee_64uid(data);
#endif

				for (int i = 0; i < nvs_len[id]; i++)
					printk("%02X ",array[i]);
				break;

			case NVS_LORAWAN_JOIN_EUI_ID:
				// Initialise to zero.
				for (int i = 0; i < nvs_len[id]; i++) {
					array[i] = 0;
					printk("%02X ",array[i]);
				}					
				break;

			case NVS_LORAWAN_APP_KEY_ID:
				// Generate random key
                sys_rand_get(data, nvs_len[id]);
				for (int i = 0; i < nvs_len[id]; i++) {
					printk("%02X ",array[i]);
				}	
				break;
		}

		// Write to NVS
		bytes_written = nvs_write(fs, id, data, nvs_len[id]);
		if (bytes_written < 0) {
			printf("Failed (%d)\n",bytes_written);
		} else {
			//printf("Saved %d bytes\n",bytes_written);
            printf("Saved\n");
		}
	}
}