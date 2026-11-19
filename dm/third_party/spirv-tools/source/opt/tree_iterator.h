// Copyright (c) 2017 Google Inc.
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

#ifndef SOURCE_OPT_TREE_ITERATOR_H_
#define SOURCE_OPT_TREE_ITERATOR_H_

#include <stack>
#include <type_traits>
#include <utility>

namespace spvtools {
namespace opt {













template <typename NodeTy>
class TreeDFIterator {
  static_assert(!std::is_pointer<NodeTy>::value &&
                    !std::is_reference<NodeTy>::value,
                "NodeTy should be a class");

  using NodeIterator =
      typename std::conditional<std::is_const<NodeTy>::value,
                                typename NodeTy::const_iterator,
                                typename NodeTy::iterator>::type;


  using NodePtr = NodeTy*;

 public:

  using reference = NodeTy&;
  using value_type = NodeTy;

  explicit inline TreeDFIterator(NodePtr top_node) : current_(top_node) {
    if (current_ && current_->begin() != current_->end())
      parent_iterators_.emplace(make_pair(current_, current_->begin()));
  }


  inline TreeDFIterator() : TreeDFIterator(nullptr) {}

  bool operator==(const TreeDFIterator& x) const {
    return current_ == x.current_;
  }

  bool operator!=(const TreeDFIterator& x) const { return !(*this == x); }

  reference operator*() const { return *current_; }

  NodePtr operator->() const { return current_; }

  TreeDFIterator& operator++() {
    MoveToNextNode();
    return *this;
  }

  TreeDFIterator operator++(int) {
    TreeDFIterator tmp = *this;
    ++*this;
    return tmp;
  }

 private:





  inline void MoveToNextNode() {
    if (!current_) return;
    if (parent_iterators_.empty()) {
      current_ = nullptr;
      return;
    }
    std::pair<NodePtr, NodeIterator>& next_it = parent_iterators_.top();

    current_ = *next_it.second;

    ++next_it.second;

    if (next_it.first->end() == next_it.second) parent_iterators_.pop();

    if (current_->begin() != current_->end())
      parent_iterators_.emplace(make_pair(current_, current_->begin()));
  }


  NodePtr current_;




  std::stack<std::pair<NodePtr, NodeIterator>> parent_iterators_;
};














template <typename NodeTy>
class PostOrderTreeDFIterator {
  static_assert(!std::is_pointer<NodeTy>::value &&
                    !std::is_reference<NodeTy>::value,
                "NodeTy should be a class");

  using NodeIterator =
      typename std::conditional<std::is_const<NodeTy>::value,
                                typename NodeTy::const_iterator,
                                typename NodeTy::iterator>::type;


  using NodePtr = NodeTy*;

 public:

  using reference = NodeTy&;
  using value_type = NodeTy;

  static inline PostOrderTreeDFIterator begin(NodePtr top_node) {
    return PostOrderTreeDFIterator(top_node);
  }

  static inline PostOrderTreeDFIterator end(NodePtr sentinel_node) {
    return PostOrderTreeDFIterator(sentinel_node, false);
  }

  bool operator==(const PostOrderTreeDFIterator& x) const {
    return current_ == x.current_;
  }

  bool operator!=(const PostOrderTreeDFIterator& x) const {
    return !(*this == x);
  }

  reference operator*() const { return *current_; }

  NodePtr operator->() const { return current_; }

  PostOrderTreeDFIterator& operator++() {
    MoveToNextNode();
    return *this;
  }

  PostOrderTreeDFIterator operator++(int) {
    PostOrderTreeDFIterator tmp = *this;
    ++*this;
    return tmp;
  }

 private:
  explicit inline PostOrderTreeDFIterator(NodePtr top_node)
      : current_(top_node) {
    if (current_) WalkToLeaf();
  }




  inline PostOrderTreeDFIterator(NodePtr sentinel_node, bool)
      : current_(sentinel_node) {}






  inline void MoveToNextNode() {
    if (!current_) return;
    if (parent_iterators_.empty()) {
      current_ = nullptr;
      return;
    }
    std::pair<NodePtr, NodeIterator>& next_it = parent_iterators_.top();

    if (next_it.second == next_it.first->end()) {

      current_ = next_it.first;
      parent_iterators_.pop();
      return;
    }


    current_ = *next_it.second;

    ++next_it.second;
    WalkToLeaf();
  }






  inline void WalkToLeaf() {
    while (current_->begin() != current_->end()) {
      NodeIterator next = ++current_->begin();
      parent_iterators_.emplace(make_pair(current_, next));

      current_ = *current_->begin();
    }
  }


  NodePtr current_;




  std::stack<std::pair<NodePtr, NodeIterator>> parent_iterators_;
};

}
}

#endif
