//
// Created by Paul Walker on 1/8/22.
//

#ifndef SST_CPPUTILS_STRINGS_H
#define SST_CPPUTILS_STRINGS_H

namespace sst
{
namespace cpputils
{

/*
 * sst::cpputils::strsplit is a (perhaps overcopying) strsplit operator which
 * you can vectorize over.
 *
 * ```
 * for (auto q : sst::cpputils::strsplit(std::string("a,fun,program"), ','))
 * {
 *    std::cout << q << std::endl;
 * }
 * ```
 *
 * will print "a\nfun\nprogram"; bit it works on any iterable with comparable content
 * including vectors. See the tests for examples.
 */

namespace internal
{
template <typename T, typename C, typename TIter = decltype(std::begin(std::declval<T>()))>
struct strsplit_iterator
{
    strsplit_iterator(const TIter &&from, const TIter &&to, const C &o)
        : iter(from), endit(to), on(o)
    {
        updateSpan();
    }
    strsplit_iterator(bool f) : isEnd{f} {};
    TIter iter, endit;
    C on;

    TIter spanStart, spanEnd;
    T contents;
    bool isEnd{false};
    void updateSpan()
    {
        spanStart = iter;
        spanEnd = iter;
        while (*spanEnd != on && spanEnd != endit)
            *spanEnd++;
        contents = T(spanStart, spanEnd);
    }
    bool operator!=(const strsplit_iterator &other) const
    {
        if (isEnd == other.isEnd && isEnd)
            return false;
        if (isEnd || other.isEnd)
            return true;

        return iter != other.iter;
    }
    void operator++()
    {
        if (spanEnd == endit)
        {
            isEnd = true;
        }
        else
        {
            iter = std::next(spanEnd);
            updateSpan();
        }
    }
    auto &operator*() const { return contents; }
};
} // namespace internal

template <typename T, typename C, typename TIter = decltype(std::begin(std::declval<T>())),
          typename = decltype(std::end(std::declval<T>()))>
auto strsplit(const T &string, const C &on)
{

    struct string_split_wrapper
    {
        const T &string;
        C on;
        auto begin() const
        {
            return internal::strsplit_iterator<T, C>{std::begin(string), std::end(string), on};
        }
        auto end() const { return internal::strsplit_iterator<T, C>{true}; }
    };
    return string_split_wrapper{string, on};
}

} // namespace cpputils
} // namespace sst
#endif // SST_CPPUTILS_STRINGS_H
