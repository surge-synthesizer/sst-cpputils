/* -*- mode: c++; -*-
 * ring_buffer: Ring buffers of interest.
 */

#pragma once

#include <array>
#include <atomic>
#include <functional>
#include <optional>
#include <tuple>
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

template <int N, std::memory_order MemoryOrder = std::memory_order_relaxed> class RingBufferInternal
{
    static_assert(IsPowerOfTwo(N), "N parameter must be a power of two.");

  public:
    RingBufferInternal() : subscribed_(false), writePos_(0), readPos_(0) {}

    // Empty the buffer. Does not clear subscribers.
    void clear()
    {
        readPos_ = 0;
        writePos_.store(0, MemoryOrder);
    }

    bool empty() { return readPos_ == writePos_.load(MemoryOrder); }

    std::size_t size() { return mask(writePos_.load(MemoryOrder) - readPos_); }

    // Utility functions for a reader subscribing to a buffer. A writer can check for these to avoid
    // writing to a buffer that nobody's listening from.
    void subscribe() { subscribed_.store(true); }
    void unsubscribe() { subscribed_.store(false); }
    bool subscribed() const { return subscribed_.load(); }

  protected:
    inline std::size_t mask(std::size_t val) { return val & (N - 1); }

    // Get the counts of how many elements to read from the buffer, starting from the current read
    // pointer. We need the second count in case we go past the end of the buffer, in which case it
    // will tell you how many to read from the beginning.
    std::pair<std::size_t, std::size_t> prepareToRead(std::size_t count) const
    {
        std::size_t sz1 = std::min(count, N - this->readPos_);
        std::size_t sz2 = (sz1 < count) ? (count - sz1) : 0;
        return std::make_pair(sz1, sz2);
    }

    // Get the counts of how many elements to write to the buffer, starting from the current write
    // pointer. We need the second count in case we go past the end of the buffer, in which case it
    // will tell you how many to write from the beginning. Also returns current write position, so
    // it is only loaded once.
    std::tuple<std::size_t, std::size_t, std::size_t> prepareToWrite(std::size_t count) const
    {
        std::size_t pos = this->writePos_.load(MemoryOrder);
        std::size_t sz1 = std::min(count, N - pos);
        std::size_t sz2 = (sz1 < count) ? (count - sz1) : 0;
        return std::make_tuple(pos, sz1, sz2);
    }

    std::atomic_bool subscribed_;
    std::atomic_size_t writePos_;
    std::size_t readPos_;
};
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
class SimpleRingBuffer : public internal::RingBufferInternal<N, MemoryOrder>
{
    static_assert(std::is_move_assignable_v<T>,
                  "SimpleRingBuffer requires types to be move-assignable.");
    static_assert(std::is_move_constructible_v<T>,
                  "SimpleRingBuffer requires types to be move-constructable.");

  public:
    typedef T value_type;
    SimpleRingBuffer() {}

    // Pop off the latest item in the buffer.
    std::optional<T> pop()
    {
        if (this->readPos_ != this->writePos_.load(MemoryOrder))
        {
            T item = std::move(buf_[this->readPos_]);
            this->readPos_ = this->mask(this->readPos_ + 1);
            return item;
        }

        return std::nullopt;
    }

    // Pop all existing items out of the buffer, leaves it in an empty state.
    std::vector<T> popall()
    {
        std::size_t sz = this->mask(this->writePos_.load(MemoryOrder) - this->readPos_);
        std::size_t sz1, sz2;
        std::tie(sz1, sz2) = this->prepareToRead(sz);
        std::vector<T> v(sz);
        auto it = v.begin();
        auto b = buf_.begin();
        it = std::move(b + this->readPos_, b + this->readPos_ + sz1, it);
        if (sz2)
        {
            std::move(b, b + sz2, it);
        }
        this->readPos_ = this->mask(this->readPos_ + sz);

        return v;
    }

    // Push an item into the buffer. Will clobber anything unread if it's full.
    void push(T unit)
    {
        std::size_t pos = this->writePos_.load(MemoryOrder);
        buf_[pos] = std::move(unit);
        this->writePos_.store(this->mask(pos + 1), MemoryOrder);
    }

    // Push an array of items into the buffer. Same limitations as push(). Uses std::copy so should
    // be substantially faster than calling push() with a single element in a loop.
    //
    // Only works if T is a copy-able type.
    template <typename U = T>
    typename std::enable_if_t<std::is_copy_constructible_v<U>> push(const T *units, std::size_t sz)
    {
        static_assert(std::is_same_v<U, T>);

        // Ensure there's no silliness.
        while (sz > N)
        {
            sz -= N;
            units += N;
        }

        std::size_t pos, sz1, sz2;
        std::tie(pos, sz1, sz2) = this->prepareToWrite(sz);
        std::copy(&units[0], &units[sz1], &buf_[pos]);
        pos = this->mask(pos + sz1);
        if (sz2)
        {
            std::copy(&units[sz1], &units[sz], &buf_[pos]);
            pos = this->mask(pos + sz2);
        }
        this->writePos_.store(pos, MemoryOrder);
    }

    // Convenience method for pushing vectors.
    template <typename U = T>
    typename std::enable_if_t<std::is_copy_constructible_v<U>> push(const std::vector<T> &v)
    {
        static_assert(std::is_same_v<U, T>);
        push(v.data(), v.size());
    }

  private:
    std::array<T, N> buf_;
};

// Version for stereo audio buffer. Stores each channel separately for faster pushes.
template <typename T, std::size_t N, std::memory_order MemoryOrder = std::memory_order_relaxed>
class StereoRingBuffer : public internal::RingBufferInternal<N, MemoryOrder>
{
    static_assert(std::is_move_assignable_v<T>,
                  "SimpleRingBuffer requires types to be move-assignable.");
    static_assert(std::is_move_constructible_v<T>,
                  "SimpleRingBuffer requires types to be move-constructable.");

  public:
    // Pop off the latest item in the buffer.
    std::optional<std::pair<T, T>> pop()
    {
        if (this->readPos_ != this->writePos_.load(MemoryOrder))
        {
            std::size_t pos = this->readPos_;
            std::pair<T, T> item{std::move(bufL_[pos]), std::move(bufR_[pos])};
            this->readPos_ = this->mask(this->readPos_ + 1);
            return item;
        }

        return std::nullopt;
    }

    // Pop all existing items out of the buffer, leaves it in an empty state.
    std::pair<std::vector<T>, std::vector<T>> popall()
    {
        std::size_t sz = this->mask(this->writePos_.load(MemoryOrder) - this->readPos_);
        std::size_t sz1, sz2;
        std::tie(sz1, sz2) = this->prepareToRead(sz);
        std::vector<T> vL(sz);
        std::vector<T> vR(sz);
        auto itL = vL.begin();
        auto itR = vR.begin();
        auto bL = bufL_.begin();
        auto bR = bufR_.begin();
        itL = std::move(bL + this->readPos_, bL + this->readPos_ + sz1, itL);
        itR = std::move(bR + this->readPos_, bR + this->readPos_ + sz1, itR);
        if (sz2)
        {
            std::move(bL, bL + sz2, itL);
            std::move(bR, bR + sz2, itR);
        }
        this->readPos_ = this->mask(this->readPos_ + sz);

        return std::make_pair(std::move(vL), std::move(vR));
    }

    // Push an item into the buffer. Will clobber anything unread if it's full.
    void push(T unitL, T unitR)
    {
        std::size_t pos = this->writePos_.load(MemoryOrder);
        bufL_[pos] = std::move(unitL);
        bufR_[pos] = std::move(unitR);
        this->writePos_.store(this->mask(pos + 1), MemoryOrder);
    }

    // Convenience method using a std::pair.
    void push(std::pair<T, T> unit) { push(std::move(unit.first), std::move(unit.second)); }

    // Push an array of items into the buffer. Same limitations as push(). Uses std::copy so should
    // be substantially faster than calling push() with a single element in a loop. Both arrays must
    // have at least sz elements.
    //
    // Only works if T is a copy-able type.
    template <typename U = T>
    typename std::enable_if_t<std::is_copy_constructible_v<U>> push(const T *unitsL,
                                                                    const T *unitsR, std::size_t sz)
    {
        static_assert(std::is_same_v<U, T>);

        // Ensure there's no silliness.
        while (sz > N)
        {
            sz -= N;
            unitsL += N;
            unitsR += N;
        }

        std::size_t pos, sz1, sz2;
        std::tie(pos, sz1, sz2) = this->prepareToWrite(sz);
        std::copy(&unitsL[0], &unitsL[sz1], &bufL_[pos]);
        std::copy(&unitsR[0], &unitsR[sz1], &bufR_[pos]);
        pos = this->mask(pos + sz1);
        if (sz2)
        {
            std::copy(&unitsL[sz1], &unitsL[sz], &bufL_[pos]);
            std::copy(&unitsR[sz1], &unitsR[sz], &bufR_[pos]);
            pos = this->mask(pos + sz2);
        }
        this->writePos_.store(pos, MemoryOrder);
    }

    // Convenience method for pushing vectors. Limits itself to the smaller vector.
    template <typename U = T>
    typename std::enable_if_t<std::is_copy_constructible_v<U>> push(const std::vector<T> &vL,
                                                                    const std::vector<T> &vR)
    {
        static_assert(std::is_same_v<U, T>);
        push(vL.data(), vR.data(), std::min(vL.size(), vR.size()));
    }

  private:
    std::array<T, N> bufL_;
    std::array<T, N> bufR_;
};

} // namespace cpputils
} // namespace sst
