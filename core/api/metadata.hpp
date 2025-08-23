#pragma once

#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/detail/mp_append.hpp>
#include <boost/mp11/detail/mp_list.hpp>
#include <boost/mp11/detail/mp_rename.hpp>
#include <boost/mp11/integral.hpp>
#include <boost/mp11/list.hpp>

#include <concepts>
#include <cstddef>
#include <functional>
#include <optional>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace metadata {

struct Assignment
{
    std::string_view name;
    std::string_view exec_name;

    // NOLINTNEXTLINE(readability-identifier-naming, bugprone-easily-swappable-parameters)
    consteval explicit Assignment(std::string_view name_, std::string_view exec_name_)
        : name{name_}
        , exec_name{exec_name_} {}

    // Example:
    //   Assignment{"lab2-1.out"} -> {"lab2-1", "lab2-1.out"}
    consteval explicit Assignment(std::string_view exec_name_out)
        : name{exec_name_out.substr(0, exec_name_out.find('.'))}
        , exec_name{exec_name_out} {}

    constexpr bool operator==(const Assignment&) const = default;
};

struct ProfOnlyTag
{
    constexpr bool operator==(const ProfOnlyTag&) const = default;
};

constexpr auto ProfOnly = ProfOnlyTag{}; // NOLINT

struct Weight
{
    std::size_t amt; // you ask: what does this even mean? idk man...

    constexpr bool operator==(const Weight&) const = default;
};

namespace detail::meta {

using namespace boost::mp11;

template <typename T>
using NormalizedT = std::decay_t<T>;

template <template <typename...> class TypeList, typename... Ts>
using NormalizedTypeList = mp_rename<mp_transform<std::decay_t, mp_list<Ts...>>, TypeList>;

template <typename T>
consteval bool is_normalized() {
    return std::same_as<NormalizedT<T>, T>;
}

static_assert(
    std::same_as<NormalizedTypeList<std::tuple, int, int&, const int, const int&>, std::tuple<int, int, int, int>>);

using MetadataAttrTs = mp_list<Assignment, ProfOnlyTag, Weight>;

// The type `T` if `T` is not void, otherwise std::monostate
template <typename T>
using TypeOrMonostate = std::conditional_t<std::same_as<NormalizedT<T>, void>, std::monostate, T>;

template <std::size_t I, typename... Ts>
using Get = mp_at<mp_list<Ts...>, mp_int<I>>;

template <typename List, typename T>
static constexpr bool Has = mp_find<List, T>::value != mp_size<List>::value;

template <typename Orig, typename... New>
struct MergeUnqImpl;

template <typename Orig, typename First, typename... New>
struct MergeUnqImpl<Orig, First, New...>
{
    static constexpr bool DoAddFirst = !Has<Orig, First>;
    using WithAddedFirst = mp_push_back<Orig, First>;
    using FirstMergeRes = std::conditional_t<DoAddFirst, WithAddedFirst, Orig>;

    using type = MergeUnqImpl<FirstMergeRes, New...>::type;
};

template <typename Orig>
struct MergeUnqImpl<Orig>
{
    using type = Orig;
};

template <typename Orig, typename... New>
using MergeUnq = MergeUnqImpl<Orig, New...>::type;

static_assert(std::same_as<MergeUnq<mp_list<int>, int>, mp_list<int>>);
static_assert(std::same_as<MergeUnq<mp_list<int>, int, int>, mp_list<int>>);
static_assert(std::same_as<MergeUnq<mp_list<int>, double>, mp_list<int, double>>);
static_assert(std::same_as<MergeUnq<mp_list<int>, double, int, double>, mp_list<int, double>>);
static_assert(std::same_as<MergeUnq<mp_list<int, double>, double, int, double>, mp_list<int, double>>);

} // namespace detail::meta

template <typename... MetadataTypes>
class Metadata
{
public:
    static_assert(
        (detail::meta::is_normalized<MetadataTypes>() && ...),
        "To prevent dangling references and other issues, all type parameters of Metadata should be normalized."
        "Use the provided deduction guide.");

    template <typename... Args>
    // prevent this from becoming move/copy constructor
        requires(sizeof...(Args) != 1 ||
                 !(std::same_as<detail::meta::NormalizedT<detail::meta::Get<0, Args...>>, Metadata<MetadataTypes...>>))
    explicit consteval Metadata(Args&&... args);

    template <typename... Args>
    friend consteval auto create(Args&&... args);

    template <typename OtherMetadataType>
    consteval auto operator|(OtherMetadataType&& attr) const;

    template <typename... OtherMetadataTypes>
    consteval auto operator|(Metadata<OtherMetadataTypes...> other) const;

    template <typename MetadataType>
    constexpr std::optional<MetadataType> get() const;

    template <typename MetadataType, typename Func>
    constexpr std::optional<detail::meta::TypeOrMonostate<std::invoke_result_t<Func, MetadataType>>>
    get_and(Func&& func) const;

    template <typename T>
    static consteval bool has();

private:
    template <typename... LArgs, typename... RArgs, std::size_t I = 0>
    static consteval auto merge_impl(std::tuple<LArgs...> largs, std::tuple<RArgs...> rargs);

    template <typename... LArgs, typename RArg>
    static consteval auto merge_impl(std::tuple<LArgs...> largs, RArg&& rarg);

    template <typename... Args>
    explicit consteval Metadata(std::tuple<Args...> data);

    std::tuple<MetadataTypes...> data_;

    template <typename... Ts>
    friend class Metadata;
};

// Deduction guides
template <typename... Args>
Metadata(Args&&...) -> Metadata<detail::meta::NormalizedT<Args>...>;

template <typename... Args>
Metadata(std::tuple<Args...>) -> Metadata<Args...>;

template <typename... Args>
consteval auto create(Args&&... args) {
    return (Metadata<>{} | ... | std::forward<Args>(args));
}

// Function template definitions

template <typename... MetadataTypes>
template <typename... Args>
    requires(sizeof...(Args) != 1 ||
             !(std::same_as<detail::meta::NormalizedT<detail::meta::Get<0, Args...>>, Metadata<MetadataTypes...>>))
consteval Metadata<MetadataTypes...>::Metadata(Args&&... args)
    : data_{std::forward<Args>(args)...} {}

template <typename... MetadataTypes>
template <typename... Args>
consteval Metadata<MetadataTypes...>::Metadata(std::tuple<Args...> data)
    : data_{data} {}

template <typename... MetadataTypes>
template <typename... OtherMetadataTypes>
consteval auto Metadata<MetadataTypes...>::operator|([[maybe_unused]] Metadata<OtherMetadataTypes...> other) const {
    return (*this | ... | std::get<OtherMetadataTypes>(other.data_));
}

template <typename... MetadataTypes>
template <typename OtherMetadataType>
consteval auto Metadata<MetadataTypes...>::operator|(OtherMetadataType&& attr) const {
    using boost::mp11::mp_apply;

    auto merge_res = merge_impl(data_, std::forward<OtherMetadataType>(attr));

    using MetadataType = mp_apply<Metadata, decltype(merge_res)>;

    return MetadataType{merge_res};
}

template <typename... MetadataTypes>
template <typename MetadataType>
constexpr std::optional<MetadataType> Metadata<MetadataTypes...>::get() const {
    if constexpr (has<MetadataType>()) {
        return std::get<MetadataType>(data_);
    }

    return std::nullopt;
}

template <typename... MetadataTypes>
template <typename MetadataType, typename Func>
constexpr std::optional<detail::meta::TypeOrMonostate<std::invoke_result_t<Func, MetadataType>>>
Metadata<MetadataTypes...>::get_and(Func&& func) const {
    if constexpr (has<MetadataType>()) {
        return std::invoke(std::forward<Func>(func), *get<MetadataType>());
    }

    return std::nullopt;
}

template <typename... MetadataTypes>
template <typename T>
consteval bool Metadata<MetadataTypes...>::has() {
    using boost::mp11::mp_find, boost::mp11::mp_list;

    using TypeList = mp_list<MetadataTypes...>;
    constexpr auto IDX = mp_find<TypeList, T>::value;

    return IDX < sizeof...(MetadataTypes);
}

template <typename... MetadataTypes>
template <typename... LArgs, typename... RArgs, std::size_t I>
consteval auto Metadata<MetadataTypes...>::merge_impl(std::tuple<LArgs...> largs, std::tuple<RArgs...> rargs) {
    if constexpr (I >= sizeof...(LArgs)) {
        return largs;
    } else {
        largs = merge_impl<I + 1>(largs, rargs);

        return merge_impl(largs, std::get<I>(rargs));
    }
}

template <typename... MetadataTypes>
template <typename... LArgs, typename RArg>
consteval auto Metadata<MetadataTypes...>::merge_impl(std::tuple<LArgs...> largs, RArg&& rarg) {
    using boost::mp11::mp_find, boost::mp11::mp_list, detail::meta::NormalizedT;

    using Ls = mp_list<LArgs...>;
    using R = NormalizedT<RArg>;

    constexpr auto IDX = mp_find<Ls, R>::value;

    if constexpr (IDX == sizeof...(LArgs)) {
        // RArg type does NOT exist in largs; just concatenate, no merge
        return std::tuple_cat(largs, // could `move` largs here, but it's consteval anyways...
                              std::tuple{std::forward<RArg>(rarg)});
    } else {
        // RArg type DOES exist in largs; merge by just replacing IDX
        std::get<IDX>(largs) = std::forward<RArg>(rarg);
        return largs;
    }
}

constexpr Metadata DEFAULT_METADATA{Weight(1)};

// Each TU has its own static global definition of this free function
// Overload resolution will always prefer the non-template version of the fn,
// allowing the user to specify "file" (TU) defaults by simply overloading the fn.
template <typename T = void>
static consteval auto global_file_metadata() {
    return Metadata{};
}

} // namespace metadata
