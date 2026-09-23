#pragma once

namespace VtableHook {
    template <class Fn>
    [[nodiscard]] Fn Install(REL::ID a_vtable, std::size_t a_slot, REL::ID a_expected, auto a_replacement) {
        REL::Relocation<std::uintptr_t> vtable{a_vtable};
        const auto current = *reinterpret_cast<const std::uintptr_t*>(vtable.address() + sizeof(void*) * a_slot);
        if (current != a_expected.address()) {
            return nullptr;
        }
        return reinterpret_cast<Fn>(vtable.write_vfunc(a_slot, a_replacement));
    }
}
