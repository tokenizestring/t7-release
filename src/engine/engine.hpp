#pragma once

#include "../stdafx.hpp"

#include <cstddef>
#include <d3d11.h>
#include <mutex>
#include <unordered_map>

namespace engine
{
    enum offset : uintptr_t
    {
        crc_ready_flag = 0x1686E948,

        d3d11_device = 0xF437780,

        db_find_asset_entry = 0x1420E20,

        dw_instant_dispatch = 0x143A620,

        live_get_xuid = 0x1EBAF40,

        markup_parse_a = 0x1CA4B60,

        markup_parse_b = 0x1F21D60,

        markup_interp = 0x1F27400,

        inv_get_item_quantity = 0x1DFCC60,

        inv_are_extra_slots_purchased = 0x1DFC580,

        inv_is_valid = 0x1DFDFE0,

        bg_unlockables_is_item_purchased = 0x25EE930,

        inv_get_player_inventory = 0x1DFCDD0,

        live_is_friend = 0x1DECFD0,

        steam_networking = 0x10B3DC50,

        cl_connectionless_cmd = 0x134CD70,

        sv_connectionless_cmd = 0x21F7990,

        net_msg_begin = 0x20FC900,

        net_msg_read_long = 0x20FE510,

        net_msg_read_string_line = 0x20FED40,

        dw_build_info_response = 0x1ED62E0,

        dw_send_info_response = 0x1ED3D20,

        dw_lobby_game = 0x1ED0AA0,

        dw_lobby_party = 0x1EBFBC0,

        dw_ui_screen = 0x1EF6920,

        dw_nat_type = 0x1E504B0,

        dw_main_mode = 0x1EDBEA0,

        dw_network_mode = 0x1EDBEB0,

        dw_string_copy = 0x227CA00,

        dw_ugc_fill = 0x20C9E20,

        cl_get_config_string = 0x1321130,

        cl_store_config_string = 0x13667E0,

        cl_parse_gamestate = 0x1364F80,

        cl_get_local_client_globals = 0x71BD0,

        sv_write_reliable_commands = 0x21FF7C0,


        sv_packet_event = 0x21F82E0,

        sv_client_num_for_xuid = 0x21EE3A0,

        net_compare_base_adr = 0x211A550,

        svs_clients = 0x1767A398,

        vote_string_update = 0xF9AA20,

        live_presence_serialize = 0x1E85450,

        live_presence_pack = 0x1E87910,

        lobby_handle_packet_internal = 0x1EEBF20,

        lobby_print_debug = 0x1EEA680,

        lobby_print_message = 0x1EEA940,

        lobby_prep_read = 0x1EEA520,

        lobby_type_name_fn = 0x1EDFA60,

        lobby_join_state = 0x156CB6D0,

        net_send = 0x22B9900,

        steam_gs_pump = 0x1EA92E0,

        lobby_pkg_int = 0x1EEA3E0,

        lobby_pkg_xuid = 0x1EEA4D0,

        lobby_pkg_array_start = 0x1EEA260,

        lobby_pkg_element = 0x1EEA320,

        lobby_pkg_short = 0x1EEA400,

        lobby_pkg_uint = 0x1EEA490,

        lobby_pkg_uint64 = 0x1EEA470,

        lobby_pkg_uchar = 0x1EEA450,

        lobby_pkg_bool = 0x1EEA2E0,

        lobby_pkg_string = 0x1EEA420,

        lobby_pkg_ushort = 0x1EEA4B0,

        lobby_pkg_data = 0x1EEA3B0,

        lobby_apply_game_state = 0x1EC7110,

        cl_get_server_command = 0x131FC70,

        cl_set_config_string = 0x1320180,

        open_menu = 0x2233950,

        menu_state = 0x2231280,

        menu_current_field = 0x184C,

        bg_cache_checksum = 0xA9CB0,

        bg_cache_state = 0x3EC4DF0,

        configstring_pool = 0x56AA998,

        configstring_offsets = 0x56A3828,

        configstring_pool_size = 0x56BA998,

        bcs_buffer = 0x52C5430,

        cg_server_command = 0xF755E0,

        cg_message_format = 0x814150,

        cmd_get_argv = 0xACA00,

        cmd_string_to_int = 0x227C850,

        cmd_string_atoi = 0x2BC2F5C,

        client_connection_ptr = 0x5359BB8,

        menu_blocked_jnz = 0x2233BF9,

        client_state_ptr = 0x1976CA80,

        client_active_flag = 0x1976CA88,

        client_data_base = 0x4C98C80,

        paragon_icon_name = 0x13CAD40,

        paragon_icon_table = 0x5776268,

        steam_p2p_send = 0x1EA7FB0,

        steam_p2p_recv_dispatch = 0x1EA83B0,

        steam_p2p_accept = 0x1EA7F00,

        net_get_packet = 0x211F6A0,

        steam_is_dlc_installed = 0x1EA4A90,

        steam_is_app_installed = 0x1EA44A0,

        steam_dlc_progress = 0x20FBFF0,

        steam_friend_list_rebuild = 0x1DEE400,

        steam_rich_presence_friendadd = 0x1EA4AF0,

        steam_rich_presence_steamid = 0x1EA4BB0,

        cl_disconnect_cmd = 0x134CCD0,

        cinematic_play = 0x12BE3C0,

        cinematic_open = 0x12BE640,

        cinematic_stop = 0x12BEA90,

        cinematic_video_count = 0x4CCE7B0,

        client_conn_flags = 0x5359BC0,

        steam_frame_callback = 0x1EA4070,

        frame_fps_cap = 0xF7CFD0,

        lua_get_vm = 0x1F05920,

        lua_run = 0x1EF8F50,

        lua_load_file = 0x1D46F00,

        lua_loadbuffer = 0x1D3F9B0,

        netchan_reassemble = 0x211C570,

        netchan_msg_table = 0x16DEAEB0,
    };

    enum class asset_type : uint32_t
    {
        material = 6,

        image = 9,
    };

    enum message_type : uint32_t
    {
        message_type_none = 0xFFFFFFFF,

        message_type_info_request = 0x00,

        message_type_info_response = 0x01,

        message_type_lobby_state_private = 0x02,

        message_type_lobby_state_game = 0x03,

        message_type_lobby_state_gamepublic = 0x04,

        message_type_lobby_state_gamecustom = 0x05,

        message_type_lobby_state_gametheater = 0x06,

        message_type_lobby_host_heartbeat = 0x07,

        message_type_lobby_host_disconnect = 0x08,

        message_type_lobby_host_disconnect_client = 0x09,

        message_type_lobby_host_leave_with_party = 0x0A,

        message_type_lobby_client_heartbeat = 0x0B,

        message_type_lobby_client_disconnect = 0x0C,

        message_type_lobby_client_reliable_data = 0x0D,

        message_type_lobby_client_content = 0x0E,

        message_type_lobby_modified_stats = 0x0F,

        message_type_join_lobby = 0x10,

        message_type_join_response = 0x11,

        message_type_join_agreement_request = 0x12,

        message_type_join_agreement_response = 0x13,

        message_type_join_complete = 0x14,

        message_type_join_member_info = 0x15,

        message_type_serverlist_info = 0x16,

        message_type_peer_to_peer_connectivity_test = 0x17,

        message_type_peer_to_peer_info = 0x18,

        message_type_lobby_migrate_test = 0x19,

        message_type_lobby_migrate_announce_host = 0x1A,

        message_type_lobby_migrate_start = 0x1B,

        message_type_ingame_migrate_to = 0x1C,

        message_type_ingame_migrate_new_host = 0x1D,

        message_type_voice_packet = 0x1E,

        message_type_voice_relay_packet = 0x1F,

        message_type_demo_state = 0x20,

        message_type_count = 0x21,
    };

#pragma pack(push, 1)

    struct client_data_s
    {
        uint8_t pad_0000[0x18];

        uint32_t match_flags;

        uint8_t pad_001C[0x342720 - 0x1C];
    };

    struct lobby_state_msg_s
    {
        uint8_t pad_0000[0x53A0];

        char ugc_name[32];

        uint32_t ugc_version;
    };

    struct client_connection_s
    {
        uint8_t pad_0000[0x4544];

        int32_t server_command_sequence;

        uint32_t pad_4548;

        char reliable_commands[128][1024];

        uint8_t pad_2454C[0x1234];
    };

    struct xasset_entry
    {
        uint32_t type;

        uint8_t pad_04[4];

        void* data;

        uint8_t pad_10[3];

        uint8_t dead;

        uint32_t next;

        uint8_t pad_18[8];
    };

    struct gfx_image
    {
        uint8_t pad_000[162];

        uint8_t dimension;

        uint8_t pad_0A3[5];

        ID3D11ShaderResourceView* srv;

        uint8_t pad_0B0[20];

        uint16_t width;

        uint16_t height;

        uint8_t pad_0C8[48];

        const char* name;

        uint8_t pad_100[8];
    };

    struct xnaddr_s
    {
        uint8_t local_addr[4];

        uint8_t pad_04[0x1A];

        uint8_t public_addr[4];

        uint16_t port;

        uint8_t pad_23[1];
    };

    struct info_lobby_s
    {
        uint8_t valid;

        uint8_t pad_01[7];

        uint64_t host_xuid;

        char host_name[32];

        uint8_t sec_id[8];

        uint8_t sec_key[16];

        uint8_t pad_48[1];

        uint8_t addr_buffer[37];

        uint8_t pad_6E[2];

        int32_t network_mode;

        int32_t main_mode;

        char ugc_name[32];

        uint64_t ugc_version;
    };

    struct info_response_s
    {
        int32_t nonce;

        int32_t ui_screen;

        uint8_t nat_type;

        uint8_t pad_09[7];

        info_lobby_s lobby[2];
    };

    struct msg_s
    {
        int32_t overflowed;

        int32_t read_only;

        uint8_t* data;

        uint8_t* split_data;

        int32_t max_size;

        int32_t cur_size;

        int32_t split_size;

        int32_t read_count;
    };

#pragma pack(pop)

    struct presence_party_member_s
    {
        char gamertag[17];
    };

    struct presence_party_s
    {
        int32_t max;

        int32_t total_count;

        int32_t available_count;

        presence_party_member_s members[18];
    };

    struct presence_title_s
    {
        int32_t activity;

        int32_t ctx;

        int32_t joinable;

        int32_t game_type_id;

        int32_t map_id;

        int32_t difficulty;

        int32_t playlist;

        int32_t startup_timestamp;

        presence_party_s party;
    };

    struct presence_platform_s
    {
        int32_t primary_presence;

        char title_id[32];

        char title_name[64];

        char title_status[64];
    };

    struct presence_data_s
    {
        int32_t version;

        int32_t flags;

        uint64_t xuid;

        bool is_dirty;

        bool is_initialized;

        int32_t id;

        const char* base_string;

        const char* params;

        const char* data;

        int32_t failure_count;

        int32_t last_update_time;

        int32_t state;

        presence_title_s title;

        presence_platform_s platform;
    };

    struct lobby_msg_s
    {
        uint8_t msg[56];

        message_type type;

        uint8_t element_flag;

        uint8_t pad_3D[3];

        int32_t mode;

        uint8_t pad_44[60];
    };

    enum netadr_type : int32_t
    {
        netadr_type_raw_udp = 3,

        netadr_type_steam = 4,
    };

    struct netadr_s
    {
        uint8_t ip[4];

        uint16_t port;

        uint8_t pad_06[2];

        int32_t type;

        int32_t extra;
    };

    uint64_t lobby_host_xuid(void* lobby);

    const netadr_s* lobby_host_netadr(void* lobby);

    struct inventory_item_s
    {
        int32_t item_id;

        int32_t quantity;

        int32_t pad_08;

        int32_t pad_0c;

        uint8_t pad_10[4];
    };

    struct player_inventory_data_s
    {
        inventory_item_s items[4000];

        int32_t count;

        int32_t pad_80004;

        int32_t state;
    };

    player_inventory_data_s* player_inventory(uint32_t controller);

    struct paragon_icon_entry
    {
        uint8_t pad_00[16];

        char* name;

        uint8_t pad_18[32];
    };

    struct paragon_icon_mode
    {
        paragon_icon_entry entries[64];

        uint8_t pad_e00[8];
    };

    struct paragon_icon_table_s
    {
        paragon_icon_mode modes[2];
    };

    bool paragon_icon_in_table(const void* entry);

    uintptr_t base();

    ID3D11Device* d3d_device();

    xasset_entry* find_entry(asset_type type, const char* name);

    gfx_image* find_image(const char* name);

    int64_t user_xuid(uint32_t controller_index);

    bool is_friend(uint32_t controller_index, int64_t xuid);

    void msg_begin_reading(msg_s* msg);

    int msg_read_long(msg_s* msg);

    char* msg_read_string_line(msg_s* msg, char* buffer, int size);

    void* lobby_game(uint32_t index);

    void* lobby_party(uint32_t index);

    int ui_screen();

    bool at_menu();

    bool start_cutscene();

    int32_t current_menu_id();

    typedef int64_t(__fastcall* open_menu_t)(uint32_t controller, int menu_id);

    inline open_menu_t open_menu_fn = nullptr;

    int nat_type();

    int main_mode();

    int network_mode();

    void string_copy(char* dest, size_t size, const char* src);

    void ugc_fill(void* dest, int mode);

    void send_info_response(uint32_t controller_index, uint64_t* recipients, int count, void* response);

    const char* store_config_string(int index, const char* value);

    void cl_disconnect();

    void cinematic_stop_all();

    bool cinematic_playing();

    bool is_connected();

    typedef char(__fastcall* instant_dispatch_demonware_t)(int64_t sender_id, uint32_t controller_index, const char* message, uint32_t message_size);

    typedef bool(__fastcall* instant_dispatch_steam_t)(void* self, void* dest, uint32_t dest_size, uint32_t* message_size, uint64_t* sender_xuid, int channel);

    typedef char(__fastcall* cl_connectionless_t)(int local_client_num, void* from, void* msg);

    typedef int64_t(__fastcall* sv_connectionless_t)(void* from, void* msg);

    inline instant_dispatch_demonware_t instant_dispatch_demonware = nullptr;

    inline instant_dispatch_steam_t instant_dispatch_steam = nullptr;

    typedef char(__fastcall* build_info_response_t)(uint32_t controller_index, uint64_t recipient, int nonce);

    inline cl_connectionless_t cl_connectionless = nullptr;

    inline sv_connectionless_t sv_connectionless = nullptr;

    typedef const char*(__fastcall* get_config_string_t)(int index);

    typedef void(__fastcall* vote_update_t)(int client, const char* vote_string);

    typedef __int64(__fastcall* live_presence_serialize_t)(void* presence, void* buffer);

    typedef void(__fastcall* live_presence_pack_t)(presence_data_s* presence, void* buffer, size_t buffer_size);

    typedef int64_t(__fastcall* handle_packet_internal_t)(uint32_t controller_index, void* adr, uint64_t xuid, int64_t lobby_type, int role, void* msg);

    typedef int64_t(__fastcall* lobby_print_debug_t)(int64_t msg);

    inline lobby_print_debug_t lobby_print_debug_fn = nullptr;

    typedef char(__fastcall* lobby_print_message_t)(int64_t msg, char force);

    inline lobby_print_message_t lobby_print_message_fn = nullptr;

    typedef bool(__fastcall* lobby_prep_read_msg_t)(lobby_msg_s* lobby_msg, void* msg);

    typedef const char*(__fastcall* lobby_type_name_t)(uint32_t type);

    typedef bool(__fastcall* lobby_package_int_t)(lobby_msg_s* lobby_msg, const char* key, int32_t* value);

    typedef bool(__fastcall* lobby_package_xuid_t)(lobby_msg_s* lobby_msg, const char* key, uint64_t* value);

    typedef bool(__fastcall* lobby_package_array_start_t)(lobby_msg_s* lobby_msg, const char* key);

    typedef char(__fastcall* lobby_package_element_t)(lobby_msg_s* lobby_msg, char add_element);

    typedef bool(__fastcall* lobby_package_short_t)(lobby_msg_s* lobby_msg, const char* key, int16_t* value);

    typedef bool(__fastcall* lobby_package_uint_t)(lobby_msg_s* lobby_msg, const char* key, uint32_t* value);

    typedef bool(__fastcall* lobby_package_uint64_t)(lobby_msg_s* lobby_msg, const char* key, uint64_t* value);

    typedef bool(__fastcall* lobby_package_uchar_t)(lobby_msg_s* lobby_msg, const char* key, uint8_t* value);

    typedef bool(__fastcall* lobby_package_bool_t)(lobby_msg_s* lobby_msg, const char* key, bool* value);

    typedef bool(__fastcall* lobby_package_string_t)(lobby_msg_s* lobby_msg, const char* key, char* value, int32_t size);

    typedef bool(__fastcall* lobby_package_ushort_t)(lobby_msg_s* lobby_msg, const char* key, uint16_t* value);

    typedef bool(__fastcall* lobby_package_data_t)(lobby_msg_s* lobby_msg, const char* key, void* value, int32_t size, uint64_t max_size);

    inline build_info_response_t build_info_response = nullptr;

    inline get_config_string_t get_config_string = nullptr;

    inline vote_update_t vote_update = nullptr;

    inline live_presence_serialize_t presence_serialize = nullptr;

    inline live_presence_pack_t presence_pack = nullptr;

    inline handle_packet_internal_t handle_packet_internal = nullptr;

    inline lobby_prep_read_msg_t lobby_prep_read_msg = nullptr;

    inline lobby_type_name_t lobby_type_name = nullptr;

    typedef bool(__fastcall* net_send_packet_t)(uint32_t sock, uint32_t length, const void* data, const void* to);

    inline net_send_packet_t net_send_packet = nullptr;

    typedef void*(__fastcall* steam_gs_pump_t)(void* server);

    inline steam_gs_pump_t steam_gs_pump_fn = nullptr;

    typedef int64_t(__fastcall* inv_get_item_quantity_t)(uint32_t controller, int item_id);

    inline inv_get_item_quantity_t inv_get_item_quantity_fn = nullptr;

    typedef bool(__fastcall* inv_are_extra_slots_purchased_t)(uint32_t controller);

    inline inv_are_extra_slots_purchased_t inv_are_extra_slots_purchased_fn = nullptr;

    typedef bool(__fastcall* bg_is_item_purchased_t)(uint32_t controller, int16_t unlockable, char ignore_bundle);

    inline bg_is_item_purchased_t bg_is_item_purchased_fn = nullptr;

    typedef char(__fastcall* markup_parse_a_t)(char** cursor, int64_t a2, void* a3, void* a4, int* a5, int* a6);

    inline markup_parse_a_t markup_parse_a_fn = nullptr;

    typedef char(__fastcall* markup_parse_b_t)(uint32_t a1, int64_t cursor, void* a3, int64_t a4, int64_t a5, void* a6, int64_t* a7, int* a8);

    inline markup_parse_b_t markup_parse_b_fn = nullptr;

    typedef char(__fastcall* markup_interp_t)(uint32_t a1, int64_t a2, void* a3, int64_t out, uint32_t out_size);

    inline markup_interp_t markup_interp_fn = nullptr;

    typedef int64_t(__fastcall* cl_parse_gamestate_t)(uint32_t local_client, void* msg, double a3, double a4);

    inline cl_parse_gamestate_t cl_parse_gamestate_fn = nullptr;

    typedef const char*(__fastcall* cl_get_config_string_t)(int index);

    typedef void*(__fastcall* cl_get_local_client_globals_t)(uint32_t local_client);

    typedef int64_t(__fastcall* sv_write_reliable_commands_t)(int32_t* client, void* msg);

    inline sv_write_reliable_commands_t sv_write_reliable_commands_fn = nullptr;

    typedef void(__fastcall* sv_packet_event_t)(void* from, uint64_t xuid, void* msg);

    inline sv_packet_event_t sv_packet_event_fn = nullptr;

    typedef int(__fastcall* sv_client_num_for_xuid_t)(uint64_t xuid);

    typedef char(__fastcall* net_compare_base_adr_t)(void* a, void* b);

    inline lobby_package_int_t lobby_package_int = nullptr;

    inline lobby_package_xuid_t lobby_package_xuid = nullptr;

    inline lobby_package_array_start_t lobby_package_array_start = nullptr;

    inline lobby_package_element_t lobby_package_element = nullptr;

    inline lobby_package_short_t lobby_package_short = nullptr;

    inline lobby_package_uint_t lobby_package_uint = nullptr;

    inline lobby_package_uint64_t lobby_package_uint64 = nullptr;

    inline lobby_package_uchar_t lobby_package_uchar = nullptr;

    inline lobby_package_bool_t lobby_package_bool = nullptr;

    inline lobby_package_string_t lobby_package_string = nullptr;

    inline lobby_package_ushort_t lobby_package_ushort = nullptr;

    inline lobby_package_data_t lobby_package_data = nullptr;

    void** steam_readp2p_slot();

    typedef char*(__fastcall* paragon_icon_name_t)(uint32_t mode, int icon_id);

    inline paragon_icon_name_t paragon_icon_name_fn = nullptr;

    typedef char(__fastcall* steam_p2p_send_t)(void* ctx, uint64_t steam_id, void* data, uint32_t size);

    inline steam_p2p_send_t steam_p2p_send_fn = nullptr;

    typedef int64_t(__fastcall* steam_p2p_dispatch_t)(uint64_t sender, uint32_t type, void* data, int size);

    inline steam_p2p_dispatch_t steam_p2p_dispatch_fn = nullptr;

    typedef char(__fastcall* steam_p2p_accept_t)(void* ctx, uint64_t* steam_id);

    inline steam_p2p_accept_t steam_p2p_accept_fn = nullptr;

    typedef char(__fastcall* net_get_packet_t)(void* sock, void* a2, void* a3, netadr_s* from, int max_size, uint32_t* out_size, void* out_buffer);

    inline net_get_packet_t net_get_packet_fn = nullptr;

    typedef bool(__fastcall* steam_is_installed_t)(uint32_t app_id);

    inline steam_is_installed_t steam_is_dlc_installed_fn = nullptr;

    inline steam_is_installed_t steam_is_app_installed_fn = nullptr;

    typedef char(__fastcall* steam_friend_list_rebuild_t)(uint32_t controller);

    inline steam_friend_list_rebuild_t steam_friend_list_rebuild_fn = nullptr;

    static constexpr uint32_t dlc_progress_slots = 5;

    static constexpr uint64_t dlc_progress_interval_ms = 2000;

    static constexpr uint64_t friend_rebuild_interval_ms = 20000;

    struct steam_install_cache
    {
        std::mutex mutex;

        std::unordered_map<uint32_t, bool> entries;

        bool query(steam_is_installed_t original, uint32_t app_id)
        {
            std::lock_guard<std::mutex> lock(mutex);

            auto entry = entries.find(app_id);

            if (entry != entries.end())
            {
                return entry->second;
            }

            bool result = original(app_id);

            entries[app_id] = result;

            return result;
        }
    };

    struct frame_throttle
    {
        uint64_t interval_ms = 0;

        uint64_t last = 0;

        bool ready()
        {
            uint64_t now = GetTickCount64();

            if (last != 0 && now - last < interval_ms)
            {
                return false;
            }

            last = now;

            return true;
        }
    };

    struct dlc_progress_entry
    {
        double value = 0.0;

        uint64_t next = 0;
    };

    inline steam_install_cache dlc_cache;

    inline steam_install_cache app_cache;

    inline frame_throttle friend_rebuild_throttle{ friend_rebuild_interval_ms };

    inline std::mutex dlc_progress_mutex;

    inline dlc_progress_entry dlc_progress[dlc_progress_slots];

    typedef double(__fastcall* steam_dlc_progress_t)(uint32_t index);

    inline steam_dlc_progress_t steam_dlc_progress_fn = nullptr;

    typedef void(__fastcall* steam_rich_presence_t)(uint64_t steam_id);

    inline steam_rich_presence_t steam_rich_presence_friendadd_fn = nullptr;

    inline steam_rich_presence_t steam_rich_presence_steamid_fn = nullptr;

    typedef void*(__fastcall* steam_frame_callback_t)();

    inline steam_frame_callback_t steam_frame_callback_fn = nullptr;

    typedef int64_t(__fastcall* apply_game_state_t)(uint32_t a1, int64_t a2, int64_t a3, int64_t message);

    inline apply_game_state_t apply_game_state_fn = nullptr;

    typedef int64_t(__fastcall* get_server_command_t)(uint32_t local_client, int32_t sequence);

    inline get_server_command_t get_server_command_fn = nullptr;

    typedef int64_t(__fastcall* cg_server_command_t)(uint32_t local_client);

    inline cg_server_command_t cg_server_command_fn = nullptr;

    typedef int64_t(__fastcall* set_config_string_t)(uint32_t local_client);

    inline set_config_string_t set_config_string_fn = nullptr;

    typedef char(__fastcall* bg_cache_checksum_t)(uint32_t controller, int64_t data);

    inline bg_cache_checksum_t bg_cache_checksum_fn = nullptr;

    typedef int64_t(__fastcall* play_video_t)(char* path, int64_t context, uint32_t flags, float speed, int64_t* callback, uint32_t video_id);

    inline play_video_t play_video_fn = nullptr;

    typedef int64_t(__fastcall* cinematic_open_t)(int64_t name, int64_t subtitle, uint32_t flags, float speed, void* callback, int video_id);

    inline cinematic_open_t cinematic_open_fn = nullptr;

    typedef int64_t(__fastcall* message_format_t)(uint32_t local_client, const char* message, int64_t context, char* out);

    inline message_format_t message_format_fn = nullptr;

    typedef char(__fastcall* netchan_reassemble_t)(int32_t client, int64_t channel, int64_t message, void* out_msgid, void* out_src, void* out_seq);

    inline netchan_reassemble_t netchan_reassemble_fn = nullptr;

    typedef int64_t(__fastcall* fps_cap_t)(uint32_t local_client);

    inline fps_cap_t fps_cap_fn = nullptr;

    typedef int64_t(__fastcall* lua_get_vm_t)(int index);

    inline lua_get_vm_t lua_get_vm_fn = nullptr;

    typedef int(__fastcall* lua_loadbuffer_t)(int64_t state, int64_t scratch, const char* buffer, uint64_t size, const char* name);

    inline lua_loadbuffer_t lua_loadbuffer_fn = nullptr;

    typedef char(__fastcall* lua_run_t)(int64_t vm, const char* name);

    inline lua_run_t lua_run_fn = nullptr;

    typedef int64_t(__fastcall* lua_load_file_t)(int64_t vm, const char* name);

    inline lua_load_file_t lua_load_file_fn = nullptr;

    static constexpr size_t netchan_msg_client_max = 18;

    static constexpr size_t netchan_msg_channel_max = 18;

    static constexpr size_t netchan_msg_entry_stride = 37;

    static constexpr size_t netchan_msg_complete_flag = 1;

    static constexpr size_t netchan_msg_bucket_next = 64;

    static constexpr size_t netchan_msg_fragment_head = 72;

    static constexpr size_t netchan_fragment_length = 8;

    static constexpr size_t netchan_fragment_index = 12;

    static constexpr size_t netchan_reassemble_stride = 1222;

    static constexpr size_t netchan_msgbuf_error = 0;

    static constexpr size_t netchan_msgbuf_capacity = 24;

    static constexpr size_t netchan_msgbuf_written = 28;

    static constexpr uint32_t serverpos_index_max = 1792;

    static constexpr uint32_t configstring_index_max = 3629;

    static constexpr size_t bcs_reassembly_max = 20480;

    static constexpr uint32_t model_notify_numargs_max = 16;

    static constexpr uint32_t model_notify_window_ms = 250;

    static constexpr uint32_t model_notify_window_max = 40;

    inline uint32_t model_notify_window_start = 0;

    inline uint32_t model_notify_count = 0;

    static constexpr int lobby_print_max_depth = 32;

    inline int lobby_print_depth = 0;

    inline uint64_t lobby_print_blocked = 0;

    static constexpr int lua_ui_vm = 0;

    static constexpr int64_t lua_inner_state_offset = 16;

    static constexpr int64_t lua_load_scratch_offset = 1384;

    static constexpr int max_reliable_commands = 128;

    static constexpr uint64_t loopback_xuid = 0xDEADFA11ull;

    static constexpr size_t client_data_stride = 938352;

    static constexpr size_t client_netadr_offset = 44;

    static constexpr uint32_t max_userdata_size = 1024;

    static constexpr uint32_t live_userdata_type = 21;

    const char* server_command(uint32_t local_client, int32_t sequence);

    int32_t server_command_sequence(uint32_t local_client);

    const char* cmd_argv(int index);

    uint32_t cmd_arg_int(const char* value);

    int32_t cmd_atoi(const char* value);

    bool configstring_pool_overflow(uint32_t index, const char* value);

    size_t bcs_length();
}
