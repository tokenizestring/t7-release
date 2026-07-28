#include "proxy/exports.g.h"

#include "engine/engine.hpp"
#include "utils/log/log.hpp"
#include "utils/crypt/crypt.hpp"
#include "utils/exceptions/exceptions.hpp"
#include "utils/resource/resource.hpp"
#include "patches/crc/crc.hpp"
#include "patches/demonware/demonware.hpp"
#include "patches/oob/oob.hpp"
#include "patches/mspreload/mspreload.hpp"
#include "patches/callvote/callvote.hpp"
#include "patches/presence/presence.hpp"
#include "patches/lobbymsg/lobbymsg.hpp"
#include "patches/matchmaking/matchmaking.hpp"
#include "patches/infoleak/infoleak.hpp"
#include "patches/inventory/inventory.hpp"
#include "patches/markup/markup.hpp"
#include "patches/notetrack/notetrack.hpp"
#include "patches/paragon/paragon.hpp"
#include "patches/p2p/p2p.hpp"
#include "patches/netchan/netchan.hpp"
#include "patches/steamqol/steamqol.hpp"
#include "patches/antiquit/antiquit.hpp"
#include "patches/hotkeys/hotkeys.hpp"
#include "patches/workshop/workshop.hpp"
#include "patches/servercmd/servercmd.hpp"
#include "patches/clientcmd/clientcmd.hpp"
#include "patches/video/video.hpp"
#include "patches/perf/perf.hpp"
#include "patches/lod/lod.hpp"
#include "patches/texstream/texstream.hpp"
#include "patches/movement/movement.hpp"
#include "patches/clientfield/clientfield.hpp"
#include "features/logo/logo.hpp"
#include "features/menulogo/menulogo.hpp"
#include "features/overlay/overlay.hpp"
#include "features/recents/recents.hpp"
#include "features/serverlist/serverlist.hpp"

static DWORD WINAPI main_thread(LPVOID param)
{
    T7_LOG_INIT();

    T7_LOG(cx("attached."));

    utils::exceptions::install();

    while (!crc::check_ready())
    {
        Sleep(100);
    }

    while (!engine::start_cutscene())
    {
        Sleep(50);
    }

    Sleep(1000);

    T7_LOG(cx("start cutscene up, installing."));

    crc::patch();

    demonware::initialize();

    oob::initialize();

    mspreload::initialize();

    callvote::initialize();

    presence::initialize();

    lobbymsg::initialize();

    matchmaking::initialize();

    infoleak::initialize();

    inventory::initialize();

    markup::initialize();

    notetrack::initialize();

    paragon::initialize();

    p2p::initialize();

    netchan::initialize();

    steamqol::initialize();

    antiquit::initialize();

    hotkeys::initialize();

    workshop::initialize();

    servercmd::initialize();

    clientcmd::initialize();

    clientfield::initialize();

    video::initialize();

    perf::initialize();

    lod::initialize();

    texstream::initialize();

    movement::initialize();

    features::logo::initialize();

    features::menulogo::initialize();

    features::overlay::initialize();

    recents::initialize();

    serverlist::initialize();

    while (true)
    {
        demonware::tick();

        antiquit::tick();

        p2p::tick();

        features::logo::tick();

        features::menulogo::tick();

        recents::tick();

        Sleep(16);
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
        {
            DisableThreadLibraryCalls(module);

            utils::resource::set_module(module);

            CreateThread(nullptr, 0, main_thread, nullptr, 0, nullptr);
        }
        break;

        case DLL_PROCESS_DETACH:
        {
            T7_LOG(cx("detached."));
        }
        break;
    }

    return TRUE;
}
