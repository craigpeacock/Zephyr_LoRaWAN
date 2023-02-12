
/*
 * Copyright (c) 2023 Craig Peacock
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define NVS_PARTITION			storage_partition
#define NVS_PARTITION_DEVICE	FIXED_PARTITION_DEVICE(NVS_PARTITION)
#define NVS_PARTITION_OFFSET	FIXED_PARTITION_OFFSET(NVS_PARTITION)

#define NVS_DEVNONCE_ID             0
#define NVS_LORAWAN_DEV_EUI_ID      1
#define NVS_LORAWAN_JOIN_EUI_ID     2
#define NVS_LORAWAN_APP_KEY_ID      3

void nvs_initialise(struct nvs_fs *fs);
void nvs_read_init_parameter(struct nvs_fs *fs, uint16_t id, void *data);

