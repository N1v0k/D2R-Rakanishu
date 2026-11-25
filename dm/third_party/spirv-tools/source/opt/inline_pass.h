// Copyright (c) 2017 The Khronos Group Inc.
// Copyright (c) 2017 Valve Corporation
// Copyright (c) 2017 LunarG Inc.
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

#ifndef SOURCE_OPT_INLINE_PASS_H_
#define SOURCE_OPT_INLINE_PASS_H_

#include <algorithm>
#include <list>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

#include "source/opt/debug_info_manager.h"
#include "source/opt/decoration_manager.h"
#include "source/opt/module.h"
#include "source/opt/pass.h"

namespace spvtools {
namespace opt {


class InlinePass : public Pass {
  using cbb_ptr = const BasicBlock*;

 public:
  virtual ~InlinePass() override = default;

 protected:
  InlinePass();



  uint32_t AddPointerToType(uint32_t type_id, spv::StorageClass storage_class);


  void AddBranch(uint32_t labelId, std::unique_ptr<BasicBlock>* block_ptr);


  void AddBranchCond(uint32_t cond_id, uint32_t true_id, uint32_t false_id,
                     std::unique_ptr<BasicBlock>* block_ptr);


  void AddLoopMerge(uint32_t merge_id, uint32_t continue_id,
                    std::unique_ptr<BasicBlock>* block_ptr);


  void AddStore(uint32_t ptrId, uint32_t valId,
                std::unique_ptr<BasicBlock>* block_ptr,
                const Instruction* line_inst, const DebugScope& dbg_scope);


  void AddLoad(uint32_t typeId, uint32_t resultId, uint32_t ptrId,
               std::unique_ptr<BasicBlock>* block_ptr,
               const Instruction* line_inst, const DebugScope& dbg_scope);


  std::unique_ptr<Instruction> NewLabel(uint32_t label_id);




  uint32_t GetFalseId();


  void MapParams(Function* calleeFn, BasicBlock::iterator call_inst_itr,
                 std::unordered_map<uint32_t, uint32_t>* callee2caller);


  bool CloneAndMapLocals(Function* calleeFn,
                         std::vector<std::unique_ptr<Instruction>>* new_vars,
                         std::unordered_map<uint32_t, uint32_t>* callee2caller,
                         analysis::DebugInlinedAtContext* inlined_at_ctx);




  uint32_t CreateReturnVar(Function* calleeFn,
                           std::vector<std::unique_ptr<Instruction>>* new_vars);



  bool IsSameBlockOp(const Instruction* inst) const;





  bool CloneSameBlockOps(std::unique_ptr<Instruction>* inst,
                         std::unordered_map<uint32_t, uint32_t>* postCallSB,
                         std::unordered_map<uint32_t, Instruction*>* preCallSB,
                         std::unique_ptr<BasicBlock>* block_ptr);

















  bool GenInlineCode(std::vector<std::unique_ptr<BasicBlock>>* new_blocks,
                     std::vector<std::unique_ptr<Instruction>>* new_vars,
                     BasicBlock::iterator call_inst_itr,
                     UptrVectorIterator<BasicBlock> call_block_itr);


  bool IsInlinableFunctionCall(const Instruction* inst);




  bool HasNoReturnInLoop(Function* func);


  void AnalyzeReturns(Function* func);


  bool IsInlinableFunction(Function* func);



  bool ContainsAbortOtherThanUnreachable(Function* func) const;


  void UpdateSucceedingPhis(
      std::vector<std::unique_ptr<BasicBlock>>& new_blocks);


  void InitializeInline();





  void FixDebugDeclares(Function* func);


  std::unordered_map<uint32_t, Function*> id2function_;



  std::unordered_map<uint32_t, BasicBlock*> id2block_;


  std::set<uint32_t> early_return_funcs_;


  std::set<uint32_t> no_return_in_loop_;


  std::set<uint32_t> inlinable_;


  uint32_t false_id_;



  std::unordered_set<uint32_t> funcs_called_from_continue_;

 private:


  void MoveInstsBeforeEntryBlock(
      std::unordered_map<uint32_t, Instruction*>* preCallSB,
      BasicBlock* new_blk_ptr, BasicBlock::iterator call_inst_itr,
      UptrVectorIterator<BasicBlock> call_block_itr);



  std::unique_ptr<BasicBlock> AddGuardBlock(
      std::vector<std::unique_ptr<BasicBlock>>* new_blocks,
      std::unordered_map<uint32_t, uint32_t>* callee2caller,
      std::unique_ptr<BasicBlock> new_blk_ptr, uint32_t entry_blk_label_id);


  InstructionList::iterator AddStoresForVariableInitializers(
      const std::unordered_map<uint32_t, uint32_t>& callee2caller,
      analysis::DebugInlinedAtContext* inlined_at_ctx,
      std::unique_ptr<BasicBlock>* new_blk_ptr,
      UptrVectorIterator<BasicBlock> callee_block_itr);


  bool InlineSingleInstruction(
      const std::unordered_map<uint32_t, uint32_t>& callee2caller,
      BasicBlock* new_blk_ptr, const Instruction* inst,
      uint32_t dbg_inlined_at);


  std::unique_ptr<BasicBlock> InlineReturn(
      const std::unordered_map<uint32_t, uint32_t>& callee2caller,
      std::vector<std::unique_ptr<BasicBlock>>* new_blocks,
      std::unique_ptr<BasicBlock> new_blk_ptr,
      analysis::DebugInlinedAtContext* inlined_at_ctx, Function* calleeFn,
      const Instruction* inst, uint32_t returnVarId);


  bool InlineEntryBlock(
      const std::unordered_map<uint32_t, uint32_t>& callee2caller,
      std::unique_ptr<BasicBlock>* new_blk_ptr,
      UptrVectorIterator<BasicBlock> callee_first_block,
      analysis::DebugInlinedAtContext* inlined_at_ctx);



  std::unique_ptr<BasicBlock> InlineBasicBlocks(
      std::vector<std::unique_ptr<BasicBlock>>* new_blocks,
      const std::unordered_map<uint32_t, uint32_t>& callee2caller,
      std::unique_ptr<BasicBlock> new_blk_ptr,
      analysis::DebugInlinedAtContext* inlined_at_ctx, Function* calleeFn);



  bool MoveCallerInstsAfterFunctionCall(
      std::unordered_map<uint32_t, Instruction*>* preCallSB,
      std::unordered_map<uint32_t, uint32_t>* postCallSB,
      std::unique_ptr<BasicBlock>* new_blk_ptr,
      BasicBlock::iterator call_inst_itr, bool multiBlocks);


  void MoveLoopMergeInstToFirstBlock(
      std::vector<std::unique_ptr<BasicBlock>>* new_blocks);




  void UpdateSingleBlockLoopContinueTarget(
      uint32_t new_id, std::vector<std::unique_ptr<BasicBlock>>* new_blocks);



  void FixDebugDeclare(Instruction* dbg_declare_inst,
                       const std::map<uint32_t, Instruction*>& access_chains);
};

}
}

#endif
