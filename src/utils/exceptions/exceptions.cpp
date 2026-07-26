#include "exceptions.hpp"
#include "../log/log.hpp"
#include "../crypt/crypt.hpp"

#include <sstream>
#include <iomanip>
#include <cstdio>
#include <DbgHelp.h>

#pragma comment(lib, "dbghelp.lib")

static HMODULE game_module = nullptr;
static uintptr_t game_base = 0;
static uintptr_t game_end = 0;

static uintptr_t self_base = 0;
static uintptr_t self_end = 0;

static std::ofstream exception_file;

static CRITICAL_SECTION dump_lock;

static bool dump_lock_ready = false;

static void emit(const std::string& line)
{
    if (exception_file.is_open())
    {
        exception_file << line << std::endl;
    }
}

static std::string stamp()
{
    SYSTEMTIME t;

    GetLocalTime(&t);

    char buffer[40];

    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d_%02d-%02d-%02d-%03d", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);

    return buffer;
}

static bool is_game_address(uintptr_t addr)
{
    return addr >= game_base && addr < game_end;
}

static std::string to_hex(uintptr_t value)
{
    std::stringstream ss;

    ss << "0x" << std::hex << std::uppercase << value;

    return ss.str();
}

static const char* code_name(DWORD code)
{
    switch (code)
    {
        case EXCEPTION_ACCESS_VIOLATION:        return "ACCESS_VIOLATION";
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:   return "ARRAY_BOUNDS_EXCEEDED";
        case EXCEPTION_BREAKPOINT:              return "BREAKPOINT";
        case EXCEPTION_DATATYPE_MISALIGNMENT:   return "DATATYPE_MISALIGNMENT";
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:      return "FLT_DIVIDE_BY_ZERO";
        case EXCEPTION_FLT_OVERFLOW:            return "FLT_OVERFLOW";
        case EXCEPTION_FLT_UNDERFLOW:           return "FLT_UNDERFLOW";
        case EXCEPTION_ILLEGAL_INSTRUCTION:     return "ILLEGAL_INSTRUCTION";
        case EXCEPTION_INT_DIVIDE_BY_ZERO:      return "INT_DIVIDE_BY_ZERO";
        case EXCEPTION_INT_OVERFLOW:            return "INT_OVERFLOW";
        case EXCEPTION_PRIV_INSTRUCTION:        return "PRIV_INSTRUCTION";
        case EXCEPTION_STACK_OVERFLOW:          return "STACK_OVERFLOW";
        default:                                return "UNKNOWN";
    }
}

static void dump_registers(CONTEXT* ctx)
{
    emit("  rax=" + to_hex(ctx->Rax) + " rbx=" + to_hex(ctx->Rbx) + " rcx=" + to_hex(ctx->Rcx));

    emit("  rdx=" + to_hex(ctx->Rdx) + " rsi=" + to_hex(ctx->Rsi) + " rdi=" + to_hex(ctx->Rdi));

    emit("  rsp=" + to_hex(ctx->Rsp) + " rbp=" + to_hex(ctx->Rbp));

    emit("  r8=" + to_hex(ctx->R8) + " r9=" + to_hex(ctx->R9) + " r10=" + to_hex(ctx->R10));

    emit("  r11=" + to_hex(ctx->R11) + " r12=" + to_hex(ctx->R12) + " r13=" + to_hex(ctx->R13));

    emit("  r14=" + to_hex(ctx->R14) + " r15=" + to_hex(ctx->R15));

    emit("  rip=" + to_hex(ctx->Rip));
}

static void dump_raw_stack(CONTEXT* ctx)
{
    emit("  raw stack (code return addresses, game / d3d11 only):");

    uintptr_t rsp = ctx->Rsp;

    int found = 0;

    for (uintptr_t offset = 0; offset < 0x8000 && found < 80; offset += 8)
    {
        uintptr_t value = *reinterpret_cast<uintptr_t*>(rsp + offset);

        if (value >= game_base && value < game_end)
        {
            emit("    +" + to_hex(offset) + " game+" + to_hex(value - game_base));

            found++;
        }
        else if (value >= self_base && value < self_end)
        {
            emit("    +" + to_hex(offset) + " d3d11+" + to_hex(value - self_base));

            found++;
        }
    }
}

static int dump_stack(CONTEXT* ctx)
{
    STACKFRAME64 frame = {};

    frame.AddrPC.Offset = ctx->Rip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = ctx->Rbp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = ctx->Rsp;
    frame.AddrStack.Mode = AddrModeFlat;

    CONTEXT ctx_copy = *ctx;

    emit("  stack:");

    int count = 0;

    for (int i = 0; i < 16; i++)
    {
        if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, GetCurrentProcess(), GetCurrentThread(), &frame, &ctx_copy, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
        {
            break;
        }

        if (frame.AddrPC.Offset == 0)
        {
            break;
        }

        uintptr_t addr = frame.AddrPC.Offset;

        if (is_game_address(addr))
        {
            emit("    [" + std::to_string(i) + "] game+" + to_hex(addr - game_base));
        }
        else
        {
            HMODULE mod = nullptr;

            GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCSTR>(addr), &mod);

            if (mod)
            {
                char mod_name[MAX_PATH] = {};

                GetModuleFileNameA(mod, mod_name, MAX_PATH);

                std::string name = mod_name;

                size_t slash = name.find_last_of("\\/");

                if (slash != std::string::npos)
                {
                    name = name.substr(slash + 1);
                }

                emit("    [" + std::to_string(i) + "] " + name + "+" + to_hex(addr - reinterpret_cast<uintptr_t>(mod)));
            }
            else
            {
                emit("    [" + std::to_string(i) + "] " + to_hex(addr));
            }
        }

        count++;
    }

    return count;
}

static LONG WINAPI handler(EXCEPTION_POINTERS* info)
{
    DWORD code = info->ExceptionRecord->ExceptionCode;

    if (code == EXCEPTION_BREAKPOINT || code == EXCEPTION_SINGLE_STEP)
    {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    if (code < 0x80000000)
    {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    if (code == 0xE06D7363)
    {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    static thread_local bool in_handler = false;

    if (in_handler || !dump_lock_ready)
    {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    in_handler = true;

    EnterCriticalSection(&dump_lock);

    uintptr_t fault_addr = reinterpret_cast<uintptr_t>(info->ExceptionRecord->ExceptionAddress);

    std::filesystem::path directory = utils::log::root_directory() / cx("exceptions").c_str();

    std::error_code ec;

    std::filesystem::create_directories(directory, ec);

    std::string name = std::string(cx("exception-")) + stamp() + cx(".txt");

    exception_file.open((directory / name).string(), std::ios::out | std::ios::trunc);

    emit("--- EXCEPTION ---");

    emit("  code: " + std::string(code_name(code)) + " (" + to_hex(code) + ")");

    if (is_game_address(fault_addr))
    {
        emit("  address: game+" + to_hex(fault_addr - game_base));
    }
    else
    {
        emit("  address: " + to_hex(fault_addr));
    }

    if (code == EXCEPTION_ACCESS_VIOLATION && info->ExceptionRecord->NumberParameters >= 2)
    {
        uintptr_t type = info->ExceptionRecord->ExceptionInformation[0];
        uintptr_t target = info->ExceptionRecord->ExceptionInformation[1];

        const char* op = (type == 0) ? "read" : (type == 1) ? "write" : "execute";

        emit("  " + std::string(op) + " at " + to_hex(target));
    }

    dump_registers(info->ContextRecord);

    if (code == EXCEPTION_STACK_OVERFLOW)
    {
        dump_raw_stack(info->ContextRecord);
    }
    else
    {
        int frames = dump_stack(info->ContextRecord);

        if (frames < 4)
        {
            dump_raw_stack(info->ContextRecord);
        }
    }

    emit("--- END EXCEPTION ---");

    exception_file.close();

    utils::log::write(std::string(cx("exception: ")) + code_name(code) + cx(" logged to exceptions/") + name);

    LeaveCriticalSection(&dump_lock);

    in_handler = false;

    return EXCEPTION_CONTINUE_SEARCH;
}

namespace utils::exceptions
{
    void install()
    {
        game_module = GetModuleHandleA(nullptr);
        game_base = reinterpret_cast<uintptr_t>(game_module);

        PIMAGE_DOS_HEADER dos = reinterpret_cast<PIMAGE_DOS_HEADER>(game_base);
        PIMAGE_NT_HEADERS nt = reinterpret_cast<PIMAGE_NT_HEADERS>(game_base + dos->e_lfanew);

        game_end = game_base + nt->OptionalHeader.SizeOfImage;

        HMODULE self = nullptr;

        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCSTR>(&game_base), &self);

        if (self != nullptr)
        {
            self_base = reinterpret_cast<uintptr_t>(self);

            PIMAGE_DOS_HEADER self_dos = reinterpret_cast<PIMAGE_DOS_HEADER>(self_base);

            PIMAGE_NT_HEADERS self_nt = reinterpret_cast<PIMAGE_NT_HEADERS>(self_base + self_dos->e_lfanew);

            self_end = self_base + self_nt->OptionalHeader.SizeOfImage;
        }

        SymInitialize(GetCurrentProcess(), nullptr, TRUE);

        InitializeCriticalSection(&dump_lock);

        dump_lock_ready = true;

        AddVectoredExceptionHandler(1, handler);

        utils::log::write(std::string(cx("exceptions: installed (game ")) + to_hex(game_base) + cx(" - ") + to_hex(game_end) + ")");
    }
}
