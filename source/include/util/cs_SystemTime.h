/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 24, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <events/cs_EventListener.h>
#include <drivers/cs_Timer.h>
#include <stdint.h>

class TimeOfDay{
    private:
    uint32_t sec_since_midnight;

    public:
    TimeOfDay(uint32_t secondsSinceMidnight) : sec_since_midnight(secondsSinceMidnight % (24*60*60)){}

    uint8_t h() {return sec_since_midnight / (60*60);}
    uint8_t m() {return (sec_since_midnight % (60*60)) / 60;}
    uint8_t s() {return sec_since_midnight % 60;}

    /**
     * Implicit cast operators
     */
    operator uint32_t(){ return sec_since_midnight; }

};

class Time {
    private:
    uint32_t posixTimeStamp;
    
    public:
    Time(uint32_t posixTime)  : posixTimeStamp(posixTime){}

    /**
     * Implicit cast operators
     */
    operator uint32_t(){ return posixTimeStamp; }
    operator TimeOfDay(){ return posixTimeStamp % (24*60*60); }

};

/**
 * This class keeps track of the real time in the current time zone.
 * It may obtain its data through the mesh, or some other way and try
 * to keep up to date using on board timing functionality.
 */
class SystemTime : public EventListener {
    public:
    static Time posix();
    static TimeOfDay now();
    // static Day day();
    // static Month month();
    // static Date date();

    virtual void handleEvent(event_t& event);

    /**
     * Changes the system wide time (i.e. SystemTime::posixTimeStamp) 
     * to the given value. Dispatches a EVT_TIME_SET event to signal 
     * any interested Listeners.
     */
	static void setTime(uint32_t time);
    
    /**
     * Creates and starts the first tick timer.
     */
    static void init();

    private:
    // state data
	static uint32_t rtcTimeStamp;
	static uint32_t posixTimeStamp;

    // timing features
    static app_timer_t              appTimerData;// = {{0}};
	static app_timer_id_t           appTimerId;// = &_appTimerData;

    static void scheduleNextTick();
    static void tick(void* unused);

    // settings
    static constexpr auto SCHEDULER_UPDATE_FREQUENCY = 2;
};

