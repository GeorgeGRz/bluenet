/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Mar 9, 2016
 * License: LGPLv3+
 */
#pragma once

/**
 * Provide a non-zero value here in applications that need to use several
 * peripherals with the same ID that are sharing certain resources
 * (for example, SPI0 and TWI0). Obviously, such peripherals cannot be used
 * simultaneously. Therefore, this definition allows to initialize the driver
 * for another peripheral from a given group only after the previously used one
 * is uninitialized. Normally, this is not possible, because interrupt handlers
 * are implemented in individual drivers.
 * This functionality requires a more complicated interrupt handling and driver
 * initialization, hence it is not always desirable to use it.
 */
#define PERIPHERAL_RESOURCE_SHARING_ENABLED 0


/* CLOCK */
//#define CLOCK_CONFIG_XTAL_FREQ          NRF_CLOCK_XTALFREQ_16MHz
//#define CLOCK_CONFIG_LF_SRC             NRF_CLOCK_LF_SRC_Xtal
//#define CLOCK_CONFIG_LF_RC_CAL_INTERVAL RC_2000MS_CALIBRATION_INTERVAL
//#define CLOCK_CONFIG_IRQ_PRIORITY       APP_IRQ_PRIORITY_LOW

/* SAADC */
#define SAADC_ENABLED 1

#if (SAADC_ENABLED == 1)
#define SAADC_CONFIG_RESOLUTION      NRF_SAADC_RESOLUTION_10BIT
#define SAADC_CONFIG_OVERSAMPLE      NRF_SAADC_OVERSAMPLE_DISABLED
#define SAADC_CONFIG_IRQ_PRIORITY    APP_IRQ_PRIORITY_LOW
#endif

/* GPIOTE */
#define GPIOTE_ENABLED 1

#if (GPIOTE_ENABLED == 1)
#define GPIOTE_CONFIG_USE_SWI_EGU false
#define GPIOTE_CONFIG_IRQ_PRIORITY APP_IRQ_PRIORITY_HIGH
#define GPIOTE_CONFIG_NUM_OF_LOW_POWER_EVENTS 5
//#define GPIOTE_CONFIG_NUM_OF_LOW_POWER_EVENTS 1
#endif

/* TIMER */
#define TIMER0_ENABLED 0

#if (TIMER0_ENABLED == 1)
#define TIMER0_CONFIG_FREQUENCY    NRF_TIMER_FREQ_16MHz
#define TIMER0_CONFIG_MODE         TIMER_MODE_MODE_Timer
#define TIMER0_CONFIG_BIT_WIDTH    TIMER_BITMODE_BITMODE_32Bit
#define TIMER0_CONFIG_IRQ_PRIORITY APP_IRQ_PRIORITY_LOW

#define TIMER0_INSTANCE_INDEX      0
#endif

#define TIMER1_ENABLED 1

#if (TIMER1_ENABLED == 1)
#define TIMER1_CONFIG_FREQUENCY    NRF_TIMER_FREQ_16MHz
#define TIMER1_CONFIG_MODE         TIMER_MODE_MODE_Timer
#define TIMER1_CONFIG_BIT_WIDTH    TIMER_BITMODE_BITMODE_16Bit
#define TIMER1_CONFIG_IRQ_PRIORITY APP_IRQ_PRIORITY_LOW

#define TIMER1_INSTANCE_INDEX      (TIMER0_ENABLED)
#endif

#define TIMER2_ENABLED 1

#if (TIMER2_ENABLED == 1)
#define TIMER2_CONFIG_FREQUENCY    NRF_TIMER_FREQ_16MHz
#define TIMER2_CONFIG_MODE         TIMER_MODE_MODE_Timer
#define TIMER2_CONFIG_BIT_WIDTH    TIMER_BITMODE_BITMODE_16Bit
#define TIMER2_CONFIG_IRQ_PRIORITY APP_IRQ_PRIORITY_LOW

#define TIMER2_INSTANCE_INDEX      (TIMER1_ENABLED+TIMER0_ENABLED)
#endif

#define TIMER_COUNT (TIMER0_ENABLED + TIMER1_ENABLED + TIMER2_ENABLED)



/* COMP */
#define COMP_ENABLED 0

#if (COMP_ENABLED == 1)
#define COMP_CONFIG_REF     		NRF_COMP_REF_Int1V8
#define COMP_CONFIG_MAIN_MODE		NRF_COMP_MAIN_MODE_SE
#define COMP_CONFIG_SPEED_MODE		NRF_COMP_SP_MODE_High
#define COMP_CONFIG_HYST			NRF_COMP_HYST_NoHyst
#define COMP_CONFIG_ISOURCE			NRF_COMP_ISOURCE_Off
#define COMP_CONFIG_IRQ_PRIORITY 	APP_IRQ_PRIORITY_LOW
#define COMP_CONFIG_INPUT        	NRF_COMP_INPUT_0
#endif

/* LPCOMP */
#define LPCOMP_ENABLED 1

#if (LPCOMP_ENABLED == 1)
#define LPCOMP_CONFIG_REFERENCE    NRF_LPCOMP_REF_SUPPLY_4_8
#define LPCOMP_CONFIG_DETECTION    NRF_LPCOMP_DETECT_CROSS
#define LPCOMP_CONFIG_IRQ_PRIORITY APP_IRQ_PRIORITY_LOW
#define LPCOMP_CONFIG_INPUT        NRF_LPCOMP_INPUT_2
#endif



///* RTC */
//#define RTC0_ENABLED 0
//
//#if (RTC0_ENABLED == 1)
//#define RTC0_CONFIG_FREQUENCY    32678
//#define RTC0_CONFIG_IRQ_PRIORITY APP_IRQ_PRIORITY_LOW
//#define RTC0_CONFIG_RELIABLE     false
//
//#define RTC0_INSTANCE_INDEX      0
//#endif
//
//#define RTC1_ENABLED 0
//
//#if (RTC1_ENABLED == 1)
//#define RTC1_CONFIG_FREQUENCY    32768
//#define RTC1_CONFIG_IRQ_PRIORITY APP_IRQ_PRIORITY_LOW
//#define RTC1_CONFIG_RELIABLE     false
//
//#define RTC1_INSTANCE_INDEX      (RTC0_ENABLED)
//#endif
//
//#define RTC_COUNT                (RTC0_ENABLED+RTC1_ENABLED)
//
//#define NRF_MAXIMUM_LATENCY_US 2000
//
///* RNG */
//#define RNG_ENABLED 0
//
//#if (RNG_ENABLED == 1)
//#define RNG_CONFIG_ERROR_CORRECTION true
//#define RNG_CONFIG_POOL_SIZE        8
//#define RNG_CONFIG_IRQ_PRIORITY     APP_IRQ_PRIORITY_LOW
//#endif
//
//
///* QDEC */
//#define QDEC_ENABLED 0
//
//#if (QDEC_ENABLED == 1)
//#define QDEC_CONFIG_REPORTPER    NRF_QDEC_REPORTPER_10
//#define QDEC_CONFIG_SAMPLEPER    NRF_QDEC_SAMPLEPER_16384us
//#define QDEC_CONFIG_PIO_A        1
//#define QDEC_CONFIG_PIO_B        2
//#define QDEC_CONFIG_PIO_LED      3
//#define QDEC_CONFIG_LEDPRE       511
//#define QDEC_CONFIG_LEDPOL       NRF_QDEC_LEPOL_ACTIVE_HIGH
//#define QDEC_CONFIG_IRQ_PRIORITY APP_IRQ_PRIORITY_LOW
//#define QDEC_CONFIG_DBFEN        false
//#define QDEC_CONFIG_SAMPLE_INTEN false
//#endif
//

//
///* WDT */
//#define WDT_ENABLED 0
//
//#if (WDT_ENABLED == 1)
//#define WDT_CONFIG_BEHAVIOUR     NRF_WDT_BEHAVIOUR_RUN_SLEEP
//#define WDT_CONFIG_RELOAD_VALUE  2000
//#define WDT_CONFIG_IRQ_PRIORITY  APP_IRQ_PRIORITY_HIGH
//#endif



