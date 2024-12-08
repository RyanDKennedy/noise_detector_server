#pragma once

/*

  1st byte - size including this byte, so size >= 3
  2nd byte - type of request
  3rd byte - bitwise or of options
  4th ... byte - data


 */

#include <stdint.h>

#define REQUEST_TYPE_EXIT                   0
#define REQUEST_TYPE_SET_THRESHOLD          1
#define REQUEST_TYPE_GET_THRESHOLD          2
#define REQUEST_TYPE_GET_TIME_OF_LAST_ALARM 3
#define REQUEST_TYPE_TEST_PRINT             4

#define REQUEST_OPTION_GLOBAL_ACK 1

#define REQUEST_OPTION_GET_THRESHOLD_RETURN 1 << 1


/**
   @brief encodes a packet
   @param packet[out] the result, this should be a char[255]
   @param packet_size[out] the size of param packet
   @param type[in] the type of packet
   @param opts[in] the options for the packet
   @param data[in] the data for the packet
   @param data_size[in] size of param data
 */
void encode_packet(char *packet, uint8_t *packet_size, uint8_t type, uint8_t opts, char *data, int data_size);

/**
   @brief decodes a packet
   @param packet[in] the packet to decode, should be of type char[255];
   @param size[out] size of packet
   @param type[out] type of packet
   @param opts[out] opts of packet
   @param data[out] the data from packet. should be passed as a char[255]
   @param data_size[out] the amount of bytes written to param data.
 */
void decode_packet(char *packet, uint8_t *size, uint8_t *type, uint8_t *opts, char *data, uint8_t *data_size);
