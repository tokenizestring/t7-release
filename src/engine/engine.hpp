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

        steam_matchmaking = 0x10B3DC30,

        steam_friends = 0x10B3DC20,

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

        lobby_print_array = 0x1EEA5E0,

        lobby_print_message = 0x1EEA940,

        msg_write_byte = 0x20FF3C0,

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

        lobby_pkg_float = 0x1EEA390,

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

        client_command = 0x193DFC0,

        client_mrp = 0x195EFA0,

        sv_cmd_argv_buffer = 0x20E2FD0,

        pmove = 0x26200C0,

        client_match_bitmask = 0x13E3B10,

        lobby_disconnect_event = 0x1EE3C10,

        localize_lookup = 0x221D280,

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

        lod_params_reader = 0x1CE5230,

        resident_mip_request = 0x1CFA3B0,

        stream_throttle = 0x1D04BA0,

        stream_frame_budget = 0x32FACC8,

        frame_fps_cap = 0xF7CFD0,

        lua_get_vm = 0x1F05920,

        lua_run = 0x1EF8F50,

        lua_load_file = 0x1D46F00,

        lua_loadbuffer = 0x1D3F9B0,

        netchan_reassemble = 0x211C570,

        netchan_msg_table = 0x16DEAEB0,

        clientfield_checksum_validate = 0x843FE0,

        clientfield_local_checksum = 0x56BA9B0,

        disconnect_reason = 0x134C920,

        validate_auth_response = 0x1EAB190,

        lobby_session = 0x155F6520,

        keycatcher = 0x5359BC4,

        cursor_visible_flag = 0x1795D25C,

        clear_held_input = 0x12F34F0,

        current_mode_index = 0x20EAC70,

        is_local_player = 0x1EBAD60,

        voip_status = 0x1EF2590,

        menu_input_mode = 0x1F1FD00,

        key_event = 0x1340DC0,

        mouse_move = 0x12FF8E0,

        net_out_of_band_print = 0x211B310,

        net_out_of_band_data = 0x211B200,

        lobby_prep_write = 0x1EEA560,

        lobby_frame_send = 0x1EEC470,

        lobby_channel_for = 0x1EECE60,

        lobby_session_getter = 0x1EC1650,

        lobby_client_resolve = 0x1EF2240,

        lobby_frame_send_addr = 0x1EEC740,

        notetrack_anim_lookup = 0x22C9650,

        lobby_handle_join_request = 0x1ED2CC0,

        lobby_handle_member_info = 0x1EDB660,

        lobby_msg_join_package = 0x1ED5DC0,

        lobby_send_join_response = 0x1ED41D0,

        lobby_netchan_get_global_channel = 0x1EECE30,

        lobby_host_session_get_lobby_state = 0x1ED0A40,

        lobby_session_get_max_clients = 0x1EF4450,

        lobby_session_get_client_count = 0x1EF4030,

        lobby_host_add_joining_client = 0x1ECAF00,

        lobby_msg_package_glob = 0x1EEA3B0,

        msg_fixed_client_info_package = 0x1ED8E60,

        msg_mutable_client_info_package = 0x1EC8400,

        network_mode_global = 0x156CE31C,

        main_mode_global = 0x156CE320,

        lobby_host_update_matchmaking_session = 0x1ECE840,

        lobby_set_matchmaking_info = 0x1EEE1D0,

        lobby_host_data_get_session = 0x1ED0AA0,

        lobby_qos_listener_enable = 0x1E8BE70,

        live_session_update = 0x1E8B8B0,

        live_session_task_def = 0x2FA2D38,

        live_session_success_callback = 0x1EEDC40,

        lobby_online_state = 0x15B19C30,

        live_matchmaking_info = 0x15B19C38,

        live_storage_get_ffotd_version = 0x1EB7D40,

        playlist_get_version_number = 0x2237670,

        lobby_session_set_max_clients = 0x1EF5B10,

        lobby_host_task_search = 0x1ED80C0,

        live_can_host_server = 0x1DFF3A0,

        qos_location_table = 0x11392050,

        qos_location_count = 0x11391E7C,

        bd_session_id_serialize = 0x28919F0,

        bd_byte_buffer_write_uint32 = 0x2886DC0,

        dedicated_session_flag = 0x2891E3C,

        bd_session_id_serialize_return = 0x2891E57,

        cbuf_add_text = 0x20DFF50,

        lobby_online_create_session = 0x1EEDF30,

        allocate_structs = 0x2BCB1FC,

        matchmaking_info_ctor = 0x144CA20,

        matchmaking_thing_ctor = 0x1438980,

        lobby_search_session = 0x1EF0440,

        demonware_fetching_done = 0x1E01340,

        demonware_signed_in = 0x1EBABF0,

        bd_matchmaking_find_sessions = 0x2891C40,

        send_join_lobby_to_adr = 0x1ED40F0,

        content_get_enabled_content_packs = 0x20F3F60,

        msg_crc_net_field_checksum = 0x2100A50,

        dw_register_sec_id_and_key = 0x143E140,

        dw_common_addr_to_netadr = 0x143C380,

        dw_netadr_to_common_addr = 0x143DA80,

        msg_lobby_state_package = 0x1EC8F20,

        send_join_member_info_to_adr = 0x1ED8680,
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

    void* active_lobby();

    void* lobby_member_xnaddr(void* lobby, uint64_t xuid);

    enum lobby_network_mode_e : int32_t
    {
        lobby_network_mode_local = 0,

        lobby_network_mode_lan = 1,

        lobby_network_mode_live = 2,
    };

    enum lobby_main_mode_e : int32_t
    {
        lobby_main_mode_cp = 0,

        lobby_main_mode_mp = 1,

        lobby_main_mode_zm = 2,
    };

    enum join_result_e : uint32_t
    {
        join_result_success = 1,
    };

    enum lobby_module_e : int32_t
    {
        lobby_module_host = 0,

        lobby_module_client = 1,

        lobby_module_peer_to_peer = 3,
    };

    struct lobby_params_s
    {
        lobby_network_mode_e network_mode;

        lobby_main_mode_e main_mode;
    };

    struct join_member_s
    {
        uint64_t xuid;

        uint64_t lobby_id;

        float skill_rating;

        float skill_variance;

        uint32_t probation_time_remaining[2];
    };

    struct msg_join_lobby_s
    {
        int32_t target_lobby;

        int32_t source_lobby;

        int32_t join_type;

        uint8_t pad_0c[4];

        uint64_t probed_xuid;

        int32_t member_count;

        uint8_t pad_1c[4];

        join_member_s members[18];

        int32_t split_screen_clients;

        int32_t playlist_id;

        int32_t playlist_ver;

        int32_t ffotd_ver;

        int16_t network_mode;

        uint8_t pad_2[2];

        uint32_t net_checksum;

        int32_t protocol;

        int32_t change_list;

        int32_t ping_band;

        uint32_t dlc_bits;

        uint64_t join_nonce;

        bool is_starter_pack;

        char password[32];

        uint8_t chunk[3];

        uint8_t pad_3[5];
    };

    struct msg_join_response_s
    {
        join_result_e response;

        char name[32];

        uint32_t server_location;

        int32_t lobby_type;

        lobby_params_s lobby_params;

        uint64_t reservation_key;
    };

#pragma pack(push, 1)
    struct serialized_adr_mm_s
    {
        uint8_t valid;

        uint8_t xnaddr[37];
    };
#pragma pack(pop)

    struct msg_mutable_client_info_s
    {
        uint8_t data[1040];
    };

    struct msg_fixed_client_info_s
    {
        uint8_t data[208];
    };

    struct msg_join_member_info_s
    {
        msg_mutable_client_info_s mutable_client_info_msg;

        msg_fixed_client_info_s fixed_client_info_msg;

        uint64_t reservation_key;

        int32_t target_lobby;

        serialized_adr_mm_s serialized_adr;
    };

    static_assert(sizeof(msg_mutable_client_info_s) == 1040, "mutable client info must be 1040 bytes");

    static_assert(sizeof(msg_fixed_client_info_s) == 208, "fixed client info must be 208 bytes");

    static_assert(offsetof(msg_join_member_info_s, fixed_client_info_msg) == 1040, "fixed client info offset");

    static_assert(offsetof(msg_join_member_info_s, reservation_key) == 1248, "reservation key offset");

    static_assert(offsetof(msg_join_member_info_s, target_lobby) == 1256, "lobbytype offset");

    static_assert(offsetof(msg_join_member_info_s, serialized_adr) == 1260, "serialized adr offset");

    struct bd_matchmaking_info_s
    {
        uint8_t pad_00[0x20];

        uint64_t bd_session_id;

        uint8_t host_addr[255];

        uint32_t host_addr_size;

        uint32_t game_type;

        uint32_t max_players;

        uint32_t num_players;
    };

    struct matchmaking_info_s : bd_matchmaking_info_s
    {
        uint64_t session_id;

        char key_exchange_key[17];

        uint32_t server_type;

        uint64_t xuid;

        uint32_t server_location;

        uint32_t latency_band;

        int32_t show_in_matchmaking;

        int32_t netcode_version;

        int32_t map_packs;

        int32_t playlist_version;

        int32_t playlist_number;

        int32_t is_empty;

        int32_t team_max;

        float skill;

        int32_t geo1;

        int32_t geo2;

        int32_t geo3;

        int32_t geo4;

        int32_t dirty;

        int32_t active;

        int32_t time_since_last_update;

        int32_t recreate_session;

        int32_t time_since_update;

        uint8_t pad2[0x14];
    };

    struct lobby_online_s
    {
        uint64_t state;

        matchmaking_info_s mm;
    };

    using task_callback_t = char(__fastcall*)(void* record);

    struct task_definition_s
    {
        uint64_t category;

        const char* name;

        int32_t payload_size;

        task_callback_t completed_callback;

        task_callback_t failure_callback;
    };

    inline constexpr uint32_t lobby_type_game = 1;

    inline constexpr int lobby_session_advertised_offset = 0x50;

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

    typedef char(__fastcall* lobby_print_array_t)(int64_t msg, int indent);

    inline lobby_print_array_t lobby_print_array_fn = nullptr;

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

    typedef bool(__fastcall* oob_print_t)(uint32_t sock, const netadr_s* to, const char* data);

    inline oob_print_t oob_print = nullptr;

    typedef bool(__fastcall* oob_data_t)(uint32_t sock, const netadr_s* to, const void* data, int len);

    inline oob_data_t oob_data = nullptr;

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

    typedef bool(__fastcall* lobby_package_float_t)(lobby_msg_s* lobby_msg, const char* key, float* value);

    inline lobby_package_float_t lobby_package_float = nullptr;

    typedef bool(__fastcall* lobby_prep_write_t)(lobby_msg_s* lobby_msg, void* buffer, int64_t size, uint32_t type);

    inline lobby_prep_write_t lobby_prep_write_fn = nullptr;

    typedef uint32_t(__fastcall* lobby_frame_send_t)(uint32_t local_client, void* lobby, int direction, void* lobby_msg, uint32_t type, uint32_t channel);

    inline lobby_frame_send_t lobby_frame_send_fn = nullptr;

    typedef uint32_t(__fastcall* lobby_channel_for_t)(int lobby_id, int sub_channel);

    inline lobby_channel_for_t lobby_channel_for_fn = nullptr;

    typedef void*(__fastcall* lobby_session_getter_t)(uint32_t type);

    typedef void*(__fastcall* lobby_client_resolve_t)(void* handle, int lobby_id);

    typedef bool(__fastcall* lobby_frame_send_addr_t)(uint32_t local_client, uint32_t channel, int direction, void* xnaddr, uint64_t dest_xuid, void* lobby_msg, uint32_t tag);

    inline lobby_frame_send_addr_t lobby_frame_send_addr_fn = nullptr;

    typedef void(__fastcall* msg_write_byte_t)(void* msg, int value);

    inline msg_write_byte_t msg_write_byte_fn = nullptr;

    static constexpr int lobby_direction_client_to_host = 0;

    static constexpr int lobby_direction_host_to_client = 1;

    static constexpr int lobby_direction_peer = 3;

    static constexpr int lobby_member_max = 18;

    static constexpr int lobby_sub_channel_state = 0;

    static constexpr int lobby_sub_channel_heartbeat = 1;

    static constexpr int lobby_sub_channel_reliable = 2;

    static constexpr int lobby_sub_channel_unreliable = 3;

    static constexpr int lobby_sub_channel_migrate = 4;

    static constexpr int lobby_session_lobby_id = 4;

    static constexpr int lobby_session_active_flag = 64;

    static constexpr size_t lobby_msg_scratch_size = 0x20000;

    static constexpr int32_t lobby_members_overflow = 30;

    static constexpr int lobby_print_nest_bytes = 8192;

    static constexpr int lobby_print_array_tag = 12;

    static constexpr int lobby_resolve_xnaddr = 4;

    static constexpr int lobby_resolve_valid = 12;

    static constexpr int lobby_resolve_valid_skip = 1;

    void** steam_readp2p_slot();

    typedef char*(__fastcall* paragon_icon_name_t)(uint32_t mode, int icon_id);

    inline paragon_icon_name_t paragon_icon_name_fn = nullptr;

    typedef float(__fastcall* notetrack_anim_lookup_t)(int64_t anim_table, int anim_index, int notetrack_hash);

    inline notetrack_anim_lookup_t notetrack_anim_lookup_fn = nullptr;

    typedef bool(__fastcall* lobby_handle_join_request_t)(int controller, netadr_s adr, int64_t xuid, lobby_msg_s* lobby_msg);

    inline lobby_handle_join_request_t lobby_handle_join_request_fn = nullptr;

    typedef int64_t(__fastcall* lobby_handle_member_info_t)(int controller, netadr_s adr, int64_t xuid, lobby_msg_s* lobby_msg);

    inline lobby_handle_member_info_t lobby_handle_member_info_fn = nullptr;

    typedef bool(__fastcall* lobby_msg_join_package_t)(msg_join_lobby_s* msg, lobby_msg_s* lobby_msg);

    inline lobby_msg_join_package_t lobby_msg_join_package_fn = nullptr;

    typedef bool(__fastcall* lobby_send_join_response_t)(int controller, int channel, lobby_module_e lobby_module, netadr_s adr, int64_t xuid, msg_join_response_s* response);

    inline lobby_send_join_response_t lobby_send_join_response_fn = nullptr;

    typedef int(__fastcall* lobby_netchan_global_channel_t)(int channel);

    inline lobby_netchan_global_channel_t lobby_netchan_global_channel_fn = nullptr;

    typedef void*(__fastcall* lobby_host_lobby_state_t)(int lobby_type);

    inline lobby_host_lobby_state_t lobby_host_lobby_state_fn = nullptr;

    typedef int(__fastcall* lobby_session_client_count_t)(void* state, int filter);

    inline lobby_session_client_count_t lobby_session_client_count_fn = nullptr;

    typedef void(__fastcall* lobby_add_joining_client_t)(int controller, netadr_s adr, int64_t xuid, msg_join_member_info_s* info, int64_t key);

    inline lobby_add_joining_client_t lobby_add_joining_client_fn = nullptr;

    typedef bool(__fastcall* lobby_package_glob_t)(lobby_msg_s* msg, const char* key, void* value, int length, uint64_t max_length);

    inline lobby_package_glob_t lobby_package_glob_fn = nullptr;

    typedef bool(__fastcall* msg_fixed_client_info_package_t)(msg_fixed_client_info_s* msg, lobby_msg_s* lobby_msg);

    inline msg_fixed_client_info_package_t msg_fixed_client_info_package_fn = nullptr;

    typedef bool(__fastcall* msg_mutable_client_info_package_t)(msg_mutable_client_info_s* msg, lobby_msg_s* lobby_msg);

    inline msg_mutable_client_info_package_t msg_mutable_client_info_package_fn = nullptr;

    typedef int64_t(__fastcall* lobby_set_matchmaking_info_t)(void* session);

    inline lobby_set_matchmaking_info_t lobby_set_matchmaking_info_fn = nullptr;

    typedef char(__fastcall* lobby_host_update_matchmaking_session_t)();

    inline lobby_host_update_matchmaking_session_t lobby_host_update_matchmaking_session_fn = nullptr;

    typedef void*(__fastcall* lobby_host_data_get_session_t)(uint32_t lobby_type);

    inline lobby_host_data_get_session_t lobby_host_data_get_session_fn = nullptr;

    typedef char(__fastcall* lobby_qos_listener_enable_t)(void* session);

    inline lobby_qos_listener_enable_t lobby_qos_listener_enable_fn = nullptr;

    typedef char(__fastcall* live_session_update_t)(void* task_def, void* matchmaking_info);

    inline live_session_update_t live_session_update_fn = nullptr;

    typedef int(__fastcall* live_storage_get_ffotd_version_t)();

    inline live_storage_get_ffotd_version_t live_storage_get_ffotd_version_fn = nullptr;

    typedef int(__fastcall* playlist_get_version_number_t)();

    inline playlist_get_version_number_t playlist_get_version_number_fn = nullptr;

    typedef char(__fastcall* lobby_session_set_max_clients_t)(void* state, int count);

    inline lobby_session_set_max_clients_t lobby_session_set_max_clients_fn = nullptr;

    typedef void(__fastcall* lobby_host_task_search_t)(int a1, int a2);

    inline lobby_host_task_search_t lobby_host_task_search_fn = nullptr;

    typedef char(__fastcall* live_can_host_server_t)(uint32_t controller, int a2, uint32_t* out);

    inline live_can_host_server_t live_can_host_server_fn = nullptr;

    typedef int64_t(__fastcall* bd_session_id_serialize_t)(void* this_ptr, void* byte_buffer);

    inline bd_session_id_serialize_t bd_session_id_serialize_fn = nullptr;

    typedef char(__fastcall* bd_byte_buffer_write_uint32_t)(void* byte_buffer, int value);

    inline bd_byte_buffer_write_uint32_t bd_byte_buffer_write_uint32_fn = nullptr;

    typedef void(__fastcall* cbuf_add_text_t)(int local_client, const char* text);

    inline cbuf_add_text_t cbuf_add_text_fn = nullptr;

    typedef char(__fastcall* lobby_online_create_session_t)(void* session);

    inline lobby_online_create_session_t lobby_online_create_session_fn = nullptr;

    typedef int64_t(__fastcall* allocate_structs_t)(void* buf, int64_t elem_size, int count, void* ctor1, void* ctor2);

    inline allocate_structs_t allocate_structs_fn = nullptr;

    typedef void(__fastcall* lobby_search_session_t)(void* session);

    inline lobby_search_session_t lobby_search_session_fn = nullptr;

    typedef bool(__fastcall* demonware_gate_t)(int local_client);

    inline demonware_gate_t demonware_fetching_done_fn = nullptr;

    inline demonware_gate_t demonware_signed_in_fn = nullptr;

    typedef void*(__fastcall* lobby_session_get_t)(uint32_t lobby_type);

    inline lobby_session_get_t lobby_session_get_fn = nullptr;

    typedef void(__fastcall* bd_find_sessions_t)(uintptr_t matchmaking, void* remote_task, int query_id, uint32_t start_index, uint32_t max_num_results, void* query, int64_t results);

    inline bd_find_sessions_t bd_find_sessions_fn = nullptr;

    typedef bool(__fastcall* send_join_lobby_t)(int controller, int channel, lobby_module_e lobby_module, netadr_s adr, uint64_t xuid, msg_join_lobby_s* msg);

    inline send_join_lobby_t send_join_lobby_fn = nullptr;

    typedef uint32_t(__fastcall* content_packs_t)(int local_client);

    inline content_packs_t content_packs_fn = nullptr;

    typedef uint32_t(__fastcall* net_field_checksum_t)();

    inline net_field_checksum_t net_field_checksum_fn = nullptr;

    typedef void(__fastcall* dw_register_sec_t)(const void* sec_id, const void* sec_key);

    inline dw_register_sec_t dw_register_sec_fn = nullptr;

    typedef void(__fastcall* dw_common_to_netadr_t)(netadr_s* out, const void* common_addr, const void* sec_id);

    inline dw_common_to_netadr_t dw_common_to_netadr_fn = nullptr;

    typedef bool(__fastcall* msg_lobby_state_package_t)(void* state, lobby_msg_s* lobby_msg);

    inline msg_lobby_state_package_t msg_lobby_state_package_fn = nullptr;

    typedef bool(__fastcall* send_member_info_t)(int controller, int channel, lobby_module_e lobby_module, netadr_s adr, uint64_t xuid, msg_join_member_info_s* info);

    inline send_member_info_t send_member_info_fn = nullptr;

    typedef void(__fastcall* dw_netadr_to_common_t)(netadr_s netadr, void* out, int64_t size, void* extra);

    inline dw_netadr_to_common_t dw_netadr_to_common_fn = nullptr;


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

    typedef void(__fastcall* lod_params_reader_t)(void* view, int* cam_pos, int* flags, void* out_params);

    inline lod_params_reader_t lod_params_reader_fn = nullptr;

    static constexpr size_t lod_params_scale_offset = 16;

    static constexpr size_t lod_params_limit_offset = 32;

    static constexpr size_t lod_params_cull_offset = 36;

    static constexpr float lod_scale_boost = 4.0f;

    static constexpr float lod_cull_disabled = -1.0f;

    static constexpr float lod_cull_multiplier = 3.0f;

    static constexpr int lod_shadow_reflection_mask = 0x38000000;

    typedef int64_t(__fastcall* resident_mip_request_t)(int64_t a1, int a2, int64_t a3);

    inline resident_mip_request_t resident_mip_request_fn = nullptr;

    static constexpr int resident_mip_floor = 7;

    typedef int64_t(__fastcall* stream_throttle_t)(char a1, int64_t a2, int64_t a3);

    inline stream_throttle_t stream_throttle_fn = nullptr;

    static constexpr int stream_budget_boost = 16;

    struct protection_settings
    {
        bool server_commands = true;

        bool connectionless = true;

        bool lobby_messages = true;

        bool demonware = true;

        bool netchan_guards = true;

        bool markup_text = true;

        bool side_load = true;

        bool vote_strings = true;

        bool player_presence = true;

        bool paragon_icons = true;

        bool p2p_userdata = true;

        bool info_leak = true;

        bool workshop_mods = true;

        bool notetrack = true;
    };

    inline protection_settings protection;

    inline bool notifications_enabled = true;

    inline bool block_p2p = false;

    inline bool block_netchan = false;

    inline bool lod_enabled = true;

    inline bool texstream_enabled = true;

    inline bool movement_tick_enabled = false;

    inline int movement_tick_ms = 8;

    inline bool clientfield_fix = true;

    inline bool matchmaking_fix = true;

    inline bool lobby_advertise = false;

    inline constexpr uint64_t min_asset_pointer = 0x100000;

    inline constexpr int lobby_session_stride = 10840;

    inline constexpr int lobby_member_array = 248;

    inline constexpr int lobby_member_stride = 48;

    inline constexpr int lobby_member_client = 8;

    inline constexpr int lobby_client_xuid = 1040;

    inline constexpr int lobby_client_gamertag = 1048;

    inline constexpr int lobby_client_clantag = 8;

    inline constexpr int lobby_client_rank = 14;

    inline constexpr int lobby_client_mode_stride = 6;

    inline constexpr int lobby_session_host_xuid = 96;

    inline constexpr int lobby_session_leader_xuid = 232;

    inline constexpr int lobby_client_conn = 1080;

    inline constexpr int conn_ip = 32;

    inline constexpr int conn_port = 164;

    inline constexpr int conn_nat = 168;

    typedef void(__fastcall* clear_input_t)(int local_client);

    typedef int(__fastcall* mode_index_t)();

    typedef char(__fastcall* is_local_t)(uint64_t xuid);

    typedef char(__fastcall* voip_status_t)(void* client, int mode);

    typedef void(__fastcall* menu_input_mode_t)(int local_client, char enable);

    typedef void(__fastcall* key_event_t)(uint32_t local_client, uint32_t key);

    inline key_event_t key_event_fn = nullptr;

    typedef int64_t(__fastcall* mouse_move_t)(uint32_t local_client, uint32_t a2, uint32_t a3, int dx, int dy);

    inline mouse_move_t mouse_move_fn = nullptr;

    inline bool allow_all_auth = true;

    typedef int64_t(__fastcall* validate_auth_response_t)(int64_t self, uint32_t* response);

    inline validate_auth_response_t validate_auth_response_fn = nullptr;

    typedef char(__fastcall* clientfield_checksum_t)(int server_checksum);

    inline clientfield_checksum_t clientfield_checksum_fn = nullptr;

    typedef int64_t(__fastcall* disconnect_reason_t)(const char* reason);

    inline disconnect_reason_t disconnect_reason_fn = nullptr;

    typedef int64_t(__fastcall* apply_game_state_t)(uint32_t a1, int64_t a2, int64_t a3, int64_t message);

    inline apply_game_state_t apply_game_state_fn = nullptr;

    typedef int64_t(__fastcall* get_server_command_t)(uint32_t local_client, int32_t sequence);

    inline get_server_command_t get_server_command_fn = nullptr;

    typedef int64_t(__fastcall* client_match_bitmask_t)(int64_t session, int index);

    inline client_match_bitmask_t client_match_bitmask_fn = nullptr;

    typedef int64_t(__fastcall* client_command_t)(uint32_t client_num);

    inline client_command_t client_command_fn = nullptr;

    typedef int64_t(__fastcall* client_mrp_t)(int64_t entity);

    inline client_mrp_t client_mrp_fn = nullptr;

    typedef void(__fastcall* sv_cmd_argv_buffer_t)(int index, char* buffer, int size);

    inline sv_cmd_argv_buffer_t sv_cmd_argv_buffer_fn = nullptr;

    typedef int64_t(__fastcall* pmove_t)(int64_t pmove);

    inline pmove_t pmove_fn = nullptr;

    typedef int64_t(__fastcall* lobby_disconnect_event_t)(uint32_t* a1, int64_t a2, uint32_t a3, int64_t a4);

    inline lobby_disconnect_event_t lobby_disconnect_event_fn = nullptr;

    typedef int64_t(__fastcall* localize_lookup_t)(int64_t key);

    inline localize_lookup_t localize_lookup_fn = nullptr;

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

    inline thread_local int lobby_print_depth = 0;

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
