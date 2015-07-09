/**
 * Author: Christopher Mason
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Apr 23, 2015
 * License: LGPLv3+
 */

#include "ble/cs_Stack.h"

#include "cfg/cs_Config.h"
#include "ble/cs_Handlers.h"

#include "structs/buffer/cs_MasterBuffer.h"

#if CHAR_MESHING==1
extern "C" {
#include <third/protocol/rbc_mesh.h>
}
#endif

#include "sd_common/ble_stack_handler_types.h"

extern "C" {
#include "app_common/pstorage.h"
#include "app_common/app_util.h"
}

#include <drivers/cs_Storage.h>
#include <cfg/cs_Settings.h>
#include "util/cs_Utils.h"

using namespace BLEpp;

#define SEC_PARAM_TIMEOUT               30                                          /**< Timeout for Pairing Request or Security Request (in seconds). */
#define SEC_PARAM_BOND                  1                                           /**< Perform bonding. */
#define SEC_PARAM_MITM                  1                                           /**< Man In The Middle protection not required. */
#define SEC_PARAM_IO_CAPABILITIES       BLE_GAP_IO_CAPS_DISPLAY_ONLY				/**< No I/O capabilities. */
#define SEC_PARAM_OOB                   0                                           /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE          7                                           /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE          16                                          /**< Maximum encryption key size. */

//#define STATIC_PASSKEY              "123456"                                    /**< Static pin. */
uint8_t                             passkey[] = STATIC_PASSKEY;


Nrf51822BluetoothStack::Nrf51822BluetoothStack() :
				_appearance(defaultAppearance), _clock_source(defaultClockSource),
//				_mtu_size(defaultMtu),
				_tx_power_level(defaultTxPowerLevel), _sec_mode({ }),
				_interval(defaultAdvertisingInterval_0_625_ms), _timeout(defaultAdvertisingTimeout_seconds),
			  	_gap_conn_params( { }),
				_inited(false), _started(false), _advertising(false), _scanning(false),
				_conn_handle(BLE_CONN_HANDLE_INVALID),
				_radio_notify(0)
{
	// setup default values.

	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&_sec_mode);

	_gap_conn_params.min_conn_interval = defaultMinConnectionInterval_1_25_ms;
	_gap_conn_params.max_conn_interval = defaultMaxConnectionInterval_1_25_ms;
	_gap_conn_params.slave_latency = defaultSlaveLatencyCount;
	_gap_conn_params.conn_sup_timeout = defaultConnectionSupervisionTimeout_10_ms;
}

Nrf51822BluetoothStack::~Nrf51822BluetoothStack() {
	shutdown();
}

// called by softdevice handler through ble_evt_dispatch on any event that passes through mesh and is not write
extern "C" void ble_evt_handler(void* p_event_data, uint16_t event_size) {
	Nrf51822BluetoothStack::getInstance().on_ble_evt((ble_evt_t *)p_event_data);
}

// called by softdevice handler on a ble event
extern "C" void ble_evt_dispatch(ble_evt_t* p_ble_evt) {

//	LOGi("Dispatch event %i", p_ble_evt->header.evt_id);

#if CHAR_MESHING==1
	//  pass the incoming BLE event to the mesh framework
	BLE_CALL(rbc_mesh_ble_evt_handler, (p_ble_evt));
#endif

	// Only dispatch functions to the scheduler which might take long to execute, such as ble write functions
	// and handle other ble events directly in the interrupt. Otherwise app scheduler buffer might overflow fast
	switch (p_ble_evt->header.evt_id) {
	case BLE_GATTS_EVT_WRITE:
		// let the scheduler execute the event handle function
		BLE_CALL(app_sched_event_put, (p_ble_evt, sizeof (ble_evt_hdr_t) + p_ble_evt->header.evt_len, ble_evt_handler));
		break;
	default:
		ble_evt_handler(p_ble_evt, 0);
//		Nrf51822BluetoothStack::getInstance().on_ble_evt(p_ble_evt);
		break;
	}
}

void Nrf51822BluetoothStack::init() {

	if (_inited)
		return;

	// Initialise SoftDevice
	uint8_t enabled;
	BLE_CALL(sd_softdevice_is_enabled, (&enabled));
	if (enabled) {
		LOGw(MSG_BLE_SOFTDEVICE_RUNNING);
		BLE_CALL(sd_softdevice_disable, ());
	}

	LOGd(MSG_BLE_SOFTDEVICE_INIT);
	// Initialize the SoftDevice handler module.
	// this would call with different clock!
	SOFTDEVICE_HANDLER_INIT(_clock_source, false);

	// enable the BLE stack
	LOGd(MSG_BLE_SOFTDEVICE_ENABLE);

	// assign ble event handler, forwards ble_evt to stack
	BLE_CALL(softdevice_ble_evt_handler_set, (ble_evt_dispatch));

//#if(SOFTDEVICE_SERIES == 110)

#if ((SOFTDEVICE_SERIES == 130) && (SOFTDEVICE_MINOR != 5)) || \
	(SOFTDEVICE_SERIES == 110)
#if(NORDIC_SDK_VERSION >= 6)
	// do not define the service_changed characteristic, of course allow future changes
#define IS_SRVC_CHANGED_CHARACT_PRESENT  1
	ble_enable_params_t ble_enable_params;
	memset(&ble_enable_params, 0, sizeof(ble_enable_params));
	ble_enable_params.gatts_enable_params.service_changed = IS_SRVC_CHANGED_CHARACT_PRESENT;
	BLE_CALL(sd_ble_enable, (&ble_enable_params) );
#endif
#endif
//#endif

	// according to the migration guide the address needs to be set directly after the sd_ble_enable call
	// due to "an issue present in the s110_nrf51822_7.0.0 release"
#if(SOFTDEVICE_SERIES == 110)
#if(NORDIC_SDK_VERSION >= 7)
	LOGd(MSG_BLE_SOFTDEVICE_ENABLE_GAP);
	BLE_CALL(sd_ble_gap_enable, () );
	ble_gap_addr_t addr;
	BLE_CALL(sd_ble_gap_address_get, (&addr) );
	BLE_CALL(sd_ble_gap_address_set, (BLE_GAP_ADDR_CYCLE_MODE_NONE, &addr) );
#endif
#endif
	// version is not saved or shown yet
	ble_version_t version( { });
	version.company_id = 12;
	BLE_CALL(sd_ble_version_get, (&version));

	sd_nvic_EnableIRQ(SWI2_IRQn);

	if (!_device_name.empty()) {
		BLE_CALL(sd_ble_gap_device_name_set,
				(&_sec_mode, (uint8_t*) _device_name.c_str(), _device_name.length()));
	} else {
		std::string name = "not set...";
		BLE_CALL(sd_ble_gap_device_name_set,
				(&_sec_mode, (uint8_t*) name.c_str(), name.length()));
	}
	BLE_CALL(sd_ble_gap_appearance_set, (_appearance));

	setConnParams();

#if SOFTDEVICE_SERIES==130
	ble_opt_t static_pin_option;
	static_pin_option.gap_opt.passkey.p_passkey = passkey;
	BLE_CALL(sd_ble_opt_set, (BLE_GAP_OPT_PASSKEY, &static_pin_option));
#elif SOFTDEVICE_SERIES==110
	ble_opt_t static_pin_option;
	static_pin_option.gap.passkey.p_passkey = passkey;
	BLE_CALL(sd_ble_opt_set, (BLE_GAP_OPT_PASSKEY, &static_pin_option));
#endif

	setTxPowerLevel();

	BLE_CALL(softdevice_sys_evt_handler_set, (sys_evt_dispatch));

	_inited = true;

}

void Nrf51822BluetoothStack::updateDeviceName(const std::string& deviceName) {
	_device_name = deviceName;
	if (!_device_name.empty()) {
		BLE_CALL(sd_ble_gap_device_name_set,
				(&_sec_mode, (uint8_t*) _device_name.c_str(), _device_name.length()));
	} else {
		std::string name = "not set...";
		BLE_CALL(sd_ble_gap_device_name_set,
				(&_sec_mode, (uint8_t*) name.c_str(), name.length()));
	}
}

void Nrf51822BluetoothStack::updateAppearance(uint16_t appearance) {
	_appearance = appearance;
	BLE_CALL(sd_ble_gap_appearance_set, (_appearance));
}

void Nrf51822BluetoothStack::startAdvertisingServices() {
	if (_started)
		return;

	for (Service* svc : _services) {
		svc->startAdvertising(this);
	}

	_started = true;
}

void Nrf51822BluetoothStack::startTicking() {
	LOGi("Start ticking ...");
	for (Service* svc : _services) {
		svc->startTicking();
	}
}

void Nrf51822BluetoothStack::stopTicking() {
	LOGi("Stop ticking ...");
	for (Service* svc : _services) {
		svc->stopTicking();
	}
}

void Nrf51822BluetoothStack::shutdown() {
	stopAdvertising();

	for (Service* svc : _services) {
		svc->stopAdvertising();
	}

	_inited = false;
}

void Nrf51822BluetoothStack::addService(Service* svc) {
	//APP_ERROR_CHECK_BOOL(_services.size() < MAX_SERVICE_COUNT);

	_services.push_back(svc);
}

void Nrf51822BluetoothStack::setTxPowerLevel() {
	BLE_CALL(sd_ble_gap_tx_power_set, (_tx_power_level));
}

void Nrf51822BluetoothStack::setConnParams() {
	BLE_CALL(sd_ble_gap_ppcp_set, (&_gap_conn_params));
}

// accepted values are -40, -30, -20, -16, -12, -8, -4, 0, and 4 dBm
// Can be done at any moment (also when advertising)
void Nrf51822BluetoothStack::setTxPowerLevel(int8_t powerLevel) {
	LOGd("Set tx power to %d", powerLevel);
	switch (powerLevel) {
	case -40: break;
	case -30: break;
	case -20: break;
	case -16: break;
	case -12: break;
	case -8: break;
	case -4: break;
	case 0: break;
	case 4: break;
	default: return;
	}
	if (_tx_power_level != powerLevel) {
		_tx_power_level = powerLevel;
		if (_inited)
			setTxPowerLevel();
	}
}

void Nrf51822BluetoothStack::startIBeacon(IBeacon* beacon) {
	if (_advertising)
		return;

	LOGi("startIBeacon ...");

	init(); // we should already be.

	startAdvertisingServices();

	uint32_t err_code __attribute__((unused));
	ble_advdata_t advdata;
	uint8_t flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
	//	uint8_t flags = BLE_GAP_ADV_FLAG_LE_GENERAL_DISC_MODE | BLE_GAP_ADV_FLAG_LE_BR_EDR_CONTROLLER
	//			| BLE_GAP_ADV_FLAG_LE_BR_EDR_HOST;

	/*
	 * 31 bytes total payload
	 *
	 * 1 byte per element for the length of it's data.
	 * in this case, 4 elements -> 4 bytes
	 *
	 * => 27 bytes available payload
	 *
	 * 3 bytes for major (1 byte for type, 2 byte for data)
	 * 3 bytes for minor (1 byte for type, 2 byte for data)
	 * 17 bytes for UUID (1 byte for type, 16 byte for data)
	 * 2 bytes for rssi (1 byte for type, 1 byte for data)
	 *
	 * -> 2 bytes left => FULL
	 *
	 * Note: each element has an overhead of 2 bytes. Since there
	 * are only 2 bytes left, there is no more space left for additional
	 * data
	 */
	ble_gap_adv_params_t adv_params;

	adv_params.type = BLE_GAP_ADV_TYPE_ADV_IND;
	adv_params.p_peer_addr = NULL;                   // Undirected advertisement
	adv_params.fp = BLE_GAP_ADV_FP_ANY;
	adv_params.interval = _interval;
	adv_params.timeout = _timeout;

	ble_advdata_manuf_data_t manufac;
	manufac.company_identifier = 0x004C; // Apple Company ID, if it is not set to apple it's not recognized as an iBeacon

	uint8_t adv_manuf_data[beacon->size()];
	memset(adv_manuf_data, 0, sizeof(adv_manuf_data));
	beacon->toArray(adv_manuf_data);

	manufac.data.p_data = adv_manuf_data;
	manufac.data.size = beacon->size();

	// Build and set advertising data
	memset(&advdata, 0, sizeof(advdata));

	advdata.flags.size = sizeof(flags);
	advdata.flags.p_data = &flags;
	advdata.p_manuf_specific_data = &manufac;

	/*
	 * 31 bytes total payload
	 *
	 * 1 byte per element for the length of it's data.
	 * in this case, 1 element -> 1 byte
	 *
	 * 30 bytes of available payload
	 *
	 * 1 byte for name type
	 * -> 29 bytes left for name
	 */
	ble_advdata_t scan_resp;
	memset(&scan_resp, 0, sizeof(scan_resp));

	//	uint8_t uidCount = _services.size();
	//	ble_uuid_t adv_uuids[uidCount];
	//
	//	uint8_t cnt = 0;
	//	for(Service* svc : _services ) {
	//		adv_uuids[cnt++] = svc->getUUID();
	//	}
	//
	//	if (cnt == 0) {
	//		LOGw("No custom services!");
	//	}

	scan_resp.name_type = BLE_ADVDATA_FULL_NAME;
	//	scan_resp.uuids_more_available.uuid_cnt = 1;
	//	scan_resp.uuids_more_available.p_uuids  = adv_uuids;

	BLE_CALL(ble_advdata_set, (&advdata, &scan_resp));

	BLE_CALL(sd_ble_gap_adv_start, (&adv_params));

	_advertising = true;

	LOGi("... OK");
}

void Nrf51822BluetoothStack::startAdvertising() {
	if (_advertising)
		return;

	LOGi(MSG_BLE_ADVERTISING_START);

	init(); // we should already be.

	startAdvertisingServices();

	uint32_t err_code __attribute__((unused));
	ble_advdata_t advdata;
	uint8_t flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

	uint8_t uidCount = _services.size();
	LOGi("Number of services: %u", uidCount);

	ble_uuid_t adv_uuids[uidCount];

	uint8_t cnt = 0;
	for (Service* svc : _services) {
		adv_uuids[cnt++] = svc->getUUID();
	}

	if (cnt == 0) {
		LOGw(MSG_BLE_NO_CUSTOM_SERVICES);
	}

	ble_gap_adv_params_t adv_params;

	adv_params.type = BLE_GAP_ADV_TYPE_ADV_IND;
	adv_params.p_peer_addr = NULL;                   // Undirected advertisement
	adv_params.fp = BLE_GAP_ADV_FP_ANY;
	adv_params.interval = _interval;
	adv_params.timeout = _timeout;

	// Build and set advertising data
	memset(&advdata, 0, sizeof(advdata));

	ble_advdata_manuf_data_t manufac;
	// TODO: made up ID, has to be replaced by official ID
	manufac.company_identifier = 0x1111; // DoBots Company ID
	manufac.data.size = 0;

	//	advdata.name_type               = BLE_ADVDATA_NO_NAME;

	/*
	 * 31 bytes total payload
	 *
	 * 1 byte per element for the length of it's data.
	 * in this case, 3 elements -> 3 bytes
	 *
	 * => 28 bytes available payload
	 *
	 * 2 bytes for flags (1 byte for type, 1 byte for data)
	 * 2 bytes for tx power level (1 byte for type, 1 byte for data)
	 * 17 bytes for UUID (1 byte for type, 16 byte for data)
	 *
	 * -> 7 bytes left
	 *
	 * Note: by adding an element, one byte will be lost because of the
	 *   addition, so there are actually only 6 bytes left after adding
	 *   an element, and 1 byte of that is used for the type, so 5 bytes
	 *   are left for the data
	 */

	// Anne: setting NO_NAME breaks the Android Nordic nRF Master Console app.
	advdata.p_tx_power_level = &_tx_power_level;
	advdata.flags.size = sizeof(flags);
	advdata.flags.p_data = &flags;
	advdata.p_manuf_specific_data = &manufac;
	// TODO -oDE: It doesn't really make sense to have this function in the library
	//  because it really depends on the application on how many and what kind
	//  of services are available and what should be advertised
	//#ifdef YOU_WANT_TO_USE_SPACE
	if (uidCount > 1) {
		advdata.uuids_more_available.uuid_cnt = 1;
		advdata.uuids_more_available.p_uuids = adv_uuids;
	} else {
		advdata.uuids_complete.uuid_cnt = 1;
		advdata.uuids_complete.p_uuids = adv_uuids;
	}
	//#endif

	// Because of the limited amount of space in the advertisement data, additional
	// data can be supplied in the scan response package. Same space restrictions apply
	// here:
	/*
	 * 31 bytes total payload
	 *
	 * 1 byte per element for the length of it's data.
	 * in this case, 1 element -> 1 byte
	 *
	 * 30 bytes of available payload
	 *
	 * 1 byte for name type
	 * -> 29 bytes left for name
	 */
	ble_advdata_t scan_resp;
	memset(&scan_resp, 0, sizeof(scan_resp));
	scan_resp.name_type = BLE_ADVDATA_FULL_NAME;

	err_code = ble_advdata_set(&advdata, &scan_resp);
	if (err_code == NRF_ERROR_DATA_SIZE) {
		log(FATAL, MSG_BLE_ADVERTISEMENT_TOO_BIG);
	}
	APP_ERROR_CHECK(err_code);

	// segfault when advertisement cannot start, do we want that!?
	//BLE_CALL(sd_ble_gap_adv_start, (&adv_params));
	err_code = sd_ble_gap_adv_start(&adv_params);
	if (err_code == NRF_ERROR_INVALID_PARAM) {
		log(FATAL, MSG_BLE_ADVERTISEMENT_CONFIG_INVALID);
	}
	APP_ERROR_CHECK(err_code);

	_advertising = true;

	LOGi(MSG_BLE_ADVERTISING_STARTED);
}

void Nrf51822BluetoothStack::stopAdvertising() {
	if (!_advertising)
		return;

	BLE_CALL(sd_ble_gap_adv_stop, ());

	_advertising = false;
}

bool Nrf51822BluetoothStack::isAdvertising() {
	return _advertising;
}

void Nrf51822BluetoothStack::startScanning() {
#if(SOFTDEVICE_SERIES != 110)
	if (_scanning)
		return;

	LOGi("startScanning");
	ble_gap_scan_params_t p_scan_params;
	// No devices in whitelist, hence non selective performed.
	p_scan_params.active = 0;            // Active scanning set.
	p_scan_params.selective = 0;            // Selective scanning not set.
	p_scan_params.interval = SCAN_INTERVAL;            // Scan interval.
	p_scan_params.window = SCAN_WINDOW;  // Scan window.
	p_scan_params.p_whitelist = NULL;         // No whitelist provided.
	p_scan_params.timeout = 0x0000;       // No timeout.

	// todo: which fields to set here?
	BLE_CALL(sd_ble_gap_scan_start, (&p_scan_params));
	_scanning = true;
#endif
}


void Nrf51822BluetoothStack::stopScanning() {
#if(SOFTDEVICE_SERIES != 110)
	if (!_scanning)
		return;

	LOGi("stopScanning");
	BLE_CALL(sd_ble_gap_scan_stop, ());
	_scanning = false;
#endif
}

bool Nrf51822BluetoothStack::isScanning() {
#if(SOFTDEVICE_SERIES != 110)
	return _scanning;
#endif
	return false;
}

//Nrf51822BluetoothStack& Nrf51822BluetoothStack::onRadioNotificationInterrupt(
//		uint32_t distanceUs, callback_radio_t callback) {
//	_callback_radio = callback;
//
//	nrf_radio_notification_distance_t distance = NRF_RADIO_NOTIFICATION_DISTANCE_NONE;
//
//	if (distanceUs >= 5500) {
//		distance = NRF_RADIO_NOTIFICATION_DISTANCE_5500US;
//	} else if (distanceUs >= 4560) {
//		distance = NRF_RADIO_NOTIFICATION_DISTANCE_4560US;
//	} else if (distanceUs >= 3620) {
//		distance = NRF_RADIO_NOTIFICATION_DISTANCE_3620US;
//	} else if (distanceUs >= 2680) {
//		distance = NRF_RADIO_NOTIFICATION_DISTANCE_2680US;
//	} else if (distanceUs >= 1740) {
//		distance = NRF_RADIO_NOTIFICATION_DISTANCE_1740US;
//	} else if (distanceUs >= 800) {
//		distance = NRF_RADIO_NOTIFICATION_DISTANCE_800US;
//	}
//
//	uint32_t result = sd_nvic_SetPriority(SWI1_IRQn, NRF_APP_PRIORITY_LOW);
//	BLE_THROW_IF(result, "Could not set radio notification IRQ priority.");
//
//	result = sd_nvic_EnableIRQ(SWI1_IRQn);
//	BLE_THROW_IF(result, "Could not enable radio notification IRQ.");
//
//	result = sd_radio_notification_cfg_set(
//			distance == NRF_RADIO_NOTIFICATION_DISTANCE_NONE ?
//					NRF_RADIO_NOTIFICATION_TYPE_NONE :
//					NRF_RADIO_NOTIFICATION_TYPE_INT_ON_BOTH, distance);
//	BLE_THROW_IF(result, "Could not configure radio notification.");
//
//	return *this;
//}

//extern "C" {
//	void SWI1_IRQHandler() { // radio notification IRQ handler
//		uint8_t radio_notify;
//		Nrf51822BluetoothStack::_stack->_radio_notify = radio_notify = ( Nrf51822BluetoothStack::_stack->_radio_notify + 1) % 2;
//		Nrf51822BluetoothStack::_stack->_callback_radio(radio_notify == 1);
//	}
//
//	void SWI2_IRQHandler(void) { // sd event IRQ handler
//		// do nothing.
//	}
//}

//void Nrf51822BluetoothStack::tick() {
//
//#if TICK_CONTINUOUSLY==0
//#if(NORDIC_SDK_VERSION > 5)
//	BLE_CALL(sd_app_evt_wait, ());
//#else
//	BLE_CALL(sd_app_event_wait, () );
//#endif
//#endif
//	while (1) {
//
//		//        uint8_t nested;
//		//        sd_nvic_critical_region_enter(&nested);
//		//        uint8_t radio_notify = _radio_notify = (radio_notify + 1) % 4;
//		//        sd_nvic_critical_region_exit(nested);
//		//        if ((radio_notify % 2 == 0) && _callback_radio) {
//		//            _callback_radio(radio_notify == 0);
//		//        }
//
//		uint16_t evt_len = _evt_buffer_size;
//		uint32_t err_code = sd_ble_evt_get(_evt_buffer, &evt_len);
//		if (err_code == NRF_ERROR_NOT_FOUND) {
//			break;
//		}
//		BLE_THROW_IF(err_code, "Error retrieving bluetooth event from stack.");
//
//		on_ble_evt((ble_evt_t *) _evt_buffer);
//
//	}
//}


/**@brief Function for handling the Device Manager events.
 *
 * @param[in]   p_evt   Data associated to the device manager event.
 */
static uint32_t device_manager_evt_handler(dm_handle_t const    * p_handle,
                                           dm_event_t const     * p_event,
                                           api_result_t           event_result)
{
	return Nrf51822BluetoothStack::getInstance().deviceManagerEvtHandler(p_handle, p_event, event_result);
}

#define SECURITY_REQUEST_DELAY          APP_TIMER_TICKS(4000, APP_TIMER_PRESCALER)  /**< Delay after connection until Security Request is sent, if necessary (ticks). */

uint32_t Nrf51822BluetoothStack::deviceManagerEvtHandler(dm_handle_t const    * p_handle,
                                           dm_event_t const     * p_event,
                                           api_result_t           event_result)
{
	LOGd("deviceManagerEvtHandler: 0x%X", p_event->event_id);

    uint32_t err_code = NRF_SUCCESS;

    m_dm_handle = *p_handle;
    APP_ERROR_CHECK(event_result);
    switch (p_event->event_id)
    {
        case DM_EVT_CONNECTION:
        	LOGi("DM_EVT_CONNECTION");
            // Start Security Request timer.
            if (m_dm_handle.device_id != DM_INVALID_ID)
            {
            	LOGi("start sec timer");
//                err_code = app_timer_start(m_sec_req_timer_id, SECURITY_REQUEST_DELAY, NULL);
//                APP_ERROR_CHECK(err_code);
            }
            break;
        default:
            break;
    }
    return NRF_SUCCESS;
}

/**@brief Function for the Device Manager initialization.
 */
//void Nrf51822BluetoothStack::device_manager_init()
//{
//    uint32_t               err_code;
//    dm_init_param_t        init_data;
//    dm_application_param_t register_param;
//
//    // Initialize persistent storage module.
////    err_code = pstorage_init();
////    APP_ERROR_CHECK(err_code);
//
//    // Clear all bonded centrals if the Bonds Delete button is pushed.
//    init_data.clear_persistent_data = true;
////    init_data.clear_persistent_data = false;
//
//    err_code = dm_init(&init_data);
//    APP_ERROR_CHECK(err_code);
//
//    memset(&register_param.sec_param, 0, sizeof(ble_gap_sec_params_t));
//
//#if(SOFTDEVICE_SERIES == 110)
//    register_param.sec_param.timeout = 30; // seconds
//#endif
//    register_param.sec_param.bond = 1;  // perform bonding.
//    register_param.sec_param.mitm = 1;  // man in the middle protection not required.
//    register_param.sec_param.io_caps = BLE_GAP_IO_CAPS_NONE;  // no display capabilities.
////    register_param.sec_param.io_caps = BLE_GAP_IO_CAPS_KEYBOARD_DISPLAY;  // no display capabilities.
//    register_param.sec_param.oob = 0;  // out of band not available.
//    register_param.sec_param.min_key_size = 7;  // min key size
//    register_param.sec_param.max_key_size = 16; // max key size.
//    register_param.evt_handler            = device_manager_evt_handler;
//    register_param.service_type           = DM_PROTOCOL_CNTXT_ALL;
//
//    BLE_CALL(dm_register, (&m_app_handle, &register_param));
//
////    ble_opt_t static_pin_option;
////    static_pin_option.gap.passkey.p_passkey = passkey;
////    err_code =  sd_ble_opt_set(BLE_GAP_OPT_PASSKEY, &static_pin_option);
//}


void Nrf51822BluetoothStack::device_manager_init()
{
    uint32_t               err_code;
    dm_init_param_t        init_data;
    dm_application_param_t register_param;

    // Initialize persistent storage module.
    err_code = pstorage_init();
    APP_ERROR_CHECK(err_code);

    // Clear all bonded centrals if the Bonds Delete button is pushed.
//    init_data.clear_persistent_data = 1;// (nrf_gpio_pin_read(BOND_DELETE_ALL_BUTTON_ID) == 0);
    init_data.clear_persistent_data = 0;// (nrf_gpio_pin_read(BOND_DELETE_ALL_BUTTON_ID) == 0);

    err_code = dm_init(&init_data);
    APP_ERROR_CHECK(err_code);

    memset(&register_param.sec_param, 0, sizeof(ble_gap_sec_params_t));

#if SOFTDEVICE_SERIES==110
    register_param.sec_param.timeout      = SEC_PARAM_TIMEOUT;
#endif
    register_param.sec_param.bond         = SEC_PARAM_BOND;
    register_param.sec_param.oob          = SEC_PARAM_OOB;
    register_param.sec_param.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
    register_param.sec_param.max_key_size = SEC_PARAM_MAX_KEY_SIZE;
    register_param.evt_handler            = device_manager_evt_handler;
    register_param.service_type           = DM_PROTOCOL_CNTXT_GATT_SRVR_ID;

    // Using static pin:
    register_param.sec_param.mitm    = SEC_PARAM_MITM;
    register_param.sec_param.io_caps = SEC_PARAM_IO_CAPABILITIES;

    err_code = dm_register(&m_app_handle, &register_param);
    APP_ERROR_CHECK(err_code);

    LOGi("device_manager_init");
}

void Nrf51822BluetoothStack::on_ble_evt(ble_evt_t * p_ble_evt) {
	if (p_ble_evt->header.evt_id != BLE_GAP_EVT_RSSI_CHANGED) {
		LOGi("on_ble_event: 0x%X", p_ble_evt->header.evt_id);
	}

	switch (p_ble_evt->header.evt_id) {
	case BLE_GAP_EVT_CONNECTED:
		_log(INFO, "address: " );
		BLEutil::printArray(p_ble_evt->evt.gap_evt.params.connected.peer_addr.addr, BLE_GAP_ADDR_LEN);
		on_connected(p_ble_evt);
		break;

	case BLE_EVT_USER_MEM_REQUEST: {

		buffer_ptr_t buffer = NULL;
		uint16_t size = 0;
		MasterBuffer::getInstance().getBuffer(buffer, size);

		_user_mem_block.len = size+6;
		_user_mem_block.p_mem = buffer-6;
		BLE_CALL(sd_ble_user_mem_reply, (getConnectionHandle(), &_user_mem_block));

		break;
	}

	case BLE_EVT_USER_MEM_RELEASE:
		// nothing to do
		break;

	case BLE_GAP_EVT_DISCONNECTED:
		on_disconnected(p_ble_evt);
		break;

	case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST:
	case BLE_GATTS_EVT_TIMEOUT:
	case BLE_GAP_EVT_RSSI_CHANGED:
		for (Service* svc : _services) {
			svc->on_ble_event(p_ble_evt);
		}
		break;

	case BLE_GATTS_EVT_WRITE: {

		if (p_ble_evt->evt.gatts_evt.params.write.op == BLE_GATTS_OP_EXEC_WRITE_REQ_NOW) {

			buffer_ptr_t buffer = NULL;
			uint16_t size = 0;
			MasterBuffer::getInstance().getBuffer(buffer, size, 0);

			uint16_t* header = (uint16_t*)buffer;

			for (Service* svc : _services) {
				svc->on_write(p_ble_evt->evt.gatts_evt.params.write, header[0]);
			}

		} else {
			for (Service* svc : _services) {
				if (svc->getHandle() == p_ble_evt->evt.gatts_evt.params.write.context.srvc_handle) {
					svc->on_write(p_ble_evt->evt.gatts_evt.params.write, p_ble_evt->evt.gatts_evt.params.write.handle);
					return;
				}
			}
		}

		break;
	}

	case BLE_GATTS_EVT_HVC:
		for (Service* svc : _services) {
			if (svc->getHandle() == p_ble_evt->evt.gatts_evt.params.write.context.srvc_handle) {
				svc->on_ble_event(p_ble_evt);
				return;
			}
		}
		break;

	case BLE_GATTS_EVT_SYS_ATTR_MISSING:
		BLE_CALL(sd_ble_gatts_sys_attr_set, (_conn_handle, NULL, 0));
		break;

	case BLE_GAP_EVT_AUTH_KEY_REQUEST:
		LOGi("reply pass key");
//		BLE_CALL(sd_ble_gap_auth_key_reply, (getConnectionHandle(), BLE_GAP_AUTH_KEY_TYPE_PASSKEY, passkey));
		break;

	case BLE_GAP_EVT_PASSKEY_DISPLAY: {
		char* p_passkey = (char *)p_ble_evt->evt.gap_evt.params.passkey_display.passkey;
		LOGi("PASSKEY: %s", p_passkey);
		break;
	}

    case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
    case BLE_GAP_EVT_SEC_INFO_REQUEST:
//        case BLE_GAP_EVT_PASSKEY_DISPLAY:
        // Don't send delayed Security Request if security procedure is already in progress.
    	LOGi("stop sec timer");
//        BLE_CALL(app_timer_stop, (m_sec_req_timer_id));
        break;

//    case BLE_GAP_EVT_AUTH_STATUS:
//        //Disconnect and start advertising again if you get PIN or KEY missing
//        break;

//	case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
//
//		// go into low power mode
//		LOGi("going into low power mode ...");
//		setTxPowerLevel(-40);

//		ble_gap_sec_params_t sec_params;
//
//#if(SOFTDEVICE_SERIES == 110)
//		sec_params.timeout = 30; // seconds
//#endif
//		sec_params.bond = 1;  // perform bonding.
//		sec_params.mitm = 1;  // man in the middle protection required.
//		sec_params.io_caps = BLE_GAP_IO_CAPS_NONE;  // no display capabilities.
////		sec_params.io_caps = BLE_GAP_IO_CAPS_KEYBOARD_ONLY;  // no display capabilities.
//		sec_params.oob = 0;  // out of band not available.
//		sec_params.min_key_size = 7;  // min key size
//		sec_params.max_key_size = 16; // max key size.
//
//#if(SOFTDEVICE_SERIES != 110)
//		// https://devzone.nordicsemi.com/documentation/nrf51/6.0.0/s120/html/a00527.html#ga7b23027c97b3df21f6cbc23170e55663
//
//		// do not store the keys for now...
//		ble_gap_sec_keyset_t sec_keyset;
//		BLE_CALL(sd_ble_gap_sec_params_reply, (p_ble_evt->evt.gap_evt.conn_handle,
//					BLE_GAP_SEC_STATUS_SUCCESS,
//					&sec_params, &sec_keyset) );
////		BLE_CALL(sd_ble_gap_sec_params_reply,
////				(p_ble_evt->evt.gap_evt.conn_handle, BLE_GAP_SEC_STATUS_SUCCESS, &sec_params, NULL));
//#else
//		BLE_CALL(sd_ble_gap_sec_params_reply, (p_ble_evt->evt.gap_evt.conn_handle,
//				BLE_GAP_SEC_STATUS_SUCCESS,
//				&sec_params) );
//#endif
//		break;
//
//    case BLE_GAP_EVT_AUTH_STATUS:
//        m_auth_status = p_ble_evt->evt.gap_evt.params.auth_status;
//		LOGi("auth status: %X", m_auth_status.auth_status);
//
//		if (m_auth_status.auth_status == BLE_GAP_SEC_STATUS_SUCCESS) {
//			// go into high power mode
//			LOGi("going back into default power mode ...");
//
//			ps_configuration_t cfg = Settings::getInstance().getConfig();
//			setTxPowerLevel(cfg.txPower);
//		}
//
//        break;
//
//    case BLE_GAP_EVT_SEC_INFO_REQUEST:
//        ble_gap_enc_info_t *             p_enc_info;
//        p_enc_info = &m_auth_status.periph_keys.enc_info;
//        if (p_enc_info->div == p_ble_evt->evt.gap_evt.params.sec_info_request.div)
//        {
//            BLE_CALL(sd_ble_gap_sec_info_reply, (getConnectionHandle(), p_enc_info, NULL));
//        }
//        else
//        {
//            // No keys found for this device
//            BLE_CALL(sd_ble_gap_sec_info_reply, (getConnectionHandle(), NULL, NULL));
//        }
//        break;

#if(SOFTDEVICE_SERIES != 110)
	case BLE_GAP_EVT_ADV_REPORT:
		for (Service* svc : _services) {
			svc->on_ble_event(p_ble_evt);
		}
		break;
#endif
	case BLE_GAP_EVT_TIMEOUT:
		break;

	case BLE_EVT_TX_COMPLETE:
		onTxComplete(p_ble_evt);
		break;

	default:
		break;

	}

	dm_ble_evt_handler(p_ble_evt);
}

void Nrf51822BluetoothStack::on_connected(ble_evt_t * p_ble_evt) {
	//ble_gap_evt_connected_t connected_evt = p_ble_evt->evt.gap_evt.params.connected;
	_advertising = false; // it seems like maybe we automatically stop advertising when we're connected.

	BLE_CALL(sd_ble_gap_conn_param_update, (p_ble_evt->evt.gap_evt.conn_handle, &_gap_conn_params));

	if (_callback_connected) {
		_callback_connected(p_ble_evt->evt.gap_evt.conn_handle);
	}
	_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
	for (Service* svc : _services) {
		svc->on_ble_event(p_ble_evt);
	}
}

void Nrf51822BluetoothStack::on_disconnected(ble_evt_t * p_ble_evt) {
	//ble_gap_evt_disconnected_t disconnected_evt = p_ble_evt->evt.gap_evt.params.disconnected;
	if (_callback_connected) {
		_callback_disconnected(p_ble_evt->evt.gap_evt.conn_handle);
	}
	_conn_handle = BLE_CONN_HANDLE_INVALID;
	for (Service* svc : _services) {
		svc->on_ble_event(p_ble_evt);
	}
}

void Nrf51822BluetoothStack::onTxComplete(ble_evt_t * p_ble_evt) {
	for (Service* svc: _services) {
		svc->onTxComplete(&p_ble_evt->evt.common_evt);
	}
}



