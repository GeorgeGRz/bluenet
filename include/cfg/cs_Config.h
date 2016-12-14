/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 4 Nov., 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

#define CROWNSTONE_COMPANY_ID                    0x038E

//! size of the buffer used for characteristics
//#define GENERAL_BUFFER_SIZE                      300

/** maximum length of strings used for characteristic values */
#define DEFAULT_CHAR_VALUE_STRING_LENGTH             25

/** define the maximum size for strings to be stored
 */
#define MAX_STRING_STORAGE_SIZE                  31

/** Command to enter the bootloader and stay there.
 *
 * This should be the same value as defined in the bootloader.
 */
#define GPREGRET_DFU_RESET                       66
#define GPREGRET_BROWNOUT_RESET                  96
#define GPREGRET_SOFT_RESET                      0

/*
 */
#define APP_TIMER_PRESCALER                      0
#define APP_TIMER_MAX_TIMERS                     10
#define APP_TIMER_OP_QUEUE_SIZE                  10

/*
 */
/** Maximum size of scheduler events. */
#define SCHED_MAX_EVENT_DATA_SIZE                ((CEIL_DIV(MAX(MAX(BLE_STACK_EVT_MSG_BUF_SIZE,    \
                                                                    ANT_STACK_EVT_STRUCT_SIZE),    \
                                                                SYS_EVT_MSG_BUF_SIZE),             \
                                                            sizeof(uint32_t))) * sizeof(uint32_t))
/** Maximum number of events in the scheduler queue. */
#define SCHED_QUEUE_SIZE                         30

//! See https://devzone.nordicsemi.com/question/84767/s132-scan-intervalwindow-adv-interval/
//! Old https://devzone.nordicsemi.com/question/21164/s130-unstable-advertising-reports-during-scan-updated/
/** Determines scan interval in units of 0.625 millisecond. */
#define SCAN_INTERVAL                            0x00A0
/** Determines scan window in units of 0.625 millisecond. */
//#define SCAN_WINDOW                            0x0050
/** Determines scan window in units of 0.625 millisecond. */
//#define SCAN_WINDOW                              0x009E
#define SCAN_WINDOW                              0x0028

//! bonding / security
#define SEC_PARAM_TIMEOUT                        30                                          /** < Timeout for Pairing Request or Security Request (in seconds). */
#define SEC_PARAM_BOND                           1                                           /** < Perform bonding. */
#define SEC_PARAM_MITM                           1                                           /** < Man In The Middle protection not required. */
#define SEC_PARAM_IO_CAPABILITIES                BLE_GAP_IO_CAPS_DISPLAY_ONLY				/** < No I/O capabilities. */
#define SEC_PARAM_OOB                            0                                           /** < Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE                   7                                           /** < Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE                   16                                          /** < Maximum encryption key size. */

#define SECURITY_REQUEST_DELAY                   1500                                        /**< Delay after connection until security request is sent, if necessary (ms). */

//! tx power used for low power mode during bonding
#define LOW_TX_POWER                             -40

//#define CLOCK_SOURCE                             NRF_CLOCK_LFCLKSRC_RC_250_PPM_TEMP_8000MS_CALIBRATION

//! duration (in ms) how long the relay pins should be set to high
#define RELAY_HIGH_DURATION                      15 //! ms
//! duration (in ms) how long to retry switching the relay if there was not enough power to switch.
#define RELAY_DELAY                              50 //! ms
#define PRE_RELAY_DELAY                          20
#define POST_RELAY_DELAY                         10

//! Max number of schedule entries in the schedule service.
#define MAX_SCHEDULE_ENTRIES                     10

#define CURRENT_LIMIT							 0

//! ----- TIMERS -----
#define PWM_TIMER                                NRF_TIMER2
//#define PWM_IRQHandler                           TIMER2_IRQHandler
#define PWM_IRQn                                 TIMER2_IRQn
#define PWM_INSTANCE_INDEX                       TIMER2_INSTANCE_INDEX
#define PWM_TIMER_ID                             2

#define CS_ADC_TIMER                             NRF_TIMER1
//#define CS_ADC_TIMER_IRQ                         TIMER1_IRQHandler
#define CS_ADC_TIMER_IRQn                        TIMER1_IRQn
#define CS_ADC_INSTANCE_INDEX                    TIMER1_INSTANCE_INDEX
#define CS_ADC_TIMER_ID                          1

//! ----- PPI -----
//! Get auto assigned
//#define CS_ADC_PPI_CHANNEL                       7



#if CONTINUOUS_POWER_SAMPLER == 1
#define CS_ADC_SAMPLE_RATE                       101
#else
#define CS_ADC_SAMPLE_RATE                       3000 //! Max 10000 / numpins (min about 500? to avoid too large difference in timestamps)
#endif

#define POWER_SAMPLE_BURST_INTERVAL              3000 //! Time to next burst sampling (ms)
#define POWER_SAMPLE_BURST_NUM_SAMPLES           75 //! Number of voltage and current samples per burst

#define POWER_SAMPLE_CONT_INTERVAL               50 //! Time to next buffer read and attempt to send msg (ms)
#define POWER_SAMPLE_CONT_NUM_SAMPLES            80 //! Number of voltage and current samples in the buffer, written by ADC, read by power service

#define CS_ADC_SAMPLE_INTERVAL_US                400

#define CS_ADC_MAX_PINS                          2
#define CS_ADC_NUM_BUFFERS                       2 //! ADC code is currently written for (max) 2
#define CS_ADC_BUF_SIZE                          (2*POWER_SAMPLE_BURST_NUM_SAMPLES)
//#define CS_ADC_BUF_SIZE                          (2*20000/CS_ADC_SAMPLE_INTERVAL_US)

#define STORAGE_REQUEST_BUFFER_SIZE              5

#define FACTORY_RESET_CODE                       0xdeadbeef
#define FACTORY_RESET_TIMEOUT                    20000 //! Timeout before recovery becomes unavailable after reset (ms)
#define FACTORY_PROCESS_TIMEOUT                  200 //! Timeout before recovery process step is executed (ms)

#define ENCYRPTION_KEY_LENGTH                    SOC_ECB_KEY_LENGTH //! 16 byte length

#define BROWNOUT_TRIGGER_THRESHOLD               NRF_POWER_THRESHOLD_V27

#define VOLTAGE_MULTIPLIER                       0.20f
#define CURRENT_MULTIPLIER                       0.0045f
//#define VOLTAGE_ZERO                             169.0f
#define VOLTAGE_ZERO                             2003
//#define CURRENT_ZERO                             168.5f
#define CURRENT_ZERO                             1997
//#define POWER_ZERO                               0.0f
#define POWER_ZERO                               1500 //! mW
#define POWER_ZERO_AVG_WINDOW                    100
// Octave: a=0.05; x=[0:1000]; y=(1-a).^x; y2=cumsum(y)*a; figure(1); plot(x,y); figure(2); plot(x,y2); find(y2 > 0.99)(1)
#define VOLTAGE_ZERO_EXP_AVG_DISCOUNT            20  // Is divided by 1000, so 20 is a discount of 0.02. //! 99% of the average is influenced by the last 228 values
#define CURRENT_ZERO_EXP_AVG_DISCOUNT            100 // Is divided by 1000, so 100 is a discount of 0.1. //! 99% of the average is influenced by the last 44 values
#define POWER_EXP_AVG_DISCOUNT                   50  // Is divided by 1000, so 50 is a discount of 0.05. //! 99% of the average is influenced by the last 90 values


#define PWM_PERIOD                               20000L //! Interval in us: 1/20000e-6 = 50 Hz

#define KEEP_ALIVE_INTERVAL                      (2 * 60) // 2 minutes, in seconds

// stack config values
//#define MIN_CONNECTION_INTERVAL                  16
#define MIN_CONNECTION_INTERVAL                  6
//#define MAX_CONNECTION_INTERVAL                  32
#define MAX_CONNECTION_INTERVAL                  16
#define CONNECTION_SUPERVISION_TIMEOUT           400
#define SLAVE_LATENCY                            10
#define ADVERTISING_TIMEOUT                      0
#define ADVERTISING_REFRESH_PERIOD               1000 //! Push the changes in the advertisement packet to the stack every x milliseconds
#define ADVERTISING_REFRESH_PERIOD_SETUP         100 //! Push the changes in the advertisement packet to the stack every x milliseconds

