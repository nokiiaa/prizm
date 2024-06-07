#include <spinlock.hh>
#include <processor.hh>
#include <panic.hh>

extern "C" void HalAcquireSpinlock(PzSpinlock *spinlock);
extern "C" void HalReleaseSpinlock(PzSpinlock *spinlock);

void PzAcquireSpinlock(PzSpinlock *spinlock)
{
    bool acquired = spinlock->Acquired;

    if (acquired)
        HalAcquireSpinlock(spinlock);

    int old_irql = PzGetCurrentIrql();

    if (old_irql < DISPATCH_LEVEL)
        PzRaiseIrql(DISPATCH_LEVEL);

    spinlock->ReturnIrql = old_irql;

    if (!acquired)
        HalAcquireSpinlock(spinlock);
}

void PzReleaseSpinlock(PzSpinlock *spinlock)
{
    HalReleaseSpinlock(spinlock);
    PzLowerIrql(spinlock->ReturnIrql);
}

bool PzIsSpinlockAcquired(PzSpinlock spinlock)
{
    return spinlock.Acquired;
}