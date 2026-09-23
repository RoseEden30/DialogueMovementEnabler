#include "Remap.h"

#include <RE/B/BSInputEventUser.h>

#include "AutoClose.h"
#include "Bindings.h"
#include "Dialogue.h"
#include "EdgeLook.h"
#include "VtableHook.h"

namespace Remap {
    namespace {
        namespace ID {
            constexpr REL::ID PlayerControlsVtable{433847};
            constexpr REL::ID PerformInputProcessing{82442};
            constexpr REL::ID UISingleton{937580};
        }

        constexpr std::size_t kProcessSlot = 1;
        constexpr std::uint8_t kUserEventDisabled = 4;

        // 0 while a menu blocks gameplay input
        constexpr std::size_t kGameplayInputOffset = 0x539;

        using Process_t = void (*)(void*, RE::InputEvent*);

        Process_t g_process{nullptr};
        const std::byte* const* g_ui{nullptr};
        bool g_debug{false};
        std::array<bool, std::to_underlying(Bindings::Group::kTotal)> g_allowed{};

        std::uint32_t g_seenOpenCount{0};
        bool g_active{false};
        std::uint32_t g_lastRenamed{0};

        [[nodiscard]] bool GameplayInputAllowed() noexcept {
            const auto ui = *g_ui;
            return ui && *reinterpret_cast<const std::uint8_t*>(ui + kGameplayInputOffset) != 0;
        }

        void OnDialogueOpened() {
            g_lastRenamed = 0;

            if (Dialogue::IsCameraEnabled()) {
                g_active = false;
                if (g_debug) {
                    REX::INFO("Dialogue camera is on, conversation left as is");
                }
                return;
            }

            const auto count = Bindings::Refresh();
            if (!count) {
                g_active = false;
                REX::ERROR("ControlMap not recognized, conversation left as is");
                return;
            }

            g_active = true;
            EdgeLook::Reset();
            if (g_debug) {
                REX::INFO("{} movement keys:{}", *count, Bindings::Describe());
            }
        }

        std::uint32_t RenameMovementKeys(RE::InputEvent* a_head) {
            std::uint32_t renamed = 0;
            for (auto* event = a_head; event; event = event->next) {
                if (!event->HasIDCode()) {
                    continue;
                }
                auto& id = *static_cast<RE::IDEvent*>(event);
                const auto action = Bindings::Find(std::to_underlying(id.deviceType), id.idCode);
                if (!action) {
                    continue;
                }
                auto& disabled = reinterpret_cast<std::uint8_t&>(id.disabled);
                const auto& name = Bindings::NameOf(*action);
                if (g_allowed[std::to_underlying(Bindings::GroupOf(*action))]) {
                    id.strUserEvent = name;
                    disabled = 0;
                    renamed |= 1u << std::to_underlying(*action);
                } else if (id.strUserEvent == name) {
                    disabled = kUserEventDisabled;
                }
            }
            return renamed;
        }

        void LogRenamed(std::uint32_t a_renamed) {
            if (a_renamed == g_lastRenamed) {
                return;
            }
            g_lastRenamed = a_renamed;
            std::string names;
            for (std::size_t i = 0; i < std::to_underlying(Bindings::Action::kTotal); ++i) {
                if (a_renamed & (1u << i)) {
                    names += ' ';
                    names += Bindings::TextOf(static_cast<Bindings::Action>(i));
                }
            }
            REX::INFO("Remapped:{}", names.empty() ? " none" : names);
        }

        void OnProcess(void* a_controls, RE::InputEvent* a_head) {
            AutoClose::Update();

            const bool dialogue = Dialogue::IsMenuOpen();
            if (dialogue) {
                if (const auto count = Dialogue::OpenCount(); count != g_seenOpenCount) {
                    g_seenOpenCount = count;
                    OnDialogueOpened();
                }
            }

            if (!dialogue || !g_active || !GameplayInputAllowed()) {
                g_process(a_controls, a_head);
                return;
            }

            const auto renamed = RenameMovementKeys(a_head);
            g_process(a_controls, EdgeLook::Attach(a_head));
            EdgeLook::Detach();

            if (g_debug) {
                LogRenamed(renamed);
            }
        }
    }

    bool Install(const Settings& a_settings) {
        g_debug = a_settings.debugLog;
        g_allowed[std::to_underlying(Bindings::Group::kMovement)] = a_settings.allowMovement;
        g_allowed[std::to_underlying(Bindings::Group::kRunning)] = a_settings.allowRunning;
        g_allowed[std::to_underlying(Bindings::Group::kJumping)] = a_settings.allowJumping;
        g_allowed[std::to_underlying(Bindings::Group::kSneaking)] = a_settings.allowSneaking;
        g_allowed[std::to_underlying(Bindings::Group::kPOV)] = a_settings.allowPOVSwitch;
        g_ui = reinterpret_cast<const std::byte* const*>(ID::UISingleton.address());

        g_process = VtableHook::Install<Process_t>(ID::PlayerControlsVtable, kProcessSlot, ID::PerformInputProcessing,
                                                   &OnProcess);
        if (!g_process) {
            REX::ERROR("PlayerControls not recognized, nothing is installed");
            return false;
        }
        return true;
    }
}
