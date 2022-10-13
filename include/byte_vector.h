#ifndef _BYTE_VECTOR_H_
#define _BYTE_VECTOR_H_

#include <stdint.h>
#include <stdlib.h>

typedef struct {
    uint8_t *array;
    size_t length;
    size_t capacity;
} ByteVector;

ByteVector *bv_create();
void bv_destroy(ByteVector *vector);

void bv_push(ByteVector *vector, uint8_t *buf, size_t buf_len);
void bv_pushb(ByteVector *vector, uint8_t byte);

uint8_t bv_popb(ByteVector *vector);

void bv_set(ByteVector *vector, size_t i, uint8_t byte);
uint8_t bv_get(ByteVector *vector, size_t i);

#endif // _BYTE_VECTOR_H_
