#include "idahide.hpp"
#include "../../utils/hook/hook.hpp"
#include "../../utils/log/log.hpp"
#include "../../utils/crypt/crypt.hpp"

#include <winternl.h>

namespace idahide
{
    typedef NTSTATUS(NTAPI* nt_query_information_process_t)(HANDLE, uint32_t, void*, uint32_t, uint32_t*);

    typedef NTSTATUS(NTAPI* nt_query_system_information_t)(uint32_t, void*, uint32_t, uint32_t*);

    typedef HANDLE(WINAPI* create_mutex_ex_a_t)(LPSECURITY_ATTRIBUTES, const char*, DWORD, DWORD);

    typedef int(WINAPI* get_window_text_a_t)(HWND, char*, int);

    static nt_query_information_process_t o_nt_query_information_process = nullptr;

    static nt_query_system_information_t o_nt_query_system_information = nullptr;

    static create_mutex_ex_a_t o_create_mutex_ex_a = nullptr;

    static get_window_text_a_t o_get_window_text_a = nullptr;

    static constexpr uint32_t process_image_file_name = 27;

    static constexpr uint32_t process_image_file_name_win32 = 43;

    static constexpr uint32_t system_process_information = 5;

    struct system_process_entry
    {
        uint32_t next_entry_offset;

        uint32_t number_of_threads;

        uint8_t reserved[48];

        UNICODE_STRING image_name;
    };

    static bool scrub_wide(wchar_t* buffer, size_t length)
    {
        if (buffer == nullptr || length < 3)
        {
            return false;
        }

        bool modified = false;

        for (size_t i = 0; i + 3 <= length; i++)
        {
            wchar_t a = buffer[i];

            wchar_t b = buffer[i + 1];

            wchar_t c = buffer[i + 2];

            if ((a == L'I' || a == L'i') && (b == L'D' || b == L'd') && (c == L'A' || c == L'a'))
            {
                buffer[i] = L'a';

                buffer[i + 1] = L'a';

                buffer[i + 2] = L'a';

                modified = true;
            }
        }

        return modified;
    }

    static bool scrub_ascii(char* buffer, size_t length)
    {
        if (buffer == nullptr || length < 3)
        {
            return false;
        }

        bool modified = false;

        for (size_t i = 0; i + 3 <= length; i++)
        {
            char a = buffer[i];

            char b = buffer[i + 1];

            char c = buffer[i + 2];

            if ((a == 'I' || a == 'i') && (b == 'D' || b == 'd') && (c == 'A' || c == 'a'))
            {
                buffer[i] = 'a';

                buffer[i + 1] = 'a';

                buffer[i + 2] = 'a';

                modified = true;
            }
        }

        return modified;
    }

    static void scrub_unicode(UNICODE_STRING& value)
    {
        if (value.Buffer != nullptr && value.Length != 0)
        {
            scrub_wide(value.Buffer, value.Length / sizeof(wchar_t));
        }
    }

    static NTSTATUS NTAPI hk_nt_query_information_process(HANDLE handle, uint32_t info_class, void* info, uint32_t info_length, uint32_t* return_length)
    {
        NTSTATUS status = o_nt_query_information_process(handle, info_class, info, info_length, return_length);

        if (status >= 0 && info != nullptr && (info_class == process_image_file_name || info_class == process_image_file_name_win32))
        {
            scrub_unicode(*static_cast<UNICODE_STRING*>(info));
        }

        return status;
    }

    static NTSTATUS NTAPI hk_nt_query_system_information(uint32_t info_class, void* info, uint32_t info_length, uint32_t* return_length)
    {
        NTSTATUS status = o_nt_query_system_information(info_class, info, info_length, return_length);

        if (status >= 0 && info != nullptr && info_class == system_process_information)
        {
            uint8_t* cursor = static_cast<uint8_t*>(info);

            while (true)
            {
                system_process_entry* entry = reinterpret_cast<system_process_entry*>(cursor);

                scrub_unicode(entry->image_name);

                if (entry->next_entry_offset == 0)
                {
                    break;
                }

                cursor += entry->next_entry_offset;
            }
        }

        return status;
    }

    static HANDLE WINAPI hk_create_mutex_ex_a(LPSECURITY_ATTRIBUTES attributes, const char* name, DWORD flags, DWORD access)
    {
        if (name != nullptr && (strcmp(name, "$ IDA trusted_idbs") == 0 || strcmp(name, "$ IDA registry mutex $") == 0))
        {
            return nullptr;
        }

        return o_create_mutex_ex_a(attributes, name, flags, access);
    }

    static int WINAPI hk_get_window_text_a(HWND window, char* text, int max_count)
    {
        int count = o_get_window_text_a(window, text, max_count);

        if (count > 0 && text != nullptr)
        {
            scrub_ascii(text, static_cast<size_t>(count));
        }

        return count;
    }

    void initialize()
    {
        HMODULE ntdll = GetModuleHandleA("ntdll.dll");

        HMODULE kernel32 = GetModuleHandleA("kernel32.dll");

        HMODULE user32 = GetModuleHandleA("user32.dll");

        if (ntdll != nullptr)
        {
            o_nt_query_information_process = reinterpret_cast<nt_query_information_process_t>(GetProcAddress(ntdll, "NtQueryInformationProcess"));

            o_nt_query_system_information = reinterpret_cast<nt_query_system_information_t>(GetProcAddress(ntdll, "NtQuerySystemInformation"));

            if (o_nt_query_information_process != nullptr)
            {
                utils::hook::attach(reinterpret_cast<void**>(&o_nt_query_information_process), hk_nt_query_information_process);
            }

            if (o_nt_query_system_information != nullptr)
            {
                utils::hook::attach(reinterpret_cast<void**>(&o_nt_query_system_information), hk_nt_query_system_information);
            }
        }

        if (kernel32 != nullptr)
        {
            o_create_mutex_ex_a = reinterpret_cast<create_mutex_ex_a_t>(GetProcAddress(kernel32, "CreateMutexExA"));

            if (o_create_mutex_ex_a != nullptr)
            {
                utils::hook::attach(reinterpret_cast<void**>(&o_create_mutex_ex_a), hk_create_mutex_ex_a);
            }
        }

        if (user32 != nullptr)
        {
            o_get_window_text_a = reinterpret_cast<get_window_text_a_t>(GetProcAddress(user32, "GetWindowTextA"));

            if (o_get_window_text_a != nullptr)
            {
                utils::hook::attach(reinterpret_cast<void**>(&o_get_window_text_a), hk_get_window_text_a);
            }
        }

        T7_LOG(cx("idahide: ida-name scrub installed (process/window/mutex)."));
    }
}
