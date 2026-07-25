#include "demonware.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/hook/hook.hpp"
#include "../../utils/mem/mem.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"

#include <cstring>
#include <cstdlib>

namespace demonware
{
    static bool allow_friends = false;

    static char __fastcall hk_instant_dispatch_demonware(int64_t sender_id, uint32_t controller_index, const char* message, uint32_t message_size)
    {
        if (message != nullptr && message_size >= 2 && message[0] == '1')
        {
            char type = message[1];

            bool blocked = type == 'f' || type == 'e';

            T7_LOG(std::string(cx("demonware: '")) + type + cx("' from ") + std::to_string(sender_id) + cx(" size ") + std::to_string(message_size) + (blocked ? cx(" dropped") : cx("")) + cx("."));

            if (blocked)
            {
                return 1;
            }
        }

        return engine::instant_dispatch_demonware(sender_id, controller_index, message, message_size);
    }

    static bool __fastcall hk_instant_dispatch_steam(void* self, void* dest, uint32_t dest_size, uint32_t* message_size, uint64_t* sender_xuid, int channel)
    {
        bool result = engine::instant_dispatch_steam(self, dest, dest_size, message_size, sender_xuid, channel);

        if (result && dest != nullptr && sender_xuid != nullptr && message_size != nullptr && *message_size >= 6)
        {
            const char* data = static_cast<const char*>(dest);

            if (*reinterpret_cast<const uint32_t*>(data) == 40 && data[4] == '1')
            {
                char type = data[5];

                bool blocked = type == 'f' || type == 'e';

                T7_LOG(std::string(cx("steam(p2p): '")) + type + cx("' from ") + std::to_string(*sender_xuid) + cx(" size ") + std::to_string(*message_size) + (blocked ? cx(" dropped") : cx("")) + cx("."));

                if (blocked)
                {
                    return false;
                }
            }
        }

        return result;
    }

    static char __fastcall hk_build_info_response(uint32_t controller_index, uint64_t recipient, int nonce)
    {
        bool allow = recipient == static_cast<uint64_t>(engine::user_xuid(controller_index)) || (allow_friends && engine::is_friend(controller_index, static_cast<int64_t>(recipient)));

        engine::info_response_s response = {};

        response.nonce = nonce;

        response.ui_screen = engine::ui_screen();

        response.nat_type = static_cast<uint8_t>(engine::nat_type());

        for (uint32_t i = 0; i < 2; i++)
        {
            engine::info_lobby_s& entry = response.lobby[i];

            const uint8_t* lobby = static_cast<const uint8_t*>(engine::lobby_game(i));

            if (lobby == nullptr || *reinterpret_cast<const int*>(lobby + 64) <= 0)
            {
                const uint8_t* party = static_cast<const uint8_t*>(engine::lobby_party(i));

                lobby = party != nullptr && *reinterpret_cast<const int*>(party + 64) > 0 ? party : nullptr;
            }

            entry.valid = lobby != nullptr ? 1 : 0;

            if (lobby != nullptr)
            {
                entry.host_xuid = *reinterpret_cast<const uint64_t*>(lobby + 96);

                engine::string_copy(entry.host_name, 32, reinterpret_cast<const char*>(lobby + 104));

                entry.network_mode = engine::network_mode();

                entry.main_mode = engine::main_mode();

                memcpy(entry.sec_id, lobby + 190, 8);

                memcpy(entry.sec_key, lobby + 198, 16);

                memcpy(entry.addr_buffer, lobby + 153, 37);

                engine::ugc_fill(entry.ugc_name, 1);

                if (!allow)
                {
                    engine::xnaddr_s* addr = reinterpret_cast<engine::xnaddr_s*>(entry.addr_buffer);

                    for (int buffer = 0; buffer < 4; buffer++) 
                    {
                        addr->local_addr[buffer] = static_cast<uint8_t>(rand() % 254 + 1);

                        addr->public_addr[buffer] = static_cast<uint8_t>(rand() % 254 + 1);
                    }
                }
            }
        }

        uint64_t target = recipient;

        engine::send_info_response(controller_index, &target, 1, &response);

        T7_LOG(std::string(cx("demonware: info response to ")) + std::to_string(recipient) + (allow ? cx(" (real)") : cx(" (spoofed)")) + cx("."));

        return 1;
    }

    void initialize()
    {
        srand(static_cast<unsigned int>(GetTickCount()));

        engine::instant_dispatch_demonware = reinterpret_cast<engine::instant_dispatch_demonware_t>(engine::base() + engine::dw_instant_dispatch);

        utils::hook::attach(reinterpret_cast<void**>(&engine::instant_dispatch_demonware), hk_instant_dispatch_demonware);

        engine::build_info_response = reinterpret_cast<engine::build_info_response_t>(engine::base() + engine::dw_build_info_response);

        utils::hook::attach(reinterpret_cast<void**>(&engine::build_info_response), hk_build_info_response);

        T7_LOG(cx("demonware: dispatch protection installed."));
    }

    void tick()
    {
        if (engine::instant_dispatch_steam != nullptr)
        {
            return;
        }

        void** slot = engine::steam_readp2p_slot();

        if (slot == nullptr)
        {
            return;
        }

        engine::instant_dispatch_steam = reinterpret_cast<engine::instant_dispatch_steam_t>(*slot);

        void* detour = reinterpret_cast<void*>(hk_instant_dispatch_steam);

        utils::mem::patch(slot, reinterpret_cast<const uint8_t*>(&detour), sizeof(void*));

        T7_LOG(cx("demonware: steam p2p protection installed."));
    }
}
