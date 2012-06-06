#ifndef LOG_ENDIAN_H
#define LOG_ENDIAN_H

#include <QtGlobal>
#include <byteswap.h>

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
#define LOG_ENDIAN(x) (x)
#else
#define LOG_ENDIAN(x) (bswap(x))
#endif

template <typename T> T bswap(T v);
template <> inline uint64_t bswap<uint64_t>(uint64_t v) { return bswap_64(v); }
template <> inline uint32_t bswap<uint32_t>(uint32_t v) { return bswap_32(v); }
template <> inline uint16_t bswap<uint16_t>(uint16_t v) { return bswap_16(v); }
template <> inline uint8_t bswap<uint8_t>(uint8_t v) { return v; }
template <> inline int64_t bswap<int64_t>(int64_t v) { return bswap_64(v); }
template <> inline int32_t bswap<int32_t>(int32_t v) { return bswap_32(v); }
template <> inline int16_t bswap<int16_t>(int16_t v) { return bswap_16(v); }
template <> inline int8_t bswap<int8_t>(int8_t v) { return v; }

#endif // LOG_ENDIAN_H
