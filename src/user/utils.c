#include "sys/sys.h"

void waitsecond()
{
    uint32_t current_timestamp = get_timestamp();

    while (current_timestamp == get_timestamp())
        ;
}

void waitbusy()
{
    // loop 1 million time
    for (uint32_t i = 0; i < 1e6; i++)
        ;
}