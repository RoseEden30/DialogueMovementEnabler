#include "Dialogue.h"

#include <RE/Starfield.h>

#include "VtableHook.h"

namespace Dialogue {
    namespace {
        namespace ID {
            constexpr REL::ID MenuVtable{460296};
            constexpr REL::ID MenuOpen{113829};
            constexpr REL::ID MenuInstance{938423};
            constexpr REL::ID CameraSetting{923936};
        }

        constexpr std::size_t kMenuOpenSlot = 7;
        constexpr std::size_t kMenuLayerOffset = 0x688;
        constexpr std::size_t kSettingValue = 0x8;

        using Handler_t = void (*)(void*);

        Handler_t g_menuOpen{nullptr};
        std::byte** g_menuInstance{nullptr};
        const std::uint8_t* g_cameraSetting{nullptr};
        RE::USER_EVENT_FLAG g_restore{};
        std::atomic<bool> g_open{false};
        std::atomic<std::uint32_t> g_openCount{0};

        class MenuSink final : public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
        public:
            RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent& a_event,
                                                  RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override {
                if (a_event.menuName == _name) {
                    if (a_event.opening) {
                        g_openCount.fetch_add(1, std::memory_order_relaxed);
                    }
                    g_open.store(a_event.opening, std::memory_order_release);
                }
                return RE::BSEventNotifyControl::kContinue;
            }

        private:
            RE::BSFixedString _name{"DialogueMenu"};
        };

        void OnMenuOpen(void* a_menu) {
            g_menuOpen(a_menu);
            if (IsCameraEnabled()) {
                return;
            }

            const auto menu = *g_menuInstance;
            const auto layer = menu ? *reinterpret_cast<std::uint32_t**>(menu + kMenuLayerOffset) : nullptr;
            if (layer) {
                RE::BSInputEnableManager::EnableUserEvent(layer, g_restore, true, RE::USER_EVENT_SENDER_ID::Menu);
            }
        }
    }

    bool Install(const Settings& a_settings) {
        g_cameraSetting = reinterpret_cast<const std::uint8_t*>(ID::CameraSetting.address() + kSettingValue);
        g_menuInstance = reinterpret_cast<std::byte**>(ID::MenuInstance.address());

        if (a_settings.allowMovement || a_settings.allowRunning) {
            g_restore |= RE::USER_EVENT_FLAG::Walking;
        }
        if (a_settings.allowJumping) {
            g_restore |= RE::USER_EVENT_FLAG::Jumping;
        }
        if (a_settings.allowPOVSwitch) {
            g_restore |= RE::USER_EVENT_FLAG::POVSwitch;
        }
        if (g_restore == RE::USER_EVENT_FLAG{}) {
            return true;
        }

        g_menuOpen = VtableHook::Install<Handler_t>(ID::MenuVtable, kMenuOpenSlot, ID::MenuOpen, &OnMenuOpen);
        if (!g_menuOpen) {
            REX::ERROR("DialogueMenu not recognized, its input layer is left as is");
        }
        return g_menuOpen != nullptr;
    }

    void RegisterMenuSink() {
        const auto ui = RE::UI::GetSingleton();
        if (!ui) {
            REX::ERROR("UI not available, conversations are not detected");
            return;
        }
        // Never destroyed
        ui->RegisterSink<RE::MenuOpenCloseEvent>(new MenuSink);
    }

    bool IsMenuOpen() noexcept { return g_open.load(std::memory_order_acquire); }

    std::uint32_t OpenCount() noexcept { return g_openCount.load(std::memory_order_relaxed); }

    bool IsCameraEnabled() noexcept { return g_cameraSetting && *g_cameraSetting != 0; }
}
