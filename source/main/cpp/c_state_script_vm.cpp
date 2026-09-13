#include "rcore/c_app.h"
#include "rcore/c_log.h"
#include "rcore/c_system.h"

#include "ccore/c_math.h"

#include "ccova/vm.h"
#include "ccova/image.h"
#include "ccova/segment_memory.h"

#include "main/c_app_data.h"
#include "main/c_script_host.h"

namespace ncore
{
    struct app_vm_t
    {
        segment_memory_t        m_external;
        segment_memory_t        m_frame;
        segment_memory_t        m_bss;
        segment_memory_t        m_data;
        segment_memory_t        m_stack;
        call_frame_t*           m_call_frames;
        const linked_program_t* linked_program;
        vm_t                    m_vm;
    };
    static app_vm_t s_app_vm;

    void state_initialize_script_vm(fsm_state_data_t* state_data, app_data_t* app_data, u64 now_ms)
    {
        app_vm_t* app_vm = &s_app_vm;

        app_vm->linked_program = open_program_image((byte*)app_data->m_script_binary, app_data->m_script_binary_size);

        // Allocate the script VM memory regions
        // CALL FRAMES
        const u32 call_frame_capacity = 16;
        void*     call_frames_memory  = nsystem::malloc(call_frame_capacity * sizeof(call_frame_t));
        app_vm->m_call_frames       = (call_frame_t*)call_frames_memory;
        // STACK
        const u32 stack_byte_size = 256;
        void*     stack_memory    = nsystem::malloc(stack_byte_size);
        // BSS
        const u32 bss_byte_size = math::alignUp(app_vm->linked_program->m_bss_byte_size, 32);
        void*     bss_memory    = nsystem::malloc(bss_byte_size);
        // DATA
        const u32 data_data_size = math::alignUp(app_vm->linked_program->m_data_data.m_size, 32);
        void*     data_memory    = nsystem::malloc(data_data_size);
        // FRAME
        const u32 frame_byte_size = math::alignUp(app_vm->linked_program->m_frame_byte_size, 32);
        void*     frame_memory    = nsystem::malloc(frame_byte_size);

        app_vm->m_bss.m_data       = (byte*)bss_memory;
        app_vm->m_bss.m_capacity   = bss_byte_size;
        app_vm->m_bss.m_size       = app_vm->linked_program->m_bss_byte_size;
        app_vm->m_data.m_data      = (byte*)data_memory;
        app_vm->m_data.m_capacity  = data_data_size;
        app_vm->m_data.m_size      = app_vm->linked_program->m_data_data.m_size;
        app_vm->m_frame.m_data     = (byte*)frame_memory;
        app_vm->m_frame.m_capacity = frame_byte_size;
        app_vm->m_frame.m_size     = app_vm->linked_program->m_frame_byte_size;
        app_vm->m_stack.m_data     = (byte*)stack_memory;
        app_vm->m_stack.m_capacity = stack_byte_size;
        app_vm->m_stack.m_size     = 0;

        initialize_vm(&app_vm->m_vm, app_vm->m_call_frames, call_frame_capacity, app_vm->m_frame, app_vm->m_bss, app_vm->m_external, app_vm->m_data, app_vm->m_stack);
        register_extern_dispatcher(&app_vm->m_vm, app_data, extern_host_fn);

        to_state_next(&app_data->m_state_data);
    }

    void update_script(app_data_t& app_data, u64 now_ms) 
    { 
        app_vm_t* app_vm = &s_app_vm;

        run_vm(&app_vm->m_vm, app_vm->linked_program);
    }

}  // namespace ncore