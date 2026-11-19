// Copyright (c) 2018 Google LLC.
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

#include "source/opt/loop_unroller.h"

#include <limits>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "source/opt/ir_builder.h"
#include "source/opt/loop_utils.h"








































namespace spvtools {
namespace opt {
namespace {


constexpr uint32_t kLoopControlDontUnrollIndex = 2;


constexpr uint32_t kLoopControlIndex = 2;






struct LoopUnrollState {
  LoopUnrollState()
      : previous_phi_(nullptr),
        previous_latch_block_(nullptr),
        previous_condition_block_(nullptr),
        new_phi(nullptr),
        new_continue_block(nullptr),
        new_condition_block(nullptr),
        new_header_block(nullptr) {}


  LoopUnrollState(Instruction* induction, BasicBlock* latch_block,
                  BasicBlock* condition, std::vector<Instruction*>&& phis)
      : previous_phi_(induction),
        previous_latch_block_(latch_block),
        previous_condition_block_(condition),
        new_phi(nullptr),
        new_continue_block(nullptr),
        new_condition_block(nullptr),
        new_header_block(nullptr) {
    previous_phis_ = std::move(phis);
  }


  void NextIterationState() {
    previous_phi_ = new_phi;
    previous_latch_block_ = new_latch_block;
    previous_condition_block_ = new_condition_block;
    previous_phis_ = std::move(new_phis_);


    new_phi = nullptr;
    new_continue_block = nullptr;
    new_condition_block = nullptr;
    new_header_block = nullptr;
    new_latch_block = nullptr;


    new_blocks.clear();
    new_inst.clear();
    ids_to_new_inst.clear();
  }


  Instruction* previous_phi_;


  std::vector<Instruction*> previous_phis_;

  std::vector<Instruction*> new_phis_;



  BasicBlock* previous_latch_block_;


  BasicBlock* previous_condition_block_;


  Instruction* new_phi;


  BasicBlock* new_continue_block;


  BasicBlock* new_condition_block;


  BasicBlock* new_header_block;


  BasicBlock* new_latch_block;



  std::unordered_map<uint32_t, BasicBlock*> new_blocks;



  std::unordered_map<uint32_t, uint32_t> new_inst;

  std::unordered_map<uint32_t, Instruction*> ids_to_new_inst;
};



class LoopUnrollerUtilsImpl {
 public:
  using BasicBlockListTy = std::vector<std::unique_ptr<BasicBlock>>;

  LoopUnrollerUtilsImpl(IRContext* c, Function* function)
      : context_(c),
        function_(*function),
        loop_condition_block_(nullptr),
        loop_induction_variable_(nullptr),
        number_of_loop_iterations_(0),
        loop_step_value_(0),
        loop_init_value_(0) {}



  bool PartiallyUnroll(Loop*, size_t factor);







  bool PartiallyUnrollResidualFactor(Loop* loop, size_t factor);




  bool FullyUnroll(Loop* loop);


  uint32_t GetPhiDefID(const Instruction* phi, uint32_t label) const;



  void CloseUnrolledLoop(Loop* loop);




  bool FoldConditionBlock(BasicBlock* condtion_block, uint32_t new_target);


  void AddBlocksToFunction(const BasicBlock* insert_point);




  bool DuplicateLoop(Loop* old_loop, Loop* new_loop);

  inline size_t GetLoopIterationCount() const {
    return number_of_loop_iterations_;
  }


  void Init(Loop* loop);




  void ReplaceInductionUseWithFinalValue(Loop* loop);


  void RemoveDeadInstructions();



  void ReplaceOutsideLoopUseWithFinalValue(Loop* loop);



  void MarkLoopControlAsDontUnroll(Loop* loop) const;

 private:




  bool AssignNewResultIds(BasicBlock* basic_block);



  void RemapOperands(Instruction* inst);





  void RemapOperands(BasicBlock* basic_block);






  bool CopyBody(Loop* loop, bool eliminate_conditions);




  bool CopyBasicBlock(Loop* loop, const BasicBlock* block_to_copy,
                      bool preserve_instructions);




  bool Unroll(Loop* loop, size_t factor);



  void ComputeLoopOrderedBlocks(Loop* loop);



  void AddBlocksToLoop(Loop* loop) const;





  void LinkLastPhisToStart(Loop* loop) const;


  void KillDebugDeclares(BasicBlock* bb);



  IRContext* context_;


  Function& function_;



  BasicBlockListTy blocks_to_add_;


  std::vector<Instruction*> invalidated_instructions_;


  LoopUnrollState state_;


  std::vector<BasicBlock*> loop_blocks_inorder_;



  BasicBlock* loop_condition_block_;


  Instruction* loop_induction_variable_;



  std::vector<Instruction*> loop_phi_instructions_;


  size_t number_of_loop_iterations_;


  int64_t loop_step_value_;


  int64_t loop_init_value_;
};







uint32_t GetPhiIndexFromLabel(const BasicBlock* block, const Instruction* phi) {
  for (uint32_t i = 1; i < phi->NumInOperands(); i += 2) {
    if (block->id() == phi->GetSingleWordInOperand(i)) {
      return i;
    }
  }
  assert(false && "Could not find operand in instruction.");
  return 0;
}

void LoopUnrollerUtilsImpl::Init(Loop* loop) {
  loop_condition_block_ = loop->FindConditionBlock();





  if (!loop_condition_block_) {
    loop_condition_block_ = state_.new_condition_block;
  }
  assert(loop_condition_block_);

  loop_induction_variable_ = loop->FindConditionVariable(loop_condition_block_);
  assert(loop_induction_variable_);

  bool found = loop->FindNumberOfIterations(
      loop_induction_variable_, &*loop_condition_block_->ctail(),
      &number_of_loop_iterations_, &loop_step_value_, &loop_init_value_);
  (void)found;
  assert(found);



  ComputeLoopOrderedBlocks(loop);
}






bool LoopUnrollerUtilsImpl::PartiallyUnrollResidualFactor(Loop* loop,
                                                          size_t factor) {
  uint32_t new_label_id = context_->TakeNextId();
  if (new_label_id == 0) return false;
  std::unique_ptr<Instruction> new_label{
      new Instruction(context_, spv::Op::OpLabel, 0, new_label_id, {})};
  std::unique_ptr<BasicBlock> new_exit_bb{new BasicBlock(std::move(new_label))};
  new_exit_bb->SetParent(&function_);


  uint32_t new_merge_id = new_exit_bb->id();



  blocks_to_add_.push_back(std::move(new_exit_bb));
  BasicBlock* new_exit_bb_raw = blocks_to_add_[0].get();
  Instruction& original_conditional_branch = *loop_condition_block_->tail();




  std::unique_ptr<Loop> new_loop = MakeUnique<Loop>(*loop);


  new_loop->ClearBlocks();

  if (!DuplicateLoop(loop, new_loop.get())) {
    return false;
  }


  AddBlocksToFunction(loop->GetMergeBlock());
  blocks_to_add_.clear();


  InstructionBuilder builder{context_, new_exit_bb_raw};

  builder.AddBranch(new_loop->GetHeaderBlock()->id());

  loop_condition_block_ = state_.new_condition_block;
  loop_induction_variable_ = state_.new_phi;


  if (!Unroll(new_loop.get(), factor)) {
    return false;
  }

  LinkLastPhisToStart(new_loop.get());
  AddBlocksToLoop(new_loop.get());



  blocks_to_add_.push_back(
      std::unique_ptr<BasicBlock>(new_loop->GetMergeBlock()));


  AddBlocksToFunction(loop->GetMergeBlock());


  context_->InvalidateAnalysesExceptFor(
      IRContext::Analysis::kAnalysisLoopAnalysis);
  analysis::DefUseManager* def_use_manager = context_->get_def_use_mgr();


  Instruction* condition_check = def_use_manager->GetDef(
      original_conditional_branch.GetSingleWordOperand(0));



  assert(loop->IsSupportedCondition(condition_check->opcode()));


  int64_t remainder = Loop::GetResidualConditionValue(
      condition_check->opcode(), loop_init_value_, loop_step_value_,
      number_of_loop_iterations_, factor);

  assert(remainder > std::numeric_limits<int32_t>::min() &&
         remainder < std::numeric_limits<int32_t>::max());

  Instruction* new_constant = nullptr;



  if (remainder < 0) {
    new_constant = builder.GetSintConstant(static_cast<int32_t>(remainder));
  } else {
    new_constant = builder.GetUintConstant(static_cast<int32_t>(remainder));
  }

  if (!new_constant) {
    return false;
  }

  uint32_t constant_id = new_constant->result_id();


  condition_check->SetInOperand(1, {constant_id});





  std::vector<Instruction*> new_inductions;
  new_loop->GetInductionVariables(new_inductions);

  std::vector<Instruction*> old_inductions;
  loop->GetInductionVariables(old_inductions);
  for (size_t index = 0; index < new_inductions.size(); ++index) {
    Instruction* new_induction = new_inductions[index];
    Instruction* old_induction = old_inductions[index];
    if (!new_induction || !old_induction) {
      return false;
    }


    uint32_t initalizer_index =
        GetPhiIndexFromLabel(new_loop->GetPreHeaderBlock(), old_induction);


    new_induction->SetInOperand(initalizer_index - 1,
                                {old_induction->result_id()});
    new_induction->SetInOperand(initalizer_index, {new_merge_id});



    uint32_t second_loop_induction = new_induction->result_id();
    auto replace_use_outside_of_loop = [loop, second_loop_induction](
                                           Instruction* user,
                                           uint32_t operand_index) {
      if (!loop->IsInsideLoop(user)) {
        user->SetOperand(operand_index, {second_loop_induction});
      }
    };

    context_->get_def_use_mgr()->ForEachUse(old_induction,
                                            replace_use_outside_of_loop);
  }

  context_->InvalidateAnalysesExceptFor(
      IRContext::Analysis::kAnalysisLoopAnalysis);

  context_->ReplaceAllUsesWith(loop->GetMergeBlock()->id(), new_merge_id);

  LoopDescriptor& loop_descriptor = *context_->GetLoopDescriptor(&function_);

  loop_descriptor.AddLoop(std::move(new_loop), loop->GetParent());

  RemoveDeadInstructions();
  return true;
}



void LoopUnrollerUtilsImpl::MarkLoopControlAsDontUnroll(Loop* loop) const {
  Instruction* loop_merge_inst = loop->GetHeaderBlock()->GetLoopMergeInst();
  assert(loop_merge_inst &&
         "Loop merge instruction could not be found after entering unroller "
         "(should have exited before this)");
  loop_merge_inst->SetInOperand(kLoopControlIndex,
                                {kLoopControlDontUnrollIndex});
}




bool LoopUnrollerUtilsImpl::Unroll(Loop* loop, size_t factor) {



  MarkLoopControlAsDontUnroll(loop);

  std::vector<Instruction*> inductions;
  loop->GetInductionVariables(inductions);
  state_ = LoopUnrollState{loop_induction_variable_, loop->GetLatchBlock(),
                           loop_condition_block_, std::move(inductions)};
  for (size_t i = 0; i < factor - 1; ++i) {
    if (!CopyBody(loop, true)) {
      return false;
    }
  }
  return true;
}

void LoopUnrollerUtilsImpl::RemoveDeadInstructions() {

  for (Instruction* inst : invalidated_instructions_) {
    context_->KillInst(inst);
  }
}

void LoopUnrollerUtilsImpl::ReplaceInductionUseWithFinalValue(Loop* loop) {
  context_->InvalidateAnalysesExceptFor(
      IRContext::Analysis::kAnalysisLoopAnalysis |
      IRContext::Analysis::kAnalysisDefUse |
      IRContext::Analysis::kAnalysisInstrToBlockMapping);

  std::vector<Instruction*> inductions;
  loop->GetInductionVariables(inductions);

  for (size_t index = 0; index < inductions.size(); ++index) {






    context_->KillNamesAndDecorates(state_.previous_phis_[index]);
    context_->KillNamesAndDecorates(inductions[index]);

    uint32_t trip_step_id = GetPhiDefID(state_.previous_phis_[index],
                                        state_.previous_latch_block_->id());
    context_->ReplaceAllUsesWith(inductions[index]->result_id(), trip_step_id);
    invalidated_instructions_.push_back(inductions[index]);
  }
}



bool LoopUnrollerUtilsImpl::FullyUnroll(Loop* loop) {

  if (!Unroll(loop, number_of_loop_iterations_)) {
    return false;
  }


  if (!FoldConditionBlock(loop_condition_block_, 1)) {
    return false;
  }


  CloseUnrolledLoop(loop);



  loop->MarkLoopForRemoval();


  if (loop->GetParent()) {
    AddBlocksToLoop(loop->GetParent());
  }


  AddBlocksToFunction(loop->GetMergeBlock());

  ReplaceInductionUseWithFinalValue(loop);

  RemoveDeadInstructions();

  context_->InvalidateAnalysesExceptFor(
      IRContext::Analysis::kAnalysisLoopAnalysis |
      IRContext::Analysis::kAnalysisDefUse);
  return true;
}

void LoopUnrollerUtilsImpl::KillDebugDeclares(BasicBlock* bb) {



  std::vector<Instruction*> to_be_killed;

  bb->ForEachInst([&to_be_killed, this](Instruction* inst) {
    if (context_->get_debug_info_mgr()->IsDebugDeclare(inst)) {
      to_be_killed.push_back(inst);
    }
  });
  for (auto* inst : to_be_killed) context_->KillInst(inst);
}





bool LoopUnrollerUtilsImpl::CopyBasicBlock(Loop* loop, const BasicBlock* itr,
                                           bool preserve_instructions) {

  BasicBlock* basic_block = itr->Clone(context_);
  if (!basic_block) return false;
  basic_block->SetParent(itr->GetParent());


  KillDebugDeclares(basic_block);



  if (!AssignNewResultIds(basic_block)) {
    return false;
  }


  if (itr == loop->GetContinueBlock()) {

    if (!preserve_instructions) {
      Instruction* merge_inst = loop->GetHeaderBlock()->GetLoopMergeInst();
      merge_inst->SetInOperand(1, {basic_block->id()});
      context_->UpdateDefUse(merge_inst);
    }

    state_.new_continue_block = basic_block;
  }


  if (itr == loop->GetHeaderBlock()) {
    state_.new_header_block = basic_block;

    if (!preserve_instructions) {

      Instruction* merge_inst = basic_block->GetLoopMergeInst();
      if (merge_inst) invalidated_instructions_.push_back(merge_inst);
    }
  }


  if (itr == loop->GetLatchBlock()) state_.new_latch_block = basic_block;


  if (itr == loop_condition_block_) {
    state_.new_condition_block = basic_block;
  }



  blocks_to_add_.push_back(std::unique_ptr<BasicBlock>(basic_block));


  state_.new_blocks[itr->id()] = basic_block;
  return true;
}

bool LoopUnrollerUtilsImpl::CopyBody(Loop* loop, bool eliminate_conditions) {


  for (const BasicBlock* itr : loop_blocks_inorder_) {
    if (!CopyBasicBlock(loop, itr, false)) {
      return false;
    }
  }


  Instruction* latch_branch = state_.previous_latch_block_->terminator();
  latch_branch->SetInOperand(0, {state_.new_header_block->id()});
  context_->UpdateDefUse(latch_branch);





  Instruction* new_latch_branch = state_.new_latch_block->terminator();
  new_latch_branch->SetInOperand(0, {loop->GetHeaderBlock()->id()});
  context_->AnalyzeUses(new_latch_branch);

  std::vector<Instruction*> inductions;
  loop->GetInductionVariables(inductions);
  for (size_t index = 0; index < inductions.size(); ++index) {
    Instruction* primary_copy = inductions[index];

    assert(primary_copy->result_id() != 0);
    Instruction* induction_clone =
        state_.ids_to_new_inst[state_.new_inst[primary_copy->result_id()]];

    state_.new_phis_.push_back(induction_clone);
    assert(induction_clone->result_id() != 0);

    if (!state_.previous_phis_.empty()) {
      state_.new_inst[primary_copy->result_id()] = GetPhiDefID(
          state_.previous_phis_[index], state_.previous_latch_block_->id());
    } else {

      state_.new_inst[primary_copy->result_id()] = primary_copy->result_id();
    }
  }

  if (eliminate_conditions &&
      state_.new_condition_block != loop_condition_block_) {
    if (!FoldConditionBlock(state_.new_condition_block, 1)) {
      return false;
    }
  }



  state_.new_inst[loop->GetHeaderBlock()->id()] = loop->GetHeaderBlock()->id();

  for (auto& pair : state_.new_blocks) {
    RemapOperands(pair.second);
  }

  for (Instruction* dead_phi : state_.new_phis_)
    invalidated_instructions_.push_back(dead_phi);


  state_.NextIterationState();
  return true;
}

uint32_t LoopUnrollerUtilsImpl::GetPhiDefID(const Instruction* phi,
                                            uint32_t label) const {
  for (uint32_t operand = 3; operand < phi->NumOperands(); operand += 2) {
    if (phi->GetSingleWordOperand(operand) == label) {
      return phi->GetSingleWordOperand(operand - 1);
    }
  }
  assert(false && "Could not find a phi index matching the provided label");
  return 0;
}

bool LoopUnrollerUtilsImpl::FoldConditionBlock(BasicBlock* condition_block,
                                               uint32_t operand_label) {

  Instruction& old_branch = *condition_block->tail();
  uint32_t new_target = old_branch.GetSingleWordOperand(operand_label);

  DebugScope scope = old_branch.GetDebugScope();
  const std::vector<Instruction> lines = old_branch.dbg_line_insts();

  context_->KillInst(&old_branch);

  InstructionBuilder builder(
      context_, condition_block,
      IRContext::Analysis::kAnalysisDefUse |
          IRContext::Analysis::kAnalysisInstrToBlockMapping);
  Instruction* new_branch = builder.AddBranch(new_target);

  if (!lines.empty()) {
    if (!new_branch->AddDebugLine(&lines.back())) {
      return false;
    }
  }
  new_branch->SetDebugScope(scope);
  return true;
}

void LoopUnrollerUtilsImpl::CloseUnrolledLoop(Loop* loop) {

  Instruction* merge_inst = loop->GetHeaderBlock()->GetLoopMergeInst();
  invalidated_instructions_.push_back(merge_inst);



  Instruction* latch_instruction = state_.previous_latch_block_->terminator();
  latch_instruction->SetInOperand(0, {loop->GetMergeBlock()->id()});
  context_->UpdateDefUse(latch_instruction);




  std::vector<Instruction*> inductions;
  loop->GetInductionVariables(inductions);






  state_.new_inst.clear();
  for (Instruction* induction : inductions) {
    uint32_t initalizer_id =
        GetPhiDefID(induction, loop->GetPreHeaderBlock()->id());

    state_.new_inst[induction->result_id()] = initalizer_id;
  }

  for (BasicBlock* block : loop_blocks_inorder_) {
    RemapOperands(block);
  }
  for (auto& block_itr : blocks_to_add_) {
    RemapOperands(block_itr.get());
  }


  for (Instruction* last_phi : state_.previous_phis_) {
    RemapOperands(last_phi);
  }
}


bool LoopUnrollerUtilsImpl::DuplicateLoop(Loop* old_loop, Loop* new_loop) {
  std::vector<BasicBlock*> new_block_order;


  for (const BasicBlock* itr : loop_blocks_inorder_) {
    if (!CopyBasicBlock(old_loop, itr, true)) {
      return false;
    }
    new_block_order.push_back(blocks_to_add_.back().get());
  }


  BasicBlock* new_merge = old_loop->GetMergeBlock()->Clone(context_);
  if (!new_merge) return false;
  new_merge->SetParent(old_loop->GetMergeBlock()->GetParent());
  if (!AssignNewResultIds(new_merge)) {
    return false;
  }
  state_.new_blocks[old_loop->GetMergeBlock()->id()] = new_merge;



  for (auto& pair : state_.new_blocks) {
    RemapOperands(pair.second);
  }

  loop_blocks_inorder_ = std::move(new_block_order);

  AddBlocksToLoop(new_loop);

  new_loop->SetHeaderBlock(state_.new_header_block);
  new_loop->SetContinueBlock(state_.new_continue_block);
  new_loop->SetLatchBlock(state_.new_latch_block);
  new_loop->SetMergeBlock(new_merge);
  return true;
}




void LoopUnrollerUtilsImpl::AddBlocksToFunction(
    const BasicBlock* insert_point) {
  for (auto basic_block_iterator = function_.begin();
       basic_block_iterator != function_.end(); ++basic_block_iterator) {
    if (basic_block_iterator->id() == insert_point->id()) {
      basic_block_iterator.InsertBefore(&blocks_to_add_);
      return;
    }
  }

  assert(
      false &&
      "Could not add basic blocks to function as insert point was not found.");
}



bool LoopUnrollerUtilsImpl::AssignNewResultIds(BasicBlock* basic_block) {
  analysis::DefUseManager* def_use_mgr = context_->get_def_use_mgr();



  uint32_t new_label_id = context_->TakeNextId();
  if (new_label_id == 0) {
    return false;
  }


  state_.new_inst[basic_block->GetLabelInst()->result_id()] = new_label_id;
  basic_block->GetLabelInst()->SetResultId(new_label_id);
  def_use_mgr->AnalyzeInstDefUse(basic_block->GetLabelInst());

  for (Instruction& inst : *basic_block) {

    for (auto& line : inst.dbg_line_insts())
      def_use_mgr->AnalyzeInstDefUse(&line);

    uint32_t old_id = inst.result_id();


    if (old_id == 0) {
      continue;
    }


    uint32_t new_id = context_->TakeNextId();
    if (new_id == 0) {
      return false;
    }
    inst.SetResultId(new_id);
    def_use_mgr->AnalyzeInstDef(&inst);




    context_->get_decoration_mgr()->CloneDecorations(old_id, new_id);


    state_.new_inst[old_id] = inst.result_id();

    if (loop_induction_variable_->result_id() == old_id) {

      state_.new_phi = &inst;
    }
    state_.ids_to_new_inst[inst.result_id()] = &inst;
  }
  return true;
}

void LoopUnrollerUtilsImpl::RemapOperands(Instruction* inst) {
  auto remap_operands_to_new_ids = [this](uint32_t* id) {
    auto itr = state_.new_inst.find(*id);

    if (itr != state_.new_inst.end()) {
      *id = itr->second;
    }
  };

  inst->ForEachInId(remap_operands_to_new_ids);
  context_->AnalyzeUses(inst);
}

void LoopUnrollerUtilsImpl::RemapOperands(BasicBlock* basic_block) {
  for (Instruction& inst : *basic_block) {
    RemapOperands(&inst);
  }
}



void LoopUnrollerUtilsImpl::ComputeLoopOrderedBlocks(Loop* loop) {
  loop_blocks_inorder_.clear();
  loop->ComputeLoopStructuredOrder(&loop_blocks_inorder_);
}


void LoopUnrollerUtilsImpl::AddBlocksToLoop(Loop* loop) const {

  for (auto& block_itr : blocks_to_add_) {
    loop->AddBasicBlock(block_itr.get());
  }


  if (loop->GetParent()) AddBlocksToLoop(loop->GetParent());
}

void LoopUnrollerUtilsImpl::LinkLastPhisToStart(Loop* loop) const {
  std::vector<Instruction*> inductions;
  loop->GetInductionVariables(inductions);

  for (size_t i = 0; i < inductions.size(); ++i) {
    Instruction* last_phi_in_block = state_.previous_phis_[i];

    uint32_t phi_index =
        GetPhiIndexFromLabel(state_.previous_latch_block_, last_phi_in_block);
    uint32_t phi_variable =
        last_phi_in_block->GetSingleWordInOperand(phi_index - 1);
    uint32_t phi_label = last_phi_in_block->GetSingleWordInOperand(phi_index);

    Instruction* phi = inductions[i];
    phi->SetInOperand(phi_index - 1, {phi_variable});
    phi->SetInOperand(phi_index, {phi_label});
  }
}



bool LoopUnrollerUtilsImpl::PartiallyUnroll(Loop* loop, size_t factor) {
  if (!Unroll(loop, factor)) return false;
  LinkLastPhisToStart(loop);
  AddBlocksToLoop(loop);
  AddBlocksToFunction(loop->GetMergeBlock());
  RemoveDeadInstructions();
  return true;
}





}







bool LoopUtils::CanPerformUnroll() {

  if (!loop_->GetHeaderBlock()->GetMergeInst()) {
    return false;
  }


  const BasicBlock* condition = loop_->FindConditionBlock();
  if (!condition) return false;


  const Instruction* induction = loop_->FindConditionVariable(condition);
  if (!induction || induction->opcode() != spv::Op::OpPhi) return false;


  if (!loop_->FindNumberOfIterations(induction, &*condition->ctail(), nullptr))
    return false;

#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION




  constexpr size_t kFuzzerIterationLimit = 100;
  size_t num_iterations;
  loop_->FindNumberOfIterations(induction, &*condition->ctail(),
                                &num_iterations);
  if (num_iterations > kFuzzerIterationLimit) {
    return false;
  }
#endif



  const Instruction& branch = *loop_->GetLatchBlock()->ctail();
  bool branching_assumption =
      branch.opcode() == spv::Op::OpBranch &&
      branch.GetSingleWordInOperand(0) == loop_->GetHeaderBlock()->id();
  if (!branching_assumption) {
    return false;
  }

  std::vector<Instruction*> inductions;
  loop_->GetInductionVariables(inductions);


  const std::vector<uint32_t>& merge_block_preds =
      context_->cfg()->preds(loop_->GetMergeBlock()->id());
  if (merge_block_preds.size() != 1) {
    return false;
  }


  const std::vector<uint32_t>& continue_block_preds =
      context_->cfg()->preds(loop_->GetContinueBlock()->id());
  if (continue_block_preds.size() != 1) {
    return false;
  }




  for (uint32_t label_id : loop_->GetBlocks()) {
    const BasicBlock* block = context_->cfg()->block(label_id);
    if (block->ctail()->opcode() == spv::Op::OpKill ||
        block->ctail()->opcode() == spv::Op::OpReturn ||
        block->ctail()->opcode() == spv::Op::OpReturnValue ||
        block->ctail()->opcode() == spv::Op::OpTerminateInvocation) {
      return false;
    }
  }

  if (!loop_->AreAllChildrenMarkedForRemoval()) {
    return false;
  }

  return true;
}

bool LoopUtils::PartiallyUnroll(size_t factor) {
  if (factor == 1 || !CanPerformUnroll()) return false;


  LoopUnrollerUtilsImpl unroller{context_,
                                 loop_->GetHeaderBlock()->GetParent()};
  unroller.Init(loop_);



  if (factor >= unroller.GetLoopIterationCount()) {
    if (!unroller.FullyUnroll(loop_)) {
      return false;
    }
    return true;
  }





  if (unroller.GetLoopIterationCount() % factor != 0) {
    if (!unroller.PartiallyUnrollResidualFactor(loop_, factor)) {
      return false;
    }
  } else {
    if (!unroller.PartiallyUnroll(loop_, factor)) {
      return false;
    }
  }

  return true;
}

bool LoopUtils::FullyUnroll() {
  if (!CanPerformUnroll()) return false;

  std::vector<Instruction*> inductions;
  loop_->GetInductionVariables(inductions);

  LoopUnrollerUtilsImpl unroller{context_,
                                 loop_->GetHeaderBlock()->GetParent()};

  unroller.Init(loop_);
  if (!unroller.FullyUnroll(loop_)) {
    return false;
  }

  return true;
}

void LoopUtils::Finalize() {


  LoopDescriptor* LD = context_->GetLoopDescriptor(&function_);
  LD->PostModificationCleanup();
}







Pass::Status LoopUnroller::Process() {
  bool changed = false;
  for (Function& f : *context()->module()) {
    if (f.IsDeclaration()) {
      continue;
    }

    LoopDescriptor* LD = context()->GetLoopDescriptor(&f);
    for (Loop& loop : *LD) {
      LoopUtils loop_utils{context(), &loop};
      if (!loop.HasUnrollLoopControl() || !loop_utils.CanPerformUnroll()) {
        continue;
      }

      if (fully_unroll_) {
        if (!loop_utils.FullyUnroll()) {
          return Status::Failure;
        }
        changed = true;
      } else {
        if (!loop_utils.PartiallyUnroll(unroll_factor_)) {
          return Status::Failure;
        }
        changed = true;
      }
    }
    LD->PostModificationCleanup();
  }

  if (changed) {
    context()->InvalidateAnalysesExceptFor(
        IRContext::Analysis::kAnalysisLoopAnalysis);
  }

  return changed ? Status::SuccessWithChange : Status::SuccessWithoutChange;
}

}
}
