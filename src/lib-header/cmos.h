#ifndef _CMOS_H
#define _CMOS_H

#include "stdtype.h"

#define CURRENT_YEAR 2023

enum
{
    cmos_address = 0x70,
    cmos_data = 0x71
};

int32_t get_update_in_progress_flag();

uint8_t get_RTC_register(int32_t reg);

void read_rtc(uint16_t *year, uint16_t *month, uint16_t *day, uint16_t *hour, uint16_t *minute, uint16_t *second);

uint32_t get_timestamp();

#endif