#include "byte_vector.h"
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#define BUFFER 64

void resize_if_needed(ByteVector *this) {
    if (this->capacity < this->length) {
        this->capacity = this->length + BUFFER;
        this->array =
            reallocarray(this->array, this->capacity, sizeof(unsigned char));
    }
}

ByteVector *bv_create() {
    ByteVector *v = malloc(sizeof(ByteVector));
    v->length = 0;
    v->capacity = BUFFER;
    v->array = calloc(v->capacity, sizeof(unsigned char));
    return v;
}

void bv_destroy(ByteVector *this) {
    free(this->array);
    free(this);
}

void bv_push(ByteVector *this, unsigned char *buf, size_t buf_len) {
    size_t i = this->length;
    this->length += buf_len;
    resize_if_needed(this);
    memcpy(this->array + i, buf, buf_len);
}

void bv_pushb(ByteVector *this, unsigned char byte) {
    size_t i = this->length++;
    resize_if_needed(this);
    this->array[i] = byte;
}

void bv_set(ByteVector *this, size_t i, unsigned char byte) {
    this->length = MAX(this->length, i + 1);
    resize_if_needed(this);
    this->array[i] = byte;
}

unsigned char bv_get(ByteVector *this, size_t i) {
    this->length = MAX(this->length, i + 1);
    resize_if_needed(this);
    return this->array[i];
}
