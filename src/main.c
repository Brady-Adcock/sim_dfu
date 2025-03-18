/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <nrfx_clock.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/settings/settings.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   3000
static const uint8_t value[4] = {0xde, 0xad, 0xbe, 0xef};
bool is_master_synch_node = false;
static struct bt_conn *current_conn = NULL;

/* UUIDs */
static const struct bt_uuid_128 smp_service_uuid = BT_UUID_INIT_128(
	0x84, 0xAA, 0x60, 0x74, 0x52, 0x8A, 0x8B, 0x86,
	0xD3, 0x4C, 0xB7, 0x1D, 0x1D, 0xDC, 0x53, 0x8D);

static const struct bt_uuid_128 example_service_uuid = BT_UUID_INIT_128(
    0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
    0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E);

static const struct bt_uuid_128 example_data_characteristic_uuid = BT_UUID_INIT_128(
    0x87, 0x61, 0x78, 0x33, 0x9B, 0x0D, 0x29, 0xAC,
    0x5A, 0x43, 0xEA, 0x49, 0x16, 0xB0, 0xAA, 0xB5);

static ssize_t write_char(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		const void *buf, uint16_t len, uint16_t offset)
{
	printk("write_session_char but don't actually do anything :)\n");
	return len;
}

ssize_t read_char(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, 
	  uint16_t len, uint16_t offset)
{
	printk("Reading device settings\n");
	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(value));
	printk("Device settings read\n");
}

#define MANUFACTURER_ID 0x9E, 0x0D // Custom manufacturer ID for Impulse Wellness LLC 

static void update_advertising_data() {
	uint8_t mf_data[] = {
		MANUFACTURER_ID,
	};
	struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
		{
			.type = 0x07, // "complete list of 128-bit UUIDs available",
			.data_len = 16, // one service so only need 16 bytes to represent it's 128 bit UUID
			.data = (const uint8_t *)smp_service_uuid.val,
		},
		{
			.type = BT_DATA_MANUFACTURER_DATA, // "Appearance",
			.data_len = sizeof(mf_data),
			.data = mf_data,
		}
	};

	struct bt_le_adv_param *adv_param = BT_LE_ADV_CONN_NAME;
	adv_param->interval_min = 0x20;
	adv_param->interval_max = 0x30;
	int err;
	err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printf("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	struct bt_conn_le_tx_power tx_power_level = {0};
	if (err) {
		printf("Connection failed (err 0x%02x)\n", err);
		return;
	}
	
	printk("Connected\n");
	current_conn = bt_conn_ref(conn);
	bt_conn_le_get_tx_power_level(current_conn, &tx_power_level); 
	printf("TX power level: %d\n", tx_power_level.phy);
	k_sleep(K_MSEC(1000));

	struct bt_conn_info info = {0};	
	err = bt_conn_get_info(current_conn, &info);
	if (err) {
		printf("Failed to get connection info %d\n", err);
		return;
	}

	printf("Conn. interval is %u units\n", info.le.interval);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printf("Disconnected (reason 0x%02x)\n", reason);

	if (current_conn) {
		bt_conn_unref(current_conn);
		current_conn = NULL;
	}

	// start advertising again with new battery and color data
	update_advertising_data();

	// TODO: check if this is the master sync node and if so, disable it
	if (is_master_synch_node) {
		printk("Master synch node disconnected, disabling\n");
	}
	
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

/* Service and characteristic definitions */
BT_GATT_SERVICE_DEFINE(example_service,
	BT_GATT_PRIMARY_SERVICE(&example_service_uuid),
	BT_GATT_CHARACTERISTIC(&example_data_characteristic_uuid.uuid,
						BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
						BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
						read_char, write_char,
						value),
);


void set_up_ble() {
	int err;

	err = bt_enable(NULL);
	if (err) {
		printf("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
		printk("Settings loaded\n");
	}
	// starts advertising
	update_advertising_data();
}
	

int main(void)
{
	printk("Hello World! %s\n", CONFIG_BOARD);
	set_up_ble();
	while (1) {
		printk("loop'n g\n");
		k_msleep(SLEEP_TIME_MS);
	}
	return 0;
}
