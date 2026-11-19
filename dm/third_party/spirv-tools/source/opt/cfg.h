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

#ifndef SOURCE_OPT_CFG_H_
#define SOURCE_OPT_CFG_H_

#include <algorithm>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "source/opt/basic_block.h"

namespace spvtools {
namespace opt {

class CFG {
 public:
  explicit CFG(Module* module);



  const std::vector<uint32_t>& preds(uint32_t blk_id) const {
    assert(label2preds_.count(blk_id));
    return label2preds_.at(blk_id);
  }



  BasicBlock* block(uint32_t blk_id) const { return id2block_.at(blk_id); }


  const BasicBlock* pseudo_entry_block() const { return &pseudo_entry_block_; }
  BasicBlock* pseudo_entry_block() { return &pseudo_entry_block_; }

  const BasicBlock* pseudo_exit_block() const { return &pseudo_exit_block_; }
  BasicBlock* pseudo_exit_block() { return &pseudo_exit_block_; }


  bool IsPseudoEntryBlock(BasicBlock* block_ptr) const {
    return block_ptr == &pseudo_entry_block_;
  }


  bool IsPseudoExitBlock(BasicBlock* block_ptr) const {
    return block_ptr == &pseudo_exit_block_;
  }






  void ComputeStructuredOrder(Function* func, BasicBlock* root,
                              std::list<BasicBlock*>* order);






  void ComputeStructuredOrder(Function* func, BasicBlock* root, BasicBlock* end,
                              std::list<BasicBlock*>* order);


  void ForEachBlockInPostOrder(BasicBlock* bb,
                               const std::function<void(BasicBlock*)>& f);



  void ForEachBlockInReversePostOrder(
      BasicBlock* bb, const std::function<void(BasicBlock*)>& f);




  bool WhileEachBlockInReversePostOrder(
      BasicBlock* bb, const std::function<bool(BasicBlock*)>& f);




  void RegisterBlock(BasicBlock* blk) {
    assert(blk->begin() != blk->end() &&
           "Basic blocks must have a terminator before registering.");
    assert(blk->tail()->IsBlockTerminator() &&
           "Basic blocks must have a terminator before registering.");
    uint32_t blk_id = blk->id();
    id2block_[blk_id] = blk;
    AddEdges(blk);
  }


  void ForgetBlock(const BasicBlock* blk) {
    id2block_.erase(blk->id());
    label2preds_.erase(blk->id());
    RemoveSuccessorEdges(blk);
  }

  void RemoveEdge(uint32_t pred_blk_id, uint32_t succ_blk_id) {
    auto pred_it = label2preds_.find(succ_blk_id);
    if (pred_it == label2preds_.end()) return;
    auto& preds_list = pred_it->second;
    auto it = std::find(preds_list.begin(), preds_list.end(), pred_blk_id);
    if (it != preds_list.end()) preds_list.erase(it);
  }


  void AddEdges(BasicBlock* blk);



  void AddEdge(uint32_t pred_blk_id, uint32_t succ_blk_id) {
    label2preds_[succ_blk_id].push_back(pred_blk_id);
  }



  void RemoveNonExistingEdges(uint32_t blk_id);


  void RemoveSuccessorEdges(const BasicBlock* bb) {
    bb->ForEachSuccessorLabel(
        [bb, this](uint32_t succ_id) { RemoveEdge(bb->id(), succ_id); });
  }







  BasicBlock* SplitLoopHeader(BasicBlock* bb);

 private:







  void ComputeStructuredSuccessors(Function* func);




  void ComputePostOrderTraversal(BasicBlock* bb,
                                 std::vector<BasicBlock*>* order,
                                 std::unordered_set<BasicBlock*>* seen);


  Module* module_;



  std::unordered_map<const BasicBlock*, std::vector<BasicBlock*>>
      block2structured_succs_;



  BasicBlock pseudo_entry_block_;


  BasicBlock pseudo_exit_block_;


  std::unordered_map<uint32_t, std::vector<uint32_t>> label2preds_;


  std::unordered_map<uint32_t, BasicBlock*> id2block_;
};

}
}

#endif
