#ifndef _LINK_LAYER_TIMER_H_
#define _LINK_LAYER_TIMER_H_

#include "link_layer.h"

void timer_setup(LLConnection *connection);
void timer_destroy(LLConnection *connection);

void timer_arm(LLConnection *connection);
void timer_disarm(LLConnection *connection);

void timer_force(LLConnection *connection);

#endif // _LINK_LAYER_TIMER_H_
