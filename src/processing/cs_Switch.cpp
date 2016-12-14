/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 19, 2016
 * License: LGPLv3+
 */

#include <processing/cs_Switch.h>

#include <cfg/cs_Boards.h>
#include <cfg/cs_Config.h>
#include <drivers/cs_Serial.h>
#include <drivers/cs_PWM.h>
#include <storage/cs_State.h>
#include <storage/cs_Settings.h>
#include <cfg/cs_Strings.h>
#include <ble/cs_Stack.h>
#include <processing/cs_Scanner.h>
#include <events/cs_EventDispatcher.h>

//#define PRINT_SWITCH_VERBOSE

// [18.07.16] added for first version of plugins to disable the use of the igbt
//#define PWM_DISABLE

#define MODIFICATION_110V

Switch::Switch():
#if (NORDIC_SDK_VERSION >= 11)
	_relayTimerId(NULL),
#else
	_relayTimerId(UINT32_MAX),
#endif
	_nextRelayVal(SWITCH_NEXT_RELAY_VAL_NONE)
{
	LOGd(FMT_CREATE, "Switch");
#if (NORDIC_SDK_VERSION >= 11)
	_relayTimerData = { {0} };
	_relayTimerId = &_relayTimerData;
#endif
}

void Switch::init() {

#ifndef PWM_DISABLE
	PWM& pwm = PWM::getInstance();
//	pwm.init(PWM::config1Ch(1600L, PIN_GPIO_SWITCH)); //! 625 Hz
//	pwm.init(pwm.config1Ch(20000L, PIN_GPIO_SWITCH)); //! 50 Hz
	uint32_t pwmPeriod;
	Settings::getInstance().get(CONFIG_PWM_PERIOD, &pwmPeriod);
	pwm.init(pwm.config1Ch(pwmPeriod, PIN_GPIO_SWITCH)); //! 50 Hz
#else
	nrf_gpio_cfg_output(PIN_GPIO_SWITCH);
#ifdef SWITCH_INVERSED
	nrf_gpio_pin_set(PIN_GPIO_SWITCH);
#else
	nrf_gpio_pin_clear(PIN_GPIO_SWITCH);
#endif
#endif

	// TODO: this could be too early! Maybe pstorage is not ready yet?
//	State::getInstance().get(STATE_SWITCH_STATE, &_switchValue, 1);

	// [23.06.16] overwrites stored value, so we can't restore old switch state
	//	setValue(0);

#if HAS_RELAY
	nrf_gpio_cfg_output(PIN_GPIO_RELAY_OFF);
	nrf_gpio_pin_clear(PIN_GPIO_RELAY_OFF);
	nrf_gpio_cfg_output(PIN_GPIO_RELAY_ON);
	nrf_gpio_pin_clear(PIN_GPIO_RELAY_ON);
#endif

	EventDispatcher::getInstance().addListener(this);
	Timer::getInstance().createSingleShot(_relayTimerId, (app_timer_timeout_handler_t)Switch::staticTimedSetRelay);
}

void Switch::start() {
	State::getInstance().get(STATE_SWITCH_STATE, &_switchValue, 1);
	LOGd("switch state: pwm=%u relay=%u", _switchValue.pwm_state, _switchValue.relay_state);
	setPwm(_switchValue.pwm_state);
}

void Switch::pwmOff() {

#ifdef PRINT_SWITCH_VERBOSE
	LOGd("PWM OFF");
#endif

	setPwm(0);
}

void Switch::pwmOn() {

#ifdef PRINT_SWITCH_VERBOSE
	LOGd("PWM ON");
#endif

	setPwm(255);
}

void Switch::dim(uint8_t value) {

#ifdef PRINT_SWITCH_VERBOSE
	LOGd("PWM %d", value);
#endif

	setPwm(value);
}

void Switch::updateSwitchState() {
	State::getInstance().set(STATE_SWITCH_STATE, &_switchValue, sizeof(switch_state_t));
}

void Switch::setPwm(uint8_t value) {
#ifndef PWM_DISABLE
#ifdef EXTENDED_SWITCH_STATE
	_switchValue.pwm_state = value;
#else
	_switchValue = value;
#endif
	PWM::getInstance().setValue(0, value);
	updateSwitchState();
#else
	LOGe("PWM disabled!!");
#endif
}

void Switch::setValue(switch_state_t value) {
	_switchValue = value;
}

switch_state_t Switch::getValue() {
#ifdef PRINT_SWITCH_VERBOSE
	LOGd(FMT_GET_INT_VAL, "Switch state", _switchValue);
#endif
	return _switchValue;
}

uint8_t Switch::getPwm() {
#ifdef EXTENDED_SWITCH_STATE
	return _switchValue.pwm_state;
#else
	return _switchValue;
#endif
}

bool Switch::getRelayState() {
#ifdef EXTENDED_SWITCH_STATE
	return _switchValue.relay_state != 0;
#else
	return _switchValue != 0;
#endif
}

void Switch::relayOn() {
	LOGd("relayOn %u", _nextRelayVal);
#ifdef MODIFICATION_110V
	if (Nrf51822BluetoothStack::getInstance().isScanning()) {
		if (_nextRelayVal != SWITCH_NEXT_RELAY_VAL_NONE) {
			_nextRelayVal = SWITCH_NEXT_RELAY_VAL_ON;
			return;
		}
		//! Try again later
		LOGd("Currently scanning, try again later");
		_nextRelayVal = SWITCH_NEXT_RELAY_VAL_ON;
//		Timer::getInstance().start(_relayTimerId, MS_TO_TICKS(RELAY_DELAY), this);
		Scanner::getInstance().pause(); // TODO: stop scanning via stack, let stack send out an event?
		return;
	}
	_nextRelayVal = SWITCH_NEXT_RELAY_VAL_NONE;
#endif

	uint16_t relayHighDuration;
	Settings::getInstance().get(CONFIG_RELAY_HIGH_DURATION, &relayHighDuration);

#ifdef PRINT_SWITCH_VERBOSE
	LOGd("trigger relay on pin for %d ms", relayHighDuration);
#endif

#if HAS_RELAY
	nrf_gpio_pin_set(PIN_GPIO_RELAY_ON);
	nrf_delay_ms(relayHighDuration);
	nrf_gpio_pin_clear(PIN_GPIO_RELAY_ON);

#ifdef EXTENDED_SWITCH_STATE
	_switchValue.relay_state = 1;
#else
	_switchValue = 255;
#endif
	updateSwitchState();
#endif
}

void Switch::relayOff() {
	LOGd("relayOff %u", _nextRelayVal);
#ifdef MODIFICATION_110V
	if (Nrf51822BluetoothStack::getInstance().isScanning()) {
		if (_nextRelayVal != SWITCH_NEXT_RELAY_VAL_NONE) {
			_nextRelayVal = SWITCH_NEXT_RELAY_VAL_OFF;
			return;
		}
		//! Try again later
		LOGd("Currently scanning, try again later");
		_nextRelayVal = SWITCH_NEXT_RELAY_VAL_OFF;
//		Timer::getInstance().start(_relayTimerId, MS_TO_TICKS(RELAY_DELAY), this);
		Scanner::getInstance().pause(); // TODO: stop scanning via stack, let stack send out an event?
		return;
	}
	_nextRelayVal = SWITCH_NEXT_RELAY_VAL_NONE;
#endif

	uint16_t relayHighDuration;
	Settings::getInstance().get(CONFIG_RELAY_HIGH_DURATION, &relayHighDuration);

#ifdef PRINT_SWITCH_VERBOSE
	LOGi("trigger relay off pin for %d ms", relayHighDuration);
#endif

#if HAS_RELAY
	nrf_gpio_pin_set(PIN_GPIO_RELAY_OFF);
	nrf_delay_ms(relayHighDuration);
	nrf_gpio_pin_clear(PIN_GPIO_RELAY_OFF);

#ifdef EXTENDED_SWITCH_STATE
	_switchValue.relay_state = 0;
#else
	_switchValue = 0;
#endif
	updateSwitchState();
#endif
}

void Switch::timedSetRelay() {
	switch (_nextRelayVal) {
	case SWITCH_NEXT_RELAY_VAL_NONE:
#ifdef MODIFICATION_110V
	Scanner::getInstance().resume();
#endif
		return;
	case SWITCH_NEXT_RELAY_VAL_ON:
		relayOn();
		break;
	case SWITCH_NEXT_RELAY_VAL_OFF:
		relayOff();
		break;
	}
#ifdef MODIFICATION_110V
	if (_nextRelayVal == SWITCH_NEXT_RELAY_VAL_NONE) {
		Timer::getInstance().start(_relayTimerId, MS_TO_TICKS(POST_RELAY_DELAY), this);
	}
#endif

}

void Switch::turnOn() {

#ifdef PRINT_SWITCH_VERBOSE
	LOGd("Turn ON");
#endif

#if HAS_RELAY
	relayOn();
#else
	pwmOn();
#endif
}

void Switch::turnOff() {

#ifdef PRINT_SWITCH_VERBOSE
	LOGd("Turn OFF");
#endif

#if HAS_RELAY
	relayOff();
#else
	pwmOff();
#endif
}

void Switch::toggle() {
#if HAS_RELAY
	if (getRelayState())
#else
	if (getPwm())
#endif
	{
		turnOff();
	} else {
		turnOn();
	}
}

void Switch::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
	switch (evt) {
	case EVT_POWER_ON:
	case EVT_TRACKED_DEVICE_IS_NEARBY: {
		turnOn();
		break;
	}
	case EVT_POWER_OFF:
	case EVT_TRACKED_DEVICE_NOT_NEARBY: {
		turnOff();
		break;
	}
	case EVT_SCAN_STOPPED: {
#ifdef MODIFICATION_110V
		if (_nextRelayVal != SWITCH_NEXT_RELAY_VAL_NONE) {
			Timer::getInstance().start(_relayTimerId, MS_TO_TICKS(PRE_RELAY_DELAY), this);
		}
#endif
		break;
	}
	}
}
