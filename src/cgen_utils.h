#ifndef __CGEN_UTILS_H
#define __CGEN_UTILS_H

const char *cgen_util_c_contents = "#include \"util.h\"\n\
\n\
haris_uint8_t haris_read_uint8(unsigned char *b)\n\
{\n\
\treturn *b;\n\
}\n\
\n\
haris_int8_t haris_read_int8(unsigned char *b)\n\
{\n\
\tif (*b & 0x80) \n\
\t\treturn -(haris_int8_t)(*b & 0x7F) - 1;\n\
\telse\n\
\t\treturn (haris_int8_t)haris_read_uint8(b);\n\
}\n\
\n\
haris_uint16_t haris_read_uint16(unsigned char *b)\n\
{\n\
\treturn (haris_uint16_t)*b << 8 | *(b+1);\n\
}\n\
\n\
haris_int16_t haris_read_int16(unsigned char *b)\n\
{\n\
\tif (*b & 0x80)\n\
\t\treturn -((haris_int16_t)(*b & 0x7F) << 8 | (haris_int16_t)*(b+1)) - 1;\n\
\telse\n\
\t\treturn (haris_int16_t)haris_read_uint16(b);\n\
}\n\
\n\
haris_uint32_t haris_read_uint32(unsigned char *b)\n\
{\n\
\treturn (haris_uint32_t)*b << 24 | (haris_uint32_t)*(b+1) << 16 |\n\
\t\t(haris_uint32_t)*(b+2) << 8 | *(b+3);\n\
}\n\
\n\
haris_int32_t haris_read_int32(unsigned char *b)\n\
{\n\
\tif (*b & 0x80)\n\
\t\treturn -((haris_int32_t)(*b & 0x7F) << 24 | (haris_int32_t)*(b+1) << 16 |\n\
\t\t\t\t\t\t (haris_int32_t)(*b+2) << 8 | (haris_int32_t)(*b+3)) - 1;\n\
\telse\n\
\t\treturn (haris_int32_t)haris_read_uint32(b);\n\
}\n\
\n\
haris_uint64_t haris_read_uint64(unsigned char *b)\n\
{\n\
\treturn (haris_uint64_t)*b << 56 | (haris_uint64_t)*(b+1) << 48 |\n\
\t\t(haris_uint64_t)*(b+2) << 40 | (haris_uint64_t)*(b+3) << 32 |\n\
\t\t(haris_uint64_t)*(b+4) << 24 | (haris_uint64_t)*(b+5) << 16 |\n\
\t\t(haris_uint64_t)*(b+6) << 8 | *(b+7);\n\
}\n\
\n\
haris_int64_t haris_read_int64(unsigned char *b)\n\
{\n\
\tif (*b & 0x80)\n\
\t\treturn -((haris_int64_t)(*b & 0x7F) << 56 | (haris_int64_t)*(b+1) << 48 |\n\
\t\t\t\t\t\t (haris_int64_t)*(b+2) << 40 | (haris_int64_t)*(b+3) << 32 |\n\
\t\t\t\t\t\t (haris_int64_t)*(b+4) << 24 | (haris_int64_t)*(b+5) << 16 |\n\
\t\t\t\t\t\t (haris_int64_t)*(b+6) << 8 | (haris_int64_t)*(b+7)) - 1;\n\
\telse\n\
\t\treturn (haris_int64_t)haris_read_uint64(b);\n\
}\n\
\n\
void haris_write_uint8(unsigned char *b, haris_uint8_t i)\n\
{\n\
\t*b = i;\n\
\treturn;\n\
}\n\
\n\
void haris_write_int8(unsigned char *b, haris_int8_t i)\n\
{\n\
\tif (i >= 0)\n\
\t\t*b = (unsigned char)i;\n\
\telse {\n\
\t\t*b = (unsigned char)(-i - 1);\n\
\t\t*b |= 0x80;\n\
\t}\n\
\treturn;\n\
}\n\
\n\
void haris_write_uint16(unsigned char *b, haris_uint16_t i)\n\
{\n\
\t*b = i >> 8;\n\
\t*(b+1) = i;\n\
\treturn;\n\
}\n\
\n\
void haris_write_int16(unsigned char *b, haris_int16_t i)\n\
{\n\
\tif (i >= 0)\n\
\t\tharis_write_uint16(b, (haris_uint16_t)i);\n\
\telse {\n\
\t\tharis_write_uint16(b, (haris_uint16_t)(-i - 1));\n\
\t\t*b |= 0x80;\n\
\t}\n\
\treturn;\n\
}\n\
\n\
void haris_write_uint32(unsigned char *b, haris_uint32_t i)\n\
{\n\
\t*b = i >> 24;\n\
\t*(b+1) = i >> 16;\n\
\t*(b+2) = i >> 8;\n\
\t*(b+3) = i;\n\
\treturn;\n\
}\n\
\n\
void haris_write_int32(unsigned char *b, haris_int32_t i)\n\
{\n\
\tif (i >= 0)\n\
\t\tharis_write_uint32(b, (haris_uint32_t)i);\n\
\telse {\n\
\t\tharis_write_uint32(b, (haris_uint32_t)(-i - 1));\n\
\t\t*b |= 0x80;\n\
\t}\n\
\treturn;\n\
}\n\
\n\
void haris_write_uint64(unsigned char *b, haris_uint64_t i)\n\
{\n\
\t*b = i >> 56;\n\
\t*(b+1) = i >> 48;\n\
\t*(b+2) = i >> 40;\n\
\t*(b+3) = i >> 32;\n\
\t*(b+4) = i >> 24;\n\
\t*(b+5) = i >> 16;\n\
\t*(b+6) = i >> 8;\n\
\t*(b+7) = i;\n\
\treturn;\n\
}\n\
\n\
void haris_write_int64(unsigned char *b, haris_int64_t i)\n\
{\n\
\tif (i >= 0)\n\
\t\tharis_write_uint64(b, (haris_uint64_t)i);\n\
\telse {\n\
\t\tharis_write_uint64(b, (haris_uint64_t)(-i - 1));\n\
\t\t*b |= 0x80;\n\
\t}\n\
\treturn;\n\
}\n\
\n\
float haris_read_float32(unsigned char *b)\n\
{\n\
\tdouble result;\n\
\tharis_int64_t shift;\n\
\tharis_uint32_t i = haris_read_uint32(b);\n\
\t\n\
\tif (i == 0) return 0.0;\n\
\t\n\
\tresult = (i & ((1LL << HARIS_FLOAT32_SIGBITS) - 1));\n\
\tresult /= (1LL << HARIS_FLOAT32_SIGBITS); \n\
\tresult += 1.0;\n\
\n\
\tshift = ((i >> HARIS_FLOAT32_SIGBITS) & 255) - HARIS_FLOAT32_BIAS;\n\
\twhile (shift > 0) { result *= 2.0; shift--; }\n\
\twhile (shift < 0) { result /= 2.0; shift++; }\n\
\n\
\tresult *= (i >> 31) & 1 ? -1.0: 1.0;\n\
\n\
\treturn (float)result;\n\
}\n\
\n\
double haris_read_float64(unsigned char *b)\n\
{\n\
\tdouble result;\n\
\tharis_int64_t shift;\n\
\tharis_uint64_t i = haris_read_uint64(b);\n\
\t\n\
\tif (i == 0) return 0.0;\n\
\t\n\
\tresult = (i & (( 1LL << HARIS_FLOAT64_SIGBITS) - 1));\n\
\tresult /= (1LL << HARIS_FLOAT64_SIGBITS); \n\
\tresult += 1.0;\n\
\n\
\tshift = ((i >> HARIS_FLOAT64_SIGBITS) & 2047) - HARIS_FLOAT64_BIAS;\n\
\twhile (shift > 0) { result *= 2.0; shift--; }\n\
\twhile (shift < 0) { result /= 2.0; shift++; }\n\
\n\
\tresult *= (i >> 63) & 1 ? -1.0: 1.0;\n\
\n\
\treturn result;\n\
}\n\
\n\
void haris_write_float32(unsigned char *b, float f)\n\
{\n\
\tdouble fnorm;\n\
\tint shift;\n\
\tlong sign, exp, significand;\n\
\tharis_uint32_t result;\n\
\n\
\tif (f == 0.0) {\n\
\t\tharis_write_uint32(b, 0);\n\
\t\treturn;\n\
\t} \n\
\n\
\tif (f < 0) {\n\
\t\tsign = 1; \n\
\t\tfnorm = -f; \n\
\t} else { \n\
\t\tsign = 0; \n\
\t\tfnorm = f; \n\
\t}\n\
\n\
\tshift = 0;\n\
\twhile (fnorm >= 2.0) { fnorm /= 2.0; shift++; }\n\
\twhile (fnorm < 1.0) { fnorm *= 2.0; shift--; }\n\
\tfnorm = fnorm - 1.0;\n\
\n\
\tsignificand = fnorm * ((1LL << HARIS_FLOAT32_SIGBITS) + 0.5f);\n\
\n\
\texp = shift + ((1<<7) - 1); \n\
\n\
\tresult = (sign<<31) | (exp<<23) | significand;\n\
\tharis_write_uint32(b, result);\n\
\treturn;\n\
}\n\
\n\
void haris_write_float64(unsigned char *b, double f)\n\
{\n\
\tdouble fnorm;\n\
\tint shift;\n\
\tlong sign, exp, significand;\n\
\tharis_uint64_t result;\n\
\n\
\tif (f == 0.0) {\n\
\t\tharis_write_uint64(b, 0);\n\
\t}\n\
\n\
\tif (f < 0) {\n\
\t\tsign = 1; \n\
\t\tfnorm = -f; \n\
\t} else { \n\
\t\tsign = 0; \n\
\t\tfnorm = f; \n\
\t}\n\
\n\
\tshift = 0;\n\
\twhile (fnorm >= 2.0) { fnorm /= 2.0; shift++; }\n\
\twhile (fnorm < 1.0) { fnorm *= 2.0; shift--; }\n\
\tfnorm = fnorm - 1.0;\n\
\n\
\tsignificand = fnorm * ((1LL<<HARIS_FLOAT64_SIGBITS) + 0.5f);\n\
\n\
\texp = shift + ((1<<10) - 1); \n\
\n\
\tresult = (sign<<63) | (exp<<52) | significand;\n\
\tharis_write_uint64(b, result);\n\
\treturn;\n\
}\n";

const char *cgen_util_h_contents = "#ifndef __UTIL_H\n\
#define __UTIL_H\n\
\n\
/* In order to generate C code, the utility library needs exact-precision\n\
\t unsigned and signed integers. In particular, in order to ensure that we don't\n\
\t overflow the native integer containers when parsing Haris messages, we need\n\
\t to be certain about the minimum size of our integers. Haris defines the\n\
\t typedefs\n\
\t haris_intN_t\n\
\t and\n\
\t haris_uintN_t\n\
\t for N in [8, 16, 32, 64]. The intN types must be signed, and the uintN types\n\
\t must be unsigned. Each type must have at least N bits (that is, haris_int8_t\n\
\t must have at least 8 bits and must be signed). If your system includes \n\
\t stdint.h (which it will if you have a standard-conforming C99 compiler), then\n\
\t the typedef's automatically generated by the code generator should be \n\
\t sufficient, and you should be able to include the generated files in your \n\
\t project without changing them. If you do not have stdint.h, then you'll have\n\
\t to manually modify the following 8 typedef's yourself (though that shouldn't\n\
\t take more than a minute to do). Make sure to remove the #include directive \n\
\t if you do not have stdint.h.\n\
*/\n\
\n\
#include <stdint.h>\n\
\n\
typedef uint_fast8_t\t\tharis_uint8_t;\n\
typedef int_fast8_t\t\t haris_int8_t;\n\
typedef uint_fast16_t\t haris_uint16_t;\n\
typedef int_fast16_t\t\tharis_int16_t;\n\
typedef uint_fast32_t\t haris_uint32_t;\n\
typedef int_fast32_t\t\tharis_int32_t;\n\
typedef uint_fast64_t\t haris_uint64_t;\n\
typedef int_fast64_t\t\tharis_int64_t;\n\
\n\
/* \"Magic numbers\" for use by float-reading and -writing functions; do not \n\
\t modify\n\
*/\n\
\n\
#define HARIS_FLOAT32_SIGBITS 23\n\
#define HARIS_FLOAT32_BIAS\t\t127\n\
#define HARIS_FLOAT64_SIGBITS 52\n\
#define HARIS_FLOAT64_BIAS\t\t1023\n\
\n\
haris_uint8_t haris_read_uint8(unsigned char *);\n\
haris_int8_t haris_read_int8(unsigned char *);\n\
haris_uint16_t haris_read_uint16(unsigned char *);\n\
haris_int16_t haris_read_int16(unsigned char *);\n\
haris_uint32_t haris_read_uint32(unsigned char *);\n\
haris_int32_t haris_read_int32(unsigned char *);\n\
haris_uint64_t haris_read_uint64(unsigned char *);\n\
haris_int64_t haris_read_int64(unsigned char *);\n\
\n\
void haris_write_uint8(unsigned char *, haris_uint8_t);\n\
void haris_write_int8(unsigned char *, haris_int8_t);\n\
void haris_write_uint16(unsigned char *, haris_uint16_t);\n\
void haris_write_int16(unsigned char *, haris_int16_t);\n\
void haris_write_uint32(unsigned char *, haris_uint32_t);\n\
void haris_write_int32(unsigned char *, haris_int32_t);\n\
void haris_write_uint64(unsigned char *, haris_uint64_t);\n\
void haris_write_int64(unsigned char *, haris_int64_t);\n\
\n\
float haris_read_float32(unsigned char *);\n\
double haris_read_float64(unsigned char *);\n\
\n\
void haris_write_float32(unsigned char *, float);\n\
void haris_write_float64(unsigned char *, double);\n\
\n\
#endif\n";

#endif
