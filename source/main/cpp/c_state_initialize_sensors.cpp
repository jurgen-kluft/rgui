#include "rcore/c_app.h"
#include "rcore/c_log.h"

#include "main/c_app_data.h"

namespace ncore
{ 
    void state_initialize_sensors(fsm_state_data_t& state_data, app_data_t& app_data, u64 now_ms)
    {
        to_state_next(state_data);  // Move to the next state in the FSM
    }

}  // namespace ncore