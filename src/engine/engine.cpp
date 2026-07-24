#include "engine.hpp"
#include "../utils/mem/mem.hpp"

#include <cstddef>

static_assert(offsetof(engine::xasset_entry, data) == 8, "xasset_entry.data");
static_assert(offsetof(engine::xasset_entry, dead) == 19, "xasset_entry.dead");
static_assert(sizeof(engine::xasset_entry) == 32, "xasset_entry size");

static_assert(offsetof(engine::gfx_image, dimension) == 162, "gfx_image.dimension");
static_assert(offsetof(engine::gfx_image, srv) == 168, "gfx_image.srv");
static_assert(offsetof(engine::gfx_image, width) == 196, "gfx_image.width");
static_assert(offsetof(engine::gfx_image, height) == 198, "gfx_image.height");
static_assert(offsetof(engine::gfx_image, name) == 248, "gfx_image.name");
static_assert(sizeof(engine::gfx_image) == 264, "gfx_image size");

static_assert(offsetof(engine::msg_s, data) == 8, "msg_s.data");
static_assert(offsetof(engine::msg_s, cur_size) == 28, "msg_s.cur_size");
static_assert(offsetof(engine::msg_s, read_count) == 36, "msg_s.read_count");

static_assert(offsetof(engine::info_lobby_s, host_xuid) == 8, "info_lobby_s.host_xuid");
static_assert(offsetof(engine::info_lobby_s, host_name) == 16, "info_lobby_s.host_name");
static_assert(offsetof(engine::info_lobby_s, sec_id) == 48, "info_lobby_s.sec_id");
static_assert(offsetof(engine::info_lobby_s, sec_key) == 56, "info_lobby_s.sec_key");
static_assert(offsetof(engine::info_lobby_s, addr_buffer) == 73, "info_lobby_s.addr_buffer");
static_assert(offsetof(engine::info_lobby_s, network_mode) == 112, "info_lobby_s.network_mode");
static_assert(offsetof(engine::info_lobby_s, ugc_name) == 120, "info_lobby_s.ugc_name");
static_assert(offsetof(engine::info_lobby_s, ugc_version) == 152, "info_lobby_s.ugc_version");
static_assert(sizeof(engine::info_lobby_s) == 160, "info_lobby_s size");

static_assert(offsetof(engine::info_response_s, nat_type) == 8, "info_response_s.nat_type");
static_assert(offsetof(engine::info_response_s, lobby) == 16, "info_response_s.lobby");
static_assert(sizeof(engine::info_response_s) == 336, "info_response_s size");

static_assert(offsetof(engine::xnaddr_s, public_addr) == 0x1E, "xnaddr_s.public_addr");
static_assert(sizeof(engine::xnaddr_s) == 37, "xnaddr_s size");

static_assert(sizeof(engine::inventory_item_s) == 20, "inventory_item_s size");
static_assert(offsetof(engine::player_inventory_data_s, count) == 80000, "player_inventory_data_s.count");
static_assert(offsetof(engine::player_inventory_data_s, state) == 80008, "player_inventory_data_s.state");

static_assert(offsetof(engine::lobby_msg_s, type) == 56, "lobby_msg_s.type");
static_assert(offsetof(engine::lobby_msg_s, mode) == 64, "lobby_msg_s.mode");
static_assert(sizeof(engine::lobby_msg_s) == 128, "lobby_msg_s size");

static_assert(offsetof(engine::paragon_icon_entry, name) == 16, "paragon_icon_entry.name");
static_assert(sizeof(engine::paragon_icon_entry) == 56, "paragon_icon_entry size");
static_assert(sizeof(engine::paragon_icon_mode) == 3592, "paragon_icon_mode size");
static_assert(sizeof(engine::paragon_icon_table_s) == 7184, "paragon_icon_table_s size");

namespace engine
{
    typedef xasset_entry* (__fastcall* find_entry_t)(uint32_t, const char*, char);

    uintptr_t base()
    {
        return utils::mem::get_process_base();
    }

    ID3D11Device* d3d_device()
    {
        uintptr_t module_base = base();

        if (module_base == 0)
        {
            return nullptr;
        }

        return *reinterpret_cast<ID3D11Device**>(module_base + d3d11_device);
    }

    bool paragon_icon_in_table(const void* entry)
    {
        uintptr_t module_base = base();

        if (module_base == 0 || entry == nullptr)
        {
            return false;
        }

        uintptr_t table = module_base + paragon_icon_table;

        uintptr_t value = reinterpret_cast<uintptr_t>(entry);

        return value >= table && value <= table + sizeof(paragon_icon_table_s) - sizeof(paragon_icon_entry);
    }

    xasset_entry* find_entry(asset_type type, const char* name)
    {
        uintptr_t module_base = base();

        if (module_base == 0 || name == nullptr || name[0] == 0)
        {
            return nullptr;
        }

        find_entry_t find = reinterpret_cast<find_entry_t>(module_base + db_find_asset_entry);

        return find(static_cast<uint32_t>(type), name, 0);
    }

    gfx_image* find_image(const char* name)
    {
        xasset_entry* entry = find_entry(asset_type::image, name);

        if (entry == nullptr)
        {
            return nullptr;
        }

        return static_cast<gfx_image*>(entry->data);
    }

    typedef int64_t(__fastcall* live_get_xuid_t)(uint32_t controller_index);

    typedef bool(__fastcall* live_is_friend_t)(uint32_t controller_index, int64_t xuid);

    int64_t user_xuid(uint32_t controller_index)
    {
        uintptr_t module_base = base();

        if (module_base == 0)
        {
            return 0;
        }

        return reinterpret_cast<live_get_xuid_t>(module_base + live_get_xuid)(controller_index);
    }

    bool is_friend(uint32_t controller_index, int64_t xuid)
    {
        uintptr_t module_base = base();

        if (module_base == 0)
        {
            return false;
        }

        return reinterpret_cast<live_is_friend_t>(module_base + live_is_friend)(controller_index, xuid);
    }

    typedef void(__fastcall* msg_begin_reading_t)(msg_s* msg);

    typedef int(__fastcall* msg_read_long_t)(msg_s* msg);

    typedef char*(__fastcall* msg_read_string_line_t)(msg_s* msg, char* buffer, int size);

    void msg_begin_reading(msg_s* msg)
    {
        uintptr_t module_base = base();

        if (module_base == 0)
        {
            return;
        }

        reinterpret_cast<msg_begin_reading_t>(module_base + net_msg_begin)(msg);
    }

    int msg_read_long(msg_s* msg)
    {
        uintptr_t module_base = base();

        if (module_base == 0)
        {
            return 0;
        }

        return reinterpret_cast<msg_read_long_t>(module_base + net_msg_read_long)(msg);
    }

    char* msg_read_string_line(msg_s* msg, char* buffer, int size)
    {
        uintptr_t module_base = base();

        if (module_base == 0)
        {
            return nullptr;
        }

        return reinterpret_cast<msg_read_string_line_t>(module_base + net_msg_read_string_line)(msg, buffer, size);
    }

    typedef void* (__fastcall* lobby_getter_t)(uint32_t index);

    typedef int(__fastcall* int_getter_t)();

    typedef void(__fastcall* string_copy_t)(char* dest, size_t size, const char* src);

    typedef void(__fastcall* ugc_fill_t)(void* dest, int mode);

    typedef char(__fastcall* send_info_response_t)(uint32_t controller_index, uint64_t* recipients, int count, void* response);

    void* lobby_game(uint32_t index)
    {
        return reinterpret_cast<lobby_getter_t>(base() + dw_lobby_game)(index);
    }

    void* lobby_party(uint32_t index)
    {
        return reinterpret_cast<lobby_getter_t>(base() + dw_lobby_party)(index);
    }

    uint64_t lobby_host_xuid(void* lobby)
    {
        return *reinterpret_cast<uint64_t*>(static_cast<uint8_t*>(lobby) + 96);
    }

    const netadr_s* lobby_host_netadr(void* lobby)
    {
        return reinterpret_cast<const netadr_s*>(static_cast<uint8_t*>(lobby) + 136);
    }

    typedef player_inventory_data_s* (__fastcall* get_player_inventory_t)(uint32_t controller);

    player_inventory_data_s* player_inventory(uint32_t controller)
    {
        uintptr_t module_base = base();

        if (module_base == 0)
        {
            return nullptr;
        }

        return reinterpret_cast<get_player_inventory_t>(module_base + inv_get_player_inventory)(controller);
    }

    int ui_screen()
    {
        return reinterpret_cast<int_getter_t>(base() + dw_ui_screen)();
    }

    int nat_type()
    {
        return reinterpret_cast<int_getter_t>(base() + dw_nat_type)();
    }

    int main_mode()
    {
        return reinterpret_cast<int_getter_t>(base() + dw_main_mode)();
    }

    int network_mode()
    {
        return reinterpret_cast<int_getter_t>(base() + dw_network_mode)();
    }

    void string_copy(char* dest, size_t size, const char* src)
    {
        reinterpret_cast<string_copy_t>(base() + dw_string_copy)(dest, size, src);
    }

    void ugc_fill(void* dest, int mode)
    {
        reinterpret_cast<ugc_fill_t>(base() + dw_ugc_fill)(dest, mode);
    }

    void send_info_response(uint32_t controller_index, uint64_t* recipients, int count, void* response)
    {
        reinterpret_cast<send_info_response_t>(base() + dw_send_info_response)(controller_index, recipients, count, response);
    }

    typedef const char*(__fastcall* store_config_string_t)(int index, const char* value);

    const char* store_config_string(int index, const char* value)
    {
        return reinterpret_cast<store_config_string_t>(base() + cl_store_config_string)(index, value);
    }

    typedef void(__fastcall* cl_disconnect_cmd_t)();

    typedef char(__fastcall* cinematic_stop_t)(int id, char stop_all);

    void cl_disconnect()
    {
        uintptr_t module_base = base();

        if (module_base == 0)
        {
            return;
        }

        reinterpret_cast<cl_disconnect_cmd_t>(module_base + cl_disconnect_cmd)();
    }

    void cinematic_stop_all()
    {
        uintptr_t module_base = base();

        if (module_base == 0)
        {
            return;
        }

        reinterpret_cast<cinematic_stop_t>(module_base + cinematic_stop)(0, 1);
    }

    bool cinematic_playing()
    {
        uintptr_t module_base = base();

        if (module_base == 0)
        {
            return false;
        }

        return *reinterpret_cast<int*>(module_base + cinematic_video_count) > 0;
    }

    bool is_connected()
    {
        uintptr_t module_base = base();

        if (module_base == 0)
        {
            return false;
        }

        return (*reinterpret_cast<int*>(module_base + client_conn_flags) & 2) != 0;
    }

    void** steam_readp2p_slot()
    {
        constexpr size_t read_p2p_vtable_offset = 0x10;

        uintptr_t module_base = base();

        if (module_base == 0)
        {
            return nullptr;
        }

        uintptr_t interface_ptr = *reinterpret_cast<uintptr_t*>(module_base + steam_networking);

        if (interface_ptr == 0)
        {
            return nullptr;
        }

        uintptr_t vtable = *reinterpret_cast<uintptr_t*>(interface_ptr);

        if (vtable == 0)
        {
            return nullptr;
        }

        return reinterpret_cast<void**>(vtable + read_p2p_vtable_offset);
    }
}
