// Extremely simple LRU cache.
//
// Has a few advantages over the myriad of others out there.
// (1) It's optionally thread safe (through a global lock, but all operations are O(1) anyway).
// (2) Has an API that will construct an object if one isn't found in the cache, thus making lookup
//     a very simple operation for the caller.

#pragma once

#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <type_traits>

namespace sst
{
namespace cpputils
{

// Key must be copy-constructible. Value is unconstrained.
template <typename Key, typename Value, bool lock_free = false> class LRU
{
    static_assert(std::is_copy_constructible_v<Key>, "Key must be copy-constructible.");

  public:
    explicit LRU(std::size_t maximum);

    // Special overload for when the key is the same as the constructor arguments.
    // Attempts to use this will fail to compile if you cannot construct a Value that way.
    std::shared_ptr<Value> get(const Key &key);

    // Regular access.
    // ConstructionArgs are the argument types to a valid constructor for the Value type.
    template <typename... ConstructionArgs>
    std::shared_ptr<Value> get(const Key &key, ConstructionArgs &&...args);

  private:
    static constexpr bool can_key_construct = (
        // Must be constructible from Key, and Key must be copyable since we use a const reference.
        (std::is_constructible_v<Value, std::remove_cv_t<Key>> &&
         std::is_copy_constructible_v<std::remove_cv_t<Key>>)
        // ...or is constructible from a const reference directly.
        || std::is_constructible_v<Value, const Key &>);

    using ListElt = std::pair<Key, std::shared_ptr<Value>>;
    using ValueIter = typename std::list<ListElt>::iterator;

    void evict();
    void to_front(ValueIter &iter);
    inline void lock();
    inline void unlock();

    std::list<ListElt> l_;
    std::unordered_map<Key, ValueIter> m_;
    const std::size_t max_;
    std::mutex lock_;
};

template <typename Key, typename Value, bool lock_free>
LRU<Key, Value, lock_free>::LRU(std::size_t maximum) : max_(maximum)
{
}

template <typename Key, typename Value, bool lock_free>
std::shared_ptr<Value> LRU<Key, Value, lock_free>::get(const Key &key)
{
    static_assert(can_key_construct, "Value must be constructible by only Key");
    lock();
    auto it = m_.find(key);
    if (it == m_.end())
    {
        auto v = std::make_shared<Value>(key);
        if (l_.size() == max_)
        {
            evict();
        }
        l_.push_front(ListElt(key, v));
        m_.emplace(key, l_.begin());
        return v;
    }
    to_front(it->second);
    auto v = it->second->second;
    unlock();
    return v;
}

template <typename Key, typename Value, bool lock_free>
template <typename... ConstructionArgs>
std::shared_ptr<Value> LRU<Key, Value, lock_free>::get(const Key &key, ConstructionArgs &&...args)
{
    static_assert(std::is_constructible_v<Value, ConstructionArgs...>);
    lock();
    auto it = m_.find(key);
    if (it == m_.end())
    {
        auto v = std::make_shared<Value>(std::forward<ConstructionArgs>(args)...);
        if (l_.size() == max_)
        {
            evict();
        }
        l_.push_front(ListElt(key, v));
        m_.emplace(key, l_.begin());
        return v;
    }
    to_front(it->second);
    auto v = it->second->second;
    unlock();
    return v;
}

template <typename Key, typename Value, bool lock_free> void LRU<Key, Value, lock_free>::evict()
{
    auto elt = l_.back();
    m_.erase(elt.first);
    l_.pop_back();
}

template <typename Key, typename Value, bool lock_free>
void LRU<Key, Value, lock_free>::to_front(ValueIter &iter)
{
    l_.splice(l_.begin(), l_, iter);
}

template <typename Key, typename Value, bool lock_free>
inline void LRU<Key, Value, lock_free>::lock()
{
    if constexpr(!lock_free)
    {
        lock_.lock();
    }
}

template <typename Key, typename Value, bool lock_free>
inline void LRU<Key, Value, lock_free>::unlock()
{
    if constexpr(!lock_free)
    {
        lock_.unlock();
    }
}

} // namespace cpputils
} // namespace sst
