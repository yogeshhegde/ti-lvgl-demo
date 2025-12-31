/**
 * @file rtc_util.c
 *
 * Utility functions for rtc wake
 *
 */

/*********************
 *      INCLUDES
 *********************/
#define _POSIX_C_SOURCE 200809L   /* for getline() */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
/*********************
 *      DEFINES
 *********************/
#define RTC_WAKEALARM_TIME  "+300"
#define RTC_WAKEALARM_PATH  "/sys/class/rtc/rtc0/wakealarm"
#define POWER_STATE_PATH    "/sys/power/state"


/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void trim(char *s);
static int wakealarm_has_value(void);
static int write_sysfs(const char *path, const char *value);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

int rtc_suspend(void)
{
    int has_val = wakealarm_has_value();
    if (has_val == -1) {
        /* Fatal error while reading wakealarm */
        return -1;
    }

    if (!has_val) {
        /* No alarm set, schedule one 300 s from now */
        if (write_sysfs(RTC_WAKEALARM_PATH, RTC_WAKEALARM_TIME) == -1)
            return -1;
    } else {
        /* Alarm already set → request suspend‑to‑RAM */
        if (write_sysfs(POWER_STATE_PATH, "mem") == -1)
            return -1;
    }

    return 0;
}


/**********************
 *   STATIC FUNCTIONS
 **********************/

/* Trim leading/trailing whitespace (including the trailing newline) */
static void trim(char *s)
{
    char *end;

    /* left trim */
    while (*s && (*s == ' ' || *s == '\t' || *s == '\n')) s++;

    /* right trim */
    end = s + strlen(s) - 1;
    while (end >= s && (*end == ' ' || *end == '\t' || *end == '\n')) {
        *end = '\0';
        end--;
    }
}

/* Return 1 if the wakealarm file contains a non‑zero value, 0 otherwise */
static int wakealarm_has_value(void)
{
    FILE *fp = fopen(RTC_WAKEALARM_PATH, "r");
    if (!fp) {
        fprintf(stderr, "Error opening %s for reading: %s\n",
                RTC_WAKEALARM_PATH, strerror(errno));
        return -1;  /* signal fatal error */
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t nread = getline(&line, &len, fp);
    fclose(fp);

    if (nread == -1) {          /* empty file or read error */
        free(line);
        return 0;               /* treat as “no value” */
    }

    trim(line);
    int has_value = (strlen(line) != 0 && strcmp(line, "0") != 0);
    free(line);
    return has_value;
}

/* Write a string (without trailing NUL) to a sysfs file */
static int write_sysfs(const char *path, const char *value)
{
    int fd = open(path, O_WRONLY);
    if (fd == -1) {
        fprintf(stderr, "Error opening %s for writing: %s\n",
                path, strerror(errno));
        return -1;
    }

    ssize_t ret = write(fd, value, strlen(value));
    if (ret == -1) {
        fprintf(stderr, "Error writing to %s: %s\n",
                path, strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}