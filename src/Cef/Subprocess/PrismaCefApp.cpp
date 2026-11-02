#include "Cef/Subprocess/PrismaCefApp.h"

#include <algorithm>
#include <string_view>
#include <utility>

#include "Cef/Subprocess/PrismaCefRenderApp.h"
#include "include/cef_command_line.h"

namespace PrismaUI::Cef {
    namespace {
        // Lean build: Chromium parts a game HUD never uses, so they never start a process, a thread or a timer.
        // Each one can be dropped again with "!name" in PrismaUI/lean.txt.
        struct LeanSwitch {
            std::string_view name;
            std::string_view value;
        };
        constexpr LeanSwitch kLeanSwitches[] = {
            {"disable-extensions", {}},
            {"disable-component-update", {}},
            {"disable-background-networking", {}},
            {"disable-spell-checking", {}},
            {"disable-print-preview", {}},
            {"disable-default-apps", {}},
            {"no-first-run", {}},
            {"no-pings", {}},
            {"disable-domain-reliability", {}},
            {"disable-breakpad", {}},
            {"disable-site-isolation-trials", {}},
            {"renderer-process-limit", "1"},
            // GPU raster without multisampling: less GPU memory per tile
            {"gpu-rasterization-msaa-sample-count", "0"},
            {"disable-features",
             "Translate,OptimizationHints,MediaRouter,DialMediaRouteProvider,GlobalMediaControls,"
             "HardwareMediaKeyHandling,AutofillServerCommunication,CalculateNativeWinOcclusion,BackForwardCache,"
             "IsolateOrigins,site-per-process,SpareRendererForSitePerProcess"},
            {"enable-features", "NetworkServiceInProcess2"},
        };
    }

    PrismaCefApp::PrismaCefApp(std::string adapterLuidValue, std::vector<std::string> leanOverrides)
        : renderHandler_(new PrismaCefRenderApp()), adapterLuidValue_(std::move(adapterLuidValue)),
          leanOverrides_(std::move(leanOverrides)) {}

    void PrismaCefApp::OnBeforeCommandLineProcessing(const CefString& process_type,
                                                     CefRefPtr<CefCommandLine> command_line) {
        if (!command_line) {
            return;
        }

        // Browser process only (empty type): Chromium hands what its children need on to them.
        if (process_type.empty()) {
            const auto dropped = [this](std::string_view name) {
                return std::ranges::any_of(leanOverrides_, [name](const std::string& line) {
                    return line.size() == name.size() + 1 && line[0] == '!' && std::string_view(line).substr(1) == name;
                });
            };
            for (const auto& lean : kLeanSwitches) {
                if (dropped(lean.name)) {
                    continue;
                }
                if (lean.value.empty()) {
                    command_line->AppendSwitch(std::string(lean.name));
                } else {
                    command_line->AppendSwitchWithValue(std::string(lean.name), std::string(lean.value));
                }
            }
            for (const auto& line : leanOverrides_) {
                if (!line.starts_with("--")) {
                    continue;
                }
                const auto equals = line.find('=');
                if (equals == std::string::npos) {
                    command_line->AppendSwitch(line.substr(2));
                } else {
                    command_line->AppendSwitchWithValue(line.substr(2, equals - 2), line.substr(equals + 1));
                }
            }
        }

        // CEF/Chromium M138+ auto de-elevates when launched as admin: it relaunches the main
        // executable de-elevated and CefInitialize returns false (CefGetExitCode == 38,
        // CEF_RESULT_CODE_NORMAL_EXIT_AUTO_DE_ELEVATED). Inside SkyrimSE.exe that would try to
        // relaunch the game itself, ignoring browser_subprocess_path. Disable it so PrismaUI
        // initializes normally when the game/mod manager runs elevated.
        command_line->AppendSwitch("do-not-de-elevate");

        command_line->AppendSwitch("disable-smooth-scrolling");
        command_line->AppendSwitch("allow-file-access-from-files");
        command_line->AppendSwitch("allow-universal-access-from-files");
        command_line->AppendSwitchWithValue("use-gl", "angle");
        command_line->AppendSwitchWithValue("use-angle", "d3d11");

        // Force Chromium's GPU process onto the same D3D11 adapter Skyrim renders on. On
        // hybrid-GPU machines Chromium otherwise composites OSR output on a different
        // adapter, so OnAcceleratedPaint's shared NT texture cannot be opened on our
        // render device (OpenSharedResource1 fails with E_INVALIDARG) and nothing draws.
        // Value is "<HighPart>,<LowPart>" decimal; Chromium copies it to the GPU process.
        if (!adapterLuidValue_.empty()) {
            command_line->AppendSwitchWithValue("use-adapter-luid", adapterLuidValue_);
        }
    }

    CefRefPtr<CefRenderProcessHandler> PrismaCefApp::GetRenderProcessHandler() { return renderHandler_; }

    CefRefPtr<CefApp> CreatePrismaCefApp(std::string adapterLuidValue, std::vector<std::string> leanOverrides) {
        return new PrismaCefApp(std::move(adapterLuidValue), std::move(leanOverrides));
    }
}
