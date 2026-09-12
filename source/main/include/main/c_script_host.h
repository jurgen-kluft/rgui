#ifndef __C_SCRIPT_HOST_H__
#define __C_SCRIPT_HOST_H__
#include "rcore/c_target.h"
#ifdef USE_PRAGMA_ONCE
    #pragma once
#endif

namespace ncore
{
    struct vm_t;
    void extern_host_fn(void* host_context, vm_t* vm, u32 import_id);

}  // namespace ncore

#endif  // __C_SCRIPT_HOST_H__
