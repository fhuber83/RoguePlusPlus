#pragma once

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <list>
#include <optional>

#include "game/Id.hpp"

namespace rogue {

/*
 * How a List finds the things its Ids name, specialized for each kind of
 * thing kept in lists (game/Game.hpp does it for the pool's creatures and
 * items):
 *
 *	static T *at(Id<T> id);			// the thing, nullptr for none
 *	static std::optional<Id<T>> id_of(const T *thing);	// nullopt for none
 */
template <typename T>
struct ListPool;

/*
 * An ordered list of creatures or items, kept as Ids of things it does not
 * own (the pool in game().pool does). Replaces the intrusive l_next/l_prev
 * links. A range-for walks it as references.
 *
 * after() and before() work like the old links did: they give the neighbour
 * of an entry, or nullptr when there is none or when the entry is not in the
 * list (any more). The legacy loops rely on that. A walk such as
 *
 *	for (tp = list.first(); tp != nullptr; tp = list.after(tp))
 *
 * stops when its body detaches tp, just as a detached node's cleared l_next
 * used to stop it. A thing must be taken out of its list before it is
 * discarded, or its Id stays listed (pool_problems() reports that).
 */
template <typename T>
class List {
	using Entries = std::list<Id<T>>;

public:
	// Walks the list as references to its things
	class iterator {
	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = T;
		using difference_type = std::ptrdiff_t;
		using pointer = T *;
		using reference = T &;

		iterator() = default;
		explicit iterator(typename Entries::const_iterator it) : it_(it) {}

		T &operator*() const { return *ListPool<T>::at(*it_); }
		iterator &operator++()
		{
			++it_;
			return *this;
		}
		iterator operator++(int)
		{
			iterator old = *this;
			++it_;
			return old;
		}
		friend bool operator==(const iterator &, const iterator &) = default;

	private:
		typename Entries::const_iterator it_;
	};

	bool empty() const { return entries_.empty(); }
	std::size_t size() const { return entries_.size(); }
	iterator begin() const { return iterator(entries_.begin()); }
	iterator end() const { return iterator(entries_.end()); }

	T *first() const { return entries_.empty() ? nullptr : ListPool<T>::at(entries_.front()); }

	T *after(const T *entry) const
	{
		auto it = find(entry);
		if (it == entries_.end() || ++it == entries_.end())
			return nullptr;
		return ListPool<T>::at(*it);
	}

	T *before(const T *entry) const
	{
		auto it = find(entry);
		if (it == entries_.end() || it == entries_.begin())
			return nullptr;
		return ListPool<T>::at(*--it);
	}

	bool contains(const T *entry) const { return find(entry) != entries_.end(); }

	// Add to the front (was list_attach)
	void push_front(T *entry) { entries_.push_front(id(entry)); }

	// Take out, if it is in the list (was list_detach)
	void remove(const T *entry)
	{
		if (auto it = find(entry); it != entries_.end())
			entries_.erase(it);
	}

	// Put entry right after pos (in the list), or at the front when pos is nullptr
	void insert_after(const T *pos, T *entry)
	{
		if (pos == nullptr)
			entries_.push_front(id(entry));
		else
			entries_.insert(std::next(find(pos)), id(entry));
	}

	// Put entry right before pos, which must be in the list
	void insert_before(const T *pos, T *entry) { entries_.insert(find(pos), id(entry)); }

	void clear() { entries_.clear(); }

	// The Ids, in order, whether their things are in use or not (for checks and saves)
	const Entries &ids() const { return entries_; }

private:
	// The Id of a thing to list, which must have one
	static Id<T> id(const T *entry) { return *ListPool<T>::id_of(entry); }

	typename Entries::const_iterator find(const T *entry) const
	{
		std::optional<Id<T>> id = ListPool<T>::id_of(entry);
		if (!id)
			return entries_.end();
		return std::find(entries_.begin(), entries_.end(), *id);
	}

	Entries entries_;
};

}  // namespace rogue
