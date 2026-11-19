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




#ifndef SOURCE_OPT_BASIC_BLOCK_H_
#define SOURCE_OPT_BASIC_BLOCK_H_

#include <functional>
#include <iterator>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "source/opt/instruction.h"
#include "source/opt/instruction_list.h"
#include "source/opt/iterator.h"

namespace spvtools {
namespace opt {

class Function;
class IRContext;


class BasicBlock {
 public:
  using iterator = InstructionList::iterator;
  using const_iterator = InstructionList::const_iterator;
  using reverse_iterator = std::reverse_iterator<InstructionList::iterator>;
  using const_reverse_iterator =
      std::reverse_iterator<InstructionList::const_iterator>;


  inline explicit BasicBlock(std::unique_ptr<Instruction> label);

  explicit BasicBlock(const BasicBlock& bb) = delete;








  BasicBlock* Clone(IRContext*) const;


  void SetParent(Function* function) { function_ = function; }


  inline Function* GetParent() const { return function_; }


  inline void AddInstruction(std::unique_ptr<Instruction> i);


  inline void AddInstructions(BasicBlock* bp);


  std::unique_ptr<Instruction>& GetLabel() { return label_; }


  Instruction* GetLabelInst() { return label_.get(); }
  const Instruction* GetLabelInst() const { return label_.get(); }



  const Instruction* GetMergeInst() const;
  Instruction* GetMergeInst();



  const Instruction* GetLoopMergeInst() const;
  Instruction* GetLoopMergeInst();


  inline uint32_t id() const { return label_->result_id(); }

  iterator begin() { return insts_.begin(); }
  iterator end() { return insts_.end(); }
  const_iterator begin() const { return insts_.cbegin(); }
  const_iterator end() const { return insts_.cend(); }
  const_iterator cbegin() const { return insts_.cbegin(); }
  const_iterator cend() const { return insts_.cend(); }

  reverse_iterator rbegin() { return reverse_iterator(end()); }
  reverse_iterator rend() { return reverse_iterator(begin()); }
  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(cend());
  }
  const_reverse_iterator rend() const {
    return const_reverse_iterator(cbegin());
  }
  const_reverse_iterator crbegin() const {
    return const_reverse_iterator(cend());
  }
  const_reverse_iterator crend() const {
    return const_reverse_iterator(cbegin());
  }




  iterator tail() {
    assert(!insts_.empty());
    return --end();
  }


  const_iterator ctail() const {
    assert(!insts_.empty());
    return --insts_.cend();
  }


  inline bool hasSuccessor() const { return ctail()->IsBranch(); }



  inline void ForEachInst(const std::function<void(Instruction*)>& f,
                          bool run_on_debug_line_insts = false);
  inline void ForEachInst(const std::function<void(const Instruction*)>& f,
                          bool run_on_debug_line_insts = false) const;




  inline bool WhileEachInst(const std::function<bool(Instruction*)>& f,
                            bool run_on_debug_line_insts = false);
  inline bool WhileEachInst(const std::function<bool(const Instruction*)>& f,
                            bool run_on_debug_line_insts = false) const;



  inline void ForEachPhiInst(const std::function<void(Instruction*)>& f,
                             bool run_on_debug_line_insts = false);




  inline bool WhileEachPhiInst(const std::function<bool(Instruction*)>& f,
                               bool run_on_debug_line_insts = false);


  void ForEachSuccessorLabel(
      const std::function<void(const uint32_t)>& f) const;



  bool WhileEachSuccessorLabel(
      const std::function<bool(const uint32_t)>& f) const;




  void ForEachSuccessorLabel(const std::function<void(uint32_t*)>& f);


  bool IsSuccessor(const BasicBlock* block) const;


  void ForMergeAndContinueLabel(const std::function<void(const uint32_t)>& f);


  bool HasPhiInstructions() {
    return !WhileEachPhiInst([](Instruction*) { return false; });
  }


  bool IsLoopHeader() const { return GetLoopMergeInst() != nullptr; }



  uint32_t MergeBlockIdIfAny() const;


  uint32_t MergeBlockId() const;



  uint32_t ContinueBlockIdIfAny() const;


  uint32_t ContinueBlockId() const;


  Instruction* terminator() { return &*tail(); }
  const Instruction* terminator() const { return &*ctail(); }



  bool IsReturn() const { return ctail()->IsReturn(); }


  bool IsReturnOrAbort() const { return ctail()->IsReturnOrAbort(); }



  void KillAllInsts(bool killLabel);





  BasicBlock* SplitBasicBlock(IRContext* context, uint32_t label_id,
                              iterator iter);






  std::string PrettyPrint(uint32_t options = 0u) const;



  void Dump() const;

 private:

  Function* function_;

  std::unique_ptr<Instruction> label_;

  InstructionList insts_;
};


std::ostream& operator<<(std::ostream& str, const BasicBlock& block);

inline BasicBlock::BasicBlock(std::unique_ptr<Instruction> label)
    : function_(nullptr), label_(std::move(label)) {}

inline void BasicBlock::AddInstruction(std::unique_ptr<Instruction> i) {
  insts_.push_back(std::move(i));
}

inline void BasicBlock::AddInstructions(BasicBlock* bp) {
  auto bEnd = end();
  (void)bEnd.MoveBefore(&bp->insts_);
}

inline bool BasicBlock::WhileEachInst(
    const std::function<bool(Instruction*)>& f, bool run_on_debug_line_insts) {
  if (label_) {
    if (!label_->WhileEachInst(f, run_on_debug_line_insts)) return false;
  }
  if (insts_.empty()) {
    return true;
  }

  Instruction* inst = &insts_.front();
  while (inst != nullptr) {
    Instruction* next_instruction = inst->NextNode();
    if (!inst->WhileEachInst(f, run_on_debug_line_insts)) return false;
    inst = next_instruction;
  }
  return true;
}

inline bool BasicBlock::WhileEachInst(
    const std::function<bool(const Instruction*)>& f,
    bool run_on_debug_line_insts) const {
  if (label_) {
    if (!static_cast<const Instruction*>(label_.get())
             ->WhileEachInst(f, run_on_debug_line_insts))
      return false;
  }
  for (const auto& inst : insts_) {
    if (!static_cast<const Instruction*>(&inst)->WhileEachInst(
            f, run_on_debug_line_insts))
      return false;
  }
  return true;
}

inline void BasicBlock::ForEachInst(const std::function<void(Instruction*)>& f,
                                    bool run_on_debug_line_insts) {
  WhileEachInst(
      [&f](Instruction* inst) {
        f(inst);
        return true;
      },
      run_on_debug_line_insts);
}

inline void BasicBlock::ForEachInst(
    const std::function<void(const Instruction*)>& f,
    bool run_on_debug_line_insts) const {
  WhileEachInst(
      [&f](const Instruction* inst) {
        f(inst);
        return true;
      },
      run_on_debug_line_insts);
}

inline bool BasicBlock::WhileEachPhiInst(
    const std::function<bool(Instruction*)>& f, bool run_on_debug_line_insts) {
  if (insts_.empty()) {
    return true;
  }

  Instruction* inst = &insts_.front();
  while (inst != nullptr) {
    Instruction* next_instruction = inst->NextNode();
    if (inst->opcode() != spv::Op::OpPhi) break;
    if (!inst->WhileEachInst(f, run_on_debug_line_insts)) return false;
    inst = next_instruction;
  }
  return true;
}

inline void BasicBlock::ForEachPhiInst(
    const std::function<void(Instruction*)>& f, bool run_on_debug_line_insts) {
  WhileEachPhiInst(
      [&f](Instruction* inst) {
        f(inst);
        return true;
      },
      run_on_debug_line_insts);
}

}
}

#endif
