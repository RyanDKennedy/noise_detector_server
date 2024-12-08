#include "request.h"

#include "string.h"
#include "assert.h"

void encode_packet(char *packet, uint8_t *packet_size, uint8_t type, uint8_t opts, char *data, int data_size)
{
    assert(data_size <= 252);

    memset(packet, 0, 255);

    packet[0] = 3 + data_size;
    packet[1] = type;
    packet[2] = opts;
    memcpy(packet + 3, data, data_size);

    *packet_size = 3 + data_size;
}

void decode_packet(char *packet, uint8_t *size, uint8_t *type, uint8_t *opts, char *data, uint8_t *data_size)
{
    *size = packet[0];
    *type = packet[1];
    *opts = packet[2];
    *data_size = *size - 3;
    memcpy(data, packet + 3, *data_size);
}
