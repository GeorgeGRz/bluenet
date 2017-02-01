/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 14, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#pragma once

#include "drivers/cs_Timer.h"
#include "drivers/cs_PWM.h"
#include "drivers/cs_Temperature.h"
#include <cstdint>
#include "storage/cs_Settings.h"
#include "events/cs_EventListener.h"
#include "events/cs_EventDispatcher.h"
#include "processing/cs_Switch.h"

#include "drivers/cs_COMP.h"

//#define TEMPERATURE_UPDATE_FREQUENCY 0.2
#define TEMPERATURE_UPDATE_FREQUENCY 2

//todo: move to code to cpp

/** Protection against temperature exceeding certain threshold
 */
class TemperatureGuard : EventListener {
public:
	TemperatureGuard() :
		EventListener(CONFIG_MAX_CHIP_TEMP),
#if (NORDIC_SDK_VERSION >= 11)
		_appTimerId(NULL),
#else
		_appTimerId(UINT32_MAX),
#endif
		_maxTemp(MAX_CHIP_TEMP)
	{
#if (NORDIC_SDK_VERSION >= 11)
		_appTimerData = { {0} };
		_appTimerId = &_appTimerData;
#endif
	}

	void init() {
		EventDispatcher::getInstance().addListener(this);

		Settings::getInstance().get(CONFIG_MAX_CHIP_TEMP, &_maxTemp);

//		Timer::getInstance().createRepeated(_appTimerId, (app_timer_timeout_handler_t)TemperatureGuard::staticTick);
		Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t)TemperatureGuard::staticTick);
	}

	void tick() {
		if (getTemperature() > _maxTemp) {
			//! Switch off all channels
			Switch::getInstance().pwmOff();
			Switch::getInstance().turnOff();
//			PWM::getInstance().switchOff();
//			//! Make sure pwm can't be set anymore
//			PWM::getInstance().deinit();
		}

//		uint32_t comp = COMP::getInstance().sample();
//		LOGd("comp:%u", comp);

		// TODO: make next time to next tick depend on current temperature
		scheduleNextTick();
	}

	void scheduleNextTick() {
		Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(TEMPERATURE_UPDATE_FREQUENCY), this);
	}

	void startTicking() {
		Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(TEMPERATURE_UPDATE_FREQUENCY), this);
	}

	void stopTicking() {
		Timer::getInstance().stop(_appTimerId);
	}

	static void staticTick(TemperatureGuard* ptr) {
		ptr->tick();
	}

	void handleEvent(uint16_t evt, void* p_data, uint16_t length) {
		switch (evt) {
		case CONFIG_MAX_CHIP_TEMP:
			_maxTemp = *(int32_t*)p_data;
			break;
		}
	}
private:
#if (NORDIC_SDK_VERSION >= 11)
	app_timer_t              _appTimerData;
	app_timer_id_t           _appTimerId;
#else
	uint32_t                 _appTimerId;
#endif
	int8_t _maxTemp;

//	void scheduleNextTick() {
//		Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(TEMPERATURE_UPDATE_FREQUENCY), this);
//	}
};


