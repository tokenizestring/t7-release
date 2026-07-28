#pragma once

#include "../../stdafx.hpp"
#include "../../engine/engine.hpp"

namespace send
{
    struct target_info
    {
        unsigned char ip[4];

        uint16_t port;

        uint64_t xuid;

        char name[36];

        engine::netadr_s netadr;
    };

    void initialize();

    bool send_relay_test(const target_info& target);

    bool send_oob_at(const target_info& target);

    bool send_join_overflow(const target_info& target);

    bool send_host_disconnect(const target_info& target);

    bool send_migrate_test(const target_info& target);

    bool send_print_nested(const target_info& target);
}
