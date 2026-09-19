#include "rcore/c_app.h"
#include "rcore/c_log.h"
#include "rcore/c_little_fs.h"

#include "lib_guition/c_sdcard.h"

#include "main/c_app_data.h"

namespace ncore
{
    void state_handle_assets(fsm_state_data_t* state_data, app_data_t* app_data, u64 now_ms)
    {
        if (state_data->m_current_state == 0)
        {
            nlog::log_info("VERIFY", "Begin verifying assets");

            // Verify sprite, font and palette packs

            // Verify script binary

            // If all assets are verified successfully, then save all of
            // them to the appropriate storage (e.g., SD card and/or LittleFS)
            if (app_data->m_sd_card_available)
            {
                if (app_data->m_sprite_pack_size > 0 && app_data->m_sprite_pack_source != EDATA_SOURCE_SD_CARD)
                {
                    nlcd::sdcard_write_file("sprite_pack.bin", (u8*)app_data->m_sprite_pack, app_data->m_sprite_pack_size);
                }
                if (app_data->m_font_pack_size > 0 && app_data->m_font_pack_source != EDATA_SOURCE_SD_CARD)
                {
                    nlcd::sdcard_write_file("font_pack.bin", (u8*)app_data->m_font_pack, app_data->m_font_pack_size);
                }
                if (app_data->m_palette_pack_size > 0 && app_data->m_palette_pack_source != EDATA_SOURCE_SD_CARD)
                {
                    nlcd::sdcard_write_file("palette_pack.bin", (u8*)app_data->m_palette_pack, app_data->m_palette_pack_size);
                }
                if (app_data->m_script_binary_size > 0 && app_data->m_script_binary_source != EDATA_SOURCE_SD_CARD)
                {
                    nlcd::sdcard_write_file("script.bin", (u8*)app_data->m_script_binary, app_data->m_script_binary_size);
                }
                if (app_data->m_house_meta_size > 0 && app_data->m_house_meta_source != EDATA_SOURCE_SD_CARD)
                {
                    nlcd::sdcard_write_file("house.bin", (u8*)app_data->m_house_meta, app_data->m_house_meta_size);
                }
            }

            if (app_data->m_little_fs_available)
            {
                if (app_data->m_sprite_pack_size > 0 && app_data->m_sprite_pack_source != EDATA_SOURCE_LITTLE_FS)
                {
                    nlittlefs::file_save("sprite_pack.bin", (u8*)app_data->m_sprite_pack, app_data->m_sprite_pack_size);
                }
                if (app_data->m_font_pack_size > 0 && app_data->m_font_pack_source != EDATA_SOURCE_LITTLE_FS)
                {
                    nlittlefs::file_save("font_pack.bin", (u8*)app_data->m_font_pack, app_data->m_font_pack_size);
                }
                if (app_data->m_palette_pack_size > 0 && app_data->m_palette_pack_source != EDATA_SOURCE_LITTLE_FS)
                {
                    nlittlefs::file_save("palette_pack.bin", (u8*)app_data->m_palette_pack, app_data->m_palette_pack_size);
                }
                if (app_data->m_script_binary_size > 0 && app_data->m_script_binary_source != EDATA_SOURCE_LITTLE_FS)
                {
                    nlittlefs::file_save("script.bin", (u8*)app_data->m_script_binary, app_data->m_script_binary_size);
                }
                if (app_data->m_house_meta_size > 0 && app_data->m_house_meta_source != EDATA_SOURCE_LITTLE_FS)
                {
                    nlittlefs::file_save("house.bin", (u8*)app_data->m_house_meta, app_data->m_house_meta_size);
                }
            }

            // Next state 
            to_state_next(state_data);

            nlog::log_info("VERIFY", "Assets are Ok");
        }
    }

}  // namespace ncore