#include <syscall/except.hh>

PzStatus UmExCallNextHandler(CpuInterruptState *state, void *params)
{
    ExCallNextHandler(state);
    return STATUS_SUCCESS;
}

PzStatus UmExContinueExecution(CpuInterruptState *state, void *params)
{
    ExContinueExecution();
    return STATUS_SUCCESS;
}

PzStatus UmExPushExceptionHandler(CpuInterruptState *state, void *params)
{
    auto *prms = (UmExPushExceptionHandlerParams *)params;
    ExPushExceptionHandler(prms->Handler, prms->StackContext);
    return STATUS_SUCCESS;
}

PzStatus UmExPopExceptionHandler(CpuInterruptState *state, void *params)
{
    ExPopExceptionHandler();
    return STATUS_SUCCESS;
}