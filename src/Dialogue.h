#pragma once

#include "Settings.h"

namespace Dialogue {
    bool Install(const Settings& a_settings);

    void RegisterMenuSink();

    [[nodiscard]] bool IsMenuOpen() noexcept;

    [[nodiscard]] std::uint32_t OpenCount() noexcept;

    // bDialogueEnable:Interface
    [[nodiscard]] bool IsCameraEnabled() noexcept;
}
