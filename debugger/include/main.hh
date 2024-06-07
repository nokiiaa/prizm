#pragma once

#define DEBUGGER_INCLUDE
#define DEBUGGER_TARGET_PTR uint32_t
#define DEBUGGER_TARGET_IPTR int32_t
#define DEBUGGER_TARGET_SIZE size_t

#include <iostream>
#include <vector>
#include <obj/apipe.hh>
#include <obj/device.hh>
#include <obj/directory.hh>
#include <obj/driver.hh>
#include <obj/event.hh>
#include <obj/iocb.hh>
#include <obj/module.hh>
#include <obj/mutex.hh>
#include <obj/process.hh>
#include <obj/semaphore.hh>
#include <obj/symlink.hh>
#include <obj/thread.hh>
#include <obj/timer.hh>
#include <obj/window.hh>
#include <obj/gfx.hh>
#include <obj/manager.hh>
#include <io/manager.hh>
#include <debug.hh>
#include <view.hh>
#include <memory>
#include <map>

struct ObjView;

extern std::map<std::string, i64> EvalContext;
extern std::map<uptr, std::shared_ptr<ObjView>> BackcheckMap;