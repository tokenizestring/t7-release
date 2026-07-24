#include "resource.hpp"

namespace utils::resource
{
    static HMODULE dll_module = nullptr;

    void set_module(HMODULE module)
    {
        dll_module = module;
    }

    bool load(int id, const void*& data, size_t& size)
    {
        HRSRC res = FindResourceA(dll_module, MAKEINTRESOURCEA(id), RT_RCDATA);

        if (!res)
        {
            return false;
        }

        HGLOBAL loaded = LoadResource(dll_module, res);

        if (!loaded)
        {
            return false;
        }

        data = LockResource(loaded);

        size = SizeofResource(dll_module, res);

        return data != nullptr;
    }
}
