/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Dec 8, 2016
 * License: LGPLv3+
 */
#include <processing/cs_EnOceanHandler.h>
#include "drivers/cs_Serial.h"
#include "util/cs_BleError.h"
#include "events/cs_EventDispatcher.h"
#include "drivers/cs_Timer.h"
#include "processing/cs_Switch.h"
#include "storage/cs_State.h"
#include "drivers/cs_RTC.h"
#include "processing/cs_Scanner.h"

#define TOGGLES 4
#define DEBOUNCE_TIMEOUT 5000 // 5 seconds
#define LEARNING_RSSI_THRESHOLD -60

#if (NORDIC_SDK_VERSION >= 11)
	app_timer_t              _appTimerData = { {0} };
	app_timer_id_t           _appTimerId = &_appTimerData;
#else
	uint32_t                 _appTimerId = UINT32_MAX;
#endif

void toggle_power(void * p_context) {
	static uint8_t count;
	count = *(uint8_t*)p_context;

	LOGi("count: %d", count);
	if (count < TOGGLES * 2) {
		count++;
		Switch::getInstance().toggle();
		Timer::getInstance().start(_appTimerId, MS_TO_TICKS(500), &count);
	} else {
		Scanner::getInstance().resume();
		count = 0;
	}
}

EnOceanHandler::EnOceanHandler() :
		_learnedSwitches({}), _learningStartTime(0)
{
	EventDispatcher::getInstance().addListener(this);
}

void EnOceanHandler::init() {
	State::getInstance().get(STATE_LEARNED_SWITCHES, _learnedSwitches, MAX_SWITCHES * sizeof(learned_enocean_t));

	_log(SERIAL_INFO, "learned switches:");
	BLEutil::printArray(_learnedSwitches, sizeof(_learnedSwitches));

	Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t) toggle_power);
}

learned_enocean_t* EnOceanHandler::findEnOcean(uint8_t * adrs_ptr) {
	for (int i = 0; i < MAX_SWITCHES; ++i) {
		if (memcmp(_learnedSwitches[i].addr, adrs_ptr, BLE_GAP_ADDR_LEN) == 0) {
			// found
			return &_learnedSwitches[i];
		}
	}
	return NULL;
}

learned_enocean_t* EnOceanHandler::newEnOcean() {

	for (int i = 0; i < MAX_SWITCHES; ++i) {
		if (memcmp(_learnedSwitches[i].addr, new uint8_t[BLE_GAP_ADDR_LEN] {}, BLE_GAP_ADDR_LEN) == 0) {
//		if (memcmp(_learnedSwitches[i].addr, emptyAddress, BLE_GAP_ADDR_LEN) == 0) {
			// found
			return &_learnedSwitches[i];
		}
	}
	return NULL;
}

bool EnOceanHandler::authenticate(uint8_t * adrs_ptr, data_telegram_t* p_data) {

#ifdef ENOCEAN_VERBOSE
	_log(SERIAL_INFO, "address: %02X %02X %02X %02X %02X %02X\r\n", adrs_ptr[5],
						adrs_ptr[4], adrs_ptr[3], adrs_ptr[2], adrs_ptr[1],
						adrs_ptr[0]);

	_log(SERIAL_INFO, "security key: ");
	BLEutil::printArray(key, 16);

	_log(SERIAL_INFO, "seqCounter: %p, bytes: ", p_data->seqCounter);
	BLEutil::printArray(&p_data->seqCounter, 4);

	_log(SERIAL_INFO, "signature: %p, bytes: ", p_data->securitySignature);
	BLEutil::printArray(&p_data->securitySignature, 4);
#endif

	nrf_ecb_hal_data_t _block __attribute__ ((aligned (4)));
	memset(&_block, 0, sizeof(_block));

	learned_enocean_t* enOcean = findEnOcean(adrs_ptr);
	if (enOcean == NULL) return false;

	memcpy(_block.key, enOcean->securityKey, 16);

	//////////////////////////////////
	// NONCE
	//////////////////////////////////

	uint8_t nonce[10];
	memcpy(nonce, adrs_ptr, BLE_GAP_ADDR_LEN);
	memcpy(&nonce[6], &p_data->seqCounter, 4);

#ifdef ENOCEAN_VERBOSE
	_log(SERIAL_INFO, "nonce: ");
	BLEutil::printArray(nonce, sizeof(nonce));
#endif

	//////////////////////////////////
	// B_0
	//////////////////////////////////

	uint8_t b_0[16] = {};
	b_0[0] = 0x49;
	memcpy(&b_0[1], nonce, sizeof(nonce));

#ifdef ENOCEAN_VERBOSE
	_log(SERIAL_INFO, "b_0: ");
	BLEutil::printArray(b_0, 16);
#endif

	//////////////////////////////////
	// B_1
	//////////////////////////////////

	uint8_t b_1[16] = {};
	b_1[0] = 0x00;
	b_1[1] = 0x09;
	b_1[2] = 0x0C;
	b_1[3] = 0xFF;
	b_1[4] = 0xDA;
	b_1[5] = 0x03;
	memcpy(&b_1[6], &p_data->seqCounter, 4);

	b_1[10] = p_data->switchState;

#ifdef ENOCEAN_VERBOSE
	_log(SERIAL_INFO, "b_1: ");
	BLEutil::printArray(b_1, 16);
#endif

	//////////////////////////////////
	// X_1
	//////////////////////////////////

	memcpy(_block.cleartext, b_0, 16);

	uint32_t err_code = sd_ecb_block_encrypt(&_block);
	APP_ERROR_CHECK(err_code);

	uint8_t x_1[16];
	memcpy(x_1, _block.ciphertext, 16);

#ifdef ENOCEAN_VERBOSE
	_log(SERIAL_INFO, "x_1: ");
	BLEutil::printArray(x_1, 16);
#endif

	//////////////////////////////////
	// X_2
	//////////////////////////////////

	memset(_block.cleartext, 0, 16);
	for (int i = 0; i < 16; ++i) {
		_block.cleartext[i] = x_1[i] ^ b_1[i];
	}

#ifdef ENOCEAN_VERBOSE
	_log(SERIAL_INFO, "x_1 ^ b_1: ");
	BLEutil::printArray(_block.cleartext, 16);
#endif

	err_code = sd_ecb_block_encrypt(&_block);
	APP_ERROR_CHECK(err_code);

	uint8_t x_2[16];
	memcpy(x_2, _block.ciphertext, 16);

#ifdef ENOCEAN_VERBOSE
	_log(SERIAL_INFO, "x_2: ");
	BLEutil::printArray(_block.ciphertext, 16);
#endif

	//////////////////////////////////
	// T
	//////////////////////////////////

	uint8_t T[4] = {};
	memcpy(T, _block.ciphertext, 4);

#ifdef ENOCEAN_VERBOSE
	_log(SERIAL_INFO, "T: ");
	BLEutil::printArray(T, 4);
#endif

	//////////////////////////////////
	// A_0
	//////////////////////////////////

	uint8_t a_0[16] = {};
	a_0[0] = 0x1;
	memcpy(&a_0[1], nonce, sizeof(nonce));

#ifdef ENOCEAN_VERBOSE
	_log(SERIAL_INFO, "a_0: ");
	BLEutil::printArray(a_0, 16);
#endif

	//////////////////////////////////
	// S_0
	//////////////////////////////////

	memcpy(_block.cleartext, a_0, 16);

	err_code = sd_ecb_block_encrypt(&_block);
	APP_ERROR_CHECK(err_code);

	uint8_t S_0[16] = {};
	memcpy(S_0, _block.ciphertext, 16);

#ifdef ENOCEAN_VERBOSE
	_log(SERIAL_INFO, "S_0: ");
	BLEutil::printArray(S_0, 16);
#endif

	//////////////////////////////////
	// U
	//////////////////////////////////

	uint8_t U[4] = {};
	for (int i = 0; i < 4; ++i) {
		U[i] = T[i] ^ S_0[i];
	}

#ifdef ENOCEAN_VERBOSE
	_log(SERIAL_INFO, "U: ");
	BLEutil::printArray(U, 4);
#endif

	//////////////////////////////////
	// VALIDATION
	//////////////////////////////////

	if (memcmp(U, &p_data->securitySignature, 4) == 0) {
		LOGi("hurray")
		return true;
	} else {
		LOGe("darn");
		return false;
	}
}

void EnOceanHandler::save() {
	_log(SERIAL_INFO, "learned switches:");
	BLEutil::printArray(_learnedSwitches, sizeof(_learnedSwitches));

	State::getInstance().set(STATE_LEARNED_SWITCHES, _learnedSwitches, MAX_SWITCHES * sizeof(learned_enocean_t));
}

bool EnOceanHandler::learnEnOcean(uint8_t * adrs_ptr, data_t* p_data) {

	if (RTC::now() - _learningStartTime < DEBOUNCE_TIMEOUT) {
		return false;
	}

	_learningStartTime = RTC::now();

	commissioning_telegram_t* telegram = (commissioning_telegram_t*) p_data->p_data;

	learned_enocean_t* enOcean = findEnOcean(adrs_ptr);
//	if (enOcean != NULL) {
//		LOGi("found existing, overwrite...");
//
//		if (memcmp(enOcean->securityKey, telegram->securityKey, 16) != 0) {
//			memcpy(enOcean->securityKey, telegram->securityKey, 16);
//			save();
//		}
//
//#ifdef ENOCEAN_VERBOSE
//		_log(SERIAL_INFO, "security key: ");
//		BLEutil::printArray(telegram->securityKey, 16);
//#endif
//
//		return true;
//	}
	if (enOcean != NULL) {
		LOGi("found existing, unlearn...");

		memset(enOcean, 0, sizeof(learned_enocean_t));
		save();

		Scanner::getInstance().pause();
		uint8_t count = TOGGLES;
		toggle_power(&count);

#ifdef ENOCEAN_VERBOSE
		_log(SERIAL_INFO, "security key: ");
		BLEutil::printArray(telegram->securityKey, 16);
#endif

		return true;
	}

	enOcean = newEnOcean();
	if (enOcean != NULL) {

		LOGi("add new...");
		memcpy(enOcean->addr, adrs_ptr, BLE_GAP_ADDR_LEN);
		memcpy(enOcean->securityKey, telegram->securityKey, 16);
		save();

#ifdef ENOCEAN_VERBOSE
		_log(SERIAL_INFO, "security key: ");
		BLEutil::printArray(telegram->securityKey, 16);
#endif

		Scanner::getInstance().pause();
		uint8_t count = 0;
		toggle_power(&count);

		return true;
	}

#ifdef ENOCEAN_VERBOSE
	_log(SERIAL_INFO, "security key: ");
	BLEutil::printArray(telegram->securityKey, 16);
#endif

	LOGe("no more space...");

	return false;
}


bool EnOceanHandler::triggerEnOcean(uint8_t * adrs_ptr, data_t* p_data) {

//	if (findEnOcean(adrs_ptr) == NULL) {
//		LOGe(">>> enOcean not learned!");
//		return false;
//	}

	data_telegram_t* telegram = (data_telegram_t*) p_data->p_data;

	// authenticate
	bool success = authenticate(adrs_ptr, telegram);

	if (success) {
		LOGi("telegram->switchState: %d", telegram->switchState);

		bool buttonA = BLEutil::isBitSet(telegram->switchState, PIN_A0);
		bool buttonB = BLEutil::isBitSet(telegram->switchState, PIN_A1);
		bool buttonC = BLEutil::isBitSet(telegram->switchState, PIN_B0);
		bool buttonD = BLEutil::isBitSet(telegram->switchState, PIN_B1);

		LOGi("%s", BLEutil::isBitSet(telegram->switchState, PIN_ACTION_TYPE) ? "pressed" : "released");
		if (buttonA) {
			LOGi("  Button A");
		}
		if (buttonB) {
			LOGi("  Button B");
		}
		if (buttonC) {
			LOGi("  Button C");
		}
		if (buttonD) {
			LOGi("  Button D");
		}

		if (buttonA || buttonC) {
//			EventDispatcher::getInstance().dispatch(EVT_POWER_ON);
			Switch::getInstance().turnOn();
		} else if (buttonB || buttonD) {
//			EventDispatcher::getInstance().dispatch(EVT_POWER_OFF);
			Switch::getInstance().turnOff();
		}

		return true;
	} else {
		return false;
	}
}

bool EnOceanHandler::parseAdvertisement(ble_gap_evt_adv_report_t* p_adv_report) {

	data_t type_data;
	data_t adv_data;

	//! Initialize advertisement report for parsing.
	adv_data.p_data = (uint8_t *)p_adv_report->data;
	adv_data.data_len = p_adv_report->dlen;

	uint32_t err_code = BLEutil::adv_report_parse(BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA,
										 &adv_data,
										 &type_data);
	if (err_code == NRF_SUCCESS)
	{
		uint16_t companyIdentifier = type_data.p_data[1] << 8 | type_data.p_data[0];
		if (companyIdentifier == ENOCEAN_COMPANY_ID)
		{
			LOGi("rssi: %d", p_adv_report->rssi);
			LOGi("type_data.data_len: %d", type_data.data_len);
			if (type_data.data_len == 22 && p_adv_report->rssi > LEARNING_RSSI_THRESHOLD) {
				// learn
				learnEnOcean(p_adv_report->peer_addr.addr, &type_data);
			} else if (type_data.data_len == 11) {
				// trigger
				triggerEnOcean(p_adv_report->peer_addr.addr, &type_data);
			}
			return true;
		}
	}

	return false;

}

void EnOceanHandler::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
	switch(evt) {
	case EVT_DEVICE_SCANNED: {
		ble_gap_evt_adv_report_t* p_adv_report = (ble_gap_evt_adv_report_t*)p_data;
		parseAdvertisement(p_adv_report);
		break;
	}
	}
}


