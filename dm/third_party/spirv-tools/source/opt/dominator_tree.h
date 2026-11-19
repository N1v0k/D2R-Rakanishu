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

#ifndef SOURCE_OPT_DOMINATOR_TREE_H_
#define SOURCE_OPT_DOMINATOR_TREE_H_

#include <algorithm>
#include <cstdint>
#include <map>
#include <utility>
#include <vector>

#include "source/opt/cfg.h"
#include "source/opt/tree_iterator.h"

namespace spvtools {
namespace opt {



struct DominatorTreeNode {
  explicit DominatorTreeNode(BasicBlock* bb)
      : bb_(bb),
        parent_(nullptr),
        children_({}),
        dfs_num_pre_(-1),
        dfs_num_post_(-1) {}

  using iterator = std::vector<DominatorTreeNode*>::iterator;
  using const_iterator = std::vector<DominatorTreeNode*>::const_iterator;


  using df_iterator = TreeDFIterator<DominatorTreeNode>;
  using const_df_iterator = TreeDFIterator<const DominatorTreeNode>;

  using post_iterator = PostOrderTreeDFIterator<DominatorTreeNode>;
  using const_post_iterator = PostOrderTreeDFIterator<const DominatorTreeNode>;

  iterator begin() { return children_.begin(); }
  iterator end() { return children_.end(); }
  const_iterator begin() const { return cbegin(); }
  const_iterator end() const { return cend(); }
  const_iterator cbegin() const { return children_.begin(); }
  const_iterator cend() const { return children_.end(); }


  df_iterator df_begin() { return df_iterator(this); }
  df_iterator df_end() { return df_iterator(); }
  const_df_iterator df_begin() const { return df_cbegin(); }
  const_df_iterator df_end() const { return df_cend(); }
  const_df_iterator df_cbegin() const { return const_df_iterator(this); }
  const_df_iterator df_cend() const { return const_df_iterator(); }


  post_iterator post_begin() { return post_iterator::begin(this); }
  post_iterator post_end() { return post_iterator::end(nullptr); }
  const_post_iterator post_begin() const { return post_cbegin(); }
  const_post_iterator post_end() const { return post_cend(); }
  const_post_iterator post_cbegin() const {
    return const_post_iterator::begin(this);
  }
  const_post_iterator post_cend() const {
    return const_post_iterator::end(nullptr);
  }

  inline uint32_t id() const { return bb_->id(); }

  BasicBlock* bb_;
  DominatorTreeNode* parent_;
  std::vector<DominatorTreeNode*> children_;





  int dfs_num_pre_;
  int dfs_num_post_;
};



class DominatorTree {
 public:

  using DominatorTreeNodeMap = std::map<uint32_t, DominatorTreeNode>;
  using iterator = TreeDFIterator<DominatorTreeNode>;
  using const_iterator = TreeDFIterator<const DominatorTreeNode>;
  using post_iterator = PostOrderTreeDFIterator<DominatorTreeNode>;
  using const_post_iterator = PostOrderTreeDFIterator<const DominatorTreeNode>;


  using DominatorTreeNodeList = std::vector<DominatorTreeNode*>;
  using roots_iterator = DominatorTreeNodeList::iterator;
  using roots_const_iterator = DominatorTreeNodeList::const_iterator;

  DominatorTree() : postdominator_(false) {}
  explicit DominatorTree(bool post) : postdominator_(post) {}




  iterator begin() { return ++iterator(GetRoot()); }
  iterator end() { return iterator(); }
  const_iterator begin() const { return cbegin(); }
  const_iterator end() const { return cend(); }
  const_iterator cbegin() const { return ++const_iterator(GetRoot()); }
  const_iterator cend() const { return const_iterator(); }



  post_iterator post_begin() { return post_iterator::begin(GetRoot()); }
  post_iterator post_end() { return post_iterator::end(GetRoot()); }
  const_post_iterator post_begin() const { return post_cbegin(); }
  const_post_iterator post_end() const { return post_cend(); }
  const_post_iterator post_cbegin() const {
    return const_post_iterator::begin(GetRoot());
  }
  const_post_iterator post_cend() const {
    return const_post_iterator::end(GetRoot());
  }

  roots_iterator roots_begin() { return roots_.begin(); }
  roots_iterator roots_end() { return roots_.end(); }
  roots_const_iterator roots_begin() const { return roots_cbegin(); }
  roots_const_iterator roots_end() const { return roots_cend(); }
  roots_const_iterator roots_cbegin() const { return roots_.begin(); }
  roots_const_iterator roots_cend() const { return roots_.end(); }




  DominatorTreeNode* GetRoot() {
    assert(roots_.size() == 1);
    return *roots_.begin();
  }

  const DominatorTreeNode* GetRoot() const {
    assert(roots_.size() == 1);
    return *roots_.begin();
  }

  const DominatorTreeNodeList& Roots() const { return roots_; }


  void DumpTreeAsDot(std::ostream& out_stream) const;




  void InitializeTree(const CFG& cfg, const Function* f);


  bool Dominates(const BasicBlock* a, const BasicBlock* b) const;


  bool Dominates(uint32_t a, uint32_t b) const;


  bool Dominates(const DominatorTreeNode* a, const DominatorTreeNode* b) const;


  bool StrictlyDominates(const BasicBlock* a, const BasicBlock* b) const;


  bool StrictlyDominates(uint32_t a, uint32_t b) const;



  bool StrictlyDominates(const DominatorTreeNode* a,
                         const DominatorTreeNode* b) const;


  BasicBlock* ImmediateDominator(const BasicBlock* A) const;


  BasicBlock* ImmediateDominator(uint32_t a) const;




  inline bool ReachableFromRoots(const BasicBlock* a) const {
    if (!a) return false;
    return ReachableFromRoots(a->id());
  }


  bool ReachableFromRoots(uint32_t a) const {
    return GetTreeNode(a) != nullptr;
  }


  bool IsPostDominator() const { return postdominator_; }


  void ClearTree() {
    nodes_.clear();
    roots_.clear();
  }



  bool Visit(std::function<bool(DominatorTreeNode*)> func) {
    for (auto n : *this) {
      if (!func(&n)) return false;
    }
    return true;
  }



  bool Visit(std::function<bool(const DominatorTreeNode*)> func) const {
    for (auto n : *this) {
      if (!func(&n)) return false;
    }
    return true;
  }





  void VisitChildrenIf(std::function<bool(DominatorTreeNode*)> func,
                       iterator node) {
    if (func(&*node)) {
      for (auto n : *node) {
        VisitChildrenIf(func, n->df_begin());
      }
    }
  }



  inline DominatorTreeNode* GetTreeNode(BasicBlock* bb) {
    return GetTreeNode(bb->id());
  }


  inline const DominatorTreeNode* GetTreeNode(BasicBlock* bb) const {
    return GetTreeNode(bb->id());
  }



  inline DominatorTreeNode* GetTreeNode(uint32_t id) {
    DominatorTreeNodeMap::iterator node_iter = nodes_.find(id);
    if (node_iter == nodes_.end()) {
      return nullptr;
    }
    return &node_iter->second;
  }


  inline const DominatorTreeNode* GetTreeNode(uint32_t id) const {
    DominatorTreeNodeMap::const_iterator node_iter = nodes_.find(id);
    if (node_iter == nodes_.end()) {
      return nullptr;
    }
    return &node_iter->second;
  }



  DominatorTreeNode* GetOrInsertNode(BasicBlock* bb);


  void ResetDFNumbering();

 private:








  void GetDominatorEdges(
      const Function* f, const BasicBlock* dummy_start_node,
      std::vector<std::pair<BasicBlock*, BasicBlock*>>* edges);


  std::vector<DominatorTreeNode*> roots_;


  DominatorTreeNodeMap nodes_;


  bool postdominator_;
};

}
}

#endif
