#ifndef LINEARSCAN_H
#define LINEARSCAN_H

#include "util.h"
#include "list.h"
#include "frame.h"

typedef struct liveInterval_ *liveInterval;

void linScan_compute(frame, list blocks, list *liveIntervals, bool *);
void linScan_complete(frame, list liveIntervals, list blocks);

#endif
