#pragma once

#include <defs.hh>
#include <lib/list.hh>
#include <lib/string.hh>
#include <obj/module.hh>

#define OPTIONAL_HEADER_START(img_base) \
	( (OptionalHeader*)( \
		((CoffHeader*)( (u8*)(img_base) + ((MzHeader*)(img_base))->PeSigOffset) ) + 1) )

void LdrInitializeLoader(KernelBootInfo *info);
PZ_KERNEL_EXPORT void *LdrLoadImage(
    PzProcessObject *process, const char *mod_name,
    void *start_address,
    u32 img_size,
    PzHandle *handle);
PZ_KERNEL_EXPORT PzStatus LdrLoadImageFile(
    PzHandle process,
    const PzString *exec_path,
    const PzString *image_name,
    PzHandle *mod_handle);
uptr LdriGetSymbolAddress(
    PzModuleObject *module,
    const char *symbol_name);
PZ_KERNEL_EXPORT uptr LdrGetSymbolAddress(
    PzHandle mod_handle,
    const char *symbol_name);
PZ_KERNEL_EXPORT void LdrUnloadImage(PzModuleObject *module_obj);