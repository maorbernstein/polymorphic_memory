#ifndef POLYMORPHIC_MEMORY_HPP
#define POLYMORPHIC_MEMORY_HPP

#include <memory>
#include <type_traits>

namespace polymorphic_memory {

namespace detail {
template <int I, class... Types>
struct ith;

template <class T, class... Remaining>
struct ith<0, T, Remaining...> {
    using type = T;
};

template <int I, class T, class... Remaining>
struct ith<I, T, Remaining...> {
    using type = typename ith<I - 1, Remaining...>::type;
};

template <int I, class... Types>
using ith_t = typename ith<I, Types...>::type;

template <class T, class... Types>
struct type_index;

template <class Key>
struct type_index<Key> {
  static_assert("Type not found!");
};

template <class Key, class... Remaining>
struct type_index<Key, Key, Remaining...> {
  static constexpr int value = 0;
};

template <class Key, class Current, class... Remaining>
struct type_index<Key, Current, Remaining...> {
  static constexpr int value = 1 + type_index<Key, Remaining...>::value;
};

template <class T, class... Types>
constexpr int type_index_v = type_index<T, Types...>::value;

template <class Base, class... Derived>
struct are_all_derived;

template <class Base>
struct are_all_derived<Base> {
    static constexpr bool value = true;
};

template <class Base, class Derived, class... Remaining>
struct are_all_derived<Base, Derived, Remaining...> {
    static constexpr bool value = std::is_base_of_v<Base, Derived> && are_all_derived<Base, Remaining...>::value;
};

template <class Base, class... Derived>
constexpr bool are_all_derived_v = are_all_derived<Base, Derived...>::value;

template <class Key, class... PossibleValues>
struct is_one_of;

template <class Key>
struct is_one_of<Key> {
    static constexpr bool value = false;
};

template <class Key, class PossibleValue, class... Remaining>
struct is_one_of<Key, PossibleValue, Remaining...> {
    static constexpr bool value = std::is_same_v<std::decay_t<Key>, PossibleValue>> || is_one_of<Key, Remaining...>::value;
};

template <class Key, class... PossibleValues>
constexpr bool is_one_of_v = is_one_of<Key, PossibleValues...>::value;

template <class... Types>
struct contains_duplicates;

template <>
struct contains_duplicates<>  {
    static constexpr bool value = false;
};

template <class First, class... Rest>
struct contains_duplicates<First, Rest...> {
    static constexpr bool value = is_one_of_v<First, Rest...> || contains_duplicates<Rest...>::value;
};

template <class... Types>
constexpr bool contains_duplicates_v = contains_duplicates<Types...>::value;

template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};

template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

template <class PolymorphicType>
std::unique_ptr<typename PolymorphicType::base_type> make_ith_copy(
    const PolymorphicType &, int) {
  assert(0);  // Invalid index
}

template <class PolymorphicType, class Current, class... Rest>
std::unique_ptr<typename PolymorphicType::base_type> make_ith_copy(
    const PolymorphicType &original, int index) {
  assert((index != -1) == (original.has_value()));
  if (!original.has_value()) {
    return nullptr;
  }
  if (index == 0) {
    const auto &casted = original.template downcast<Current>();
    return std::make_unique<Current>(casted);
  }
  return make_ith_copy<PolymorphicType, Rest...>(original, index - 1);
}

template <class InputPolymorphicType, class OutputPolymorphicType>
OutputPolymorphicType make_other_copy(const InputPolymorphicType &, int) {
  assert(0);  // Invalid index
}

template <class InputPolymorphicType, class OutputPolymorphicType,
          class Current, class... Rest>
OutputPolymorphicType make_other_copy(const InputPolymorphicType &original,
                                      int index) {
  assert((index != -1) == (original.has_value()));
  if (!original.has_value()) {
    return OutputPolymorphicType{};
  }
  if (index == 0) {
    const auto &casted = original.template downcast<Current>();
    return OutputPolymorphicType(casted);
  }
  return make_other_copy<InputPolymorphicType, OutputPolymorphicType, Rest...>(
      original, index - 1);
}

}

    
template <class Base, class... DerivedClasses>
class static_ptr {
    std::variant<std::monostate, DerivedClasses...> storage;
    static_assert(are_all_derived_v<Base, DerivedClasses...>);
    static_assert(!contains_duplicates_v<DerivedClasses...>);

    public:
    using base_type = Base;

    static_ptr() = default;
    template <
        class DerivedClass,
        class = std::enable_if_t<detail::is_one_of_v<DerivedClass, DerivedClasses...>>>
    static_ptr(DerivedClass &&derived)
        : storage(std::forward<DerivedClass>(derived)) {}
    template <
        class DerivedClass,
        class = std::enable_if_t<detail::is_one_of_v<DerivedClass, DerivedClasses...>>>
    bool is_derived() const {
    return std::holds_alternative<DerivedClass>(storage);
    }
    template <class DerivedClass,
            class = std::enable_if_t<std::is_base_of_v<Base, DerivedClass>>>
    DerivedClass &unsafe_downcast() {
    return static_cast<DerivedClass &>(*get());
    }
    template <class DerivedClass,
            class = std::enable_if_t<std::is_base_of_v<Base, DerivedClass>>>
    const DerivedClass &unsafe_downcast() const {
    return static_cast<const DerivedClass &>(*get());
    }
    template <class DerivedClass,
            class = std::enable_if_t<std::is_base_of_v<Base, DerivedClass>>>
    DerivedClass &downcast() {
    return dynamic_cast<DerivedClass &>(*get());
    }
    template <class DerivedClass,
            class = std::enable_if_t<std::is_base_of_v<Base, DerivedClass>>>
    const DerivedClass &downcast() const {
    return dynamic_cast<const DerivedClass &>(*get());
    }
    template <class DerivedClass,
            class = std::enable_if_t<std::is_base_of_v<Base, DerivedClass>>>
    DerivedClass *downcast_if() {
    return dynamic_cast<DerivedClass *>(get());
    }
    template <class DerivedClass,
            class = std::enable_if_t<std::is_base_of_v<Base, DerivedClass>>>
    const DerivedClass *downcast_if() const {
    return dynamic_cast<const DerivedClass *>(get());
    }
    template <
        class DerivedClass,
        class = std::enable_if_t<detail::is_one_of_v<DerivedClass, DerivedClasses...>>>
    DerivedClass unsafe_downcast_copy() const {
    return DerivedClass(unsafe_downcast<DerivedClass>());
    }
    template <
        class DerivedClass,
        class = std::enable_if_t<detail::is_one_of_v<DerivedClass, DerivedClasses...>>>
    DerivedClass downcast_copy() const {
    return DerivedClass(downcast<DerivedClass>());
    }
    template <
        class DerivedClass,
        class = std::enable_if_t<detail::is_one_of_v<DerivedClass, DerivedClasses...>>>
    std::optional<DerivedClass> downcast_if_copy() const {
    const auto derived_ptr = downcast_if<DerivedClass>();
    if (derived_ptr) {
        return *derived_ptr;
    }
    return std::nullopt;
    }
    Base *operator->() {
    return std::visit(
        overloaded{
            [](auto &arg) { return static_cast<Base *>(&arg); },
            [](std::monostate &) { return static_cast<Base *>(nullptr); }},
        storage);
    }
    const Base *operator->() const {
    return std::visit(overloaded{[](const auto &arg) {
                                    return static_cast<const Base *>(&arg);
                                    },
                                    [](const std::monostate &) {
                                    return static_cast<const Base *>(nullptr);
                                    }},
                        storage);
    }
    Base *get() { return operator->(); }
    const Base *get() const { return operator->(); }
    explicit operator bool() const { return operator->(); }
    bool has_value() const { return operator->(); }
    Base &operator*() { return *(operator->()); }
    const Base &operator*() const { return *(operator->()); }
};


template <class Base, class... DerivedClasses>
class dynamic_ptr {
    std::unique_ptr<Base> storage;
    int stored_type_index = -1;

    public:
    using base_type = Base;

    dynamic_ptr() = default;
    dynamic_ptr(const dynamic_ptr &to_copy)
        : storage(make_ith_copy<
                dynamic_ptr<Base, DerivedClasses...>,
                DerivedClasses...>(to_copy, to_copy.stored_type_index)),
        stored_type_index(to_copy.stored_type_index) {}
    dynamic_ptr &operator=(
        dynamic_ptr to_copy) {
    swap(*this, to_copy);
    return *this;
    }
    template <
        class DerivedClass,
        class = std::enable_if_t<detail::is_one_of_v<DerivedClass, DerivedClasses...>>>
    dynamic_ptr(DerivedClass &&derived)
        : storage(std::make_unique<std::decay_t<DerivedClass>>(
            std::forward<DerivedClass>(derived))),
        stored_type_index(
            detail::type_index_v<std::decay_t<DerivedClass>, DerivedClasses...>) {}
    int index() const { return stored_type_index; }
    template <
        class DerivedClass,
        class = std::enable_if_t<detail::is_one_of_v<DerivedClass, DerivedClasses...>>>
    bool is_derived() const {
    return stored_type_index ==
            detail::type_index_v<std::decay_t<DerivedClass>, DerivedClasses...>;
    }
    template <class DerivedClass,
            class = std::enable_if_t<std::is_base_of_v<Base, DerivedClass>>>
    DerivedClass &unsafe_downcast() {
    return static_cast<DerivedClass &>(*storage);
    }
    template <class DerivedClass,
            class = std::enable_if_t<std::is_base_of_v<Base, DerivedClass>>>
    const DerivedClass &unsafe_downcast() const {
    return static_cast<const DerivedClass &>(*storage);
    }
    template <class DerivedClass,
            class = std::enable_if_t<std::is_base_of_v<Base, DerivedClass>>>
    DerivedClass &downcast() {
    return dynamic_cast<DerivedClass &>(*storage);
    }
    template <class DerivedClass,
            class = std::enable_if_t<std::is_base_of_v<Base, DerivedClass>>>
    const DerivedClass &downcast() const {
    return dynamic_cast<const DerivedClass &>(*storage);
    }
    template <class DerivedClass,
            class = std::enable_if_t<std::is_base_of_v<Base, DerivedClass>>>
    DerivedClass *downcast_if() {
    return dynamic_cast<DerivedClass *>(get());
    }
    template <class DerivedClass,
            class = std::enable_if_t<std::is_base_of_v<Base, DerivedClass>>>
    const DerivedClass *downcast_if() const {
    return dynamic_cast<const DerivedClass *>(get());
    }
    template <
        class DerivedClass,
        class = std::enable_if_t<detail::is_one_of_v<DerivedClass, DerivedClasses...>>>
    DerivedClass unsafe_downcast_copy() const {
    return DerivedClass(unsafe_downcast<DerivedClass>());
    }
    template <
        class DerivedClass,
        class = std::enable_if_t<detail::is_one_of_v<DerivedClass, DerivedClasses...>>>
    DerivedClass downcast_copy() const {
    return DerivedClass(downcast<DerivedClass>());
    }
    template <
        class DerivedClass,
        class = std::enable_if_t<detail::is_one_of_v<DerivedClass, DerivedClasses...>>>
    std::optional<DerivedClass> downcast_if_copy() const {
    const auto derived_ptr = downcast_if<DerivedClass>();
    if (derived_ptr) {
        return *derived_ptr;
    }
    return std::nullopt;
    }
    Base *operator->() { return storage.operator->(); }
    const Base *operator->() const { return storage.operator->(); }
    Base *get() { return operator->(); }
    const Base *get() const { return operator->(); }
    bool has_value() const { return operator->(); }
    Base &operator*() { return *(operator->()); }
    const Base &operator*() const { return *(operator->()); }
    friend void swap(dynamic_ptr &first,
                    dynamic_ptr &second) {
    std::swap(first.storage, second.storage);
    std::swap(first.stored_type_index, second.stored_type_index);
    }
    template <
        class OtherBase, class... OtherDerivedClasses,
        class = std::enable_if_t<
            std::is_base_of<Base, OtherBase> || std::is_base_of<OtherBase, Base>>>
    operator dynamic_ptr<OtherBase, OtherDerivedClasses...>()
        const {
    return detail::make_other_copy<
        dynamic_ptr<Base, DerivedClasses...>,
        dynamic_ptr<OtherBase, OtherDerivedClasses...>,
        DerivedClasses...>(*this, stored_type_index);
    }
};
    

}


#endif