#pragma once

#include <algorithm>
#include <cstddef>
#include <list>

namespace rogue {

/*
 * An ordered list of creatures or items that it does not own (the pool in
 * game().pool does). Replaces the intrusive l_next/l_prev links.
 *
 * after() and before() work like the old links did: they give the neighbour
 * of an entry, or nullptr when there is none or when the entry is not in the
 * list (any more). The legacy loops rely on that. A walk such as
 *
 *	for (tp = list.first(); tp != nullptr; tp = list.after(tp))
 *
 * stops when its body detaches tp, just as a detached node's cleared l_next
 * used to stop it. Legacy code uses the attach() and detach() macros of
 * rogue.h, which call push_front() and remove().
 *
 * Member names avoid the lowercase macros of the legacy headers (next, prev,
 * max, ...).
 */
template <typename T>
class List {
public:
	using iterator = typename std::list<T *>::const_iterator;

	bool empty() const { return entries_.empty(); }
	std::size_t size() const { return entries_.size(); }
	iterator begin() const { return entries_.begin(); }
	iterator end() const { return entries_.end(); }

	T *first() const { return entries_.empty() ? nullptr : entries_.front(); }

	T *after(const T *entry) const
	{
		auto it = find(entry);
		if (it == entries_.end() || ++it == entries_.end())
			return nullptr;
		return *it;
	}

	T *before(const T *entry) const
	{
		auto it = find(entry);
		if (it == entries_.end() || it == entries_.begin())
			return nullptr;
		return *--it;
	}

	bool contains(const T *entry) const { return find(entry) != entries_.end(); }

	// Add to the front (was list_attach)
	void push_front(T *entry) { entries_.push_front(entry); }

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
			entries_.push_front(entry);
		else
			entries_.insert(std::next(find(pos)), entry);
	}

	// Put entry right before pos, which must be in the list
	void insert_before(const T *pos, T *entry) { entries_.insert(find(pos), entry); }

	void clear() { entries_.clear(); }

private:
	iterator find(const T *entry) const
	{
		return std::find(entries_.begin(), entries_.end(), entry);
	}

	std::list<T *> entries_;
};

}  // namespace rogue
