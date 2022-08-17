/*
MIT License

Copyright (c) 2021-2022 L. E. Spalt

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"d2d1.lib")
#pragma comment(lib,"dcomp.lib")
#pragma comment(lib,"dwrite.lib")


#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <windows.h>
#include "iracing.h"
#include "Config.h"
#include "OverlayDebug.h"
#include "OverlayHUD.h"

enum class Hotkey
{
    UiEdit,
    Standings,
    DDU,
    HUD,
    Inputs,
    Relative,
    Cover
};

static void registerHotkeys()
{
    UnregisterHotKey( NULL, (int)Hotkey::UiEdit );
    UnregisterHotKey( NULL, (int)Hotkey::Standings );
    UnregisterHotKey( NULL, (int)Hotkey::DDU );
    UnregisterHotKey( NULL, (int)Hotkey::HUD);
    UnregisterHotKey( NULL, (int)Hotkey::Inputs );
    UnregisterHotKey( NULL, (int)Hotkey::Relative );
    UnregisterHotKey( NULL, (int)Hotkey::Cover );

    UINT vk, mod;

    if( parseHotkey( g_cfg.getString("General","ui_edit_hotkey","alt-j"),&mod,&vk) )
        RegisterHotKey( NULL, (int)Hotkey::UiEdit, mod, vk );

    if( parseHotkey( g_cfg.getString("OverlayStandings","toggle_hotkey","ctrl-space"),&mod,&vk) )
        RegisterHotKey( NULL, (int)Hotkey::Standings, mod, vk );

    if( parseHotkey( g_cfg.getString("OverlayDDU","toggle_hotkey","ctrl-1"),&mod,&vk) )
        RegisterHotKey( NULL, (int)Hotkey::DDU, mod, vk );

    if( parseHotkey( g_cfg.getString("OverlayInputs","toggle_hotkey","ctrl-2"),&mod,&vk) )
        RegisterHotKey( NULL, (int)Hotkey::Inputs, mod, vk );

    if( parseHotkey( g_cfg.getString("OverlayRelative","toggle_hotkey","ctrl-3"),&mod,&vk) )
        RegisterHotKey( NULL, (int)Hotkey::Relative, mod, vk );

    if( parseHotkey( g_cfg.getString("OverlayCover","toggle_hotkey","ctrl-4"),&mod,&vk) )
        RegisterHotKey( NULL, (int)Hotkey::Cover, mod, vk );

    if (parseHotkey(g_cfg.getString("OverlayHUD", "toggle_hotkey", "ctrl-5"), &mod, &vk))
        RegisterHotKey(NULL, (int)Hotkey::HUD, mod, vk);
}

static void handleConfigChange( std::vector<Overlay*> overlays, ConnectionStatus status )
{
    registerHotkeys();

    ir_handleConfigChange();

    for( Overlay* o : overlays )
    {
        o->enable( g_cfg.getBool(o->getName(),"enabled",true) && (
            status == ConnectionStatus::DRIVING ||
            status == ConnectionStatus::CONNECTED && o->canEnableWhileNotDriving() ||
            status == ConnectionStatus::DISCONNECTED && o->canEnableWhileDisconnected()
            ));
        o->configChanged();
    }
}

static void giveFocusToIracing()
{
    HWND hwnd = FindWindow( "SimWinClass", NULL );
    if( hwnd )
        SetForegroundWindow( hwnd );
}

int main()
{
    // Bump priority up so we get time from the sim
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

    // Load the config and watch it for changes
    g_cfg.load();
    g_cfg.watchForChanges();

    // Register global hotkeys
    registerHotkeys();

    printf("\n====================================================================================\n\n");
    printf("Fuel Config:\n");
    printf("    Auto Refuel:\t\t%s\n", g_cfg.getBool("OverlayHUD", "fuel_auto_refuel", "") ? "true": "false");
    printf("    All Laps Count:\t\t%s\n", g_cfg.getBool("OverlayHUD", "fuel_all_laps_count", "") ? "true": "false");
    printf("    Laps to Average:\t\t%.0f\n", g_cfg.getFloat("OverlayHUD", "fuel_estimate_avg_green_laps", 0.0));
    printf("    Additional Laps:\t\t%.01f\n", g_cfg.getFloat("OverlayHUD", "fuel_additional_fuel", 0.0));
    printf("\n====================================================================================\n");

    // Create overlays
    std::vector<Overlay*> overlays;
    overlays.push_back( new OverlayHUD() );
#ifdef _DEBUG
    overlays.push_back( new OverlayDebug() );
#endif

    ConnectionStatus    status          = ConnectionStatus::UNKNOWN;
    bool                uiEdit          = false;
    unsigned            frameCnt        = 0;
    int                 carIdx          = 0;
    int                 lap             = 0;

	ConnectionStatus    prevStatus      = status;
	bool                prevOnPitRoad   = false;
    SessionType         prevSessionType = SessionType::UNKNOWN;
    int                 prevLap         = 0;

    while( true )
    {
        prevStatus = status;
		prevSessionType = ir_session.sessionType;
        prevLap = lap;

        // Refresh connection and session info
        status = ir_tick();
        if( status != prevStatus )
        {
            if( status == ConnectionStatus::DISCONNECTED )
                printf("Waiting for iRacing connection...\n");
            else
                printf("iRacing connected (%s)\n", ConnectionStatusStr[(int)status]);

            // Enable user-selected overlays, but only if we're driving
            handleConfigChange( overlays, status );
        }

        dbg( "connection status: %s, session type: %s, session state: %d, pace mode: %d, on track: %d, flags: 0x%X", ConnectionStatusStr[(int)status], SessionTypeStr[(int)ir_session.sessionType], ir_SessionState.getInt(), ir_PaceMode.getInt(), (int)ir_IsOnTrackCar.getBool(), ir_SessionFlags.getInt() );

        if (status == ConnectionStatus::CONNECTED || status == ConnectionStatus::DRIVING)
        {
            prevOnPitRoad = ir_OnPitRoad.getBool();
		    carIdx = ir_session.driverCarIdx;
            lap = ir_isPreStart() ? 0 : std::max(0, ir_CarIdxLap.getInt(carIdx));
        }

        if( ir_session.sessionType != prevSessionType )
        {
            dbg("Session Type Changed from (%d) to (%d)\n", prevSessionType, ir_session.sessionType);

            for( Overlay* o : overlays )
                o->sessionChanged();
        }

        // Update/render overlays
        {
            if( !g_cfg.getBool("General", "performance_mode_30hz", false) )
            {
                // Update everything every frame, roughly every 16ms (~60Hz)
                for( Overlay* o : overlays )
                    o->update();
            }
            else
            {
                // To save perf, update half of the (enabled) overlays on even frames and the other half on odd, for ~30Hz overall
                unsigned int cnt = 0;
                for( Overlay* o : overlays )
                {
                    if( o->isEnabled() )
                        cnt++;

                    if( (cnt & 1) == (frameCnt & 1) )
                        o->update();
                }
            }
        }

        // Watch for config change signal
        if( g_cfg.hasChanged() )
        {
            g_cfg.load();
            handleConfigChange( overlays, status );
        }

        // Message pump
        MSG msg = {};
        while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            // Handle hotkeys
            if( msg.message == WM_HOTKEY )
            {
                if( msg.wParam == (int)Hotkey::UiEdit )
                {
                    uiEdit = !uiEdit;
                    for( Overlay* o : overlays )
                        o->enableUiEdit( uiEdit );

                    // When we're exiting edit mode, attempt to make iRacing the foreground window again for best perf
                    // without the user having to manually click into iRacing.
                    if( !uiEdit )
                        giveFocusToIracing();
                }
                else
                {
                    switch( msg.wParam )
                    {
                    /*case (int)Hotkey::Standings:
                        g_cfg.setBool( "OverlayStandings", "enabled", !g_cfg.getBool("OverlayStandings","enabled",true) );
                        break;
                    case (int)Hotkey::DDU:
                        g_cfg.setBool( "OverlayDDU", "enabled", !g_cfg.getBool("OverlayDDU","enabled",false) );
                        break;
                    case (int)Hotkey::Inputs:
                        g_cfg.setBool( "OverlayInputs", "enabled", !g_cfg.getBool("OverlayInputs","enabled",true) );
                        break;
                    case (int)Hotkey::Relative:
                        g_cfg.setBool( "OverlayRelative", "enabled", !g_cfg.getBool("OverlayRelative","enabled",true) );
                        break;
                    case (int)Hotkey::Cover:
                        g_cfg.setBool( "OverlayCover", "enabled", !g_cfg.getBool("OverlayCover","enabled",true) );
                        break;*/
                    case (int)Hotkey::HUD:
                        g_cfg.setBool("OverlayHUD", "enabled", !g_cfg.getBool("OverlayHUD", "enabled", true));
                        break;
                    }
                    
                    g_cfg.save();
                    handleConfigChange( overlays, status );
                }
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);            
        }

        frameCnt++;
    }

    for( Overlay* o : overlays )
        delete o;
}
