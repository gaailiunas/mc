#include "var.h"
#include <stdio.h>

varint_t read_varint(char *data, int len)
{
    int value = 0;
    int position = 0;
    
    int bytes = 0;

    for (int i = 0; i < len; i++) {
        bytes++;
        value |= (data[i] & SEGMENT_BITS) << position;
        if ((data[i] & CONTINUE_BIT) == 0)
            break;
        position += 7;
        if (position >= 32) {
            fprintf(stderr, "varint too big\n");
            break;
        }
    }

    return (varint_t){value, bytes};
}
