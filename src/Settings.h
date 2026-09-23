#pragma once

#include <filesystem>

struct Settings {
    bool enabled{true};
    bool allowMovement{true};
    bool allowRunning{false};
    bool allowJumping{false};
    bool allowSneaking{false};
    bool allowPOVSwitch{false};
    bool debugLog{false};

    // Metres
    bool autoClose{true};
    float autoCloseDistance{19.0f};
    float autoCloseTolerance{6.0f};

    bool edgeRotation{true};
    float edgeSize{0.12f};
    float edgeSizeBottom{0.03f};  // dialogue lines sit near the bottom
    float edgeSpeed{12.0f};

    static Settings Load(const std::filesystem::path& a_path);
};
