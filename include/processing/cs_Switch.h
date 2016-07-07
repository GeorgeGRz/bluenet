/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 19, 2016
 * License: LGPLv3+
 */
#pragma once

#include <ble/cs_Nordic.h>

class Switch {
public:
	//! Gets a static singleton (no dynamic memory allocation)
	static Switch& getInstance() {
		static Switch instance;
		return instance;
	}

	void init();

	void pwmOff();

	void pwmOn();

	void turnOn();

	void turnOff();

	void dim(uint8_t value);

	void setValue(uint8_t value);

	uint8_t getValue();

	void relayOn();

	void relayOff();

	void relaySwitch();

	static void staticRelaySwitch(Switch *ptr) {
		ptr->relaySwitch();
	}

private:
	Switch();

	uint8_t _switchValue;
	app_timer_id_t _relayDelayTimer;

};

