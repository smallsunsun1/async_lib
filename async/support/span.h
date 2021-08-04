#ifndef ASYNC_SUPPORT_SPAN_
#define ASYNC_SUPPORT_SPAN_

#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <type_traits>

namespace sss {
namespace async {

template <typename T, typename Container>
using has_data =
    std::is_convertible<decltype(std::declval<Container>().data()), T* const*>;
template <typename T, typename Container>
inline constexpr bool has_data_v = has_data<T, Container>::value;
template <typename T>
using has_size = std::is_integral<decltype(std::declval<T>().size())>;
template <typename T>
inline constexpr bool has_size_v = has_size<T>::value;

template <typename T>
class span {
 public:
  using value_type = std::remove_cv_t<T>;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;
  using iterator = pointer;
  using const_iterator = const_pointer;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using size_type = size_t;
  using difference_type = ptrdiff_t;

  template <typename C>
  using enable_if_convetible_from =
      std::enable_if_t<has_data_v<T, C> && has_size_v<C>>;

  template <typename U>
  using enable_if_const_view = std::enable_if_t<std::is_const_v<T>, U>;

  template <typename U>
  using enable_if_mutable_view = std::enable_if_t<!std::is_const_v<T>, U>;

  static constexpr size_type npos = ~(size_type(0));

  constexpr span() noexcept : ptr_(nullptr), len_(0) {}

  constexpr span(pointer ptr, size_type length) noexcept
      : ptr_(ptr), len_(length) {}

  template <size_t N>
  constexpr span(T (&arr)[N]) noexcept : span(arr, N) {}

  template <typename Container, typename = enable_if_convetible_from<Container>,
            typename = enable_if_mutable_view<Container>>
  constexpr span(Container& container) noexcept
      : ptr_(container.data()), len_(container.size()) {}

  template <typename Container, typename = enable_if_convetible_from<Container>,
            typename = enable_if_const_view<Container>>
  constexpr span(const Container& container) noexcept
      : ptr_(container.data()), len_(container.size()) {}

  template <typename LazyT = T, typename = enable_if_const_view<LazyT>>
  span(std::initializer_list<value_type> v) noexcept
      : span(v.begin(), v.size()) {}

  constexpr pointer data() const noexcept { return ptr_; }

  constexpr size_type size() const noexcept { return len_; }

  constexpr size_type length() const noexcept { return size(); }

  constexpr bool empty() const noexcept { return size() == 0; }

  constexpr reference operator[](size_type i) const noexcept {
    assert(i < len_ && "indices must smaller than length");
    return ptr_[i];
  }

  constexpr reference at(size_type i) const noexcept {
    assert(i < len_ && "indices must smaller than length");
    return ptr_[i];
  }
  constexpr reference front() const noexcept {
    assert(len_ > 0 && "length must be greater than 0");
    return *ptr_;
  }
  constexpr reference back() const noexcept {
    assert(len_ > 0 && "length must be greater than 0");
    return ptr_[len_ - 1];
  }
  constexpr iterator begin() const noexcept { return ptr_; }
  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr iterator end() const noexcept { return ptr_ + len_ - 1; }
  constexpr const_iterator cend() const noexcept { return end(); }
  constexpr reverse_iterator rbegin() const noexcept {
    return reverse_iterator(end());
  }
  constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
  constexpr reverse_iterator rend() const noexcept {
    return reverse_iterator(begin());
  }
  constexpr const_reverse_iterator crend() const noexcept { return rend(); }
  void remove_prefix(size_type n) noexcept {
    assert(len_ >= n && "length must be larger than n");
    ptr_ += n;
    len_ -= n;
  }
  void remove_suffix(size_type n) noexcept {
    assert(len_ >= n && "length must be larger than n");
    len_ -= n;
  }
  constexpr span subspan(size_type pos = 0,
                         size_type len = npos) const noexcept {
    return (pos <= size()) ? span(data() + pos, std::min(size() - pos, len))
                           : span();
  }
  constexpr span first(size_type len) const noexcept {
    return (len <= size()) ? span(data(), len) : span();
  }
  constexpr span last(size_type len) const noexcept {
    return (len <= size()) ? span(size() - len + data(), len) : span();
  }

  bool operator==(const span& other) const {
    return ptr_ == other.ptr_ && len_ == other.len_;
  }
  bool operator!=(const span& other) const {
    return !(this->operator==(other));
  }

 private:
  pointer ptr_;
  size_type len_;
};

template <typename T>
inline constexpr typename span<T>::size_type span<T>::npos;

template <typename T>
constexpr span<T> MakeSpan(T* ptr, size_t size) noexcept {
  return span<T>(ptr, size);
}

template <typename Container>
constexpr auto MakeSpan(Container& container) noexcept {
  return MakeSpan(container.data(), container.size());
}

template <typename T, size_t N>
constexpr span<T> MakeSpan(T (&array)[N]) noexcept {
  return span<T>(array, N);
}

template <typename T>
constexpr span<T> MakeSpan(T* begin, T* end) noexcept {
  assert(end - begin > 0 && "end must be larger than begin");
  return MakeSpan(begin, std::distance(begin, end));
}

template <typename T>
constexpr span<const T> MakeConstSpan(T* ptr, size_t size) noexcept {
  return span<const T>(ptr, size);
}

template <typename Container>
constexpr auto MakeConstSpan(const Container& container) noexcept {
  return MakeConstSpan(container.data(), container.size());
}

template <typename T, size_t N>
constexpr span<const T> MakeConstSpan(T (&array)[N]) noexcept {
  return span<const T>(array, N);
}

template <typename T>
constexpr span<const T> MakeConstSpan(T* begin, T* end) noexcept {
  assert(end - begin > 0 && "end must be larger than begin");
  return MakeConstSpan(begin, std::distance(begin, end));
}

}  // namespace async
}  // namespace sss

namespace std {
template <typename T>
struct hash<sss::async::span<T>> {
  size_t operator()(sss::async::span<T> val) {
    return std::hash<T*>{}(val.data()) + std::hash<size_t>{}(val.size());
  }
};

}  // namespace std

#endif /* ASYNC_SUPPORT_SPAN_ */
