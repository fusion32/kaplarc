#ifndef DEF_H_
#define DEF_H_

#include "debug.h"
#include <stdint.h>
using int8	= int8_t;
using uint8	= uint8_t;
using int16	= int16_t;
using uint16	= uint16_t;
using int32	= int32_t;
using uint32	= uint32_t;
using int64	= int64_t;
using uint64	= uint64_t;
using uintptr	= uintptr_t;

template<typename T, size_t N>
constexpr size_t array_size(T (&arr)[N]){
	return N;
}

#endif //DEF_H_
