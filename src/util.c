#include "util.h"

uint_fast8_t read_uint8(unsigned char *b)
{
  return *b;
}

int_fast8_t read_int8(unsigned char *b)
{
  if (*b & 0x80) 
    return -(int_fast8_t)(*b & 0x7F) - 1;
  else
    return (int_fast8_t)read_uint8(b);
}

uint_fast16_t read_uint16(unsigned char *b)
{
  return (uint_fast16_t)*b << 8 | *(b+1);
}

int_fast16_t read_int16(unsigned char *b)
{
  if (*b & 0x80)
    return -((int_fast16_t)(*b & 0x7F) << 8 | (int_fast16_t)*(b+1)) - 1;
  else
    return (int_fast16_t)read_uint16(b);
}

uint_fast32_t read_uint32(unsigned char *b)
{
  return (uint_fast32_t)*b << 24 | (uint_fast32_t)*(b+1) << 16 |
    (uint_fast32_t)*(b+2) << 8 | *(b+3);
}

int_fast32_t read_int32(unsigned char *b)
{
  if (*b & 0x80)
    return -((int_fast32_t)(*b & 0x7F) << 24 | (int_fast32_t)*(b+1) << 16 |
             (int_fast32_t)(*b+2) << 8 | (int_fast32_t)(*b+3)) - 1;
  else
    return (int_fast32_t)read_uint32(b);
}

uint_fast64_t read_uint64(unsigned char *b)
{
  return (uint_fast64_t)*b << 56 | (uint_fast64_t)*(b+1) << 48 |
    (uint_fast64_t)*(b+2) << 40 | (uint_fast64_t)*(b+3) << 32 |
    (uint_fast64_t)*(b+4) << 24 | (uint_fast64_t)*(b+5) << 16 |
    (uint_fast64_t)*(b+6) << 8 | *(b+7);
}

int_fast64_t read_int64(unsigned char *b)
{
  if (*b & 0x80)
    return -((int_fast64_t)(*b & 0x7F) << 56 | (int_fast64_t)*(b+1) << 48 |
             (int_fast64_t)*(b+2) << 40 | (int_fast64_t)*(b+3) << 32 |
             (int_fast64_t)*(b+4) << 24 | (int_fast64_t)*(b+5) << 16 |
             (int_fast64_t)*(b+6) << 8 | (int_fast64_t)*(b+7)) - 1;
  else
    return (int_fast64_t)read_uint64(b);
}

void write_uint8(unsigned char *b, uint_fast8_t i)
{
  *b = i;
  return;
}

void write_int8(unsigned char *b, int_fast8_t i)
{
  if (i >= 0)
    *b = (unsigned char)i;
  else {
    *b = (unsigned char)(-i - 1);
    *b |= 0x80;
  }
  return;
}

void write_uint16(unsigned char *b, uint_fast16_t i)
{
  *b = i >> 8;
  *(b+1) = i;
  return;
}

void write_int16(unsigned char *b, int_fast16_t i)
{
  if (i >= 0)
    write_uint16(b, (uint_fast16_t)i);
  else {
    write_uint16(b, (uint_fast16_t)(-i - 1));
    *b |= 0x80;
  }
  return;
}

void write_uint32(unsigned char *b, uint_fast32_t i)
{
  *b = i >> 24;
  *(b+1) = i >> 16;
  *(b+2) = i >> 8;
  *(b+3) = i;
  return;
}

void write_int32(unsigned char *b, int_fast32_t i)
{
  if (i >= 0)
    write_uint32(b, (uint_fast32_t)i);
  else {
    write_uint32(b, (uint_fast32_t)(-i - 1));
    *b |= 0x80;
  }
  return;
}

void write_uint64(unsigned char *b, uint_fast64_t i)
{
  *b = i >> 56;
  *(b+1) = i >> 48;
  *(b+2) = i >> 40;
  *(b+3) = i >> 32;
  *(b+4) = i >> 24;
  *(b+5) = i >> 16;
  *(b+6) = i >> 8;
  *(b+7) = i;
  return;
}

void write_int64(unsigned char *b, int_fast64_t i)
{
  if (i >= 0)
    write_uint64(b, (uint_fast64_t)i);
  else {
    write_uint64(b, (uint_fast64_t)(-i - 1));
    *b |= 0x80;
  }
  return;
}

float read_float32(unsigned char *b)
{
  double result;
  long long shift;
  uint_fast32_t i = read_uint32(b);
  
  if (i == 0) return 0.0;
  
  result = (i & ((1LL << FLOAT32_SIGBITS) - 1));
  result /= (1LL << FLOAT32_SIGBITS); 
  result += 1.0;

  shift = ((i >> FLOAT32_SIGBITS) & 255) - FLOAT32_BIAS;
  while (shift > 0) { result *= 2.0; shift--; }
  while (shift < 0) { result /= 2.0; shift++; }

  result *= (i>>31)&1 ? -1.0: 1.0;

  return (float)result;
}

double read_float64(unsigned char *b)
{
  double result;
  long long shift;
  uint_fast64_t i = read_uint64(b);
  
  if (i == 0) return 0.0;
  
  result = (i & (( 1LL << FLOAT64_SIGBITS) - 1));
  result /= (1LL << FLOAT64_SIGBITS); 
  result += 1.0;

  shift = ((i >> FLOAT64_SIGBITS) & 2047) - FLOAT64_BIAS;
  while (shift > 0) { result *= 2.0; shift--; }
  while (shift < 0) { result /= 2.0; shift++; }

  result *= (i>>63)&1 ? -1.0: 1.0;

  return result;
}

void write_float32(unsigned char *b, float f)
{
  double fnorm;
  int shift;
  long sign, exp, significand;
  uint_fast32_t result;

  if (f == 0.0) {
    write_uint32(b, 0);
    return;
  } 

  if (f < 0) {
    sign = 1; 
    fnorm = -f; 
  } else { 
    sign = 0; 
    fnorm = f; 
  }

  shift = 0;
  while (fnorm >= 2.0) { fnorm /= 2.0; shift++; }
  while (fnorm < 1.0) { fnorm *= 2.0; shift--; }
  fnorm = fnorm - 1.0;

  significand = fnorm * ((1LL << FLOAT32_SIGBITS) + 0.5f);

  exp = shift + ((1<<7) - 1); 

  result = (sign<<31) | (exp<<23) | significand;
  write_uint32(b, result);
  return;
}

void write_float64(unsigned char *b, double f)
{
  double fnorm;
  int shift;
  long sign, exp, significand;
  uint_fast64_t result;

  if (f == 0.0) {
    write_uint64(b, 0);
  }

  if (f < 0) {
    sign = 1; 
    fnorm = -f; 
  } else { 
    sign = 0; 
    fnorm = f; 
  }

  shift = 0;
  while (fnorm >= 2.0) { fnorm /= 2.0; shift++; }
  while (fnorm < 1.0) { fnorm *= 2.0; shift--; }
  fnorm = fnorm - 1.0;

  significand = fnorm * ((1LL<<FLOAT64_SIGBITS) + 0.5f);

  exp = shift + ((1<<10) - 1); 

  result = (sign<<63) | (exp<<52) | significand;
  write_uint64(b, result);
  return;
}
