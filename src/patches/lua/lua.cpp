#include "lua.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/hook/hook.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"

#include <mutex>
#include <vector>
#include <cstring>
#include <fstream>
#include <sstream>

namespace lua
{
    static std::mutex queue_mutex;

    static std::vector<std::string> pending;

    static bool test_latch = false;

    static const char* active_source = nullptr;

    void queue(const std::string& code)
    {
        std::lock_guard<std::mutex> lock(queue_mutex);

        pending.push_back(code);
    }

    static int64_t __fastcall hk_load_file(int64_t vm, const char* name)
    {
        if (active_source != nullptr)
        {
            int64_t inner = *reinterpret_cast<int64_t*>(vm + engine::lua_inner_state_offset);

            if (inner == 0)
            {
                return 1;
            }

            return engine::lua_loadbuffer_fn(vm, inner + engine::lua_load_scratch_offset, active_source, std::strlen(active_source), name);
        }

        return engine::lua_load_file_fn(vm, name);
    }

    static bool run_now(const std::string& code)
    {
        if (engine::lua_get_vm_fn == nullptr || engine::lua_run_fn == nullptr)
        {
            return false;
        }

        int64_t vm = engine::lua_get_vm_fn(engine::lua_ui_vm);

        if (vm == 0)
        {
            return false;
        }

        active_source = code.c_str();

        char ok = engine::lua_run_fn(vm, cx("=t7-release").c_str());

        active_source = nullptr;

        return ok != 0;
    }

    void tick()
    {
        bool test_down = (GetAsyncKeyState(VK_F10) & 0x8000) != 0;

        if (test_down && !test_latch)
        {
            test_latch = true;

            std::ifstream file(cx("t7_lua.txt"));

            if (file.good())
            {
                std::stringstream buffer;

                buffer << file.rdbuf();

                std::string code = buffer.str();

                if (!code.empty())
                {
                    queue(code);

                    T7_LOG(cx("lua: F10 running t7_lua.txt."));
                }
                else
                {
                    T7_LOG(cx("lua: t7_lua.txt is empty."));
                }
            }
            else
            {
                T7_LOG(cx("lua: t7_lua.txt not found in the game folder."));
            }
        }
        else if (!test_down)
        {
            test_latch = false;
        }

        std::vector<std::string> jobs;

        {
            std::lock_guard<std::mutex> lock(queue_mutex);

            if (pending.empty())
            {
                return;
            }

            jobs.swap(pending);
        }

        for (const std::string& code : jobs)
        {
            bool ok = run_now(code);

            T7_LOG(std::string(cx("lua: ran snippet -> ")) + (ok ? cx("ok") : cx("failed")) + cx("."));
        }
    }

    void initialize()
    {
        engine::lua_get_vm_fn = reinterpret_cast<engine::lua_get_vm_t>(engine::base() + engine::lua_get_vm);

        engine::lua_loadbuffer_fn = reinterpret_cast<engine::lua_loadbuffer_t>(engine::base() + engine::lua_loadbuffer);

        engine::lua_run_fn = reinterpret_cast<engine::lua_run_t>(engine::base() + engine::lua_run);

        engine::lua_load_file_fn = reinterpret_cast<engine::lua_load_file_t>(engine::base() + engine::lua_load_file);

        utils::hook::attach(reinterpret_cast<void**>(&engine::lua_load_file_fn), hk_load_file);

        T7_LOG(cx("lua: source executor ready (F10 runs t7_lua.txt)."));
    }
}
