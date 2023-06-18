#ifndef __ARM64_UTILS_H__
#define __ARM64_UTILS_H__


namespace X265_NS
{
void transpose8x8(uint8_t *dst, const uint8_t *src, intptr_t dstride, intptr_t sstride);
void transpose16x16(uint8_t *dst, const uint8_t *src, intptr_t dstride, intptr_t sstride);
void transpose32x32(uint8_t *dst, const uint8_t *src, intptr_t dstride, intptr_t sstride);
void transpose8x8(uint16_t *dst, const uint16_t *src, intptr_t dstride, intptr_t sstride);
void transpose16x16(uint16_t *dst, const uint16_t *src, intptr_t dstride, intptr_t sstride);
void transpose32x32(uint16_t *dst, const uint16_t *src, intptr_t dstride, intptr_t sstride);
}

#endif
