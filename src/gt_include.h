#ifndef __GT_INCLUDE_H
#define __GT_INCLUDE_H
#include <math.h>
#include "gt_signal.h"
#include "gt_spinlock.h"
#include "gt_tailq.h"
#include "gt_bitops.h"
extern int policy; 
#define NUM_CPUS 4
#include "gt_uthread.h"
#include "gt_pq.h"
#include "gt_kthread.h"
#endif
