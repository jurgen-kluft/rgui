#include "rcore/c_app.h"
#include "rcore/c_log.h"

#include "ccova/linked_program.h"

#include "lib_guition/c_sdcard.h"

#include "main/c_app_data.h"

namespace ncore
{
    static bool s_has_newer_version(const char* filename, u32 current_version)
    {
        u32 pack_version = 0;
        nlcd::sdcard_read_bytes(filename, 0, (u8*)&pack_version, sizeof(pack_version));
        return (pack_version > current_version);
    }

    void state_handle_sd_card(fsm_state_data_t* state_data, app_data_t* app_data, u64 now_ms)
    {
        if (state_data->m_current_state == 0)
        {
            nlog::log_info("SD Card", "Initializing");

            if (nlcd::sdcard_initialize() == false)
            {
                nlog::log_warn("SD Card", "Failed to initialize");
                to_state_error(state_data);
                return;
            }

            // Mark the SD card as available
            app_data->m_sd_card_available = true;

            // Scan files for the availability of:
            // - sprite_pack.bin
            // - font_pack.bin
            // - palette_pack.bin
            // - script.bin
            const char* filename = "sprite_pack.bin";
            if (app_data->m_sprite_pack_size == 0 || s_has_newer_version(filename, app_data->m_sprite_pack->m_version))
            {
                if (nlcd::sdcard_read_file(filename, (u8*)app_data->m_sprite_pack, app_data->m_sprite_pack_capacity, app_data->m_sprite_pack_size))
                {
                    app_data->m_sprite_pack_source = EDATA_SOURCE_SD_CARD;
                }
            }

            filename = "font_pack.bin";
            if (app_data->m_font_pack_size == 0 || s_has_newer_version(filename, app_data->m_font_pack->m_version))
            {
                if (nlcd::sdcard_read_file(filename, (u8*)app_data->m_font_pack, app_data->m_font_pack_capacity, app_data->m_font_pack_size))
                {
                    app_data->m_font_pack_source = EDATA_SOURCE_SD_CARD;
                }
            }

            filename = "palette_pack.bin";
            if (app_data->m_palette_pack_size == 0 || s_has_newer_version(filename, app_data->m_palette_pack->m_version))
            {
                if (nlcd::sdcard_read_file(filename, (u8*)app_data->m_palette_pack, app_data->m_palette_pack_capacity, app_data->m_palette_pack_size))
                {
                    app_data->m_palette_pack_source = EDATA_SOURCE_SD_CARD;
                }
            }

            filename = "script.bin";
            if (app_data->m_script_binary_size == 0 || s_has_newer_version(filename, app_data->m_script_binary->m_version))
            {
                if (nlcd::sdcard_read_file(filename, (u8*)app_data->m_script_binary, app_data->m_script_binary_capacity, app_data->m_script_binary_size))
                {
                    app_data->m_script_binary_source = EDATA_SOURCE_SD_CARD;
                }
            }

            // Next state
            to_state_next(state_data);

            nlog::log_info("SD Card", "Ok");
        }
    }

}  // namespace ncore