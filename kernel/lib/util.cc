#include <lib/util.hh>

extern "C" void HalRepMemcpy(void *dest, const void *src, int count);

void MemCopy(void *dest, const void *src, int count)
{
    if (count > 0)
        HalRepMemcpy(dest, src, count);
}

void MemSet(void *dest, int value, int count)
{
    for (int i = 0; i < count; i++)
        ((char *)dest)[i] = value;
}