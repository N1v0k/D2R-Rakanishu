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

#ifndef SOURCE_OPT_DOMINATOR_ANALYSIS_H_
#define SOURCE_OPT_DOMINATOR_ANALYSIS_H_

#include <cstdint>
#include <map>

#include "source/opt/dominator_tree.h"

namespace spvtools {
namespace opt {


class DominatorAnalysisBase {
 public:
  explicit DominatorAnalysisBase(bool is_post_dom) : tree_(is_post_dom) {}


  inline void InitializeTree(const CFG& cfg, const Function* f) {
    tree_.InitializeTree(cfg, f);
  }


  inline bool Dominates(const BasicBlock* a, const BasicBlock* b) const {
    if (!a || !b) return false;
    return Dominates(a->id(), b->id());
  }



  inline bool Dominates(uint32_t a, uint32_t b) const {
    return tree_.Dominates(a, b);
  }


  bool Dominates(Instruction* a, Instruction* b) const;


  inline bool StrictlyDominates(const BasicBlock* a,
                                const BasicBlock* b) const {
    if (!a || !b) return false;
    return StrictlyDominates(a->id(), b->id());
  }



  inline bool StrictlyDominates(uint32_t a, uint32_t b) const {
    return tree_.StrictlyDominates(a, b);
  }



  inline BasicBlock* ImmediateDominator(const BasicBlock* node) const {
    if (!node) return nullptr;
    return tree_.ImmediateDominator(node);
  }



  inline BasicBlock* ImmediateDominator(uint32_t node_id) const {
    return tree_.ImmediateDominator(node_id);
  }


  inline bool IsReachable(const BasicBlock* node) const {
    if (!node) return false;
    return tree_.ReachableFromRoots(node->id());
  }


  inline bool IsReachable(uint32_t node_id) const {
    return tree_.ReachableFromRoots(node_id);
  }


  inline void DumpAsDot(std::ostream& out) const { tree_.DumpTreeAsDot(out); }


  inline bool IsPostDominator() const { return tree_.IsPostDominator(); }




  inline DominatorTree& GetDomTree() { return tree_; }
  inline const DominatorTree& GetDomTree() const { return tree_; }


  inline void ClearTree() { tree_.ClearTree(); }



  void Visit(std::function<bool(DominatorTreeNode*)> func) {
    tree_.Visit(func);
  }



  void Visit(std::function<bool(const DominatorTreeNode*)> func) const {
    tree_.Visit(func);
  }



  BasicBlock* CommonDominator(BasicBlock* b1, BasicBlock* b2) const;

 protected:
  DominatorTree tree_;
};


class DominatorAnalysis : public DominatorAnalysisBase {
 public:
  DominatorAnalysis() : DominatorAnalysisBase(false) {}
};


class PostDominatorAnalysis : public DominatorAnalysisBase {
 public:
  PostDominatorAnalysis() : DominatorAnalysisBase(true) {}
};

}
}

#endif
