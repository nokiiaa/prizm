#pragma once

#include <defs.hh>

typedef struct { bool Acquired; int ReturnIrql; } PzSpinlock;

PZ_KERNEL_EXPORT void PzAcquireSpinlock(PzSpinlock *spinlock);
PZ_KERNEL_EXPORT void PzReleaseSpinlock(PzSpinlock *spinlock);
PZ_KERNEL_EXPORT bool PzIsSpinlockAcquired(PzSpinlock spinlock);