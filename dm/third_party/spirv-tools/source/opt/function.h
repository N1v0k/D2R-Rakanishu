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

#ifndef SOURCE_OPT_FUNCTION_H_
#define SOURCE_OPT_FUNCTION_H_

#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "source/opt/basic_block.h"
#include "source/opt/instruction.h"
#include "source/opt/iterator.h"

namespace spvtools {
namespace opt {

class CFG;
class IRContext;
class Module;


class Function {
 public:
  using iterator = UptrVectorIterator<BasicBlock>;
  using const_iterator = UptrVectorIterator<BasicBlock, true>;
  using ParamList = std::vector<std::unique_ptr<Instruction>>;



  inline explicit Function(std::unique_ptr<Instruction> def_inst);

  explicit Function(const Function& f) = delete;





  Function* Clone(IRContext*) const;

  Instruction& DefInst() { return *def_inst_; }
  const Instruction& DefInst() const { return *def_inst_; }


  inline void AddParameter(std::unique_ptr<Instruction> p);

  inline void AddDebugInstructionInHeader(std::unique_ptr<Instruction> p);

  inline void AddBasicBlock(std::unique_ptr<BasicBlock> b);

  inline void AddBasicBlock(std::unique_ptr<BasicBlock> b, iterator ip);
  template <typename T>
  inline void AddBasicBlocks(T begin, T end, iterator ip);



  inline void MoveBasicBlockToAfter(uint32_t id, BasicBlock* ip);


  inline void RemoveEmptyBlocks();



  inline void RemoveParameter(uint32_t id);







  using RewriteParamFn = std::function<void(
      std::unique_ptr<Instruction>&&, std::back_insert_iterator<ParamList>&)>;
  void RewriteParams(RewriteParamFn& replacer) {
    ParamList new_params;
    auto appender = std::back_inserter(new_params);
    for (auto& param : params_) {
      replacer(std::move(param), appender);
    }
    params_ = std::move(new_params);
  }


  inline void SetFunctionEnd(std::unique_ptr<Instruction> end_inst);



  inline void AddNonSemanticInstruction(
      std::unique_ptr<Instruction> non_semantic);


  inline Instruction* EndInst() { return end_inst_.get(); }
  inline const Instruction* EndInst() const { return end_inst_.get(); }


  inline uint32_t result_id() const { return def_inst_->result_id(); }


  inline uint32_t type_id() const { return def_inst_->type_id(); }


  inline uint32_t control_mask() const { return def_inst_->GetSingleWordInOperand(0); }


  const std::unique_ptr<BasicBlock>& entry() const { return blocks_.front(); }


  BasicBlock* tail() { return blocks_.back().get(); }
  const BasicBlock* tail() const { return blocks_.back().get(); }

  iterator begin() { return iterator(&blocks_, blocks_.begin()); }
  iterator end() { return iterator(&blocks_, blocks_.end()); }
  const_iterator begin() const { return cbegin(); }
  const_iterator end() const { return cend(); }
  const_iterator cbegin() const {
    return const_iterator(&blocks_, blocks_.cbegin());
  }
  const_iterator cend() const {
    return const_iterator(&blocks_, blocks_.cend());
  }


  iterator FindBlock(uint32_t bb_id) {
    return std::find_if(begin(), end(), [bb_id](const BasicBlock& it_bb) {
      return bb_id == it_bb.id();
    });
  }




  void ForEachInst(const std::function<void(Instruction*)>& f,
                   bool run_on_debug_line_insts = false,
                   bool run_on_non_semantic_insts = false);
  void ForEachInst(const std::function<void(const Instruction*)>& f,
                   bool run_on_debug_line_insts = false,
                   bool run_on_non_semantic_insts = false) const;




  bool WhileEachInst(const std::function<bool(Instruction*)>& f,
                     bool run_on_debug_line_insts = false,
                     bool run_on_non_semantic_insts = false);
  bool WhileEachInst(const std::function<bool(const Instruction*)>& f,
                     bool run_on_debug_line_insts = false,
                     bool run_on_non_semantic_insts = false) const;




  void ForEachParam(const std::function<void(const Instruction*)>& f,
                    bool run_on_debug_line_insts = false) const;
  void ForEachParam(const std::function<void(Instruction*)>& f,
                    bool run_on_debug_line_insts = false);



  void ForEachDebugInstructionsInHeader(
      const std::function<void(Instruction*)>& f);

  BasicBlock* InsertBasicBlockAfter(std::unique_ptr<BasicBlock>&& new_block,
                                    BasicBlock* position);

  BasicBlock* InsertBasicBlockBefore(std::unique_ptr<BasicBlock>&& new_block,
                                     BasicBlock* position);


  bool HasEarlyReturn() const;


  bool IsRecursive() const;





  std::string PrettyPrint(uint32_t options = 0u) const;



  void Dump() const;


  bool IsDeclaration() { return begin() == end(); }


  void ReorderBasicBlocksInStructuredOrder();

 private:



  template <class It>
  void ReorderBasicBlocks(It begin, It end);

  template <class It>
  bool ContainsAllBlocksInTheFunction(It begin, It end);


  std::unique_ptr<Instruction> def_inst_;

  ParamList params_;

  InstructionList debug_insts_in_header_;

  std::vector<std::unique_ptr<BasicBlock>> blocks_;

  std::unique_ptr<Instruction> end_inst_;

  std::vector<std::unique_ptr<Instruction>> non_semantic_;
};


std::ostream& operator<<(std::ostream& str, const Function& func);

inline Function::Function(std::unique_ptr<Instruction> def_inst)
    : def_inst_(std::move(def_inst)), end_inst_() {}

inline void Function::AddParameter(std::unique_ptr<Instruction> p) {
  params_.emplace_back(std::move(p));
}

inline void Function::AddDebugInstructionInHeader(
    std::unique_ptr<Instruction> p) {
  debug_insts_in_header_.push_back(std::move(p));
}

inline void Function::AddBasicBlock(std::unique_ptr<BasicBlock> b) {
  AddBasicBlock(std::move(b), end());
}

inline void Function::AddBasicBlock(std::unique_ptr<BasicBlock> b,
                                    iterator ip) {
  b->SetParent(this);
  ip.InsertBefore(std::move(b));
}

template <typename T>
inline void Function::AddBasicBlocks(T src_begin, T src_end, iterator ip) {
  blocks_.insert(ip.Get(), std::make_move_iterator(src_begin),
                 std::make_move_iterator(src_end));
}

inline void Function::MoveBasicBlockToAfter(uint32_t id, BasicBlock* ip) {
  std::unique_ptr<BasicBlock> block_to_move = std::move(*FindBlock(id).Get());
  blocks_.erase(std::find(std::begin(blocks_), std::end(blocks_), nullptr));

  assert(block_to_move->GetParent() == ip->GetParent() &&
         "Both blocks have to be in the same function.");

  InsertBasicBlockAfter(std::move(block_to_move), ip);
}

inline void Function::RemoveEmptyBlocks() {
  auto first_empty =
      std::remove_if(std::begin(blocks_), std::end(blocks_),
                     [](const std::unique_ptr<BasicBlock>& bb) -> bool {
                       return bb->GetLabelInst()->opcode() == spv::Op::OpNop;
                     });
  blocks_.erase(first_empty, std::end(blocks_));
}

inline void Function::RemoveParameter(uint32_t id) {
  params_.erase(std::remove_if(params_.begin(), params_.end(),
                               [id](const std::unique_ptr<Instruction>& param) {
                                 return param->result_id() == id;
                               }),
                params_.end());
}

inline void Function::SetFunctionEnd(std::unique_ptr<Instruction> end_inst) {
  end_inst_ = std::move(end_inst);
}

inline void Function::AddNonSemanticInstruction(
    std::unique_ptr<Instruction> non_semantic) {
  non_semantic_.emplace_back(std::move(non_semantic));
}

template <class It>
void Function::ReorderBasicBlocks(It begin, It end) {

  assert(ContainsAllBlocksInTheFunction(begin, end));




  std::for_each(blocks_.begin(), blocks_.end(),
                [](std::unique_ptr<BasicBlock>& bb) { bb.release(); });
  std::transform(begin, end, blocks_.begin(), [](BasicBlock* bb) {
    return std::unique_ptr<BasicBlock>(bb);
  });
}

template <class It>
bool Function::ContainsAllBlocksInTheFunction(It begin, It end) {
  std::unordered_multiset<BasicBlock*> range(begin, end);
  if (range.size() != blocks_.size()) {
    return false;
  }

  for (auto& bb : blocks_) {
    if (range.count(bb.get()) == 0) return false;
  }
  return true;
}

}
}

#endif
