#define _GNU_SOURCE

#include "byte_vector.h"
#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#define BUFFER 64

/**
 * @brief Resizes a vector if its current capacity is too low.
 *
 * @param this The vector to resize.
 */
void resize_if_needed(ByteVector *this) {
    if (this->capacity < this->length) {
        this->capacity = this->length + BUFFER;
        this->array =
            reallocarray(this->array, this->capacity, sizeof(uint8_t));
    }
}

ByteVector *bv_create() {
    ByteVector *v = malloc(sizeof(ByteVector));
    v->length = 0;
    v->capacity = BUFFER;
    v->array = calloc(v->capacity, sizeof(uint8_t));
    return v;
}

void bv_destroy(ByteVector *this) {

    if (this == NULL)
        return;

    free(this->array);
    free(this);
}

void bv_push(ByteVector *this, uint8_t *buf, size_t buf_len) {
    size_t i = this->length;
    this->length += buf_len;
    resize_if_needed(this);
    memcpy(this->array + i, buf, buf_len);
}

void bv_pushb(ByteVector *this, uint8_t byte) {
    size_t i = this->length++;
    resize_if_needed(this);
    this->array[i] = byte;
}

uint8_t bv_popb(ByteVector *this) {
    if (this->length == 0)
        return 0;

    return this->array[--this->length];
}

void bv_set(ByteVector *this, size_t i, uint8_t byte) {
    this->length = MAX(this->length, i + 1);
    resize_if_needed(this);
    this->array[i] = byte;
}

uint8_t bv_get(ByteVector *this, size_t i) {
    if (i >= this->length)
        return 0;

    return this->array[i];
}
