#include "utility.h"

void reverseBytes(char* bytes, int count)
{
  int swaps = ((count % 2) == 1) ? (count - 1) / 2 : count / 2;
  char tmp;
  for(int i = 0; i < swaps; ++i){
    tmp = bytes[i]; 
    bytes[i] = bytes[count - 1 - i];
    bytes[count - i] = tmp;
  }
}

// reminder: Endianess determines the order in which bytes are stored in memory. Consider a 
// 32-bit integer 'n' assigned the hex value 0xa3b2c1d0. Its memory layout on each system can 
// be illustrated as:
//
//    lower addresses --------------------------------------> higher addresses
//            +----+----+----+----+            +----+----+----+----+
//            |0xd0|0xc1|0xb2|0xa3|            |0xa3|0xb2|0xc1|0xd0|
//            +----+----+----+----+            +----+----+----+----+
//            |                                |
//            &x                               &x
//
//              [little-endian]                      [big-endian]
//
//         little-end (LSB) of x at            big-end (MSB) of x at
//         lower address.                      lower address.
//
// Independent of the endianess however &x always returns the byte at the lower address due to
// the way C/C++ implements pointer arithmetic. If this were not the case you could not reliably
// increment a pointer to move through an array or buffer, e.g,
//
//   char buffer[10];
//   char* p = buffer;
//   for(int i{0}; i<10; ++i)
//     cout << (*p)++ << endl;
//
// would be platform dependent if '&' (address-of) operator returned the address of the LSB and
// not (as it does) the lowest address used by the operator target. If the former were true you
// would have to increment the pointer on little-endian systems and decrement it on big-endian
// systems.
//
// Thus this function takes advantage of the platform independence of the address-of operator to
// test for endianess.
bool isSystemLittleEndian()
{
  uint32_t n {0x00000001};                           // LSB == 1.
  uint8_t* p = reinterpret_cast<uint8_t*>(&n);       // pointer to byte at lowest address.
  return (*p);                                       // value of byte at lowest address.
}

// Extracts a type T from a byte buffer containing the bytes of T stored in little-endian 
// format (in the buffer). The endianess of the system is accounted for.

uint16_t extractLittleEndianUint16(char* buffer)
// predicate: buffer size is at least sizeof(uint16_t) bytes.
{
  if(!isSystemLittleEndian())
    reverseBytes(buffer, sizeof(uint16_t));
  return *(reinterpret_cast<uint16_t*>(buffer));
}

uint32_t extractLittleEndianUint32(char* buffer)
// predicate: buffer size is at least sizeof(uint32_t) bytes.
{
  if(!isSystemLittleEndian())
    reverseBytes(buffer, sizeof(uint32_t));
  return *(reinterpret_cast<uint32_t*>(buffer));
}

uint64_t extractLittleEndianUint64(char* buffer)
// predicate: buffer size is at least sizeof(uint64_t) bytes.
{
  if(!isSystemLittleEndian())
    reverseBytes(buffer, sizeof(uint64_t));
  return *(reinterpret_cast<uint64_t*>(buffer));
}

int16_t extractLittleEndianInt16(char* buffer)
{
// predicate: buffer size is at least sizeof(int16_t) bytes.
  if(!isSystemLittleEndian())
    reverseBytes(buffer, sizeof(int16_t));
  return *(reinterpret_cast<int16_t*>(buffer));
}

int32_t extractLittleEndianInt32(char* buffer)
{
// predicate: buffer size is at least sizeof(int32_t) bytes.
  if(!isSystemLittleEndian())
    reverseBytes(buffer, sizeof(int32_t));
  return *(reinterpret_cast<int32_t*>(buffer));
}

int64_t extractLittleEndianInt64(char* buffer)
{
// predicate: buffer size is at least sizeof(int64_t) bytes.
  if(!isSystemLittleEndian())
    reverseBytes(buffer, sizeof(int64_t));
  return *(reinterpret_cast<int64_t*>(buffer));
}

