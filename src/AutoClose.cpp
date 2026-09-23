#include "AutoClose.h"

#include <RE/P/PlayerCharacter.h>

#include "Dialogue.h"

namespace AutoClose {
    namespace {
        namespace ID {
            constexpr REL::ID MenuTopicManager{938421};
            constexpr REL::ID GetSmartPointer{35638};
            constexpr REL::ID ReleaseRef{38742};
            constexpr REL::ID GetDistance{63359};
            constexpr REL::ID RequestExit{117439};  // Exit button request
        }

        constexpr std::size_t kSpeakerHandleOffset = 0x20;
        constexpr std::size_t kInDialogueOffset = 0xF8;

        constexpr float kNoDistance = 1.0e30f;
        constexpr auto kInterval = std::chrono::milliseconds{100};
        constexpr auto kRetryDelay = std::chrono::seconds{1};
        constexpr auto kRestartWindow = std::chrono::seconds{2};

        using GetSmartPointer_t = void** (*)(void**, const std::uint32_t*);
        using ReleaseRef_t = void (*)(void*);
        using GetDistance_t = float (*)(const void*, const void*, bool);
        using RequestExit_t = void (*)();

        struct Game {
            const std::byte* const* menuTopicManager;
            GetSmartPointer_t getSmartPointer;
            ReleaseRef_t releaseRef;
            GetDistance_t getDistance;
            RequestExit_t requestExit;
        };

        Game g_game{};
        bool g_enabled{false};
        bool g_debug{false};
        float g_maxDistance{0.0f};
        float g_tolerance{0.0f};

        struct Session {
            std::uint32_t speaker{0};
            bool armed{false};
            bool ignored{false};
            bool tooFarOnOpen{false};
            float minDistance{0.0f};
            std::chrono::steady_clock::time_point nextCheck{};
        };
        Session g_session;
        bool g_inConversation{false};
        std::uint32_t g_lastExitSpeaker{0};
        std::chrono::steady_clock::time_point g_lastExitAt{};

        [[nodiscard]] const std::byte* ActiveManager() {
            const auto manager = *g_game.menuTopicManager;
            return manager && *reinterpret_cast<const std::uint8_t*>(manager + kInDialogueOffset) != 0 ? manager
                                                                                                       : nullptr;
        }

        class Speaker {
        public:
            explicit Speaker(std::uint32_t a_handle) {
                if (a_handle != 0) {
                    g_game.getSmartPointer(&_pointer, &a_handle);
                }
            }

            ~Speaker() {
                if (_pointer) {
                    g_game.releaseRef(_pointer);
                }
            }

            Speaker(const Speaker&) = delete;
            Speaker& operator=(const Speaker&) = delete;

            [[nodiscard]] const void* get() const noexcept { return _pointer; }

        private:
            void* _pointer{nullptr};
        };

        [[nodiscard]] float DistanceTo(std::uint32_t a_speaker) {
            const Speaker speaker{a_speaker};
            const auto player = RE::PlayerCharacter::GetSingleton();
            if (!speaker.get() || !player) {
                return -1.0f;
            }
            const auto distance = g_game.getDistance(player, speaker.get(), true);
            return std::isfinite(distance) && distance < kNoDistance ? distance : -1.0f;
        }

        void RequestExit() {
            if (const auto tasks = SFSE::GetTaskInterface()) {
                tasks->AddTask([] { g_game.requestExit(); });
            } else {
                g_game.requestExit();
            }
        }
    }

    bool Install(const Settings& a_settings) {
        if (!a_settings.autoClose) {
            return true;
        }
        if (a_settings.autoCloseDistance <= 0.0f) {
            REX::WARN("fAutoCloseDistance must be above 0, automatic close disabled");
            return false;
        }

        g_game = {
            reinterpret_cast<const std::byte* const*>(ID::MenuTopicManager.address()),
            reinterpret_cast<GetSmartPointer_t>(ID::GetSmartPointer.address()),
            reinterpret_cast<ReleaseRef_t>(ID::ReleaseRef.address()),
            reinterpret_cast<GetDistance_t>(ID::GetDistance.address()),
            reinterpret_cast<RequestExit_t>(ID::RequestExit.address()),
        };
        g_enabled = true;
        g_debug = a_settings.debugLog;
        g_maxDistance = a_settings.autoCloseDistance;
        g_tolerance = std::max(a_settings.autoCloseTolerance, 0.0f);
        return true;
    }

    void Update() {
        if (!g_enabled) {
            return;
        }

        const auto manager = ActiveManager();
        if (!manager) {
            g_inConversation = false;
            return;
        }

        const auto now = std::chrono::steady_clock::now();
        const auto speaker = *reinterpret_cast<const std::uint32_t*>(manager + kSpeakerHandleOffset);
        if (!g_inConversation || speaker != g_session.speaker) {
            g_inConversation = true;
            const bool restarted = speaker == g_lastExitSpeaker && now - g_lastExitAt < kRestartWindow;
            g_session = Session{.speaker = speaker, .ignored = restarted || Dialogue::IsCameraEnabled()};
            if (restarted && g_debug) {
                REX::INFO("AutoClose: conversation reopened by the game, left open");
            }
        }
        if (g_session.ignored || now < g_session.nextCheck) {
            return;
        }
        g_session.nextCheck = now + kInterval;

        const auto distance = DistanceTo(speaker);
        if (distance < 0.0f) {
            return;
        }

        if (!g_session.armed) {
            g_session.armed = true;
            g_session.minDistance = distance;
            g_session.tooFarOnOpen = distance > g_maxDistance;
            if (g_debug) {
                REX::INFO("AutoClose: speaker at {:.1f} m", distance);
            }
            return;
        }

        const bool leaving = !g_session.tooFarOnOpen || distance > g_session.minDistance + g_tolerance;
        if (distance > g_maxDistance && leaving) {
            g_lastExitSpeaker = speaker;
            g_lastExitAt = now;
            g_session.nextCheck = now + kRetryDelay;
            RequestExit();
            if (g_debug) {
                REX::INFO("AutoClose: {:.1f} m away, leaving the conversation", distance);
            }
            return;
        }

        g_session.minDistance = std::min(g_session.minDistance, distance);
    }
}
