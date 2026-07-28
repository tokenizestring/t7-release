#include "recents.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"

#include <ctime>
#include <cstring>
#include <cstdio>
#include <sstream>

namespace recents
{
    static constexpr int max_entries = 200;

    static constexpr int scan_interval = 2000;

    static constexpr int save_interval = 30000;

    static entry entries[max_entries];

    static int entry_count = 0;

    static DWORD last_scan = 0;

    static DWORD last_save = 0;

    static bool dirty = false;

    static std::filesystem::path storage_path()
    {
        return utils::log::root_directory() / cx("recents").c_str() / cx("recents").c_str();
    }

    static bool addr_match(const address_record& a, const address_record& b)
    {
        if (a.ip[0] != b.ip[0])
        {
            return false;
        }

        if (a.ip[1] != b.ip[1])
        {
            return false;
        }

        if (a.ip[2] != b.ip[2])
        {
            return false;
        }

        if (a.ip[3] != b.ip[3])
        {
            return false;
        }

        if (a.port != b.port)
        {
            return false;
        }

        return true;
    }

    static void push_address(entry& e, const address_record& addr)
    {
        for (int i = 0; i < e.addr_count; i++)
        {
            if (addr_match(e.history[i], addr))
            {
                return;
            }
        }

        if (e.addr_count < max_addresses)
        {
            e.history[e.addr_count] = addr;

            e.addr_count++;
        }
        else
        {
            for (int i = 0; i < max_addresses - 1; i++)
            {
                e.history[i] = e.history[i + 1];
            }

            e.history[max_addresses - 1] = addr;
        }
    }

    static entry* find_by_xuid(uint64_t xuid)
    {
        for (int i = 0; i < entry_count; i++)
        {
            if (entries[i].xuid == xuid)
            {
                return &entries[i];
            }
        }

        return nullptr;
    }

    static void sort_entries()
    {
        for (int i = 1; i < entry_count; i++)
        {
            entry tmp = entries[i];

            int j = i - 1;

            while (j >= 0 && entries[j].last_seen < tmp.last_seen)
            {
                entries[j + 1] = entries[j];

                j--;
            }

            entries[j + 1] = tmp;
        }
    }

    static void upsert(uint64_t xuid, const char* name, const unsigned char* ip, uint16_t port, bool has_addr)
    {
        uint64_t now = static_cast<uint64_t>(time(nullptr));

        entry* e = find_by_xuid(xuid);

        if (e != nullptr)
        {
            int k = 0;

            while (k < 35 && name[k] != 0)
            {
                e->name[k] = name[k];

                k++;
            }

            e->name[k] = 0;

            e->last_seen = now;

            if (has_addr)
            {
                address_record ar = {};

                ar.ip[0] = ip[0];

                ar.ip[1] = ip[1];

                ar.ip[2] = ip[2];

                ar.ip[3] = ip[3];

                ar.port = port;

                if (e->has_addr)
                {
                    if (!addr_match(e->current, ar))
                    {
                        push_address(*e, e->current);

                        e->current = ar;
                    }
                }
                else
                {
                    e->current = ar;

                    e->has_addr = true;
                }
            }

            dirty = true;

            return;
        }

        if (entry_count >= max_entries)
        {
            sort_entries();

            entry_count = max_entries - 1;
        }

        entry& ne = entries[entry_count];

        memset(&ne, 0, sizeof(entry));

        ne.xuid = xuid;

        int k = 0;

        while (k < 35 && name[k] != 0)
        {
            ne.name[k] = name[k];

            k++;
        }

        ne.name[k] = 0;

        ne.last_seen = now;

        if (has_addr)
        {
            ne.current.ip[0] = ip[0];

            ne.current.ip[1] = ip[1];

            ne.current.ip[2] = ip[2];

            ne.current.ip[3] = ip[3];

            ne.current.port = port;

            ne.has_addr = true;
        }

        ne.addr_count = 0;

        entry_count++;

        dirty = true;
    }

    static void scan_lobby()
    {
        uint8_t* base = reinterpret_cast<uint8_t*>(engine::base());

        uint64_t local_xuid = static_cast<uint64_t>(engine::user_xuid(0));

        for (int s = 0; s < 2; s++)
        {
            uint8_t* session = base + engine::lobby_session + engine::lobby_session_stride * s;

            for (int i = 0; i < 18; i++)
            {
                uint8_t* member = session + engine::lobby_member_array + engine::lobby_member_stride * i;

                if (*reinterpret_cast<uint64_t*>(member) == 0)
                {
                    continue;
                }

                uint8_t* client = *reinterpret_cast<uint8_t**>(member + engine::lobby_member_client);

                if (reinterpret_cast<uintptr_t>(client) < 0x10000)
                {
                    continue;
                }

                uint64_t xuid = *reinterpret_cast<uint64_t*>(client + engine::lobby_client_xuid);

                if (xuid == 0 || xuid == local_xuid)
                {
                    continue;
                }

                const char* name = reinterpret_cast<const char*>(client + engine::lobby_client_gamertag);

                unsigned char ip[4] = {};

                uint16_t port = 0;

                bool has_addr = false;

                uint8_t* conn = *reinterpret_cast<uint8_t**>(client + engine::lobby_client_conn);

                if (reinterpret_cast<uintptr_t>(conn) >= 0x10000)
                {
                    ip[0] = conn[engine::conn_ip];

                    ip[1] = conn[engine::conn_ip + 1];

                    ip[2] = conn[engine::conn_ip + 2];

                    ip[3] = conn[engine::conn_ip + 3];

                    port = *reinterpret_cast<uint16_t*>(conn + engine::conn_port);

                    has_addr = true;
                }

                upsert(xuid, name, ip, port, has_addr);
            }
        }

        sort_entries();
    }

    static bool parse_addr(const std::string& s, address_record& out)
    {
        int ip0 = 0;

        int ip1 = 0;

        int ip2 = 0;

        int ip3 = 0;

        int port = 0;

        if (sscanf(s.c_str(), "%d.%d.%d.%d:%d", &ip0, &ip1, &ip2, &ip3, &port) != 5)
        {
            return false;
        }

        out.ip[0] = static_cast<unsigned char>(ip0);

        out.ip[1] = static_cast<unsigned char>(ip1);

        out.ip[2] = static_cast<unsigned char>(ip2);

        out.ip[3] = static_cast<unsigned char>(ip3);

        out.port = static_cast<uint16_t>(port);

        return true;
    }

    static void save()
    {
        std::filesystem::path path = storage_path();

        std::error_code ec;

        std::filesystem::create_directories(path.parent_path(), ec);

        std::ofstream file(path.string(), std::ios::out | std::ios::trunc);

        if (!file.is_open())
        {
            return;
        }

        for (int i = 0; i < entry_count; i++)
        {
            const entry& e = entries[i];

            file << e.xuid << "|" << e.name << "|" << e.last_seen << "|";

            if (e.has_addr)
            {
                file << "1|"
                     << static_cast<int>(e.current.ip[0]) << "."
                     << static_cast<int>(e.current.ip[1]) << "."
                     << static_cast<int>(e.current.ip[2]) << "."
                     << static_cast<int>(e.current.ip[3]) << ":"
                     << e.current.port;
            }
            else
            {
                file << "0|0.0.0.0:0";
            }

            file << "|" << e.addr_count;

            for (int a = 0; a < e.addr_count; a++)
            {
                file << (a == 0 ? "|" : ";")
                     << static_cast<int>(e.history[a].ip[0]) << "."
                     << static_cast<int>(e.history[a].ip[1]) << "."
                     << static_cast<int>(e.history[a].ip[2]) << "."
                     << static_cast<int>(e.history[a].ip[3]) << ":"
                     << e.history[a].port;
            }

            file << "\n";
        }

        dirty = false;
    }

    static void load()
    {
        std::filesystem::path path = storage_path();

        std::ifstream file(path.string());

        if (!file.is_open())
        {
            return;
        }

        entry_count = 0;

        std::string line;

        while (std::getline(file, line) && entry_count < max_entries)
        {
            std::istringstream ss(line);

            std::string tok;

            entry& e = entries[entry_count];

            memset(&e, 0, sizeof(entry));

            if (!std::getline(ss, tok, '|'))
            {
                continue;
            }

            e.xuid = strtoull(tok.c_str(), nullptr, 10);

            if (e.xuid == 0)
            {
                continue;
            }

            if (!std::getline(ss, tok, '|'))
            {
                continue;
            }

            int k = 0;

            while (k < 35 && k < static_cast<int>(tok.size()))
            {
                e.name[k] = tok[k];

                k++;
            }

            e.name[k] = 0;

            if (!std::getline(ss, tok, '|'))
            {
                continue;
            }

            e.last_seen = strtoull(tok.c_str(), nullptr, 10);

            if (!std::getline(ss, tok, '|'))
            {
                continue;
            }

            e.has_addr = tok == "1";

            if (!std::getline(ss, tok, '|'))
            {
                continue;
            }

            if (e.has_addr)
            {
                parse_addr(tok, e.current);
            }

            if (!std::getline(ss, tok, '|'))
            {
                entry_count++;

                continue;
            }

            int ac = atoi(tok.c_str());

            if (ac > max_addresses)
            {
                ac = max_addresses;
            }

            if (ac > 0 && std::getline(ss, tok))
            {
                std::istringstream hs(tok);

                std::string addr_tok;

                e.addr_count = 0;

                while (std::getline(hs, addr_tok, ';') && e.addr_count < ac)
                {
                    if (parse_addr(addr_tok, e.history[e.addr_count]))
                    {
                        e.addr_count++;
                    }
                }
            }

            entry_count++;
        }

        sort_entries();

        T7_LOG(std::string(cx("recents: loaded ")) + std::to_string(entry_count) + cx(" entries."));
    }

    void initialize()
    {
        load();

        T7_LOG(cx("recents: initialized."));
    }

    void tick()
    {
        DWORD now = GetTickCount();

        if (now - last_scan < scan_interval)
        {
            return;
        }

        last_scan = now;

        scan_lobby();

        if (dirty && now - last_save >= save_interval)
        {
            save();

            last_save = now;
        }
    }

    int count()
    {
        return entry_count;
    }

    int page_count()
    {
        if (entry_count == 0)
        {
            return 1;
        }

        return (entry_count + entries_per_page - 1) / entries_per_page;
    }

    const entry* get(int index)
    {
        if (index < 0 || index >= entry_count)
        {
            return nullptr;
        }

        return &entries[index];
    }

    void format_ago(uint64_t timestamp, char* buf, int buf_size)
    {
        uint64_t now = static_cast<uint64_t>(time(nullptr));

        if (timestamp > now)
        {
            snprintf(buf, buf_size, "now");

            return;
        }

        uint64_t diff = now - timestamp;

        if (diff < 60)
        {
            snprintf(buf, buf_size, "now");
        }
        else if (diff < 3600)
        {
            snprintf(buf, buf_size, "%llum ago", static_cast<unsigned long long>(diff / 60));
        }
        else if (diff < 86400)
        {
            snprintf(buf, buf_size, "%lluh ago", static_cast<unsigned long long>(diff / 3600));
        }
        else
        {
            snprintf(buf, buf_size, "%llud ago", static_cast<unsigned long long>(diff / 86400));
        }
    }
}
