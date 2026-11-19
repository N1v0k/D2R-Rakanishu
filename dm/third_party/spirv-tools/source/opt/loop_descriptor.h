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

#ifndef SOURCE_OPT_LOOP_DESCRIPTOR_H_
#define SOURCE_OPT_LOOP_DESCRIPTOR_H_

#include <algorithm>
#include <cstdint>
#include <map>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "source/opt/basic_block.h"
#include "source/opt/dominator_analysis.h"
#include "source/opt/module.h"
#include "source/opt/tree_iterator.h"
#include "source/util/status.h"

namespace spvtools {
namespace opt {

class IRContext;
class CFG;
class LoopDescriptor;


class Loop {

  using ChildrenList = std::vector<Loop*>;

 public:
  using iterator = ChildrenList::iterator;
  using const_iterator = ChildrenList::const_iterator;
  using BasicBlockListTy = std::unordered_set<uint32_t>;

  explicit Loop(IRContext* context)
      : context_(context),
        loop_header_(nullptr),
        loop_continue_(nullptr),
        loop_merge_(nullptr),
        loop_preheader_(nullptr),
        loop_latch_(nullptr),
        parent_(nullptr),
        loop_is_marked_for_removal_(false) {}

  Loop(IRContext* context, DominatorAnalysis* analysis, BasicBlock* header,
       BasicBlock* continue_target, BasicBlock* merge_target);


  inline iterator begin() { return nested_loops_.begin(); }
  inline iterator end() { return nested_loops_.end(); }
  inline const_iterator begin() const { return cbegin(); }
  inline const_iterator end() const { return cend(); }
  inline const_iterator cbegin() const { return nested_loops_.begin(); }
  inline const_iterator cend() const { return nested_loops_.end(); }



  inline BasicBlock* GetHeaderBlock() { return loop_header_; }
  inline const BasicBlock* GetHeaderBlock() const { return loop_header_; }
  inline void SetHeaderBlock(BasicBlock* header) { loop_header_ = header; }



  inline void UpdateLoopMergeInst() {
    assert(GetHeaderBlock()->GetLoopMergeInst() &&
           "The loop is not structured");
    Instruction* merge_inst = GetHeaderBlock()->GetLoopMergeInst();
    merge_inst->SetInOperand(0, {GetMergeBlock()->id()});
  }



  inline BasicBlock* GetContinueBlock() { return loop_continue_; }
  inline const BasicBlock* GetContinueBlock() const { return loop_continue_; }




  inline BasicBlock* GetLatchBlock() { return loop_latch_; }
  inline const BasicBlock* GetLatchBlock() const { return loop_latch_; }





  void SetLatchBlock(BasicBlock* latch);



  void SetContinueBlock(BasicBlock* continue_block);



  inline BasicBlock* GetMergeBlock() { return loop_merge_; }
  inline const BasicBlock* GetMergeBlock() const { return loop_merge_; }







  void SetMergeBlock(BasicBlock* merge);






  inline BasicBlock* GetPreHeaderBlock() { return loop_preheader_; }


  inline const BasicBlock* GetPreHeaderBlock() const { return loop_preheader_; }




  void SetPreHeaderBlock(BasicBlock* preheader);



  BasicBlock* GetOrCreatePreHeaderBlock();


  inline bool HasNestedLoops() const { return nested_loops_.size() != 0; }



  void GetExitBlocks(std::unordered_set<uint32_t>* exit_blocks) const;





  void GetMergingBlocks(std::unordered_set<uint32_t>* merging_blocks) const;




  bool IsLCSSA() const;



  inline size_t GetDepth() const {
    size_t lvl = 1;
    for (const Loop* loop = GetParent(); loop; loop = loop->GetParent()) lvl++;
    return lvl;
  }

  inline size_t NumImmediateChildren() const { return nested_loops_.size(); }

  inline bool HasChildren() const { return !nested_loops_.empty(); }


  inline void AddNestedLoop(Loop* nested) {
    assert(!nested->GetParent() && "The loop has another parent.");
    nested_loops_.push_back(nested);
    nested->SetParent(this);
  }

  inline Loop* GetParent() { return parent_; }
  inline const Loop* GetParent() const { return parent_; }

  inline bool HasParent() const { return parent_; }


  inline bool IsNested() const { return parent_ != nullptr; }




  inline const BasicBlockListTy& GetBlocks() const {
    return loop_basic_blocks_;
  }


  inline bool IsInsideLoop(const BasicBlock* bb) const {
    return IsInsideLoop(bb->id());
  }


  inline bool IsInsideLoop(uint32_t bb_id) const {
    return loop_basic_blocks_.count(bb_id);
  }


  bool IsInsideLoop(Instruction* inst) const;


  void AddBasicBlock(const BasicBlock* bb) { AddBasicBlock(bb->id()); }


  void AddBasicBlock(uint32_t id) {
    for (Loop* loop = this; loop != nullptr; loop = loop->parent_) {
      loop->loop_basic_blocks_.insert(id);
    }
  }




  void RemoveBasicBlock(uint32_t bb_id) {
    for (Loop* loop = this; loop != nullptr; loop = loop->parent_) {
      loop->loop_basic_blocks_.erase(bb_id);
    }
  }




  void ClearBlocks() { loop_basic_blocks_.clear(); }


  void AddBasicBlockToLoop(const BasicBlock* bb) {
    assert(IsBasicBlockInLoopSlow(bb) &&
           "Basic block does not belong to the loop");

    AddBasicBlock(bb);
  }


  void GetInductionVariables(std::vector<Instruction*>& inductions) const;




  Instruction* FindConditionVariable(const BasicBlock* condition) const;






  bool FindNumberOfIterations(const Instruction* induction,
                              const Instruction* branch, size_t* iterations,
                              int64_t* step_amount = nullptr,
                              int64_t* init_value = nullptr) const;




  inline bool HasUnrollLoopControl() const {
    assert(loop_header_);
    if (!loop_header_->GetLoopMergeInst()) return false;

    return loop_header_->GetLoopMergeInst()->GetSingleWordOperand(2) == 1;
  }



  BasicBlock* FindConditionBlock() const;


  inline void RemoveChildLoop(Loop* loop) {
    nested_loops_.erase(
        std::find(nested_loops_.begin(), nested_loops_.end(), loop));
    loop->SetParent(nullptr);
  }



  inline void MarkLoopForRemoval() { loop_is_marked_for_removal_ = true; }


  inline bool IsMarkedForRemoval() const { return loop_is_marked_for_removal_; }


  inline bool AreAllChildrenMarkedForRemoval() const {
    for (const Loop* child : nested_loops_) {
      if (!child->IsMarkedForRemoval()) {
        return false;
      }
    }
    return true;
  }



  bool IsSafeToClone() const;



  inline void SetParent(Loop* parent) { parent_ = parent; }


  bool ShouldHoistInstruction(const Instruction& inst) const;



  bool AreAllOperandsOutsideLoop(const Instruction& inst) const;




  bool GetInductionInitValue(const Instruction* induction,
                             int64_t* value) const;



  Instruction* GetInductionStepOperation(const Instruction* induction) const;




  bool IsSupportedStepOp(spv::Op step) const;




  bool IsSupportedCondition(spv::Op condition) const;






  void ComputeLoopStructuredOrder(std::vector<BasicBlock*>* ordered_loop_blocks,
                                  bool include_pre_header = false,
                                  bool include_merge = false) const;




  static int64_t GetResidualConditionValue(spv::Op condition,
                                           int64_t initial_value,
                                           int64_t step_value,
                                           size_t number_of_iterations,
                                           size_t unroll_factor);



  Instruction* GetConditionInst() const;


  IRContext* GetContext() const { return context_; }





  BasicBlock* FindLatchBlock();

 private:
  IRContext* context_;

  BasicBlock* loop_header_;


  BasicBlock* loop_continue_;


  BasicBlock* loop_merge_;


  BasicBlock* loop_preheader_;


  BasicBlock* loop_latch_;


  Loop* parent_;


  ChildrenList nested_loops_;



  BasicBlockListTy loop_basic_blocks_;




  bool IsBasicBlockInLoopSlow(const BasicBlock* bb);


  BasicBlock* FindLoopPreheader(DominatorAnalysis* dom_analysis);



  inline void SetLatchBlockImpl(BasicBlock* latch) { loop_latch_ = latch; }

  inline void SetMergeBlockImpl(BasicBlock* merge) { loop_merge_ = merge; }






  int64_t GetIterations(spv::Op condition, int64_t condition_value,
                        int64_t init_value, int64_t step_value) const;



  bool loop_is_marked_for_removal_;



  friend class LoopDescriptor;
  friend class LoopUtils;
};




class LoopDescriptor {
 public:

  using iterator = PostOrderTreeDFIterator<Loop>;
  using const_iterator = PostOrderTreeDFIterator<const Loop>;

  using pre_iterator = TreeDFIterator<Loop>;
  using const_pre_iterator = TreeDFIterator<const Loop>;


  using Status = utils::Status;


  LoopDescriptor(IRContext* context, const Function* f);


  LoopDescriptor(const LoopDescriptor&) = delete;

  LoopDescriptor(LoopDescriptor&& other) : placeholder_top_loop_(nullptr) {


    loops_ = std::move(other.loops_);
    other.loops_.clear();
    basic_block_to_loop_ = std::move(other.basic_block_to_loop_);
    other.basic_block_to_loop_.clear();
    placeholder_top_loop_ = std::move(other.placeholder_top_loop_);
  }


  ~LoopDescriptor();


  inline size_t NumLoops() const { return loops_.size(); }



  inline Loop& GetLoopByIndex(size_t index) const {
    assert(loops_.size() > index &&
           "Index out of range (larger than loop count)");
    return *loops_[index];
  }



  std::vector<Loop*> GetLoopsInBinaryLayoutOrder();


  inline Loop* operator[](uint32_t block_id) const {
    return FindLoopForBasicBlock(block_id);
  }


  inline Loop* operator[](const BasicBlock* bb) const {
    return (*this)[bb->id()];
  }



  inline iterator begin() { return iterator::begin(&placeholder_top_loop_); }
  inline iterator end() { return iterator::end(&placeholder_top_loop_); }
  inline const_iterator begin() const { return cbegin(); }
  inline const_iterator end() const { return cend(); }
  inline const_iterator cbegin() const {
    return const_iterator::begin(&placeholder_top_loop_);
  }
  inline const_iterator cend() const {
    return const_iterator::end(&placeholder_top_loop_);
  }



  inline pre_iterator pre_begin() {
    return ++pre_iterator(&placeholder_top_loop_);
  }
  inline pre_iterator pre_end() { return pre_iterator(); }
  inline const_pre_iterator pre_begin() const { return pre_cbegin(); }
  inline const_pre_iterator pre_end() const { return pre_cend(); }
  inline const_pre_iterator pre_cbegin() const {
    return ++const_pre_iterator(&placeholder_top_loop_);
  }
  inline const_pre_iterator pre_cend() const { return const_pre_iterator(); }


  inline void SetBasicBlockToLoop(uint32_t bb_id, Loop* loop) {
    basic_block_to_loop_[bb_id] = loop;
  }



  inline void AddLoop(std::unique_ptr<Loop>&& loop_to_add, Loop* parent) {
    loops_to_add_.emplace_back(std::make_pair(parent, std::move(loop_to_add)));
  }





  Status CreatePreHeaderBlocksIfMissing();



  void PostModificationCleanup();


  inline void ForgetBasicBlock(uint32_t bb_id) {
    basic_block_to_loop_.erase(bb_id);
  }



  Loop* AddLoopNest(std::unique_ptr<Loop> new_loop);


  void RemoveLoop(Loop* loop);

  void SetAsTopLoop(Loop* loop) {
    assert(std::find(placeholder_top_loop_.begin(), placeholder_top_loop_.end(),
                     loop) == placeholder_top_loop_.end() &&
           "already registered");
    placeholder_top_loop_.nested_loops_.push_back(loop);
  }

  Loop* GetPlaceholderRootLoop() { return &placeholder_top_loop_; }
  const Loop* GetPlaceholderRootLoop() const { return &placeholder_top_loop_; }

 private:


  using LoopContainerType = std::vector<Loop*>;

  using LoopsToAddContainerType =
      std::vector<std::pair<Loop*, std::unique_ptr<Loop>>>;


  void PopulateList(IRContext* context, const Function* f);


  inline Loop* FindLoopForBasicBlock(uint32_t block_id) const {
    std::unordered_map<uint32_t, Loop*>::const_iterator it =
        basic_block_to_loop_.find(block_id);
    return it != basic_block_to_loop_.end() ? it->second : nullptr;
  }


  void ClearLoops();



  LoopContainerType loops_;


  Loop placeholder_top_loop_;

  std::unordered_map<uint32_t, Loop*> basic_block_to_loop_;



  LoopsToAddContainerType loops_to_add_;
};

}
}

#endif
