#pragma once

#include <compare>
#include <cstddef>
#include <functional>
#include <string_view>

#define SKYBOLT_NO_RTTI
#ifndef SKYBOLT_NO_RTTI

#include <typeindex>

namespace skybolt {

using type_info = std::type_info;
using type_index = std::type_index;

template <typename T>
[[nodiscard]] const std::type_info& typeid_() noexcept
{
    return typeid(T);
}

template <typename T>
[[nodiscard]] const type_info& typeid_(const T& obj) noexcept
{
	return typeid(obj);
}

} // namespace skybolt

#else // SKYBOLT_ENABLE_RTTI

namespace skybolt {

class type_info
{
public:
	constexpr type_info() noexcept = default;

	constexpr explicit type_info(const void* id, std::string_view name = {}) noexcept
		: mId(id)
		, mName(name)
	{
	}

	[[nodiscard]] constexpr const char* name() const noexcept
	{
		return mName.data();
	}

	[[nodiscard]] constexpr std::string_view name_view() const noexcept
	{
		return mName;
	}

	[[nodiscard]] std::size_t hash_code() const noexcept
	{
		return std::hash<const void*>{}(mId);
	}

	[[nodiscard]] constexpr bool before(const type_info& other) const noexcept
	{
		return mId < other.mId;
	}

	[[nodiscard]] constexpr const void* id() const noexcept
	{
		return mId;
	}

	friend constexpr bool operator==(const type_info& a, const type_info& b) noexcept = default;

	friend constexpr auto operator<=>(const type_info& a, const type_info& b) noexcept
	{
		return a.mId <=> b.mId;
	}

private:
	const void* mId = nullptr;
	std::string_view mName{};
};

class type_index
{
public:
	constexpr type_index() noexcept = default;

	constexpr type_index(const type_info& info) noexcept
		: mInfo(info)
	{
	}

	[[nodiscard]] constexpr const char* name() const noexcept
	{
		return mInfo.name();
	}

	[[nodiscard]] std::size_t hash_code() const noexcept
	{
		return mInfo.hash_code();
	}

	[[nodiscard]] constexpr const type_info& info() const noexcept
	{
		return mInfo;
	}

	friend constexpr bool operator==(const type_index& a, const type_index& b) noexcept = default;

	friend constexpr auto operator<=>(const type_index& a, const type_index& b) noexcept
	{
		return a.mInfo <=> b.mInfo;
	}

private:
	type_info mInfo{};
};

namespace detail {

template <typename T>
struct type_tag
{
	static inline const int value = 0;
};

template <typename T>
[[nodiscard]] constexpr type_info get_type_info() noexcept
{
	using Decayed = std::remove_cv_t<std::remove_reference_t<T>>;
	return type_info(&type_tag<Decayed>::value);
}

} // namespace detail

template <typename T>
[[nodiscard]] constexpr type_info typeid_() noexcept
{
	return detail::get_type_info<T>();
}

template <typename T>
[[nodiscard]] constexpr type_info typeid_(T&&) noexcept
{
    // sizeof(T) == 0 ensures this assert only triggers if this function is actually instantiated
    static_assert(sizeof(T) == 0, 
        "typeid_(expression) requires runtime RTTI, but SKYBOLT_NO_RTTI was defined. Undefine SKYBOLT_NO_RTTI to support runtime objects, or use typeid_<T>() for compile-time types.");
    return {};
}

template <typename T>
[[nodiscard]] constexpr type_index type_index_of() noexcept
{
	return type_index(typeid_<T>());
}

} // namespace skybolt

namespace std
{

template <>
struct hash<skybolt::type_info>
{
	[[nodiscard]] std::size_t operator()(const skybolt::type_info& value) const noexcept
	{
		return value.hash_code();
	}
};

template <>
struct hash<skybolt::type_index>
{
	[[nodiscard]] std::size_t operator()(const skybolt::type_index& value) const noexcept
	{
		return value.hash_code();
	}
};


} // namespace std

#endif