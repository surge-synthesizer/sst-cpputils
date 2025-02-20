/*
 * sst-cpputils - an open source library of things we needed in C++
 * built by Surge Synth Team.
 *
 * Provides a collection of tools useful for writing C++-17 code
 *
 * Copyright 2022-2024, various authors, as described in the GitHub
 * transaction log.
 *
 * sst-cpputils is released under the MIT License found in the "LICENSE"
 * file in the root of this repository
 *
 * All source in sst-cpputils available at
 * https://github.com/surge-synthesizer/sst-cpputils
 */

#ifndef INCLUDE_SST_CPPUTILS_ACTIVE_SET_OVERLAY_H
#define INCLUDE_SST_CPPUTILS_ACTIVE_SET_OVERLAY_H

#include <cstdint>

namespace sst::cpputils
{

/**
 * active_set_overlay manages a small linked list of 'active' set
 * items of type T which are pre-allocated. Think voice, smoother,
 * etc... all sitting in a std::array<T, N> where at any time only
 * a small but non-contiguous subset of them is running.
 *
 * It provides methods to add and remove to the active set and
 * to traverse the active set using std iterator semantics.
 *
 * There are a variety of ways to do this, but the way I did it
 * here assumes the class T also contains utility pointers in
 * itself called `T* activeSetNext` and `activeSetPrev` which are
 * initialized to nullptr.
 */
template <typename T> struct active_set_overlay
{
    struct participant
    {
        T *activeSetNext{nullptr};
        T *activeSetPrev{nullptr};
    };

    void addToActive(T &s) { addToActive(&s); }
    void addToActive(T *s)
    {
        if (s->activeSetNext == nullptr && s->activeSetPrev == nullptr)
        {
            // Add this to the list
            if (s == activeHead)
            {
                // Nothing to do. We have a one element list
            }
            else
            {
                s->activeSetNext = activeHead;
                if (activeHead)
                {
                    activeHead->activeSetPrev = s;
                }
                activeHead = s;
                ++activeCount;
            }
        }
    }

    bool removeFromActive(T &s) { return removeFromActive(&s); }
    bool removeFromActive(T *s)
    {
        if (!(s->activeSetNext || s->activeSetPrev || s == activeHead))
        {
            return false;
        }
        --activeCount;

        if (s == activeHead)
            activeHead = s->activeSetNext;

        if (s->activeSetPrev)
        {
            s->activeSetPrev->activeSetNext = s->activeSetNext;
        }
        if (s->activeSetNext)
        {
            s->activeSetNext->activeSetPrev = s->activeSetPrev;
        }

        s->activeSetNext = nullptr;
        s->activeSetPrev = nullptr;
        return true;
    }

    void removeAll()
    {
        while (begin() != end())
        {
            removeFromActive(*begin());
        }
    }

    struct iterator
    {
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T *;
        using reference = T &;

        explicit iterator(pointer ptr = nullptr) : nodeptr(ptr) {}

        reference operator*() const { return *nodeptr; }
        pointer operator->() const { return nodeptr; }

        iterator &operator++()
        {
            if (nodeptr)
            {
                nodeptr = nodeptr->activeSetNext;
            }
            return *this;
        }

        // Post-increment
        iterator operator++(int)
        {
            iterator temp = *this;
            ++(*this);
            return temp;
        }

        bool operator==(const iterator &other) const { return nodeptr == other.nodeptr; }
        bool operator!=(const iterator &other) const { return nodeptr != other.nodeptr; }

      private:
        pointer nodeptr;
    };

    iterator begin() const { return iterator(activeHead); }
    iterator end() const { return iterator(nullptr); }

    iterator erase(const iterator &el)
    {
        auto nx = el->activeSetNext;
        removeFromActive(*el);
        return iterator(nx);
    }

    T *activeHead{nullptr};
    size_t activeCount{0}; // mostly for debugging
};

} // namespace sst::cpputils
#endif // ACTIVE_SET_OVERLAY_H
