#pragma once

#include "../../stdafx.hpp"

#include <cstdint>

namespace serverlist
{
    struct player
    {
        uint64_t xuid;

        char name[33];

        char clan[8];

        int client_num;
    };

    struct entry
    {
        uint64_t xuid;

        char ip[24];

        uint16_t port;

        uint32_t num_players;

        uint32_t max_players;

        uint32_t server_type;

        bool dedicated;

        uint8_t xnaddr[37];

        uint8_t sec_id[8];

        uint8_t sec_key[16];

        bool probed;

        uint64_t last_state_ms;

        uint64_t steam_lobby_id;

        int player_count;

        player players[18];
    };

    inline constexpr int entries_per_page = 10;

    inline bool auto_probe = false;

    void initialize();

    void tick();

    void set_active(bool value);

    void clear();

    int count();

    int page_count();

    const entry* get(int index);

    void dispatch_probe(int index);

    bool wants(uint64_t host_xuid);

    void send_member_info(uint64_t host_xuid, void* adr, uint64_t reservation_key);

    void set_players(uint64_t host_xuid, const player* list, int list_count, uint64_t steam_lobby_id);

    void join_steam_lobby(int index);

    void send_steam_chat(int index, const char* message);

    void lock();

    void unlock();
}
