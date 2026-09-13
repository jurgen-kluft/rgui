#include "rcore/c_app.h"
#include "rcore/c_log.h"
#include "rcore/c_little_fs.h"

#include "ccore/c_runes.h"

#include "main/c_app_data.h"

namespace ncore
{
    static void little_fs_handle_file(void* ctx, void* user_ctx)
    {
        app_data_t* app_data = (app_data_t*)user_ctx;

        const char* filename = nlittlefs::scanner_filename(ctx);

        // Implement the file handling logic here
        nlog::log_infof("LITTLE-FS", "File : %s\n", va_list_t(va_t(filename)));

        if (ascii::compare(filename, "sprite_pack.bin") == 0)
        {
            u32 filesize = 0;
            if (nlittlefs::scanner_fileload(ctx, (u8*)app_data->m_sprite_pack, app_data->m_sprite_pack_capacity, filesize))
            {
                app_data->m_sprite_pack_size   = filesize;
                app_data->m_sprite_pack_source = EDATA_SOURCE_LITTLE_FS;
            }
        }
        else if (ascii::compare(filename, "font_pack.bin") == 0)
        {
            u32 filesize = 0;
            if (nlittlefs::scanner_fileload(ctx, (u8*)app_data->m_font_pack, app_data->m_font_pack_capacity, filesize))
            {
                app_data->m_font_pack_size   = filesize;
                app_data->m_font_pack_source = EDATA_SOURCE_LITTLE_FS;
            }
        }
        else if (ascii::compare(filename, "palette_pack.bin") == 0)
        {
            u32 filesize = 0;
            if (nlittlefs::scanner_fileload(ctx, (u8*)app_data->m_palette_pack, app_data->m_palette_pack_capacity, filesize))
            {
                app_data->m_palette_pack_size   = filesize;
                app_data->m_palette_pack_source = EDATA_SOURCE_LITTLE_FS;
            }
        }
        else if (ascii::compare(filename, "script.bin") == 0)
        {
            u32 filesize = 0;
            if (nlittlefs::scanner_fileload(ctx, (u8*)app_data->m_script_binary, app_data->m_script_binary_capacity, filesize))
            {
                app_data->m_script_binary_size   = filesize;
                app_data->m_script_binary_source = EDATA_SOURCE_LITTLE_FS;
            }
        }
    }

    void state_handle_little_fs(fsm_state_data_t* state_data, app_data_t* app_data, u64 now_ms)
    {
        if (state_data->m_current_state == 0)
        {
            nlog::log_info("LITTLE-FS", "Initializing");
            if (!nlittlefs::initialize())
            {
                nlog::log_warn("LITTLE-FS", "Failed to Mount");
                to_state_next(state_data);
                return;
            }

            // Mark the LittleFS as available
            app_data->m_little_fs_available = true;

            // Scan files for the availability of:
            // - sprite_pack.bin
            // - font_pack.bin
            // - palette_pack.bin
            // - script.bin
        }
    }

}  // namespace ncore