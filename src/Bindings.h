#pragma once

namespace Bindings {
    enum class Action : std::uint8_t {
        kForward,
        kBack,
        kStrafeLeft,
        kStrafeRight,
        kMoveUp,
        kMoveDown,
        kMove,
        kRun,
        kSprint,
        kToggleAlwaysRun,
        kJump,
        kSneak,
        kTogglePOV,
        kZoomIn,
        kZoomOut,

        kTotal
    };

    enum class Group : std::uint8_t { kMovement, kRunning, kJumping, kSneaking, kPOV, kZoom, kTotal };

    [[nodiscard]] Group GroupOf(Action a_action) noexcept;
    [[nodiscard]] std::string_view TextOf(Action a_action) noexcept;
    [[nodiscard]] const RE::BSFixedString& NameOf(Action a_action);

    std::optional<std::size_t> Refresh();

    [[nodiscard]] std::optional<Action> Find(std::uint32_t a_device, std::int32_t a_key);

    [[nodiscard]] std::string Describe();
}
