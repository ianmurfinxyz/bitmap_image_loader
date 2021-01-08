#ifndef _UTILITY_H_
#define _UTILITY_H_

// Reverses the order of the bytes in the buffer 'bytes' of size 'count'.
void reverseBytes(char* bytes, int count);

// Tests if the platform executing this program is little endian (run-time test).
bool isSystemLittleEndian();

// The following functions extract a type T from a byte buffer containing the bytes of 
// an instance of T stored in little-endian format (in the buffer). The endianess of 
// the system is accounted for such that these functions are platform independent.

// predicate: buffer size is at least sizeof(uint16_t) bytes.
uint16_t extractLittleEndianUint16(char* buffer);

// predicate: buffer size is at least sizeof(uint32_t) bytes.
uint32_t extractLittleEndianUint32(char* buffer);

// predicate: buffer size is at least sizeof(uint64_t) bytes.
uint64_t extractLittleEndianUint64(char* buffer);

// predicate: buffer size is at least sizeof(int16_t) bytes.
int16_t extractLittleEndianInt16(char* buffer);

// predicate: buffer size is at least sizeof(int32_t) bytes.
int32_t extractLittleEndianInt32(char* buffer);

// predicate: buffer size is at least sizeof(int64_t) bytes.
int64_t extractLittleEndianInt64(char* buffer);

#endif
