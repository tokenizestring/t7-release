#pragma once

#include "../../stdafx.hpp"

namespace recents
{
    static constexpr int max_addresses = 8;

    static constexpr int entries_per_page = 12;

    struct address_record
    {
        unsigned char ip[4];

        uint16_t port;
    };

    struct entry
    {
        uint64_t xuid;

        char name[36];

        uint64_t last_seen;

        bool has_addr;

        address_record current;

        int addr_count;

        address_record history[max_addresses];
    };

    void initialize();

    void tick();

    int count();

    int page_count();

    const entry* get(int index);

    void format_ago(uint64_t timestamp, char* buf, int buf_size);
}
