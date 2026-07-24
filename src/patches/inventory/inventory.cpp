#include "inventory.hpp"
#include "../../engine/engine.hpp"
#include "../../utils/hook/hook.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"

#include <cstdint>

namespace inventory
{
    static bool unlock_all = true;

    static bool spoof_slots = true;

    static int64_t __fastcall hk_get_item_quantity(uint32_t controller, int item_id)
    {
        int64_t owned = engine::inv_get_item_quantity_fn(controller, item_id);

        if (unlock_all && owned == 0)
        {
            return 255;
        }

        return owned; 
    }

    static bool __fastcall hk_are_extra_slots_purchased(uint32_t controller)
    {
        if (spoof_slots)
        {
            return true;
        }

        return engine::inv_are_extra_slots_purchased_fn(controller);
    }

    static bool __fastcall hk_is_item_purchased(uint32_t controller, int16_t unlockable, char ignore_bundle)
    {
        if (unlock_all)
        {
            return true;
        }

        return engine::bg_is_item_purchased_fn(controller, unlockable, ignore_bundle);
    }

    void initialize()
    {
        engine::inv_get_item_quantity_fn = reinterpret_cast<engine::inv_get_item_quantity_t>(engine::base() + engine::inv_get_item_quantity);

        utils::hook::attach(reinterpret_cast<void**>(&engine::inv_get_item_quantity_fn), hk_get_item_quantity);

        engine::inv_are_extra_slots_purchased_fn = reinterpret_cast<engine::inv_are_extra_slots_purchased_t>(engine::base() + engine::inv_are_extra_slots_purchased);

        utils::hook::attach(reinterpret_cast<void**>(&engine::inv_are_extra_slots_purchased_fn), hk_are_extra_slots_purchased);

        engine::bg_is_item_purchased_fn = reinterpret_cast<engine::bg_is_item_purchased_t>(engine::base() + engine::bg_unlockables_is_item_purchased);

        utils::hook::attach(reinterpret_cast<void**>(&engine::bg_is_item_purchased_fn), hk_is_item_purchased);

        T7_LOG(cx("inventory: unlock hooks installed"));
    }
}
