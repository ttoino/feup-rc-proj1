#ifndef _BYTE_VECTOR_H_
#define _BYTE_VECTOR_H_

#include <stdlib.h>

typedef struct {
    unsigned char *array;
    size_t length;
    size_t capacity;
} ByteVector;

ByteVector *bv_create();
void bv_destroy(ByteVector *vector);

void bv_push(ByteVector *vector, unsigned char *buf, size_t buf_len);
void bv_pushb(ByteVector *vector, unsigned char byte);

void bv_set(ByteVector *vector, size_t i, unsigned char byte);
unsigned char bv_get(ByteVector *vector, size_t i);

#endif // _BYTE_VECTOR_H_
