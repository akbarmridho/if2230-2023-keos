#ifndef _STDTYPE
#define _STDTYPE

/**
 * Unsigned integer representing object size
*/
typedef unsigned int size_t;

/**
 * 32-bit unsigned integer
 */
typedef unsigned int uint32_t;

/**
 * 16-bit unsigned integer
 */
typedef unsigned short uint16_t;

/**
 * 8-bit unsigned integer
 */
typedef unsigned char uint8_t;

/**
 * 32-bit signed integer
 */
typedef signed int int32_t;

/**
 * 16-bit signed integer
 */
typedef signed short int16_t;

/**
 * 8-bit signed integer
 */
typedef signed char int8_t;

/**
 * Naive implementation of boolean (uint8_t)
*/
typedef uint8_t bool;
#define TRUE 1
#define FALSE 0

#endif