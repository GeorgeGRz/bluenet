/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 2, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#include "processing/cs_Scanner.h"

#if BUILD_MESHING == 1
#include <mesh/cs_MeshControl.h>
#endif
#include <storage/cs_Settings.h>

#include <cfg/cs_DeviceTypes.h>
#include <ble/cs_CrownstoneManufacturer.h>

#include <events/cs_EventDispatcher.h>

#include <cfg/cs_UuidConfig.h>

//#define PRINT_SCANNER_VERBOSE
//#define PRINT_DEBUG

Scanner::Scanner() :
	_opCode(SCAN_START),
	_scanning(false),
	_running(false),
	_scanDuration(SCAN_DURATION),
	_scanSendDelay(SCAN_SEND_DELAY),
	_scanBreakDuration(SCAN_BREAK_DURATION),
	_scanFilter(SCAN_FILTER),
	_filterSendFraction(SCAN_FILTER_SEND_FRACTION),
	_scanCount(0),
#if (NORDIC_SDK_VERSION >= 11)
	_appTimerId(NULL),
#else
	_appTimerId(UINT32_MAX),
#endif
	_stack(NULL)
{
#if (NORDIC_SDK_VERSION >= 11)
	_appTimerData = { {0} };
	_appTimerId = &_appTimerData;
#endif

	_scanResult = new ScanResult();

	//! [29.01.16] the scan result needs it's own buffer, not the master buffer,
	//! since it is now decoupled from writing to a characteristic.
	//! if we used the master buffer we would overwrite the scan results
	//! if we write / read from a characteristic that uses the master buffer
	//! during a scan!
	_scanResult->assign(_scanBuffer, sizeof(_scanBuffer));
}

void Scanner::init() {
	Settings& settings = Settings::getInstance();
	settings.get(CONFIG_SCAN_DURATION, &_scanDuration);
	settings.get(CONFIG_SCAN_SEND_DELAY, &_scanSendDelay);
	settings.get(CONFIG_SCAN_BREAK_DURATION, &_scanBreakDuration);
	settings.get(CONFIG_SCAN_FILTER, &_scanFilter);

	EventDispatcher::getInstance().addListener(this);
	Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t)Scanner::staticTick);
}

void Scanner::setStack(Nrf51822BluetoothStack* stack) {
	_stack = stack;
}

void Scanner::manualStartScan() {
	if (!_stack) {
		LOGe(STR_ERR_FORGOT_TO_ASSIGN_STACK);
		return;
	}

//	LOGi(FMT_INIT, "scan result");
	_scanResult->clear();
	_scanning = true;

	if (!_stack->isScanning()) {
#ifdef PRINT_SCANNER_VERBOSE
		LOGi(FMT_START, "Scanner");
#endif
		_stack->startScanning();
	}
}

void Scanner::manualStopScan() {
	if (!_stack) {
		LOGe(STR_ERR_FORGOT_TO_ASSIGN_STACK);
		return;
	}

	_scanning = false;

	if (_stack->isScanning()) {
#ifdef PRINT_SCANNER_VERBOSE
		LOGi(FMT_STOP, "Scanner");
#endif
		_stack->stopScanning();
	}
}

bool Scanner::isScanning() {
	if (!_stack) {
		LOGe(STR_ERR_FORGOT_TO_ASSIGN_STACK);
		return false;
	}

	return _scanning && _stack->isScanning();
}

ScanResult* Scanner::getResults() {
	return _scanResult;
}

void Scanner::staticTick(Scanner* ptr) {
	ptr->executeScan();
}

void Scanner::start() {
	if (!_running) {
		_running = true;
		_scanCount = 0;
		_opCode = SCAN_START;
		executeScan();
	} else {
		LOGi(FMT_ALREADY, "scanning");
	}
}

void Scanner::delayedStart(uint16_t delay) {
	if (!_running) {
		LOGi("delayed start by %d ms", delay);
		_running = true;
		_scanCount = 0;
		_opCode = SCAN_START;
		Timer::getInstance().start(_appTimerId, MS_TO_TICKS(delay), this);
	} else {
		LOGd(FMT_ALREADY, "scanning");
	}
}

void Scanner::delayedStart() {
	delayedStart(_scanBreakDuration);
}

void Scanner::stop() {
	if (_running) {
		_running = false;
		_opCode = SCAN_STOP;
		LOGi("Force STOP");
		manualStopScan();
		//! no need to execute scan on stop is there? we want to stop after all
	//	executeScan();
	//	_running = false;
	} else if (_scanning) {
		manualStopScan();
	} else {
		LOGi(STR_ERR_ALREADY_STOPPED);
	}
}

void Scanner::executeScan() {

	if (!_running) return;

#ifdef PRINT_SCANNER_VERBOSE
	LOGd("Execute Scan");
#endif

	switch(_opCode) {
	case SCAN_START: {

		//! start scanning
		manualStartScan();
		if (_filterSendFraction > 0) {
			_scanCount = (_scanCount+1) % _filterSendFraction;
		}

		//! set timer to trigger in SCAN_DURATION sec, then stop again
		Timer::getInstance().start(_appTimerId, MS_TO_TICKS(_scanDuration), this);

		_opCode = SCAN_STOP;
		break;
	}
	case SCAN_STOP: {

		//! stop scanning
		manualStopScan();

#ifdef PRINT_DEBUG
		_scanResult->print();
#endif

//		//! Wait SCAN_SEND_WAIT ms before sending the results, so that it can listen to the mesh before sending
//		Timer::getInstance().start(_appTimerId, MS_TO_TICKS(_scanSendDelay), this);
//
//		_opCode = SCAN_SEND_RESULT;

		// Skip the sending the results
		//! Wait SCAN_BREAK ms, then start scanning again
		Timer::getInstance().start(_appTimerId, MS_TO_TICKS(_scanBreakDuration), this);

		_opCode = SCAN_START;
		break;
	}
	case SCAN_SEND_RESULT: {

		notifyResults();

		//! Wait SCAN_BREAK ms, then start scanning again
		Timer::getInstance().start(_appTimerId, MS_TO_TICKS(_scanBreakDuration), this);

		_opCode = SCAN_START;
		break;
	}
	}

}

void Scanner::notifyResults() {

#ifdef PRINT_SCANNER_VERBOSE
	LOGd("Notify scan results");
#endif

#if BUILD_MESHING == 1
	if (Settings::getInstance().isSet(CONFIG_MESH_ENABLED)) {
		MeshControl::getInstance().sendScanMessage(_scanResult->getList()->list, _scanResult->getSize());
	}
#endif

	buffer_ptr_t buffer;
	uint16_t length;
	_scanResult->getBuffer(buffer, length);

	EventDispatcher::getInstance().dispatch(EVT_SCANNED_DEVICES, buffer, length);
}

void Scanner::onBleEvent(ble_evt_t * p_ble_evt) {
	switch (p_ble_evt->header.evt_id) {
	case BLE_GAP_EVT_ADV_REPORT:
		onAdvertisement(&p_ble_evt->evt.gap_evt.params.adv_report);
		break;
	}
}

bool Scanner::isFiltered(data_t* p_adv_data) {
	//! If we want to send filtered scans once every N times, and now is that time, then just return false
	if (_filterSendFraction > 0 && _scanCount == 0) {
		return false;
	}

	data_t type_data;

	uint32_t err_code = BLEutil::findAdvType(BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA,
										 p_adv_data->data,
										 p_adv_data->len,
										 &type_data);
	if (err_code == NRF_SUCCESS) {
//		_logFirst(SERIAL_INFO, "found manufac data:");
//		BLEutil::printArray(type_data.p_data, type_data.data_len);

		//! [28.01.16] can't cast to uint16_t because it's possible that p_data is not
		//! word aligned!! So have to shift it by hand
		uint16_t companyIdentifier = type_data.data[1] << 8 | type_data.data[0];
		if (type_data.len >= 3 &&
			companyIdentifier == CROWNSTONE_COMPANY_ID) {
//			LOGi("is dobots device!");

//			_logFirst(SERIAL_INFO, "parse data");
//			BLEutil::printArray(type_data.p_data+2, type_data.data_len-2);
//
			CrownstoneManufacturer CrownstoneManufacturer;
			CrownstoneManufacturer.parse(type_data.data+2, type_data.len-2);

			switch (CrownstoneManufacturer.getDeviceType()) {
			case DEVICE_GUIDESTONE: {
//				LOGi("found guidestone");
				return _scanFilter & SCAN_FILTER_GUIDESTONE_MSK;
			}
			case DEVICE_CROWNSTONE_PLUG: {
//				LOGi("found crownstone");
				return _scanFilter & SCAN_FILTER_CROWNSTONE_MSK;
			}
			default:
				return false;
			}

		}
	}

	err_code = BLEutil::findAdvType(BLE_GAP_AD_TYPE_SERVICE_DATA,
										 p_adv_data->data,
										 p_adv_data->len,
										 &type_data);
	if (err_code == NRF_SUCCESS) {
		if (type_data.len == 19) {
			uint16_t companyIdentifier = type_data.data[1] << 8 | type_data.data[0];
			switch (companyIdentifier) {
			case GUIDESTONE_SERVICE_DATA_UUID: {
//				LOGi("found guidestone");
				return _scanFilter & SCAN_FILTER_GUIDESTONE_MSK;
			}
			case CROWNSTONE_PLUG_SERVICE_DATA_UUID:
			case CROWNSTONE_BUILT_SERVICE_DATA_UUID: {
//				LOGi("found crownstone");
				return _scanFilter & SCAN_FILTER_CROWNSTONE_MSK;
			}
			default:
				return false;
			}
		}
	}

	return false;
}

void Scanner::onAdvertisement(ble_gap_evt_adv_report_t* p_adv_report) {

	if (isScanning()) {

		EventDispatcher::getInstance().dispatch(EVT_DEVICE_SCANNED, p_adv_report, sizeof(ble_gap_evt_adv_report_t));

		// Skip updating the scanResult
		return;

		//! we do active scanning, to avoid handling each device twice, only
		//! check the scan responses (as long as we don't care about the
		//! advertisement data)
		if (p_adv_report->scan_rsp) {
            data_t adv_data;

            //! Initialize advertisement report for parsing.
            adv_data.data = (uint8_t *)p_adv_report->data;
            adv_data.len = p_adv_report->dlen;

			if (!isFiltered(&adv_data)) {
				_scanResult->update(p_adv_report->peer_addr.addr, p_adv_report->rssi);
			}
		}
	}
}

void Scanner::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
	switch (evt) {
	case EVT_BLE_EVENT: {
		onBleEvent((ble_evt_t*)p_data);
		break;
	}
	case CONFIG_SCAN_DURATION: {
		_scanDuration = *(uint32_t*)p_data;
		break;
	}
	case CONFIG_SCAN_SEND_DELAY: {
		_scanSendDelay = *(uint32_t*)p_data;
		break;
	}
	case CONFIG_SCAN_BREAK_DURATION: {
		_scanBreakDuration = *(uint32_t*)p_data;
		break;
	}
	case CONFIG_SCAN_FILTER: {
		_scanFilter = *(uint8_t*)p_data;
		break;
	}
	case CONFIG_SCAN_FILTER_SEND_FRACTION: {
		_filterSendFraction = *(uint32_t*)p_data;
		break;
	}
	}

}
