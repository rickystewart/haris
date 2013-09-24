#include "util.h"

haris_uint8_t haris_read_uint8(unsigned char *b)
{
  return *b;
}

haris_int8_t haris_read_int8(unsigned char *b)
{
  if (*b & 0x80) 
    return -(haris_int8_t)(*b & 0x7F) - 1;
  else
    return (haris_int8_t)haris_read_uint8(b);
}

haris_uint16_t haris_read_uint16(unsigned char *b)
{
  return (haris_uint16_t)*b << 8 | *(b+1);
}

haris_int16_t haris_read_int16(unsigned char *b)
{
  if (*b & 0x80)
    return -((haris_int16_t)(*b & 0x7F) << 8 | (haris_int16_t)*(b+1)) - 1;
  else
    return (haris_int16_t)haris_read_uint16(b);
}

haris_uint32_t haris_read_uint32(unsigned char *b)
{
  return (haris_uint32_t)*b << 24 | (haris_uint32_t)*(b+1) << 16 |
    (haris_uint32_t)*(b+2) << 8 | *(b+3);
}

haris_int32_t haris_read_int32(unsigned char *b)
{
  if (*b & 0x80)
    return -((haris_int32_t)(*b & 0x7F) << 24 | (haris_int32_t)*(b+1) << 16 |
             (haris_int32_t)(*b+2) << 8 | (haris_int32_t)(*b+3)) - 1;
  else
    return (haris_int32_t)haris_read_uint32(b);
}

haris_uint64_t haris_read_uint64(unsigned char *b)
{
  return (haris_uint64_t)*b << 56 | (haris_uint64_t)*(b+1) << 48 |
    (haris_uint64_t)*(b+2) << 40 | (haris_uint64_t)*(b+3) << 32 |
    (haris_uint64_t)*(b+4) << 24 | (haris_uint64_t)*(b+5) << 16 |
    (haris_uint64_t)*(b+6) << 8 | *(b+7);
}

haris_int64_t haris_read_int64(unsigned char *b)
{
  if (*b & 0x80)
    return -((haris_int64_t)(*b & 0x7F) << 56 | (haris_int64_t)*(b+1) << 48 |
             (haris_int64_t)*(b+2) << 40 | (haris_int64_t)*(b+3) << 32 |
             (haris_int64_t)*(b+4) << 24 | (haris_int64_t)*(b+5) << 16 |
             (haris_int64_t)*(b+6) << 8 | (haris_int64_t)*(b+7)) - 1;
  else
    return (haris_int64_t)haris_read_uint64(b);
}

void haris_write_uint8(unsigned char *b, haris_uint8_t i)
{
  *b = i;
  return;
}

void haris_write_int8(unsigned char *b, haris_int8_t i)
{
  if (i >= 0)
    *b = (unsigned char)i;
  else {
    *b = (unsigned char)(-i - 1);
    *b |= 0x80;
  }
  return;
}

void haris_write_uint16(unsigned char *b, haris_uint16_t i)
{
  *b = i >> 8;
  *(b+1) = i;
  return;
}

void haris_write_int16(unsigned char *b, haris_int16_t i)
{
  if (i >= 0)
    haris_write_uint16(b, (haris_uint16_t)i);
  else {
    haris_write_uint16(b, (haris_uint16_t)(-i - 1));
    *b |= 0x80;
  }
  return;
}

void haris_write_uint32(unsigned char *b, haris_uint32_t i)
{
  *b = i >> 24;
  *(b+1) = i >> 16;
  *(b+2) = i >> 8;
  *(b+3) = i;
  return;
}

void haris_write_int32(unsigned char *b, haris_int32_t i)
{
  if (i >= 0)
    haris_write_uint32(b, (haris_uint32_t)i);
  else {
    haris_write_uint32(b, (haris_uint32_t)(-i - 1));
    *b |= 0x80;
  }
  return;
}

void haris_write_uint64(unsigned char *b, haris_uint64_t i)
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

void haris_write_int64(unsigned char *b, haris_int64_t i)
{
  if (i >= 0)
    haris_write_uint64(b, (haris_uint64_t)i);
  else {
    haris_write_uint64(b, (haris_uint64_t)(-i - 1));
    *b |= 0x80;
  }
  return;
}

float haris_read_float32(unsigned char *b)
{
  double result;
  haris_int64_t shift;
  haris_uint32_t i = haris_read_uint32(b);
  
  if (i == 0) return 0.0;
  
  result = (i & ((1LL << FLOAT32_SIGBITS) - 1));
  result /= (1LL << FLOAT32_SIGBITS); 
  result += 1.0;

  shift = ((i >> FLOAT32_SIGBITS) & 255) - FLOAT32_BIAS;
  while (shift > 0) { result *= 2.0; shift--; }
  while (shift < 0) { result /= 2.0; shift++; }

  result *= (i >> 31) & 1 ? -1.0: 1.0;

  return (float)result;
}

double haris_read_float64(unsigned char *b)
{
  double result;
  haris_int64_t shift;
  haris_uint64_t i = haris_read_uint64(b);
  
  if (i == 0) return 0.0;
  
  result = (i & (( 1LL << FLOAT64_SIGBITS) - 1));
  result /= (1LL << FLOAT64_SIGBITS); 
  result += 1.0;

  shift = ((i >> FLOAT64_SIGBITS) & 2047) - FLOAT64_BIAS;
  while (shift > 0) { result *= 2.0; shift--; }
  while (shift < 0) { result /= 2.0; shift++; }

  result *= (i >> 63) & 1 ? -1.0: 1.0;

  return result;
}

void haris_write_float32(unsigned char *b, float f)
{
  double fnorm;
  int shift;
  long sign, exp, significand;
  haris_uint32_t result;

  if (f == 0.0) {
    haris_write_uint32(b, 0);
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
  haris_write_uint32(b, result);
  return;
}

void haris_write_float64(unsigned char *b, double f)
{
  double fnorm;
  int shift;
  long sign, exp, significand;
  haris_uint64_t result;

  if (f == 0.0) {
    haris_write_uint64(b, 0);
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
  haris_write_uint64(b, result);
  return;
}
