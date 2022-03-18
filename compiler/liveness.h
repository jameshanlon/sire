#ifndef LIVENESS_H
#define LIVENESS_H

#include "list.h"

void liveness_compute(list blocks);
void liveness_eliminateDead(list blocks);

#endif
