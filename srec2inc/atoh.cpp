/**
 *  @file   atoh.cpp
 *  @brief  Standard Library Extension - Convert an ASCII hex string to hexadecimal
 *  Copyright (c) 2012 TranZer Corp. All rights reserved.
 */
 
#include <iostream>
#include <stdint.h>
#include "atoh.h"

template<typename T>
T atoh( const char *string )
{
    // this is the hex version of the string
    T value = 0;
    // temporary for parsing
    char ch;
    // the stripped digit and a bailout scenario for mismatched size protection
    uint8_t digit = 0, bail = sizeof(T) << 1;
    
    #ifdef DEBUG
     fprintf(stderr, "%s: %d, Template size in bytes = %d\n", __FILE__, __LINE__, bail);
    #endif
    // loop until the string has ended
    while ( ( ch = *string++ ) != 0 ) 
    {
        // service digits 0 - 9
        if ( ( ch >= '0' ) && ( ch <= '9' ) ) 
        {
            digit = ch - '0';
        }
        // and then lowercase a - f
        else if ( ( ch >= 'a' ) && ( ch <= 'f' ) ) 
        {
            digit = ch - 'a' + 10;
        }
        // and uppercase A - F
        else if ( ( ch >= 'A' ) && ( ch <= 'F' ) ) 
        {
            digit = ch - 'A' + 10;
        }
        // stopping where we are if an inapproprate value is found
        else 
        {
            break;
        }
        // if the return is 8, 16, or 32 bits - only parse that amount
        if ( bail-- == 0 ) 
		{
            fprintf(stderr, "%s: %d, Template overflow\n", __FILE__, __LINE__);
			break;
        }
        // and build the value now, preparing for the next pass
        value = (value << 4) + digit;
    }

    return value;
}

template uint8_t  atoh<uint8_t> (const char* string );
template uint16_t atoh<uint16_t>(const char* string );
template uint32_t atoh<uint32_t>(const char* string );
template uint64_t atoh<uint64_t>(const char* string );
