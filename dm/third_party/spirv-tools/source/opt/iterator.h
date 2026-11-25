// Copyright (c) 2016 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SOURCE_OPT_ITERATOR_H_
#define SOURCE_OPT_ITERATOR_H_

#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace spvtools {
namespace opt {





template <typename ValueType, bool IsConst = false>
class UptrVectorIterator {
 public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = ValueType;

  using pointer = value_type*;
  using reference = value_type&;
  using difference_type = std::ptrdiff_t;


  using Uptr = std::unique_ptr<ValueType>;
  using UptrVector = typename std::conditional<IsConst, const std::vector<Uptr>,
                                               std::vector<Uptr>>::type;
  using UnderlyingIterator =
      typename std::conditional<IsConst, typename UptrVector::const_iterator,
                                typename UptrVector::iterator>::type;



  UptrVectorIterator(UptrVector* container, const UnderlyingIterator& it)
      : container_(container), iterator_(it) {}
  UptrVectorIterator(const UptrVectorIterator&) = default;
  UptrVectorIterator& operator=(const UptrVectorIterator&) = default;

  inline UptrVectorIterator& operator++();
  inline UptrVectorIterator operator++(int);
  inline UptrVectorIterator& operator--();
  inline UptrVectorIterator operator--(int);

  reference operator*() const { return **iterator_; }
  pointer operator->() { return (*iterator_).get(); }
  reference operator[](ptrdiff_t index) { return **(iterator_ + index); }

  inline bool operator==(const UptrVectorIterator& that) const;
  inline bool operator!=(const UptrVectorIterator& that) const;

  inline ptrdiff_t operator-(const UptrVectorIterator& that) const;
  inline bool operator<(const UptrVectorIterator& that) const;






  template <bool IsConstForMethod = IsConst>
  inline typename std::enable_if<!IsConstForMethod, UptrVectorIterator>::type
  InsertBefore(Uptr value);






  template <bool IsConstForMethod = IsConst>
  inline typename std::enable_if<!IsConstForMethod, UptrVectorIterator>::type
  InsertBefore(UptrVector* valueVector);






  template <bool IsConstForMethod = IsConst>
  inline typename std::enable_if<!IsConstForMethod, UptrVectorIterator>::type
  Erase();


  UnderlyingIterator Get() const { return iterator_; }


  UptrVectorIterator End() const {
    return UptrVectorIterator(container_, container_->end());
  }

 private:
  UptrVector* container_;
  UnderlyingIterator iterator_;
};


template <typename IteratorType>
class IteratorRange {
 public:
  IteratorRange(const IteratorType& b, const IteratorType& e)
      : begin_(b), end_(e) {}
  IteratorRange(IteratorType&& b, IteratorType&& e)
      : begin_(std::move(b)), end_(std::move(e)) {}

  IteratorType begin() const { return begin_; }
  IteratorType end() const { return end_; }

  bool empty() const { return begin_ == end_; }
  size_t size() const { return end_ - begin_; }

 private:
  IteratorType begin_;
  IteratorType end_;
};



template <typename IteratorType>
inline IteratorRange<IteratorType> make_range(const IteratorType& begin,
                                              const IteratorType& end) {
  return {begin, end};
}



template <typename IteratorType>
inline IteratorRange<IteratorType> make_range(IteratorType&& begin,
                                              IteratorType&& end) {
  return {std::forward<IteratorType>(begin), std::forward<IteratorType>(end)};
}


template <typename ValueType,
          class IteratorType = UptrVectorIterator<ValueType>>
inline IteratorRange<IteratorType> make_range(
    std::vector<std::unique_ptr<ValueType>>& container) {
  return {IteratorType(&container, container.begin()),
          IteratorType(&container, container.end())};
}


template <typename ValueType,
          class IteratorType = UptrVectorIterator<ValueType, true>>
inline IteratorRange<IteratorType> make_const_range(
    const std::vector<std::unique_ptr<ValueType>>& container) {
  return {IteratorType(&container, container.cbegin()),
          IteratorType(&container, container.cend())};
}







template <typename SubIterator, typename Predicate>
class FilterIterator {
 public:

  using iterator_category = typename SubIterator::iterator_category;
  using value_type = typename SubIterator::value_type;
  using pointer = typename SubIterator::pointer;
  using reference = typename SubIterator::reference;
  using difference_type = typename SubIterator::difference_type;

  using Range = IteratorRange<FilterIterator>;

  FilterIterator(const IteratorRange<SubIterator>& iteration_range,
                 Predicate predicate)
      : cur_(iteration_range.begin()),
        end_(iteration_range.end()),
        predicate_(predicate) {
    if (!IsPredicateSatisfied()) {
      MoveToNextPosition();
    }
  }

  FilterIterator(const SubIterator& end, Predicate predicate)
      : FilterIterator({end, end}, predicate) {}

  inline FilterIterator& operator++() {
    MoveToNextPosition();
    return *this;
  }
  inline FilterIterator operator++(int) {
    FilterIterator old = *this;
    MoveToNextPosition();
    return old;
  }

  reference operator*() const { return *cur_; }
  pointer operator->() { return &*cur_; }

  inline bool operator==(const FilterIterator& rhs) const {
    return cur_ == rhs.cur_ && end_ == rhs.end_;
  }
  inline bool operator!=(const FilterIterator& rhs) const {
    return !(*this == rhs);
  }


  SubIterator Get() const { return cur_; }


  FilterIterator GetEnd() const { return FilterIterator(end_, predicate_); }

 private:


  bool IsPredicateSatisfied() { return cur_ == end_ || predicate_(*cur_); }

  void MoveToNextPosition() {
    if (cur_ == end_) return;

    do {
      ++cur_;
    } while (!IsPredicateSatisfied());
  }

  SubIterator cur_;
  SubIterator end_;
  Predicate predicate_;
};

template <typename SubIterator, typename Predicate>
FilterIterator<SubIterator, Predicate> MakeFilterIterator(
    const IteratorRange<SubIterator>& sub_iterator_range, Predicate predicate) {
  return FilterIterator<SubIterator, Predicate>(sub_iterator_range, predicate);
}

template <typename SubIterator, typename Predicate>
FilterIterator<SubIterator, Predicate> MakeFilterIterator(
    const SubIterator& begin, const SubIterator& end, Predicate predicate) {
  return MakeFilterIterator(make_range(begin, end), predicate);
}

template <typename SubIterator, typename Predicate>
typename FilterIterator<SubIterator, Predicate>::Range MakeFilterIteratorRange(
    const SubIterator& begin, const SubIterator& end, Predicate predicate) {
  return typename FilterIterator<SubIterator, Predicate>::Range(
      MakeFilterIterator(begin, end, predicate),
      MakeFilterIterator(end, end, predicate));
}

template <typename VT, bool IC>
inline UptrVectorIterator<VT, IC>& UptrVectorIterator<VT, IC>::operator++() {
  ++iterator_;
  return *this;
}

template <typename VT, bool IC>
inline UptrVectorIterator<VT, IC> UptrVectorIterator<VT, IC>::operator++(int) {
  auto it = *this;
  ++(*this);
  return it;
}

template <typename VT, bool IC>
inline UptrVectorIterator<VT, IC>& UptrVectorIterator<VT, IC>::operator--() {
  --iterator_;
  return *this;
}

template <typename VT, bool IC>
inline UptrVectorIterator<VT, IC> UptrVectorIterator<VT, IC>::operator--(int) {
  auto it = *this;
  --(*this);
  return it;
}

template <typename VT, bool IC>
inline bool UptrVectorIterator<VT, IC>::operator==(
    const UptrVectorIterator& that) const {
  return container_ == that.container_ && iterator_ == that.iterator_;
}

template <typename VT, bool IC>
inline bool UptrVectorIterator<VT, IC>::operator!=(
    const UptrVectorIterator& that) const {
  return !(*this == that);
}

template <typename VT, bool IC>
inline ptrdiff_t UptrVectorIterator<VT, IC>::operator-(
    const UptrVectorIterator& that) const {
  assert(container_ == that.container_);
  return iterator_ - that.iterator_;
}

template <typename VT, bool IC>
inline bool UptrVectorIterator<VT, IC>::operator<(
    const UptrVectorIterator& that) const {
  assert(container_ == that.container_);
  return iterator_ < that.iterator_;
}

template <typename VT, bool IC>
template <bool IsConstForMethod>
inline
    typename std::enable_if<!IsConstForMethod, UptrVectorIterator<VT, IC>>::type
    UptrVectorIterator<VT, IC>::InsertBefore(Uptr value) {
  auto index = iterator_ - container_->begin();
  container_->insert(iterator_, std::move(value));
  return UptrVectorIterator(container_, container_->begin() + index);
}

template <typename VT, bool IC>
template <bool IsConstForMethod>
inline
    typename std::enable_if<!IsConstForMethod, UptrVectorIterator<VT, IC>>::type
    UptrVectorIterator<VT, IC>::InsertBefore(UptrVector* values) {
  const auto pos = iterator_ - container_->begin();
  const auto origsz = container_->size();
  container_->resize(origsz + values->size());
  std::move_backward(container_->begin() + pos, container_->begin() + origsz,
                     container_->end());
  std::move(values->begin(), values->end(), container_->begin() + pos);
  return UptrVectorIterator(container_, container_->begin() + pos);
}

template <typename VT, bool IC>
template <bool IsConstForMethod>
inline
    typename std::enable_if<!IsConstForMethod, UptrVectorIterator<VT, IC>>::type
    UptrVectorIterator<VT, IC>::Erase() {
  auto index = iterator_ - container_->begin();
  (void)container_->erase(iterator_);
  return UptrVectorIterator(container_, container_->begin() + index);
}

}
}

#endif
