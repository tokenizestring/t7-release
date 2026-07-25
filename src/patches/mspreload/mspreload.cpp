#include "mspreload.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/hook/hook.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"

#include <cstring>

namespace mspreload
{
    static constexpr int msload_config_string = 3627;

    static const char* __fastcall hk_get_config_string(int index)
    {
        if (index == msload_config_string)
        {
            const char* value = engine::get_config_string(index);

            if (value != nullptr && (_strnicmp(value, "mspreload", 9) == 0 || _strnicmp(value, "msload", 6) == 0))
            {
                T7_LOG(std::string(cx("mspreload: forced side-load ")) + value + cx(", dropped."));

                engine::store_config_string(index, "");
            }
        }

        return engine::get_config_string(index);
    }

    void initialize()
    {
        engine::get_config_string = reinterpret_cast<engine::get_config_string_t>(engine::base() + engine::cl_get_config_string);

        utils::hook::attach(reinterpret_cast<void**>(&engine::get_config_string), hk_get_config_string);

        T7_LOG(cx("mspreload: protection installed."));
    }
}
