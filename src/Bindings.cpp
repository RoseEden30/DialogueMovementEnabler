#include "Bindings.h"

#include <RE/B/BSFixedString.h>

#include <array>
#include <unordered_map>

namespace Bindings {
    namespace {
        namespace ID {
            constexpr REL::ID ControlMapSingleton{938003};
            constexpr REL::ID ControlMapVtable{469954};
        }

        constexpr std::size_t kContextTableOffset = 0x10;
        constexpr std::size_t kMainGameplay = 0;
        constexpr std::size_t kDeviceCount = 4;
        constexpr std::uint32_t kMouse = 1;
        constexpr std::int32_t kLastPickButton = 1;
        constexpr std::int32_t kNoKey = 0xFF;
        constexpr std::uint32_t kMaxKeysPerDevice = 4096;

        struct KeyMapping {
            const void* userEvent;
            std::int32_t key;
            std::int32_t modifier;
            std::byte unk10[0x18];
        };
        static_assert(sizeof(KeyMapping) == 0x28);

        struct DeviceKeys {
            std::uint32_t count;
            std::uint32_t unk04;
            const KeyMapping* keys;
        };
        static_assert(sizeof(DeviceKeys) == 0x10);

        constexpr auto kActionCount = static_cast<std::size_t>(Action::kTotal);

        constexpr std::array<std::string_view, kActionCount> kNames{
            "Forward"sv,  "Back"sv,  "StrafeLeft"sv, "StrafeRight"sv, "MoveUp"sv,
            "MoveDown"sv, "Move"sv,  "Run"sv,        "Sprint"sv,      "ToggleAlwaysRun"sv,
            "Jump"sv,     "Sneak"sv, "TogglePOV"sv,  "ZoomIn"sv,      "ZoomOut"sv};

        constexpr std::array<Group, kActionCount> kGroups{
            Group::kMovement, Group::kMovement, Group::kMovement, Group::kMovement, Group::kMovement,
            Group::kMovement, Group::kMovement, Group::kRunning,  Group::kRunning,  Group::kRunning,
            Group::kJumping,  Group::kSneaking, Group::kPOV,      Group::kZoom,     Group::kZoom};

        std::unordered_map<std::uint64_t, Action> g_keys;

        [[nodiscard]] std::uint64_t KeyOf(std::uint32_t a_device, std::int32_t a_key) noexcept {
            return (static_cast<std::uint64_t>(a_device) << 32) | static_cast<std::uint32_t>(a_key);
        }

        [[nodiscard]] std::optional<Action> Lookup(const KeyMapping& a_mapping) {
            const auto& name = *reinterpret_cast<const RE::BSFixedString*>(&a_mapping.userEvent);
            for (std::size_t i = 0; i < kActionCount; ++i) {
                if (name == kNames[i]) {
                    return static_cast<Action>(i);
                }
            }
            return std::nullopt;
        }

        [[nodiscard]] const std::byte* ControlMap() {
            const auto map = *REL::Relocation<std::byte**>{ID::ControlMapSingleton};
            if (!map) {
                return nullptr;
            }
            const auto vtable = *reinterpret_cast<const std::uintptr_t*>(map);
            return vtable == REL::Relocation<std::uintptr_t>{ID::ControlMapVtable}.address() ? map : nullptr;
        }
    }

    Group GroupOf(Action a_action) noexcept { return kGroups[static_cast<std::size_t>(a_action)]; }

    std::string_view TextOf(Action a_action) noexcept { return kNames[static_cast<std::size_t>(a_action)]; }

    const RE::BSFixedString& NameOf(Action a_action) {
        // Never destroyed
        static const auto* const names = [] {
            auto built = new std::array<RE::BSFixedString, kActionCount>;
            for (std::size_t i = 0; i < kActionCount; ++i) {
                (*built)[i] = RE::BSFixedString{kNames[i]};
            }
            return built;
        }();
        return (*names)[static_cast<std::size_t>(a_action)];
    }

    std::optional<std::size_t> Refresh() {
        g_keys.clear();

        const auto map = ControlMap();
        if (!map) {
            return std::nullopt;
        }

        const auto block = reinterpret_cast<const std::byte* const*>(map + kContextTableOffset)[kMainGameplay];
        if (!block) {
            return std::nullopt;
        }

        for (std::uint32_t device = 0; device < kDeviceCount; ++device) {
            const auto& perDevice = reinterpret_cast<const DeviceKeys*>(block)[device];
            if (!perDevice.keys || perDevice.count > kMaxKeysPerDevice) {
                continue;
            }
            for (std::uint32_t i = 0; i < perDevice.count; ++i) {
                const auto& mapping = perDevice.keys[i];
                if (mapping.key == kNoKey || mapping.modifier != kNoKey ||
                    (device == kMouse && mapping.key <= kLastPickButton)) {
                    continue;
                }
                if (const auto action = Lookup(mapping)) {
                    g_keys.try_emplace(KeyOf(device, mapping.key), *action);
                }
            }
        }
        return g_keys.size();
    }

    std::optional<Action> Find(std::uint32_t a_device, std::int32_t a_key) {
        const auto it = g_keys.find(KeyOf(a_device, a_key));
        return it == g_keys.end() ? std::nullopt : std::optional{it->second};
    }

    std::string Describe() {
        std::string text;
        for (const auto& [key, action] : g_keys) {
            text += std::format(" {}:{}={}", key >> 32, static_cast<std::int32_t>(key & 0xFFFFFFFF), TextOf(action));
        }
        return text;
    }
}
