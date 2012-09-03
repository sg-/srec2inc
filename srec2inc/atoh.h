/**
 *  @file   atoh.h
 *  @brief  Standard Library Extension - Convert an ASCII hex string to hexadecimal
 *  Copyright (c) 2012 TranZer Corp. All rights reserved.
 */

#ifndef ATOH_H
#define ATOH_H

/** A templated extension to stdlib.h to include ascii to hex conversions
 *
 * Example:
 * @code
 * #include "mbed.h"
 * #include "atoh.h"
 * 
 * int main() {
 *     uint64_t result = atoh<uint64_t>("0123456789abcdef" );
 *     uint32_t lo = result & 0x00000000ffffffff;
 *     uint32_t hi = (result >> 32);
 *     printf( "0x%08X%08X\n", hi, lo );
 *     printf( "0x%08X\n", atoh<uint32_t>( "12345678" ) );
 *     printf( "0x%04X\n", atoh<uint16_t>( "1234" ) );
 *     printf( "0x%02X\n", atoh<uint8_t> ( "12" ) );
 * }
 * @endcode
 */

/** A templated extension to stdlib.h to include ascii to hex conversions
 *  uint8_t, uint16_t, uint32_t and uint64_t types are forward declared
 *  @param string An ASCII string of hex digits
 *  @returns The binary equivelant of the string
 */
template<typename T>T atoh( const char* string );

#endif

