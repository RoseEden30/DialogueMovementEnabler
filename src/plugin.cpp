#include "AutoClose.h"
#include "Dialogue.h"
#include "EdgeLook.h"
#include "Remap.h"
#include "Settings.h"
#include "Version.h"

namespace {
    std::filesystem::path IniPath() {
        std::array<wchar_t, 4096> buffer{};
        const auto length = REX::W32::GetModuleFileNameW(REX::W32::GetCurrentModule(), buffer.data(),
                                                         static_cast<std::uint32_t>(buffer.size()));
        return std::filesystem::path{std::wstring_view{buffer.data(), length}}.replace_extension(L".ini");
    }

    const Settings& GetSettings() {
        static const Settings settings = Settings::Load(IniPath());
        return settings;
    }

    void OnMessage(SFSE::MessagingInterface::Message* a_message) {
        if (a_message->type == SFSE::MessagingInterface::kPostDataLoad) {
            Dialogue::RegisterMenuSink();
        }
    }
}

SFSE_PLUGIN_VERSION = []() noexcept {
    SFSE::PluginVersionData data{};
    data.PluginVersion(Version::VERSION);
    data.PluginName(Version::NAME);
    data.AuthorName(Version::AUTHOR);
    data.UsesAddressLibrary(true);
    data.IsLayoutDependent(true);
    data.CompatibleVersions({SFSE::RUNTIME_LATEST});
    return data;
}();

SFSE_PLUGIN_LOAD(const SFSE::LoadInterface* a_sfse) {
    SFSE::Init(a_sfse);

    const auto& settings = GetSettings();
    if (!settings.enabled) {
        REX::INFO("Disabled in the .ini");
        return true;
    }

    if (!settings.allowMovement && !settings.allowRunning && !settings.allowJumping && !settings.allowPOVSwitch &&
        !settings.autoClose && !settings.edgeRotation) {
        REX::INFO("Every feature is off in the .ini");
        return true;
    }

    if (!Remap::Install(settings)) {
        return true;
    }
    Dialogue::Install(settings);
    AutoClose::Install(settings);
    EdgeLook::Install(settings);

    if (!SFSE::GetMessagingInterface()->RegisterListener(OnMessage)) {
        REX::ERROR("Could not listen for data load, conversations are not detected");
        return true;
    }

    REX::INFO("Movement {}, running {}, jumping {}, sneaking {}, POV switch {}, auto close {}, edge rotation {}",
              settings.allowMovement, settings.allowRunning, settings.allowJumping, settings.allowSneaking,
              settings.allowPOVSwitch, settings.autoClose, settings.edgeRotation);
    return true;
}
