#include "Settings.h"

#include <SimpleIni.h>

Settings Settings::Load(const std::filesystem::path& a_path) {
    Settings settings;

    CSimpleIniA ini;
    ini.SetUnicode();
    if (ini.LoadFile(a_path.c_str()) < 0) {
        REX::WARN("{} not found, using defaults", a_path.filename().string());
        return settings;
    }

    settings.enabled = ini.GetBoolValue("General", "bEnabled", settings.enabled);
    settings.allowMovement = ini.GetBoolValue("Dialogue", "bAllowMovement", settings.allowMovement);
    settings.allowRunning = ini.GetBoolValue("Dialogue", "bAllowRunning", settings.allowRunning);
    settings.allowJumping = ini.GetBoolValue("Dialogue", "bAllowJumping", settings.allowJumping);
    settings.allowSneaking = ini.GetBoolValue("Dialogue", "bAllowSneaking", settings.allowSneaking);
    settings.allowPOVSwitch = ini.GetBoolValue("Dialogue", "bAllowPOVSwitch", settings.allowPOVSwitch);
    settings.debugLog = ini.GetBoolValue("Dialogue", "bDebugLog", settings.debugLog);

    const auto readFloat = [&ini](const char* a_section, const char* a_key, float a_default) {
        const auto value = ini.GetDoubleValue(a_section, a_key, a_default);
        return std::isfinite(value) ? static_cast<float>(value) : a_default;
    };

    settings.autoClose = ini.GetBoolValue("AutoClose", "bAutoClose", settings.autoClose);
    settings.autoCloseDistance = readFloat("AutoClose", "fAutoCloseDistance", settings.autoCloseDistance);
    settings.autoCloseTolerance = readFloat("AutoClose", "fAutoCloseTolerance", settings.autoCloseTolerance);

    settings.edgeRotation = ini.GetBoolValue("Camera", "bEdgeRotation", settings.edgeRotation);
    settings.edgeSize = readFloat("Camera", "fEdgeSize", settings.edgeSize);
    settings.edgeSizeBottom = readFloat("Camera", "fEdgeSizeBottom", settings.edgeSizeBottom);
    settings.edgeSpeed = readFloat("Camera", "fEdgeSpeed", settings.edgeSpeed);

    return settings;
}
