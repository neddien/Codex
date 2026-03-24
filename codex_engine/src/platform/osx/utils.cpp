#include <engine/system/utils.h>

#include <mach/mach.h>

namespace codex::sys {
    u32 get_engine_thread_count() noexcept
    {
        thread_act_array_t     thread_list;
        mach_msg_type_number_t thread_count;

        task_t task = mach_task_self();

        if (task_threads(task, &thread_list, &thread_count) != KERN_SUCCESS)
            return -1;

        vm_deallocate(task, (vm_address_t)thread_list, thread_count * sizeof(thread_act_t));

        return thread_count;
    }

    u64 get_current_thread_id() noexcept
    {
        u64 tid;
        pthread_threadid_np(nullptr, &tid);
        return tid;
    }
} // namespace codex::sys
