#pragma once

#include <defs.hh>

#define NAMESPACE_KERNEL_OBJECT  1
#define NAMESPACE_USER_OBJECT    2
#define NAMESPACE_THREAD_PROCESS 3

uptr IdGenerateUniqueId(int id_namespace);