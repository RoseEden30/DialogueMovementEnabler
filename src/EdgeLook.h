#pragma once

#include "Settings.h"

namespace RE {
    class InputEvent;
}

namespace EdgeLook {
    bool Install(const Settings& a_settings);

    [[nodiscard]] RE::InputEvent* Attach(RE::InputEvent* a_head);
    void Detach();

    void Reset();
}
