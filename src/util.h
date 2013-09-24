#ifndef __UTIL_H
#define __UTIL_H

/* util.h needs access to stdint.h for these typedefs:
- uint_fast8_t, int_fast8_t
- uint_fast16_t, int_fast16_t
- uint_fast32_t, int_fast32_t
- uint_fast64_t, int_fast64_t
If your environment exposes stdint.h, then these typedefs will necessarily
be available to the generated code. If your environment does not expose
stdint.h, then you will have to define these types yourself.
*/
#include <stdint.h>

typedef uint_fast8_t haris_uint8_t;
typedef int_fast8_t haris_int8_t;
typedef uint_fast16_t haris_uint16_t;
typedef int_fast16_t haris_int16_t;
typedef uint_fast32_t haris_uint32_t;
typedef int_fast32_t haris_int32_t;
typedef uint_fast64_t haris_uint64_t;
typedef int_fast64_t haris_int64_t;

#define FLOAT32_SIGBITS 23
#define FLOAT32_BIAS    127
#define FLOAT64_SIGBITS 52
#define FLOAT64_BIAS    1023

haris_uint8_t read_uint8(unsigned char *);
haris_int8_t read_int8(unsigned char *);
haris_uint16_t read_uint16(unsigned char *);
haris_int16_t read_int16(unsigned char *);
haris_uint32_t read_uint32(unsigned char *);
haris_int32_t read_int32(unsigned char *);
haris_uint64_t read_uint64(unsigned char *);
haris_int64_t read_int64(unsigned char *);

void write_uint8(unsigned char *, haris_uint8_t);
void write_int8(unsigned char *, haris_int8_t);
void write_uint16(unsigned char *, haris_uint16_t);
void write_int16(unsigned char *, haris_int16_t);
void write_uint32(unsigned char *, haris_uint32_t);
void write_int32(unsigned char *, haris_int32_t);
void write_uint64(unsigned char *, haris_uint64_t);
void write_int64(unsigned char *, haris_int64_t);

float read_float32(unsigned char *);
double read_float64(unsigned char *);

void write_float32(unsigned char *, float);
void write_float64(unsigned char *, double);

#endif
