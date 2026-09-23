#include "EdgeLook.h"

#include <RE/B/BSInputEventUser.h>

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
    #define NOMINMAX
#endif
#include <Windows.h>
#undef ERROR

namespace EdgeLook {
    namespace {
        namespace ID {
            constexpr REL::ID MouseMoveEventVtable{469755};
            constexpr REL::ID HasIDCode{35677};
            constexpr REL::ID QUserEvent{124035};
        }

        constexpr std::uint32_t kMouseDevice = 1;
        constexpr std::uint32_t kMouseMoveType = 1;
        constexpr std::int32_t kNoIDCode = -1;
        constexpr float kReferenceRate = 60.0f;
        constexpr float kMaxFrameTime = 0.1f;

        struct LookEvent {
            const void* vtable;
            std::uint32_t deviceType;
            std::uint32_t deviceID;
            std::uint32_t eventType;
            std::uint32_t pad14;
            RE::InputEvent* next;
            std::uint32_t timeCode;
            std::uint32_t status;
            RE::BSFixedString userEvent;
            std::int32_t idCode;
            std::uint8_t disabled;
            std::uint8_t pad35[3];
            std::int32_t x;
            std::int32_t y;
        };
        static_assert(offsetof(LookEvent, userEvent) == 0x28);
        static_assert(offsetof(LookEvent, disabled) == 0x34);
        static_assert(offsetof(LookEvent, x) == 0x38);
        static_assert(offsetof(LookEvent, y) == 0x3C);

        bool g_enabled{false};
        bool g_debug{false};
        float g_edgeSize{0.0f};
        float g_edgeSpeed{0.0f};

        LookEvent& g_event = *new LookEvent{};  // never destroyed
        bool g_attached{false};
        float g_carryX{0.0f};
        float g_carryY{0.0f};
        std::chrono::steady_clock::time_point g_lastFrame{};
        bool g_logged{false};

        [[nodiscard]] float EdgePush(float a_position) noexcept {
            if (a_position < g_edgeSize) {
                return -(g_edgeSize - a_position) / g_edgeSize;
            }
            if (a_position > 1.0f - g_edgeSize) {
                return (a_position - (1.0f - g_edgeSize)) / g_edgeSize;
            }
            return 0.0f;
        }

        [[nodiscard]] bool CursorPosition(float& a_x, float& a_y) {
            const auto window = ::GetForegroundWindow();
            DWORD process{};
            if (!window || !::GetWindowThreadProcessId(window, &process) || process != ::GetCurrentProcessId()) {
                return false;
            }

            POINT point{};
            RECT client{};
            if (!::GetCursorPos(&point) || !::ScreenToClient(window, &point) || !::GetClientRect(window, &client) ||
                client.right <= 0 || client.bottom <= 0) {
                return false;
            }
            if (point.x < 0 || point.y < 0 || point.x >= client.right || point.y >= client.bottom) {
                return false;
            }

            a_x = (static_cast<float>(point.x) + 0.5f) / static_cast<float>(client.right);
            a_y = (static_cast<float>(point.y) + 0.5f) / static_cast<float>(client.bottom);
            return true;
        }
    }

    bool Install(const Settings& a_settings) {
        if (!a_settings.edgeRotation) {
            return true;
        }
        if (!(a_settings.edgeSize > 0.0f && a_settings.edgeSize <= 0.5f && a_settings.edgeSpeed > 0.0f &&
              a_settings.edgeSpeed <= 1000.0f)) {
            REX::WARN("fEdgeSize must be in (0, 0.5] and fEdgeSpeed in (0, 1000], edge rotation disabled");
            return false;
        }

        const auto vtable = reinterpret_cast<const std::uintptr_t*>(ID::MouseMoveEventVtable.address());
        if (vtable[1] != ID::HasIDCode.address() || vtable[2] != ID::QUserEvent.address()) {
            REX::ERROR("MouseMoveEvent not recognized, edge rotation disabled");
            return false;
        }

        g_event.vtable = vtable;
        g_event.deviceType = kMouseDevice;
        g_event.eventType = kMouseMoveType;
        g_event.timeCode = static_cast<std::uint32_t>(-1);
        g_event.idCode = kNoIDCode;

        g_enabled = true;
        g_debug = a_settings.debugLog;
        g_edgeSize = a_settings.edgeSize;
        g_edgeSpeed = a_settings.edgeSpeed;
        return true;
    }

    void Reset() {
        if (g_enabled && g_event.userEvent.empty()) {
            g_event.userEvent = RE::BSFixedString{"Look"};
        }
        g_carryX = 0.0f;
        g_carryY = 0.0f;
        g_lastFrame = std::chrono::steady_clock::now();
        g_logged = false;
    }

    RE::InputEvent* Attach(RE::InputEvent* a_head) {
        if (!g_enabled) {
            return a_head;
        }

        const auto now = std::chrono::steady_clock::now();
        const auto frameTime = std::min(std::chrono::duration<float>(now - g_lastFrame).count(), kMaxFrameTime);
        g_lastFrame = now;

        float x{};
        float y{};
        const bool inside = CursorPosition(x, y);
        const auto pushX = inside ? EdgePush(x) : 0.0f;
        const auto pushY = inside ? EdgePush(y) : 0.0f;
        if (pushX == 0.0f && pushY == 0.0f) {
            g_carryX = 0.0f;
            g_carryY = 0.0f;
            return a_head;
        }

        const auto scale = g_edgeSpeed * kReferenceRate * frameTime;
        g_carryX += pushX * scale;
        g_carryY += pushY * scale;
        const auto stepX = static_cast<std::int32_t>(g_carryX);
        const auto stepY = static_cast<std::int32_t>(g_carryY);
        g_carryX -= static_cast<float>(stepX);
        g_carryY -= static_cast<float>(stepY);
        if (stepX == 0 && stepY == 0) {
            return a_head;
        }

        g_event.x = stepX;
        g_event.y = stepY;
        g_event.status = std::to_underlying(RE::InputEvent::Status::kUnhandled);
        g_event.disabled = 0;
        g_event.next = a_head;
        g_attached = true;
        return reinterpret_cast<RE::InputEvent*>(&g_event);
    }

    void Detach() {
        if (!g_attached) {
            return;
        }
        g_attached = false;
        g_event.next = nullptr;

        if (g_debug && !g_logged) {
            g_logged = true;
            const bool taken = g_event.status != std::to_underlying(RE::InputEvent::Status::kUnhandled);
            REX::INFO("EdgeLook: look event {}", taken ? "taken" : "ignored");
        }
    }
}
