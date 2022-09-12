/* -*- mode: c++; -*-
 * ring_buffer: Ring buffers of interest.
 */

#pragma once

#include <array>
#include <atomic>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace sst
{
namespace cpputils
{

namespace internal
{
// Utility function to make sure our inputs are powers of two.
// Can't use the Juce one because we're in a split-out library.
static constexpr bool IsPowerOfTwo(size_t x) { return x && (x & (x - 1)) == 0; }
} // namespace internal

// Lock-free, single-producer, single-consumer ring buffer.
// Provides a "subscribed" method to mark when nobody's subscribed,
// optionally allowing the producer to skip writing anything.
//
// Manages its own memory. Pulling an item from the buffer will move it out.
//
// Capacity is N-1. We don't do the unmasked integer trick, in favor of not allowing the reader to
// read the same data over and over again if the writer is writing way faster than the reader is
// capable of reading.
//
// Remember that this is LOCK-FREE. That means if you let the buffer fill up, you might get some
// unexpected behavior! For example, imagine you have a full buffer:
//   8-sized buffer.
//   0 1 2 3 4 5 6 7
//   W R
//
// Now you call popall, which will pop 8 elements. But before we start popping, push() gets called
// and completes. Now the buffer looks like:
//   0 1 2 3 4 5 6 7
//    WR
//
// Now we start reading it. Our newest element is overwritten, we've skipped an element. Now imagine
// even more gets written, to where we're at:
//   0 1 2 3 4 5 6 7
//     R     W
//
// Now we return data that goes <new, new, new, old, old, old, old>. Totally out of order!
//
// This same behavior can happen with pop() over multiple calls. So if you're going to use a
// lock-free ring buffer like this class, make sure whatever you're using that reads the ring won't
// fall down and die if this case happens.
//
// TODO: We could make a version of this that doesn't write if the buffer is full, specialized on
// memory_order_seq_cst, and making readPos_ an atomic. Since readPos_ doesn't get updated until the
// item is moved out, we should be able to depend on the atomics for sequencing and that would be a
// safe variation (since the write would need to check readPos_ to check for fullness).
//
// TODO: Could make this iterator-based; I tried to avoid it since the begin/end iterators wouldn't
// behave like traditional container iterators.
template <typename T, std::size_t N, std::memory_order MemoryOrder = std::memory_order_relaxed>
class SimpleRingBuffer
{
    static_assert(std::is_move_assignable_v<T>,
                  "SimpleRingBuffer requires types to be move-assignable.");
    static_assert(std::is_move_constructible_v<T>,
                  "SimpleRingBuffer requires types to be move-constructable.");
    static_assert(internal::IsPowerOfTwo(N), "N parameter must be a power of two.");

  public:
    SimpleRingBuffer() : subscribed_(false), writePos_(0), readPos_(0) {}

    // Empty the buffer. Does not clear subscribers.
    void clear()
    {
        readPos_ = 0;
        writePos_.store(0, MemoryOrder);
    }

    bool empty() { return readPos_ == writePos_.load(MemoryOrder); }

    // Pop off the latest item in the buffer.
    std::optional<T> pop()
    {
        if (readPos_ != writePos_.load(MemoryOrder))
        {
            T item = std::move(buf_[readPos_]);
            readPos_ = mask(readPos_ + 1);
            return item;
        }

        return std::nullopt;
    }

    // Pop all existing items out of the buffer, leaves it in an empty state.
    std::vector<T> popall()
    {
        std::size_t sz = mask(writePos_.load(MemoryOrder) - readPos_);
        std::size_t sz1 = std::min(sz, N - readPos_);
        std::size_t sz2 = (sz1 < sz) ? (sz - sz1) : 0;
        std::vector<T> v(sz);
        auto it = v.begin();
        it = std::move(&buf_[readPos_], &buf_[readPos_+sz1], it);
        if (sz2)
        {
            std::move(&buf_[0], &buf_[sz2], it);
        }
        readPos_ = mask(readPos_ + sz);

        return v;
    }

    // Push an item into the buffer. Will clobber anything unread if it's full.
    void push(T unit)
    {
        std::size_t pos = writePos_.load(MemoryOrder);
        buf_[pos] = std::move(unit);
        writePos_.store(mask(pos + 1), MemoryOrder);
    }

    // Push an array of items into the buffer. Same limitations as push(). Uses std::copy so should
    // be substantially faster than calling push() with a single element in a loop.
    //
    // Only works if T is a copy-able type.
    template <typename U = T>
    typename std::enable_if_t<std::is_copy_constructible_v<U>>
    push(const T *units, std::size_t sz)
    {
        static_assert(std::is_same_v<U, T>);
        // Ensure there's no silliness.
        while (sz > N)
        {
            sz -= N;
            units += N;
        }

        std::size_t pos = writePos_.load(MemoryOrder);
        std::size_t sz1 = std::min(sz, N - pos);
        std::size_t sz2 = (sz1 < sz) ? (sz - sz1) : 0;
        std::copy(&units[0], &units[sz1], &buf_[pos]);
        pos = mask(pos + sz1);
        if (sz2)
        {
            std::copy(&units[sz1], &units[sz], &buf_[pos]);
            pos = mask(pos + sz2);
        }
        writePos_.store(pos, MemoryOrder);
    }

    // Convenience method for pushing vectors.
    template <typename U = T>
    typename std::enable_if_t<std::is_copy_constructible_v<U>>
    push(const std::vector<T> &v)
    {
        static_assert(std::is_same_v<U, T>);
        push(v.data(), v.size());
    }

    // Utility functions for a reader subscribing to a buffer. A writer can check for these to avoid
    // writing to a buffer that nobody's listening from.
    void subscribe() { subscribed_.store(true); }
    void unsubscribe() { subscribed_.store(false); }
    bool subscribed() const { return subscribed_.load(); }

  private:
    inline std::size_t mask(std::size_t val) { return val & (N - 1); }

    std::atomic_bool subscribed_;
    std::atomic_size_t writePos_;
    std::size_t readPos_;
    std::array<T, N> buf_;
};

} // namespace cpputils
} // namespace sst
