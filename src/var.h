#ifndef VAR_H 
#define VAR_H 

#define SEGMENT_BITS 0x7f
#define CONTINUE_BIT 0x80

typedef struct varint {
    int value;
    int byteslen;
} varint_t ;

varint_t read_varint(char *data, int len);

#endif // VAR_H
