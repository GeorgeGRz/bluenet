/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jun 3, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once


//TODO: make a enum class of these
enum CommandHandlerTypes {
	CTRL_CMD_SETUP                       = 0,
	CTRL_CMD_FACTORY_RESET               = 1,
	CTRL_CMD_STATE_GET                   = 2,
	CTRL_CMD_STATE_SET                   = 3,
	CTRL_CMD_GET_BOOTLOADER_VERSION      = 4,
	CTRL_CMD_GET_UICR_DATA               = 5,
	CTRL_CMD_SET_IBEACON_CONFIG_ID       = 6,

	CTRL_CMD_RESET                       = 10,
	CTRL_CMD_GOTO_DFU                    = 11,
	CTRL_CMD_NOP                         = 12,
	CTRL_CMD_DISCONNECT                  = 13,

	CTRL_CMD_SWITCH                      = 20,
	CTRL_CMD_MULTI_SWITCH                = 21,
	CTRL_CMD_PWM                         = 22,
	CTRL_CMD_RELAY                       = 23,

	CTRL_CMD_SET_TIME                    = 30,
	CTRL_CMD_INCREASE_TX                 = 31,
	CTRL_CMD_RESET_ERRORS                = 32,
	CTRL_CMD_MESH_COMMAND                = 33,
	CTRL_CMD_SET_SUN_TIME                = 34,

	CTRL_CMD_ALLOW_DIMMING               = 40,
	CTRL_CMD_LOCK_SWITCH                 = 41,

	CTRL_CMD_UART_MSG                    = 50,

	CTRL_CMD_SAVE_BEHAVIOUR              = 60,
	CTRL_CMD_REPLACE_BEHAVIOUR           = 61,
	CTRL_CMD_REMOVE_BEHAVIOUR            = 62,
	CTRL_CMD_GET_BEHAVIOUR               = 63,
	CTRL_CMD_GET_BEHAVIOUR_INDICES       = 64,
	CTRL_CMD_GET_BEHAVIOUR_DEBUG         = 69,

	CTRL_CMD_REGISTER_TRACKED_DEVICE     = 70,

	CTRL_CMD_GET_UPTIME                  = 80,
	CTRL_CMD_GET_ADC_RESTARTS            = 81,
	CTRL_CMD_GET_SWITCH_HISTORY          = 82,
	CTRL_CMD_GET_POWER_SAMPLES           = 83,
//	CTLR_CMD_GET_CPU_STATS               = 84,

	CTRL_CMD_MICROAPP_UPLOAD             = 90,

	CTRL_CMD_UNKNOWN                     = 0xFFFF
};

enum AdvCommandTypes {
	ADV_CMD_NOOP                       = 0,
	ADV_CMD_MULTI_SWITCH               = 1,
	ADV_CMD_SET_TIME                   = 2,
	ADV_CMD_SET_BEHAVIOUR_SETTINGS     = 3,
	ADV_CMD_UPDATE_TRACKED_DEVICE      = 4,
	ADV_CMD_UNKNOWN                    = 0xFF
};
