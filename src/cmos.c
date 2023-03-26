#include "lib-header/cmos.h"
#include "lib-header/portio.h"

int32_t get_update_in_progress_flag()
{
    out(cmos_address, 0x0A);
    return (in(cmos_data)&0x80);
}

uint8_t get_RTC_register(int32_t reg)
{
    out(cmos_address, reg);
    return in(cmos_data);
}

void read_rtc(uint16_t *year, uint16_t *month, uint16_t *day, uint16_t *hour, uint16_t *minute, uint16_t *second)
{
    // assume value from cmos is consistent

    while (get_update_in_progress_flag())
        ;

    *second = get_RTC_register(0x00);
    *minute = get_RTC_register(0x02);
    *hour = get_RTC_register(0x04);
    *day = get_RTC_register(0x07);
    *month = get_RTC_register(0x08);
    *year = get_RTC_register(0x09);

    uint8_t registerB = get_RTC_register(0x0B);

    // Convert BCD to binary values

    if (!(registerB & 0x04))
    {
        *second = (*second & 0x0F) + ((*second / 16) * 10);
        *minute = (*minute & 0x0F) + ((*minute / 16) * 10);
        *hour = ((*hour & 0x0F) + (((*hour & 0x70) / 16) * 10)) | (*hour & 0x80);
        *day = (*day & 0x0F) + ((*day / 16) * 10);
        *month = (*month & 0x0F) + ((*month / 16) * 10);
        *year = (*year & 0x0F) + ((*year / 16) * 10);
    }

    // Convert 12 hour clock to 24 hour clock

    if (!(registerB & 0x02) && (*hour & 0x80))
    {
        *hour = ((*hour & 0x7F) + 12) % 24;
    }

    // calculate full digit year
    *year += (CURRENT_YEAR / 100) * 100;
    if (*year < CURRENT_YEAR)
        *year += 100;
}