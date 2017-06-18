/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 23, 2016
 * License: LGPLv3+
 */
#pragma once

#include <ble/cs_Nordic.h>
#include <drivers/cs_Timer.h>
#include <structs/cs_ScheduleEntries.h>
#include <events/cs_EventListener.h>
#include <processing/cs_TimeSync.h>

//todo: just remove it totally if it's not necessary to disable it
#define SCHEDULER_ENABLED 1

#define SCHEDULER_UPDATE_FREQUENCY 2

class Scheduler : EventListener {
public:
	//! Gets a static singleton (no dynamic memory allocation)
	static Scheduler& getInstance() {
		static Scheduler instance;
		return instance;
	}

	static void staticTick(Scheduler* ptr) {
		ptr->tick();
	}

	inline uint32_t getTime() {
		return _posixTimeStamp;
	}

	// todo: move time / set time to separate class
	/** Returns the current time as posix time
	 * returns 0 when no time was set yet
	 */
	void setTime(uint32_t time);

	void start() {
		scheduleNextTick();
	}

	void addScheduleEntry(schedule_entry_t* entry);

	void removeScheduleEntry(schedule_entry_t* entry);

	inline ScheduleList* getScheduleList() {
#if SCHEDULER_ENABLED==1
		return _scheduleList;
#else
		return NULL;
#endif
	}

protected:
	inline void scheduleNextTick() {
		Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(SCHEDULER_UPDATE_FREQUENCY), this);
	}

	void tick();

	void writeScheduleList();

	void readScheduleList();

	void publishScheduleEntries();

	/* Handle events as EventListener
	 */
	void handleEvent(uint16_t evt, void* p_data, uint16_t length);

	void syncTime();

private:
	Scheduler();

#if SCHEDULER_ENABLED==1
	uint8_t _schedulerBuffer[sizeof(schedule_list_t)];
#endif

	app_timer_t              _appTimerData;
	app_timer_id_t           _appTimerId;

	uint32_t _rtcTimeStamp;
	uint32_t _posixTimeStamp;
	ScheduleList* _scheduleList;

	TimeSync* _timeSync;
};

