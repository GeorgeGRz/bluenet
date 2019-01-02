/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 6, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <stdint.h>

/**
 * Event types.
 */
enum EventType {
	Configuration_Base = 0x000,
	State_Base         = 0x080,
	General_Base       = 0x100,
	Uart_Base          = 0x180,
};

//! for Configuration event type see cs_ConfigHelper.h

/**
 * General event types.
 */
enum GeneralEventType {
	EVT_POWER_OFF = General_Base,
	EVT_POWER_ON,
	EVT_POWER_TOGGLE,
	EVT_ADVERTISEMENT_UPDATED,
	EVT_SCAN_STARTED,
	EVT_SCAN_STOPPED,
	EVT_SCANNED_DEVICES,
	EVT_DEVICE_SCANNED,
	EVT_POWER_SAMPLES_START,
	EVT_POWER_SAMPLES_END,
	EVT_CURRENT_USAGE_ABOVE_THRESHOLD_PWM,
	EVT_CURRENT_USAGE_ABOVE_THRESHOLD,
	EVT_DIMMER_ON_FAILURE_DETECTED,
	EVT_DIMMER_OFF_FAILURE_DETECTED,
//	EVT_POWER_CONSUMPTION,
//	EVT_ENABLED_MESH,
//	EVT_ENABLED_ENCRYPTION,
//	EVT_ENABLED_IBEACON,
//	EVT_CHARACTERISTICS_UPDATED,
	EVT_TRACKED_DEVICES,
	EVT_TRACKED_DEVICE_IS_NEARBY,
	EVT_TRACKED_DEVICE_NOT_NEARBY,
	EVT_MESH_TIME, // Sent when the mesh received the current time
	EVT_SCHEDULE_ENTRIES_UPDATED,
	EVT_BLE_EVENT,
	EVT_BLE_CONNECT,
	EVT_BLE_DISCONNECT,
	EVT_STATE_NOTIFICATION,
	EVT_BROWNOUT_IMPENDING,
	EVT_SESSION_NONCE_SET,
	EVT_KEEP_ALIVE,
	EVT_PWM_FORCED_OFF,
	EVT_SWITCH_FORCED_OFF,
	EVT_RELAY_FORCED_ON,
	EVT_CHIP_TEMP_ABOVE_THRESHOLD,
	EVT_CHIP_TEMP_OK,
	EVT_PWM_TEMP_ABOVE_THRESHOLD,
	EVT_PWM_TEMP_OK,
	EVT_EXTERNAL_STATE_MSG_CHAN_0,
	EVT_EXTERNAL_STATE_MSG_CHAN_1,
	EVT_TIME_SET, // Sent when the time has been set or changed.
	EVT_PWM_POWERED,
	EVT_PWM_ALLOWED, // Sent when pwm allowed flag is set. Payload is boolean.
	EVT_SWITCH_LOCKED, // Sent when switch locked flag is set. Payload is boolean.
	EVT_STORAGE_WRITE_DONE, // Sent when storage write is done and queue is empty.
	EVT_SETUP_DONE, // Sent when setup was done (and all settings have been stored).
	EVT_DO_RESET_DELAYED, // Sent to perform a reset in a few seconds (currently done by command handler). Payload is evt_do_reset_delayed_t.
	EVT_SWITCHCRAFT_ENABLED, // Sent when switchcraft flag is set. Payload is boolean.
	EVT_STORAGE_WRITE, // Sent when an item is going to be written to storage.
	EVT_STORAGE_ERASE, // Sent when a flash page is going to be erased.
	EVT_ADC_RESTARTED, // Sent when ADC has been restarted: the next buffer is expected to be different from the previous ones.
	EVT_TICK_500_MS, // Sent about every 500 ms.
	EVT_ADV_BACKGROUND, // Sent when a background advertisement has been received. Data: evt_adv_background_t with encrypted payload as data.
	EVT_ADV_BACKGROUND_PARSED, // Sent when a background advertisement has been validated and parsed. Data: evt_adv_background_t with parsed payload as data.
	EVT_ALL = 0xFFFF
};

enum UartEventType {
	EVT_SET_LOG_LEVEL = Uart_Base,
	EVT_ENABLE_LOG_POWER,
	EVT_ENABLE_LOG_CURRENT,
	EVT_ENABLE_LOG_VOLTAGE,
	EVT_ENABLE_LOG_FILTERED_CURRENT,
	EVT_CMD_RESET,
	EVT_ENABLE_ADVERTISEMENT,
	EVT_ENABLE_MESH,
	EVT_TOGGLE_ADC_VOLTAGE_VDD_REFERENCE_PIN,
	EVT_ENABLE_ADC_DIFFERENTIAL_CURRENT,
	EVT_ENABLE_ADC_DIFFERENTIAL_VOLTAGE,
	EVT_INC_VOLTAGE_RANGE,
	EVT_DEC_VOLTAGE_RANGE,
	EVT_INC_CURRENT_RANGE,
	EVT_DEC_CURRENT_RANGE,
	EVT_RX_CONTROL,
};

struct __attribute__((packed)) evt_do_reset_delayed_t {
	uint8_t resetCode;
	uint16_t delayMs;
};

struct __attribute__((__packed__)) evt_adv_background_t {
	uint8_t protocol;
	uint8_t sphereId;
	uint8_t* data;
	uint8_t dataSize;
	uint8_t* macAddress;
	int8_t rssi;
};

struct __attribute__((__packed__)) evt_adv_background_payload_t {
	uint8_t locationId;
	uint8_t profileId;
	int8_t rssiOffset;
	uint8_t flags;
};
