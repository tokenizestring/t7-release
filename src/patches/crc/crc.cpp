#include "crc.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/mem/mem.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"
#include "../../resources/resources.hpp"

static constexpr utils::mem::obfuscated_patch zero_rcx_6({ 0x48, 0x31, 0xC9, 0x90, 0x90, 0x90 });

static constexpr utils::mem::obfuscated_patch zero_rcx_8({ 0x48, 0x31, 0xC9, 0x90, 0x90, 0x90, 0x90, 0x90 });

static constexpr utils::mem::obfuscated_patch zero_rax_rdx({ 0x48, 0x31, 0xC0, 0x48, 0x31, 0xD2 });

namespace crc
{
    bool check_ready()
    {
        uintptr_t base = engine::base();

        if (base == 0)
        {
            return false;
        }

        return *reinterpret_cast<__int64*>(base + engine::crc_ready_flag) != 0;
    }

    void patch() 
    {
        utils::mem::apply_resource_patches(RES_CRC_ZERO_RCX_6, cx("zero_rcx_6").c_str(), zero_rcx_6);

        utils::mem::apply_resource_patches(RES_CRC_ZERO_RCX_8, cx("zero_rcx_8").c_str(), zero_rcx_8);

        utils::mem::apply_resource_patches(RES_CRC_ZERO_RAX_RDX, cx("zero_rax_rdx").c_str(), zero_rax_rdx);

        T7_LOG(cx("crc: all patches applied."));
    }
}
