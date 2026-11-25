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

#include "source/opt/inline_pass.h"

#include <unordered_set>
#include <utility>

#include "source/cfa.h"
#include "source/opt/reflect.h"
#include "source/util/make_unique.h"

namespace spvtools {
namespace opt {
namespace {

constexpr int kSpvFunctionCallFunctionId = 2;
constexpr int kSpvFunctionCallArgumentId = 3;
constexpr int kSpvReturnValueId = 0;
constexpr int kSpvDebugDeclareVarInIdx = 3;
constexpr int kSpvAccessChainBaseInIdx = 0;
}

uint32_t InlinePass::AddPointerToType(uint32_t type_id,
                                      spv::StorageClass storage_class) {
  uint32_t resultId = context()->TakeNextId();
  if (resultId == 0) {
    return resultId;
  }

  std::unique_ptr<Instruction> type_inst(
      new Instruction(context(), spv::Op::OpTypePointer, 0, resultId,
                      {{spv_operand_type_t::SPV_OPERAND_TYPE_STORAGE_CLASS,
                        {uint32_t(storage_class)}},
                       {spv_operand_type_t::SPV_OPERAND_TYPE_ID, {type_id}}}));
  context()->AddType(std::move(type_inst));
  analysis::Type* pointeeTy;
  std::unique_ptr<analysis::Pointer> pointerTy;
  std::tie(pointeeTy, pointerTy) =
      context()->get_type_mgr()->GetTypeAndPointerType(
          type_id, spv::StorageClass::Function);
  context()->get_type_mgr()->RegisterType(resultId, *pointerTy);
  return resultId;
}

void InlinePass::AddBranch(uint32_t label_id,
                           std::unique_ptr<BasicBlock>* block_ptr) {
  std::unique_ptr<Instruction> newBranch(
      new Instruction(context(), spv::Op::OpBranch, 0, 0,
                      {{spv_operand_type_t::SPV_OPERAND_TYPE_ID, {label_id}}}));
  (*block_ptr)->AddInstruction(std::move(newBranch));
}

void InlinePass::AddBranchCond(uint32_t cond_id, uint32_t true_id,
                               uint32_t false_id,
                               std::unique_ptr<BasicBlock>* block_ptr) {
  std::unique_ptr<Instruction> newBranch(
      new Instruction(context(), spv::Op::OpBranchConditional, 0, 0,
                      {{spv_operand_type_t::SPV_OPERAND_TYPE_ID, {cond_id}},
                       {spv_operand_type_t::SPV_OPERAND_TYPE_ID, {true_id}},
                       {spv_operand_type_t::SPV_OPERAND_TYPE_ID, {false_id}}}));
  (*block_ptr)->AddInstruction(std::move(newBranch));
}

void InlinePass::AddLoopMerge(uint32_t merge_id, uint32_t continue_id,
                              std::unique_ptr<BasicBlock>* block_ptr) {
  std::unique_ptr<Instruction> newLoopMerge(new Instruction(
      context(), spv::Op::OpLoopMerge, 0, 0,
      {{spv_operand_type_t::SPV_OPERAND_TYPE_ID, {merge_id}},
       {spv_operand_type_t::SPV_OPERAND_TYPE_ID, {continue_id}},
       {spv_operand_type_t::SPV_OPERAND_TYPE_LOOP_CONTROL, {0}}}));
  (*block_ptr)->AddInstruction(std::move(newLoopMerge));
}

void InlinePass::AddStore(uint32_t ptr_id, uint32_t val_id,
                          std::unique_ptr<BasicBlock>* block_ptr,
                          const Instruction* line_inst,
                          const DebugScope& dbg_scope) {
  std::unique_ptr<Instruction> newStore(
      new Instruction(context(), spv::Op::OpStore, 0, 0,
                      {{spv_operand_type_t::SPV_OPERAND_TYPE_ID, {ptr_id}},
                       {spv_operand_type_t::SPV_OPERAND_TYPE_ID, {val_id}}}));
  if (line_inst != nullptr) {
    newStore->AddDebugLine(line_inst);
  }
  newStore->SetDebugScope(dbg_scope);
  (*block_ptr)->AddInstruction(std::move(newStore));
}

void InlinePass::AddLoad(uint32_t type_id, uint32_t resultId, uint32_t ptr_id,
                         std::unique_ptr<BasicBlock>* block_ptr,
                         const Instruction* line_inst,
                         const DebugScope& dbg_scope) {
  std::unique_ptr<Instruction> newLoad(
      new Instruction(context(), spv::Op::OpLoad, type_id, resultId,
                      {{spv_operand_type_t::SPV_OPERAND_TYPE_ID, {ptr_id}}}));
  if (line_inst != nullptr) {
    newLoad->AddDebugLine(line_inst);
  }
  newLoad->SetDebugScope(dbg_scope);
  (*block_ptr)->AddInstruction(std::move(newLoad));
}

std::unique_ptr<Instruction> InlinePass::NewLabel(uint32_t label_id) {
  std::unique_ptr<Instruction> newLabel(
      new Instruction(context(), spv::Op::OpLabel, 0, label_id, {}));
  return newLabel;
}

uint32_t InlinePass::GetFalseId() {
  if (false_id_ != 0) return false_id_;
  false_id_ = get_module()->GetGlobalValue(spv::Op::OpConstantFalse);
  if (false_id_ != 0) return false_id_;
  uint32_t boolId = get_module()->GetGlobalValue(spv::Op::OpTypeBool);
  if (boolId == 0) {
    boolId = context()->TakeNextId();
    if (boolId == 0) {
      return 0;
    }
    get_module()->AddGlobalValue(spv::Op::OpTypeBool, boolId, 0);
  }
  false_id_ = context()->TakeNextId();
  if (false_id_ == 0) {
    return 0;
  }
  get_module()->AddGlobalValue(spv::Op::OpConstantFalse, false_id_, boolId);
  return false_id_;
}

void InlinePass::MapParams(
    Function* calleeFn, BasicBlock::iterator call_inst_itr,
    std::unordered_map<uint32_t, uint32_t>* callee2caller) {
  int param_idx = 0;
  calleeFn->ForEachParam(
      [&call_inst_itr, &param_idx, &callee2caller](const Instruction* cpi) {
        const uint32_t pid = cpi->result_id();
        (*callee2caller)[pid] = call_inst_itr->GetSingleWordOperand(
            kSpvFunctionCallArgumentId + param_idx);
        ++param_idx;
      });
}

bool InlinePass::CloneAndMapLocals(
    Function* calleeFn, std::vector<std::unique_ptr<Instruction>>* new_vars,
    std::unordered_map<uint32_t, uint32_t>* callee2caller,
    analysis::DebugInlinedAtContext* inlined_at_ctx) {
  auto callee_block_itr = calleeFn->begin();
  auto callee_var_itr = callee_block_itr->begin();
  while (callee_var_itr->opcode() == spv::Op::OpVariable ||
         callee_var_itr->GetCommonDebugOpcode() ==
             CommonDebugInfoDebugDeclare) {
    if (callee_var_itr->opcode() != spv::Op::OpVariable) {
      ++callee_var_itr;
      continue;
    }

    std::unique_ptr<Instruction> var_inst(callee_var_itr->Clone(context()));
    uint32_t newId = context()->TakeNextId();
    if (newId == 0) {
      return false;
    }
    get_decoration_mgr()->CloneDecorations(callee_var_itr->result_id(), newId);
    var_inst->SetResultId(newId);
    var_inst->UpdateDebugInlinedAt(
        context()->get_debug_info_mgr()->BuildDebugInlinedAtChain(
            callee_var_itr->GetDebugInlinedAt(), inlined_at_ctx));
    (*callee2caller)[callee_var_itr->result_id()] = newId;
    new_vars->push_back(std::move(var_inst));
    ++callee_var_itr;
  }
  return true;
}

uint32_t InlinePass::CreateReturnVar(
    Function* calleeFn, std::vector<std::unique_ptr<Instruction>>* new_vars) {
  uint32_t returnVarId = 0;
  const uint32_t calleeTypeId = calleeFn->type_id();
  analysis::TypeManager* type_mgr = context()->get_type_mgr();
  assert(type_mgr->GetType(calleeTypeId)->AsVoid() == nullptr &&
         "Cannot create a return variable of type void.");

  uint32_t returnVarTypeId =
      type_mgr->FindPointerToType(calleeTypeId, spv::StorageClass::Function);

  if (returnVarTypeId == 0) {
    returnVarTypeId =
        AddPointerToType(calleeTypeId, spv::StorageClass::Function);
    if (returnVarTypeId == 0) {
      return 0;
    }
  }


  returnVarId = context()->TakeNextId();
  if (returnVarId == 0) {
    return 0;
  }

  std::unique_ptr<Instruction> var_inst(new Instruction(
      context(), spv::Op::OpVariable, returnVarTypeId, returnVarId,
      {{spv_operand_type_t::SPV_OPERAND_TYPE_STORAGE_CLASS,
        {(uint32_t)spv::StorageClass::Function}}}));
  new_vars->push_back(std::move(var_inst));
  get_decoration_mgr()->CloneDecorations(calleeFn->result_id(), returnVarId);



  auto const pointee_type =
      type_mgr->GetType(returnVarTypeId)->AsPointer()->pointee_type();
  if (pointee_type->AsPointer() != nullptr) {
    if (pointee_type->AsPointer()->storage_class() ==
        spv::StorageClass::PhysicalStorageBuffer) {
      get_decoration_mgr()->AddDecoration(
          returnVarId, uint32_t(spv::Decoration::AliasedPointer));
    }
  }

  return returnVarId;
}

bool InlinePass::IsSameBlockOp(const Instruction* inst) const {
  return inst->opcode() == spv::Op::OpSampledImage ||
         inst->opcode() == spv::Op::OpImage;
}

bool InlinePass::CloneSameBlockOps(
    std::unique_ptr<Instruction>* inst,
    std::unordered_map<uint32_t, uint32_t>* postCallSB,
    std::unordered_map<uint32_t, Instruction*>* preCallSB,
    std::unique_ptr<BasicBlock>* block_ptr) {
  return (*inst)->WhileEachInId([&postCallSB, &preCallSB, &block_ptr,
                                 this](uint32_t* iid) {
    const auto mapItr = (*postCallSB).find(*iid);
    if (mapItr == (*postCallSB).end()) {
      const auto mapItr2 = (*preCallSB).find(*iid);
      if (mapItr2 != (*preCallSB).end()) {

        const Instruction* inInst = mapItr2->second;
        std::unique_ptr<Instruction> sb_inst(inInst->Clone(context()));
        if (!CloneSameBlockOps(&sb_inst, postCallSB, preCallSB, block_ptr)) {
          return false;
        }

        const uint32_t rid = sb_inst->result_id();
        const uint32_t nid = context()->TakeNextId();
        if (nid == 0) {
          return false;
        }
        get_decoration_mgr()->CloneDecorations(rid, nid);
        sb_inst->SetResultId(nid);
        (*postCallSB)[rid] = nid;
        *iid = nid;
        (*block_ptr)->AddInstruction(std::move(sb_inst));
      }
    } else {

      *iid = mapItr->second;
    }
    return true;
  });
}

void InlinePass::MoveInstsBeforeEntryBlock(
    std::unordered_map<uint32_t, Instruction*>* preCallSB,
    BasicBlock* new_blk_ptr, BasicBlock::iterator call_inst_itr,
    UptrVectorIterator<BasicBlock> call_block_itr) {
  for (auto cii = call_block_itr->begin(); cii != call_inst_itr;
       cii = call_block_itr->begin()) {
    Instruction* inst = &*cii;
    inst->RemoveFromList();
    std::unique_ptr<Instruction> cp_inst(inst);

    if (IsSameBlockOp(&*cp_inst)) {
      auto* sb_inst_ptr = cp_inst.get();
      (*preCallSB)[cp_inst->result_id()] = sb_inst_ptr;
    }
    new_blk_ptr->AddInstruction(std::move(cp_inst));
  }
}

std::unique_ptr<BasicBlock> InlinePass::AddGuardBlock(
    std::vector<std::unique_ptr<BasicBlock>>* new_blocks,
    std::unordered_map<uint32_t, uint32_t>* callee2caller,
    std::unique_ptr<BasicBlock> new_blk_ptr, uint32_t entry_blk_label_id) {
  const auto guard_block_id = context()->TakeNextId();
  if (guard_block_id == 0) {
    return nullptr;
  }
  AddBranch(guard_block_id, &new_blk_ptr);
  new_blocks->push_back(std::move(new_blk_ptr));

  new_blk_ptr = MakeUnique<BasicBlock>(NewLabel(guard_block_id));



  (*callee2caller)[entry_blk_label_id] = guard_block_id;
  return new_blk_ptr;
}

InstructionList::iterator InlinePass::AddStoresForVariableInitializers(
    const std::unordered_map<uint32_t, uint32_t>& callee2caller,
    analysis::DebugInlinedAtContext* inlined_at_ctx,
    std::unique_ptr<BasicBlock>* new_blk_ptr,
    UptrVectorIterator<BasicBlock> callee_first_block_itr) {
  auto callee_itr = callee_first_block_itr->begin();
  while (callee_itr->opcode() == spv::Op::OpVariable ||
         callee_itr->GetCommonDebugOpcode() == CommonDebugInfoDebugDeclare) {
    if (callee_itr->opcode() == spv::Op::OpVariable &&
        callee_itr->NumInOperands() == 2) {
      assert(callee2caller.count(callee_itr->result_id()) &&
             "Expected the variable to have already been mapped.");
      uint32_t new_var_id = callee2caller.at(callee_itr->result_id());



      uint32_t val_id = callee_itr->GetSingleWordInOperand(1);
      AddStore(new_var_id, val_id, new_blk_ptr, callee_itr->dbg_line_inst(),
               context()->get_debug_info_mgr()->BuildDebugScope(
                   callee_itr->GetDebugScope(), inlined_at_ctx));
    }
    if (callee_itr->GetCommonDebugOpcode() == CommonDebugInfoDebugDeclare) {
      InlineSingleInstruction(
          callee2caller, new_blk_ptr->get(), &*callee_itr,
          context()->get_debug_info_mgr()->BuildDebugInlinedAtChain(
              callee_itr->GetDebugScope().GetInlinedAt(), inlined_at_ctx));
    }
    ++callee_itr;
  }
  return callee_itr;
}

bool InlinePass::InlineSingleInstruction(
    const std::unordered_map<uint32_t, uint32_t>& callee2caller,
    BasicBlock* new_blk_ptr, const Instruction* inst, uint32_t dbg_inlined_at) {


  if (inst->opcode() == spv::Op::OpReturnValue ||
      inst->opcode() == spv::Op::OpReturn)
    return true;


  std::unique_ptr<Instruction> cp_inst(inst->Clone(context()));
  cp_inst->ForEachInId([&callee2caller](uint32_t* iid) {
    const auto mapItr = callee2caller.find(*iid);
    if (mapItr != callee2caller.end()) {
      *iid = mapItr->second;
    }
  });


  const uint32_t rid = cp_inst->result_id();
  if (rid != 0) {
    const auto mapItr = callee2caller.find(rid);
    if (mapItr == callee2caller.end()) {
      return false;
    }
    uint32_t nid = mapItr->second;
    cp_inst->SetResultId(nid);
    get_decoration_mgr()->CloneDecorations(rid, nid);
  }

  cp_inst->UpdateDebugInlinedAt(dbg_inlined_at);
  new_blk_ptr->AddInstruction(std::move(cp_inst));
  return true;
}

std::unique_ptr<BasicBlock> InlinePass::InlineReturn(
    const std::unordered_map<uint32_t, uint32_t>& callee2caller,
    std::vector<std::unique_ptr<BasicBlock>>* new_blocks,
    std::unique_ptr<BasicBlock> new_blk_ptr,
    analysis::DebugInlinedAtContext* inlined_at_ctx, Function* calleeFn,
    const Instruction* inst, uint32_t returnVarId) {

  if (inst->opcode() == spv::Op::OpReturnValue) {
    assert(returnVarId != 0);
    uint32_t valId = inst->GetInOperand(kSpvReturnValueId).words[0];
    const auto mapItr = callee2caller.find(valId);
    if (mapItr != callee2caller.end()) {
      valId = mapItr->second;
    }
    AddStore(returnVarId, valId, &new_blk_ptr, inst->dbg_line_inst(),
             context()->get_debug_info_mgr()->BuildDebugScope(
                 inst->GetDebugScope(), inlined_at_ctx));
  }

  uint32_t returnLabelId = 0;
  for (auto callee_block_itr = calleeFn->begin();
       callee_block_itr != calleeFn->end(); ++callee_block_itr) {
    if (spvOpcodeIsAbort(callee_block_itr->tail()->opcode())) {
      returnLabelId = context()->TakeNextId();
      break;
    }
  }
  if (returnLabelId == 0) return new_blk_ptr;

  if (inst->opcode() == spv::Op::OpReturn ||
      inst->opcode() == spv::Op::OpReturnValue)
    AddBranch(returnLabelId, &new_blk_ptr);
  new_blocks->push_back(std::move(new_blk_ptr));
  return MakeUnique<BasicBlock>(NewLabel(returnLabelId));
}

bool InlinePass::InlineEntryBlock(
    const std::unordered_map<uint32_t, uint32_t>& callee2caller,
    std::unique_ptr<BasicBlock>* new_blk_ptr,
    UptrVectorIterator<BasicBlock> callee_first_block,
    analysis::DebugInlinedAtContext* inlined_at_ctx) {
  auto callee_inst_itr = AddStoresForVariableInitializers(
      callee2caller, inlined_at_ctx, new_blk_ptr, callee_first_block);

  while (callee_inst_itr != callee_first_block->end()) {


    if (callee_inst_itr->GetShaderDebugOpcode() ==
        NonSemanticShaderDebugInfoDebugFunctionDefinition) {
      ++callee_inst_itr;
      continue;
    }

    if (!InlineSingleInstruction(
            callee2caller, new_blk_ptr->get(), &*callee_inst_itr,
            context()->get_debug_info_mgr()->BuildDebugInlinedAtChain(
                callee_inst_itr->GetDebugScope().GetInlinedAt(),
                inlined_at_ctx))) {
      return false;
    }
    ++callee_inst_itr;
  }
  return true;
}

std::unique_ptr<BasicBlock> InlinePass::InlineBasicBlocks(
    std::vector<std::unique_ptr<BasicBlock>>* new_blocks,
    const std::unordered_map<uint32_t, uint32_t>& callee2caller,
    std::unique_ptr<BasicBlock> new_blk_ptr,
    analysis::DebugInlinedAtContext* inlined_at_ctx, Function* calleeFn) {
  auto callee_block_itr = calleeFn->begin();
  ++callee_block_itr;

  while (callee_block_itr != calleeFn->end()) {
    new_blocks->push_back(std::move(new_blk_ptr));
    const auto mapItr =
        callee2caller.find(callee_block_itr->GetLabelInst()->result_id());
    if (mapItr == callee2caller.end()) return nullptr;
    new_blk_ptr = MakeUnique<BasicBlock>(NewLabel(mapItr->second));

    auto tail_inst_itr = callee_block_itr->end();
    for (auto inst_itr = callee_block_itr->begin(); inst_itr != tail_inst_itr;
         ++inst_itr) {


      if (inst_itr->GetShaderDebugOpcode() ==
          NonSemanticShaderDebugInfoDebugFunctionDefinition)
        continue;
      if (!InlineSingleInstruction(
              callee2caller, new_blk_ptr.get(), &*inst_itr,
              context()->get_debug_info_mgr()->BuildDebugInlinedAtChain(
                  inst_itr->GetDebugScope().GetInlinedAt(), inlined_at_ctx))) {
        return nullptr;
      }
    }

    ++callee_block_itr;
  }
  return new_blk_ptr;
}

bool InlinePass::MoveCallerInstsAfterFunctionCall(
    std::unordered_map<uint32_t, Instruction*>* preCallSB,
    std::unordered_map<uint32_t, uint32_t>* postCallSB,
    std::unique_ptr<BasicBlock>* new_blk_ptr,
    BasicBlock::iterator call_inst_itr, bool multiBlocks) {

  for (Instruction* inst = call_inst_itr->NextNode(); inst;
       inst = call_inst_itr->NextNode()) {
    inst->RemoveFromList();
    std::unique_ptr<Instruction> cp_inst(inst);


    if (multiBlocks) {
      if (!CloneSameBlockOps(&cp_inst, postCallSB, preCallSB, new_blk_ptr)) {
        return false;
      }


      if (IsSameBlockOp(&*cp_inst)) {
        const uint32_t rid = cp_inst->result_id();
        (*postCallSB)[rid] = rid;
      }
    }
    new_blk_ptr->get()->AddInstruction(std::move(cp_inst));
  }

  return true;
}

void InlinePass::MoveLoopMergeInstToFirstBlock(
    std::vector<std::unique_ptr<BasicBlock>>* new_blocks) {


  auto& first = new_blocks->front();
  auto& last = new_blocks->back();
  assert(first != last);


  auto loop_merge_itr = last->tail();
  --loop_merge_itr;
  assert(loop_merge_itr->opcode() == spv::Op::OpLoopMerge);
  std::unique_ptr<Instruction> cp_inst(loop_merge_itr->Clone(context()));
  first->tail().InsertBefore(std::move(cp_inst));


  loop_merge_itr->RemoveFromList();
  delete &*loop_merge_itr;
}

void InlinePass::UpdateSingleBlockLoopContinueTarget(
    uint32_t new_id, std::vector<std::unique_ptr<BasicBlock>>* new_blocks) {
  auto& header = new_blocks->front();
  auto* merge_inst = header->GetLoopMergeInst();










  std::unique_ptr<BasicBlock> new_block =
      MakeUnique<BasicBlock>(NewLabel(new_id));
  auto& old_backedge = new_blocks->back();
  auto old_branch = old_backedge->tail();


  std::unique_ptr<Instruction> br(&*old_branch);
  new_block->AddInstruction(std::move(br));


  AddBranch(new_id, &old_backedge);
  new_blocks->push_back(std::move(new_block));


  merge_inst->SetInOperand(1u, {new_id});
}

bool InlinePass::GenInlineCode(
    std::vector<std::unique_ptr<BasicBlock>>* new_blocks,
    std::vector<std::unique_ptr<Instruction>>* new_vars,
    BasicBlock::iterator call_inst_itr,
    UptrVectorIterator<BasicBlock> call_block_itr) {


  std::unordered_map<uint32_t, uint32_t> callee2caller;

  std::unordered_map<uint32_t, Instruction*> preCallSB;

  std::unordered_map<uint32_t, uint32_t> postCallSB;

  analysis::DebugInlinedAtContext inlined_at_ctx(&*call_inst_itr);




  context()->InvalidateAnalyses(IRContext::kAnalysisDefUse);






  bool caller_is_loop_header = call_block_itr->GetLoopMergeInst() != nullptr;


  std::unique_ptr<BasicBlock> single_trip_loop_cont_blk;

  Function* calleeFn = id2function_[call_inst_itr->GetSingleWordOperand(
      kSpvFunctionCallFunctionId)];


  MapParams(calleeFn, call_inst_itr, &callee2caller);



  if (!CloneAndMapLocals(calleeFn, new_vars, &callee2caller, &inlined_at_ctx)) {
    return false;
  }



  uint32_t entry_blk_label_id = calleeFn->begin()->GetLabelInst()->result_id();
  callee2caller[entry_blk_label_id] = call_block_itr->id();
  std::unique_ptr<BasicBlock> new_blk_ptr =
      MakeUnique<BasicBlock>(NewLabel(call_block_itr->id()));


  MoveInstsBeforeEntryBlock(&preCallSB, new_blk_ptr.get(), call_inst_itr,
                            call_block_itr);

  if (caller_is_loop_header &&
      (*(calleeFn->begin())).GetMergeInst() != nullptr) {







    new_blk_ptr = AddGuardBlock(new_blocks, &callee2caller,
                                std::move(new_blk_ptr), entry_blk_label_id);
    if (new_blk_ptr == nullptr) return false;
  }


  const uint32_t calleeTypeId = calleeFn->type_id();
  uint32_t returnVarId = 0;
  analysis::Type* calleeType = context()->get_type_mgr()->GetType(calleeTypeId);
  if (calleeType->AsVoid() == nullptr) {
    returnVarId = CreateReturnVar(calleeFn, new_vars);
    if (returnVarId == 0) {
      return false;
    }
  }

  calleeFn->WhileEachInst([&callee2caller, this](const Instruction* cpi) {

    const uint32_t rid = cpi->result_id();
    if (rid != 0 && callee2caller.find(rid) == callee2caller.end()) {
      const uint32_t nid = context()->TakeNextId();
      if (nid == 0) return false;
      callee2caller[rid] = nid;
    }
    return true;
  });


  calleeFn->ForEachDebugInstructionsInHeader(
      [&new_blk_ptr, &callee2caller, &inlined_at_ctx, this](Instruction* inst) {
        InlineSingleInstruction(
            callee2caller, new_blk_ptr.get(), inst,
            context()->get_debug_info_mgr()->BuildDebugInlinedAtChain(
                inst->GetDebugScope().GetInlinedAt(), &inlined_at_ctx));
      });


  if (!InlineEntryBlock(callee2caller, &new_blk_ptr, calleeFn->begin(),
                        &inlined_at_ctx)) {
    return false;
  }


  new_blk_ptr =
      InlineBasicBlocks(new_blocks, callee2caller, std::move(new_blk_ptr),
                        &inlined_at_ctx, calleeFn);
  if (new_blk_ptr == nullptr) return false;

  new_blk_ptr = InlineReturn(callee2caller, new_blocks, std::move(new_blk_ptr),
                             &inlined_at_ctx, calleeFn,
                             &*(calleeFn->tail()->tail()), returnVarId);


  if (returnVarId != 0) {
    const uint32_t resId = call_inst_itr->result_id();
    assert(resId != 0);
    AddLoad(calleeTypeId, resId, returnVarId, &new_blk_ptr,
            call_inst_itr->dbg_line_inst(), call_inst_itr->GetDebugScope());
  }


  if (!MoveCallerInstsAfterFunctionCall(&preCallSB, &postCallSB, &new_blk_ptr,
                                        call_inst_itr,
                                        calleeFn->begin() != calleeFn->end()))
    return false;


  new_blocks->push_back(std::move(new_blk_ptr));

  if (caller_is_loop_header && (new_blocks->size() > 1)) {
    MoveLoopMergeInstToFirstBlock(new_blocks);


    auto& header = new_blocks->front();
    auto* merge_inst = header->GetLoopMergeInst();
    if (merge_inst->GetSingleWordInOperand(1u) == header->id()) {
      auto new_id = context()->TakeNextId();
      if (new_id == 0) return false;
      UpdateSingleBlockLoopContinueTarget(new_id, new_blocks);
    }
  }


  for (auto& blk : *new_blocks) {
    id2block_[blk->id()] = &*blk;
  }



  context()->KillNamesAndDecorates(&*call_inst_itr);

  return true;
}

bool InlinePass::IsInlinableFunctionCall(const Instruction* inst) {
  if (inst->opcode() != spv::Op::OpFunctionCall) return false;
  const uint32_t calleeFnId =
      inst->GetSingleWordOperand(kSpvFunctionCallFunctionId);
  const auto ci = inlinable_.find(calleeFnId);
  if (ci == inlinable_.cend()) return false;

  if (early_return_funcs_.find(calleeFnId) != early_return_funcs_.end()) {


    std::string message =
        "The function '" + id2function_[calleeFnId]->DefInst().PrettyPrint() +
        "' could not be inlined because the return instruction "
        "is not at the end of the function. This could be fixed by "
        "running merge-return before inlining.";
    consumer()(SPV_MSG_WARNING, "", {0, 0, 0}, message.c_str());
    return false;
  }

  return true;
}

void InlinePass::UpdateSucceedingPhis(
    std::vector<std::unique_ptr<BasicBlock>>& new_blocks) {
  const auto firstBlk = new_blocks.begin();
  const auto lastBlk = new_blocks.end() - 1;
  const uint32_t firstId = (*firstBlk)->id();
  const uint32_t lastId = (*lastBlk)->id();
  const BasicBlock& const_last_block = *lastBlk->get();
  const_last_block.ForEachSuccessorLabel(
      [&firstId, &lastId, this](const uint32_t succ) {
        BasicBlock* sbp = this->id2block_[succ];
        sbp->ForEachPhiInst([&firstId, &lastId](Instruction* phi) {
          phi->ForEachInId([&firstId, &lastId](uint32_t* id) {
            if (*id == firstId) *id = lastId;
          });
        });
      });
}

bool InlinePass::HasNoReturnInLoop(Function* func) {


  if (!context()->get_feature_mgr()->HasCapability(spv::Capability::Shader))
    return false;
  const auto structured_analysis = context()->GetStructuredCFGAnalysis();

  bool return_in_loop = false;
  for (auto& blk : *func) {
    auto terminal_ii = blk.cend();
    --terminal_ii;
    if (spvOpcodeIsReturn(terminal_ii->opcode()) &&
        structured_analysis->ContainingLoop(blk.id()) != 0) {
      return_in_loop = true;
      break;
    }
  }
  return !return_in_loop;
}

void InlinePass::AnalyzeReturns(Function* func) {

  if (HasNoReturnInLoop(func)) {
    no_return_in_loop_.insert(func->result_id());
  }

  for (auto& blk : *func) {
    auto terminal_ii = blk.cend();
    --terminal_ii;
    if (spvOpcodeIsReturn(terminal_ii->opcode()) && &blk != func->tail()) {
      early_return_funcs_.insert(func->result_id());
      break;
    }
  }
}

bool InlinePass::IsInlinableFunction(Function* func) {

  if (func->cbegin() == func->cend()) return false;


  if (func->control_mask() & uint32_t(spv::FunctionControlMask::DontInline)) {
    return false;
  }






  AnalyzeReturns(func);
  if (no_return_in_loop_.find(func->result_id()) == no_return_in_loop_.cend()) {
    return false;
  }

  if (func->IsRecursive()) {
    return false;
  }






  bool func_is_called_from_continue =
      funcs_called_from_continue_.count(func->result_id()) != 0;

  if (func_is_called_from_continue && ContainsAbortOtherThanUnreachable(func)) {
    return false;
  }

  return true;
}

bool InlinePass::ContainsAbortOtherThanUnreachable(Function* func) const {
  return !func->WhileEachInst([](Instruction* inst) {
    return inst->opcode() == spv::Op::OpUnreachable ||
           !spvOpcodeIsAbort(inst->opcode());
  });
}

void InlinePass::InitializeInline() {
  false_id_ = 0;


  id2function_.clear();
  id2block_.clear();
  inlinable_.clear();
  no_return_in_loop_.clear();
  early_return_funcs_.clear();
  funcs_called_from_continue_ =
      context()->GetStructuredCFGAnalysis()->FindFuncsCalledFromContinue();

  for (auto& fn : *get_module()) {

    id2function_[fn.result_id()] = &fn;
    for (auto& blk : fn) {
      id2block_[blk.id()] = &blk;
    }

    if (IsInlinableFunction(&fn)) inlinable_.insert(fn.result_id());
  }
}

InlinePass::InlinePass() {}

void InlinePass::FixDebugDeclares(Function* func) {
  std::map<uint32_t, Instruction*> access_chains;
  std::vector<Instruction*> debug_declare_insts;

  func->ForEachInst([&access_chains, &debug_declare_insts](Instruction* inst) {
    if (inst->opcode() == spv::Op::OpAccessChain) {
      access_chains[inst->result_id()] = inst;
    }
    if (inst->GetCommonDebugOpcode() == CommonDebugInfoDebugDeclare) {
      debug_declare_insts.push_back(inst);
    }
  });

  for (auto& inst : debug_declare_insts) {
    FixDebugDeclare(inst, access_chains);
  }
}

void InlinePass::FixDebugDeclare(
    Instruction* dbg_declare_inst,
    const std::map<uint32_t, Instruction*>& access_chains) {
  do {
    uint32_t var_id =
        dbg_declare_inst->GetSingleWordInOperand(kSpvDebugDeclareVarInIdx);



    auto it = access_chains.find(var_id);
    if (it == access_chains.end()) {
      return;
    }
    Instruction* access_chain = it->second;









    bool has_non_constant_index = false;
    for (uint32_t i = kSpvAccessChainBaseInIdx + 1;
         i < access_chain->NumInOperands(); ++i) {
      uint32_t idx_id = access_chain->GetSingleWordInOperand(i);
      bool found_constant = false;
      for (auto& inst : context()->module()->types_values()) {
        if (inst.result_id() == idx_id) {
          found_constant = spvOpcodeIsConstant(inst.opcode());
          break;
        }
      }
      if (!found_constant) {
        has_non_constant_index = true;
        break;
      }
    }
    if (has_non_constant_index) {
      context()->KillInst(dbg_declare_inst);
      return;
    }

    std::vector<Operand> operands;
    for (int i = 0; i < kSpvDebugDeclareVarInIdx; i++) {
      operands.push_back(dbg_declare_inst->GetInOperand(i));
    }

    uint32_t access_chain_base =
        access_chain->GetSingleWordInOperand(kSpvAccessChainBaseInIdx);
    operands.push_back(Operand(SPV_OPERAND_TYPE_ID, {access_chain_base}));
    operands.push_back(
        dbg_declare_inst->GetInOperand(kSpvDebugDeclareVarInIdx + 1));

    for (uint32_t i = kSpvAccessChainBaseInIdx + 1;
         i < access_chain->NumInOperands(); ++i) {
      operands.push_back(access_chain->GetInOperand(i));
    }

    for (uint32_t i = kSpvDebugDeclareVarInIdx + 2;
         i < dbg_declare_inst->NumInOperands(); ++i) {
      operands.push_back(dbg_declare_inst->GetInOperand(i));
    }

    dbg_declare_inst->SetInOperands(std::move(operands));
  } while (true);
}

}
}
