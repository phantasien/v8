// Copyright 2014 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_BASE_BITS_H_
#define V8_BASE_BITS_H_

#include "include/v8stdint.h"
#if V8_CC_MSVC
#include <intrin.h>
#endif
#if V8_OS_WIN32
#include "src/base/win32-headers.h"
#endif

namespace v8 {
namespace base {
namespace bits {

// CountPopulation32(value) returns the number of bits set in |value|.
inline uint32_t CountPopulation32(uint32_t value) {
#if V8_HAS_BUILTIN_POPCOUNT
  return __builtin_popcount(value);
#else
  value = ((value >> 1) & 0x55555555) + (value & 0x55555555);
  value = ((value >> 2) & 0x33333333) + (value & 0x33333333);
  value = ((value >> 4) & 0x0f0f0f0f) + (value & 0x0f0f0f0f);
  value = ((value >> 8) & 0x00ff00ff) + (value & 0x00ff00ff);
  value = ((value >> 16) & 0x0000ffff) + (value & 0x0000ffff);
  return value;
#endif
}


// CountLeadingZeros32(value) returns the number of zero bits following the most
// significant 1 bit in |value| if |value| is non-zero, otherwise it returns 32.
inline uint32_t CountLeadingZeros32(uint32_t value) {
#if V8_HAS_BUILTIN_CLZ
  return value ? __builtin_clz(value) : 32;
#elif V8_CC_MSVC
  unsigned long result;  // NOLINT(runtime/int)
  if (!_BitScanReverse(&result, value)) return 32;
  return static_cast<uint32_t>(31 - result);
#else
  value = value | (value >> 1);
  value = value | (value >> 2);
  value = value | (value >> 4);
  value = value | (value >> 8);
  value = value | (value >> 16);
  return CountPopulation32(~value);
#endif
}


// CountTrailingZeros32(value) returns the number of zero bits preceding the
// least significant 1 bit in |value| if |value| is non-zero, otherwise it
// returns 32.
inline uint32_t CountTrailingZeros32(uint32_t value) {
#if V8_HAS_BUILTIN_CTZ
  return value ? __builtin_ctz(value) : 32;
#elif V8_CC_MSVC
  unsigned long result;  // NOLINT(runtime/int)
  if (!_BitScanForward(&result, value)) return 32;
  return static_cast<uint32_t>(result);
#else
  if (value == 0) return 32;
  unsigned count = 0;
  for (value ^= value - 1; value >>= 1; ++count)
    ;
  return count;
#endif
}


// Returns true iff |value| is a power of 2.
inline bool IsPowerOfTwo32(uint32_t value) {
  return value && !(value & (value - 1));
}


// Returns true iff |value| is a power of 2.
inline bool IsPowerOfTwo64(uint64_t value) {
  return value && !(value & (value - 1));
}


// RoundUpToPowerOfTwo32(value) returns the smallest power of two which is
// greater than or equal to |value|. If you pass in a |value| that is already a
// power of two, it is returned as is. |value| must be less than or equal to
// 0x80000000u. Implementation is from "Hacker's Delight" by Henry S. Warren,
// Jr., figure 3-3, page 48, where the function is called clp2.
uint32_t RoundUpToPowerOfTwo32(uint32_t value);


// RoundDownToPowerOfTwo32(value) returns the greatest power of two which is
// less than or equal to |value|. If you pass in a |value| that is already a
// power of two, it is returned as is.
inline uint32_t RoundDownToPowerOfTwo32(uint32_t value) {
  if (value > 0x80000000u) return 0x80000000u;
  uint32_t result = RoundUpToPowerOfTwo32(value);
  if (result > value) result >>= 1;
  return result;
}


inline uint32_t RotateRight32(uint32_t value, uint32_t shift) {
  if (shift == 0) return value;
  return (value >> shift) | (value << (32 - shift));
}


inline uint64_t RotateRight64(uint64_t value, uint64_t shift) {
  if (shift == 0) return value;
  return (value >> shift) | (value << (64 - shift));
}

}  // namespace bits
}  // namespace base
}  // namespace v8

#endif  // V8_BASE_BITS_H_
