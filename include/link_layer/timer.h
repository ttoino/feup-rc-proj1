#ifndef _LINK_LAYER_TIMER_H_
#define _LINK_LAYER_TIMER_H_

#include "link_layer.h"

/**
 * @brief Sets up the timer for a given connection.
 *
 * @param connection The connection.
 */
int timer_setup(LLConnection *connection);
/**
 * @brief Destroys the timer for a given connection.
 *
 * @param connection The connection
 */
int timer_destroy(LLConnection *connection);

/**
 * @brief Arms the timer for a given connection.
 *
 * @param connection The connection
 */
int timer_arm(LLConnection *connection);
/**
 * @brief Disarms the timer for a given connection.
 *
 * @param connection The connection
 */
int timer_disarm(LLConnection *connection);

/**
 * @brief Forces the timer handler of a given connection to be run.
 *
 * @param connection The connection
 */
int timer_force(LLConnection *connection);

#endif // _LINK_LAYER_TIMER_H_
