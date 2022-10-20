#ifndef _BYTE_VECTOR_H_
#define _BYTE_VECTOR_H_

#include <stdint.h>
#include <stdlib.h>

/**
 * @brief A struct representing a vector of bytes.
 */
typedef struct {
    /**
     * @brief The internal array.
     */
    uint8_t *array;
    /**
     * @brief The length of the vector.
     *
     * How much of the #array is filled up.
     */
    size_t length;
    /**
     * @brief The capacity of the vector.
     *
     * The size of the #array.
     */
    size_t capacity;
} ByteVector;

/**
 * @brief Creates a new vector with length 0.
 *
 * @return The new vector.
 */
ByteVector *bv_create();
/**
 * @brief Destroys a vector.
 *
 * @param vector The vector to destroy.
 */
void bv_destroy(ByteVector *vector);

/**
 * @brief Pushes an array of bytes to end of a vector.
 *
 * @param vector The vector to push to.
 * @param buf The array to push.
 * @param buf_len The size of the array.
 */
void bv_push(ByteVector *vector, const uint8_t *buf, size_t buf_len);
/**
 * @brief Pushes a byte to the end of a vector.
 *
 * @param vector The vector to push to.
 * @param byte The byte to push.
 */
void bv_pushb(ByteVector *vector, uint8_t byte);

/**
 * @brief Pops a byte from the end of a vector.
 *
 * @note If the vector is empty, 0 is returned.
 *
 * @param vector The vector to pop from.
 *
 * @return The byte that was popped.
 */
uint8_t bv_popb(ByteVector *vector);

/**
 * @brief Sets a byte at an index of a vector.
 *
 * @note If the index is outside the bounds of the vector, it is expanded.
 *
 * @param vector The vector.
 * @param i The index.
 * @param byte The byte.
 */
void bv_set(ByteVector *vector, size_t i, uint8_t byte);
/**
 * @brief Gets a byte at and index of a vector.
 *
 * @note If the index is outside the bounds of the vector, 0 is returned.
 *
 * @param vector The vector.
 * @param i The index.
 *
 * @return The byte that was popped.
 */
uint8_t bv_get(ByteVector *vector, size_t i);

#endif // _BYTE_VECTOR_H_
