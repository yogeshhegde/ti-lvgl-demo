/**
 * @file rtc_util.h
 *
 * rtc_util.h - Header file for the utility functions
 * used by the rtc wake
 *
 */

#ifndef RTC_UTIL_H
#define RTC_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdarg.h>


/**********************
 * GLOBAL PROTOTYPES
 **********************/
int rtc_suspend(void);

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *      MACROS
 **********************/


#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*RTC_UTIL_H*/
