/* File : mbTestServer.h
 */

#ifndef MBTESTSERVER_H
#define MBTESTSERVER_H

#include <inttypes.h>
#include <stdint.h>

#define SERVER_ID         17
#define INVALID_SERVER_ID 18

const uint16_t UT_BITS_ADDRESS = 0x13;
const uint16_t UT_BITS_NB = 0x25;
const uint8_t UT_BITS_TAB[] = { 0xCD, 0x6B, 0xB2, 0x0E, 0x1B };

const uint16_t UT_INPUT_BITS_ADDRESS = 0xC4;
const uint16_t UT_INPUT_BITS_NB = 0x16;
const uint8_t UT_INPUT_BITS_TAB[] = { 0xAC, 0xDB, 0x35 };

const uint16_t UT_REGISTERS_ADDRESS = 0x6B;

/* Raise a manual exception when this adress is used for the first byte */
const uint16_t UT_REGISTERS_ADDRESS_SPECIAL = 0x6C;
const uint16_t UT_REGISTERS_NB = 0x3;
const uint16_t UT_REGISTERS_TAB[] = { 0x022B, 0x0001, 0x0064 };

/* If the following value is used, a bad response is sent.
   It's better to test with a lower value than
   UT_REGISTERS_NB_POINTS to try to raise a segfault. */
const uint16_t UT_REGISTERS_NB_SPECIAL = 0x2;

const uint16_t UT_INPUT_REGISTERS_ADDRESS = 0x08;
const uint16_t UT_INPUT_REGISTERS_NB = 0x1;
const uint16_t UT_INPUT_REGISTERS_TAB[] = { 0x000A };

const float UT_REAL = 916.540649;
const uint32_t UT_IREAL = 0x4465229a;

#endif
