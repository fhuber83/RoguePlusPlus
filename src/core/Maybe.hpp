#pragma once

#include <concepts>
#include <optional>

namespace rogue {

/*
 * A reference to a thing that may be missing: a non-owning optional
 * reference, where the legacy code passed a pointer that can be null (no
 * weapon in hand, nothing to pick up). It has the shape of C++26's
 * std::optional<T &>, which it becomes once the compiler has it: empty by
 * default or from std::nullopt, set from a T &, tested as a bool or with
 * has_value(), and read with *, -> or value().
 *
 * maybe(pointer) makes one from a pointer that may be null, for code that
 * still holds pointers.
 */
template <typename T>
class Maybe {
public:
	constexpr Maybe() noexcept = default;
	constexpr Maybe(std::nullopt_t) noexcept {}
	constexpr Maybe(T &thing) noexcept : thing_(&thing) {}

	// A Maybe<const T> from a Maybe<T>
	template <typename U>
		requires std::convertible_to<U *, T *>
	constexpr Maybe(Maybe<U> other) noexcept : thing_(other ? &*other : nullptr)
	{
	}

	constexpr bool has_value() const noexcept { return thing_ != nullptr; }
	constexpr explicit operator bool() const noexcept { return has_value(); }

	constexpr T &operator*() const noexcept { return *thing_; }
	constexpr T *operator->() const noexcept { return thing_; }
	constexpr T &value() const
	{
		if (thing_ == nullptr)
			throw std::bad_optional_access();
		return *thing_;
	}

	constexpr void reset() noexcept { thing_ = nullptr; }

	// Whether both name the same thing (or are both empty)
	friend constexpr bool operator==(Maybe a, Maybe b) noexcept { return a.thing_ == b.thing_; }
	friend constexpr bool operator==(Maybe a, std::nullopt_t) noexcept { return !a; }

private:
	T *thing_ = nullptr;
};

template <typename T>
constexpr Maybe<T> maybe(T *thing) noexcept
{
	return thing == nullptr ? Maybe<T>() : Maybe<T>(*thing);
}

}  // namespace rogue
