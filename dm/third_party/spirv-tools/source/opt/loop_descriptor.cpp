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

#include "source/opt/loop_descriptor.h"

#include <algorithm>
#include <limits>
#include <stack>
#include <utility>
#include <vector>

#include "source/opt/cfg.h"
#include "source/opt/constants.h"
#include "source/opt/dominator_tree.h"
#include "source/opt/ir_context.h"
#include "source/opt/iterator.h"
#include "source/opt/pass.h"
#include "source/opt/tree_iterator.h"
#include "source/util/make_unique.h"

namespace spvtools {
namespace opt {



Instruction* Loop::GetInductionStepOperation(
    const Instruction* induction) const {

  assert(induction->opcode() == spv::Op::OpPhi);

  Instruction* step = nullptr;

  analysis::DefUseManager* def_use_manager = context_->get_def_use_mgr();


  for (uint32_t operand_id = 1; operand_id < induction->NumInOperands();
       operand_id += 2) {

    BasicBlock* incoming_block =
        context_->cfg()->block(induction->GetSingleWordInOperand(operand_id));



    if (IsInsideLoop(incoming_block)) {
      step = def_use_manager->GetDef(
          induction->GetSingleWordInOperand(operand_id - 1));
      break;
    }
  }

  if (!step || !IsSupportedStepOp(step->opcode())) {
    return nullptr;
  }


  uint32_t lhs = step->GetSingleWordInOperand(0);
  uint32_t rhs = step->GetSingleWordInOperand(1);



  if (lhs != induction->result_id() && rhs != induction->result_id()) {
    return nullptr;
  }

  if (def_use_manager->GetDef(lhs)->opcode() != spv::Op::OpConstant &&
      def_use_manager->GetDef(rhs)->opcode() != spv::Op::OpConstant) {
    return nullptr;
  }

  return step;
}



bool Loop::IsSupportedStepOp(spv::Op step) const {
  switch (step) {
    case spv::Op::OpISub:
    case spv::Op::OpIAdd:
      return true;
    default:
      return false;
  }
}

bool Loop::IsSupportedCondition(spv::Op condition) const {
  switch (condition) {

    case spv::Op::OpULessThan:
    case spv::Op::OpSLessThan:

    case spv::Op::OpUGreaterThan:
    case spv::Op::OpSGreaterThan:


    case spv::Op::OpSGreaterThanEqual:
    case spv::Op::OpUGreaterThanEqual:

    case spv::Op::OpSLessThanEqual:
    case spv::Op::OpULessThanEqual:

      return true;
    default:
      return false;
  }
}

int64_t Loop::GetResidualConditionValue(spv::Op condition,
                                        int64_t initial_value,
                                        int64_t step_value,
                                        size_t number_of_iterations,
                                        size_t factor) {
  int64_t remainder =
      initial_value + (number_of_iterations % factor) * step_value;




  switch (condition) {
    case spv::Op::OpSGreaterThanEqual:
    case spv::Op::OpUGreaterThanEqual: {
      remainder -= 1;
      break;
    }
    case spv::Op::OpSLessThanEqual:
    case spv::Op::OpULessThanEqual: {
      remainder += 1;
      break;
    }

    default:
      break;
  }
  return remainder;
}

Instruction* Loop::GetConditionInst() const {
  BasicBlock* condition_block = FindConditionBlock();
  if (!condition_block) {
    return nullptr;
  }
  Instruction* branch_conditional = &*condition_block->tail();
  if (!branch_conditional ||
      branch_conditional->opcode() != spv::Op::OpBranchConditional) {
    return nullptr;
  }
  Instruction* condition_inst = context_->get_def_use_mgr()->GetDef(
      branch_conditional->GetSingleWordInOperand(0));
  if (IsSupportedCondition(condition_inst->opcode())) {
    return condition_inst;
  }

  return nullptr;
}




bool Loop::GetInductionInitValue(const Instruction* induction,
                                 int64_t* value) const {
  Instruction* constant_instruction = nullptr;
  analysis::DefUseManager* def_use_manager = context_->get_def_use_mgr();

  for (uint32_t operand_id = 0; operand_id < induction->NumInOperands();
       operand_id += 2) {
    BasicBlock* bb = context_->cfg()->block(
        induction->GetSingleWordInOperand(operand_id + 1));

    if (!IsInsideLoop(bb)) {
      constant_instruction = def_use_manager->GetDef(
          induction->GetSingleWordInOperand(operand_id));
    }
  }

  if (!constant_instruction) return false;

  const analysis::Constant* constant =
      context_->get_constant_mgr()->FindDeclaredConstant(
          constant_instruction->result_id());
  if (!constant) return false;

  if (value) {
    const analysis::Integer* type = constant->type()->AsInteger();
    if (!type) {
      return false;
    }

    *value = type->IsSigned() ? constant->GetSignExtendedValue()
                              : constant->GetZeroExtendedValue();
  }

  return true;
}

Loop::Loop(IRContext* context, DominatorAnalysis* dom_analysis,
           BasicBlock* header, BasicBlock* continue_target,
           BasicBlock* merge_target)
    : context_(context),
      loop_header_(header),
      loop_continue_(continue_target),
      loop_merge_(merge_target),
      loop_preheader_(nullptr),
      parent_(nullptr),
      loop_is_marked_for_removal_(false) {
  assert(context);
  assert(dom_analysis);
  loop_preheader_ = FindLoopPreheader(dom_analysis);
  loop_latch_ = FindLatchBlock();
}

BasicBlock* Loop::FindLoopPreheader(DominatorAnalysis* dom_analysis) {
  CFG* cfg = context_->cfg();
  DominatorTree& dom_tree = dom_analysis->GetDomTree();
  DominatorTreeNode* header_node = dom_tree.GetTreeNode(loop_header_);


  BasicBlock* loop_pred = nullptr;

  auto header_pred = cfg->preds(loop_header_->id());
  for (uint32_t p_id : header_pred) {
    DominatorTreeNode* node = dom_tree.GetTreeNode(p_id);
    if (node && !dom_tree.Dominates(header_node, node)) {

      if (loop_pred && node->bb_ != loop_pred) {


        return nullptr;
      }
      loop_pred = node->bb_;
    }
  }


  assert(loop_pred && "The header node is the entry block ?");




  bool is_preheader = true;
  uint32_t loop_header_id = loop_header_->id();
  const auto* const_loop_pred = loop_pred;
  const_loop_pred->ForEachSuccessorLabel(
      [&is_preheader, loop_header_id](const uint32_t id) {
        if (id != loop_header_id) is_preheader = false;
      });
  if (is_preheader) return loop_pred;
  return nullptr;
}

bool Loop::IsInsideLoop(Instruction* inst) const {
  const BasicBlock* parent_block = context_->get_instr_block(inst);
  if (!parent_block) return false;
  return IsInsideLoop(parent_block);
}

bool Loop::IsBasicBlockInLoopSlow(const BasicBlock* bb) {
  assert(bb->GetParent() && "The basic block does not belong to a function");
  DominatorAnalysis* dom_analysis =
      context_->GetDominatorAnalysis(bb->GetParent());
  if (dom_analysis->IsReachable(bb) &&
      !dom_analysis->Dominates(GetHeaderBlock(), bb))
    return false;

  return true;
}

BasicBlock* Loop::GetOrCreatePreHeaderBlock() {
  if (loop_preheader_) return loop_preheader_;

  CFG* cfg = context_->cfg();
  loop_header_ = cfg->SplitLoopHeader(loop_header_);
  if (loop_header_ == nullptr) {
    return nullptr;
  }
  return loop_preheader_;
}

void Loop::SetContinueBlock(BasicBlock* continue_block) {
  assert(IsInsideLoop(continue_block));
  loop_continue_ = continue_block;
}

void Loop::SetLatchBlock(BasicBlock* latch) {
#ifndef NDEBUG
  assert(latch->GetParent() && "The basic block does not belong to a function");

  const auto* const_latch = latch;
  const_latch->ForEachSuccessorLabel([this](uint32_t id) {
    assert((!IsInsideLoop(id) || id == GetHeaderBlock()->id()) &&
           "A predecessor of the continue block does not belong to the loop");
  });
#endif
  assert(IsInsideLoop(latch) && "The continue block is not in the loop");

  SetLatchBlockImpl(latch);
}

void Loop::SetMergeBlock(BasicBlock* merge) {
#ifndef NDEBUG
  assert(merge->GetParent() && "The basic block does not belong to a function");
#endif
  assert(!IsInsideLoop(merge) && "The merge block is in the loop");

  SetMergeBlockImpl(merge);
  if (GetHeaderBlock()->GetLoopMergeInst()) {
    UpdateLoopMergeInst();
  }
}

void Loop::SetPreHeaderBlock(BasicBlock* preheader) {
  if (preheader) {
    assert(!IsInsideLoop(preheader) && "The preheader block is in the loop");
    assert(preheader->tail()->opcode() == spv::Op::OpBranch &&
           "The preheader block does not unconditionally branch to the header "
           "block");
    assert(preheader->tail()->GetSingleWordOperand(0) ==
               GetHeaderBlock()->id() &&
           "The preheader block does not unconditionally branch to the header "
           "block");
  }
  loop_preheader_ = preheader;
}

BasicBlock* Loop::FindLatchBlock() {
  CFG* cfg = context_->cfg();

  DominatorAnalysis* dominator_analysis =
      context_->GetDominatorAnalysis(loop_header_->GetParent());





  for (uint32_t block_id : cfg->preds(loop_header_->id())) {
    if (dominator_analysis->Dominates(loop_continue_->id(), block_id)) {
      return cfg->block(block_id);
    }
  }

  assert(
      false &&
      "Every loop should have a latch block dominated by the continue target");
  return nullptr;
}

void Loop::GetExitBlocks(std::unordered_set<uint32_t>* exit_blocks) const {
  CFG* cfg = context_->cfg();
  exit_blocks->clear();

  for (uint32_t bb_id : GetBlocks()) {
    const BasicBlock* bb = cfg->block(bb_id);
    bb->ForEachSuccessorLabel([exit_blocks, this](uint32_t succ) {
      if (!IsInsideLoop(succ)) {
        exit_blocks->insert(succ);
      }
    });
  }
}

void Loop::GetMergingBlocks(
    std::unordered_set<uint32_t>* merging_blocks) const {
  assert(GetMergeBlock() && "This loop is not structured");
  CFG* cfg = context_->cfg();
  merging_blocks->clear();

  std::stack<const BasicBlock*> to_visit;
  to_visit.push(GetMergeBlock());
  while (!to_visit.empty()) {
    const BasicBlock* bb = to_visit.top();
    to_visit.pop();
    merging_blocks->insert(bb->id());
    for (uint32_t pred_id : cfg->preds(bb->id())) {
      if (!IsInsideLoop(pred_id) && !merging_blocks->count(pred_id)) {
        to_visit.push(cfg->block(pred_id));
      }
    }
  }
}

namespace {

inline bool IsBasicBlockSafeToClone(IRContext* context, BasicBlock* bb) {
  for (Instruction& inst : *bb) {
    if (!inst.IsBranch() && !context->IsCombinatorInstruction(&inst))
      return false;
  }

  return true;
}

}

bool Loop::IsSafeToClone() const {
  CFG& cfg = *context_->cfg();

  for (uint32_t bb_id : GetBlocks()) {
    BasicBlock* bb = cfg.block(bb_id);
    assert(bb);
    if (!IsBasicBlockSafeToClone(context_, bb)) return false;
  }


  if (GetHeaderBlock()->GetLoopMergeInst()) {
    std::unordered_set<uint32_t> blocks;
    GetMergingBlocks(&blocks);
    blocks.erase(GetMergeBlock()->id());
    for (uint32_t bb_id : blocks) {
      BasicBlock* bb = cfg.block(bb_id);
      assert(bb);
      if (!IsBasicBlockSafeToClone(context_, bb)) return false;
    }
  }

  return true;
}

bool Loop::IsLCSSA() const {
  CFG* cfg = context_->cfg();
  analysis::DefUseManager* def_use_mgr = context_->get_def_use_mgr();

  std::unordered_set<uint32_t> exit_blocks;
  GetExitBlocks(&exit_blocks);


  IRContext* ir_context = context_;

  for (uint32_t bb_id : GetBlocks()) {
    for (Instruction& insn : *cfg->block(bb_id)) {



      if (!def_use_mgr->WhileEachUser(
              &insn,
              [&exit_blocks, ir_context, this](Instruction* use) -> bool {
                BasicBlock* parent = ir_context->get_instr_block(use);
                assert(parent && "Invalid analysis");
                if (IsInsideLoop(parent)) return true;
                if (use->opcode() != spv::Op::OpPhi) return false;
                return exit_blocks.count(parent->id());
              }))
        return false;
    }
  }
  return true;
}

bool Loop::ShouldHoistInstruction(const Instruction& inst) const {
  return inst.IsOpcodeCodeMotionSafe() && AreAllOperandsOutsideLoop(inst) &&
         (!inst.IsLoad() || inst.IsReadOnlyLoad());
}

bool Loop::AreAllOperandsOutsideLoop(const Instruction& inst) const {
  analysis::DefUseManager* def_use_mgr = GetContext()->get_def_use_mgr();

  const std::function<bool(const uint32_t*)> operand_outside_loop =
      [this, &def_use_mgr](const uint32_t* id) {
        return !this->IsInsideLoop(def_use_mgr->GetDef(*id));
      };

  return inst.WhileEachInId(operand_outside_loop);
}

void Loop::ComputeLoopStructuredOrder(
    std::vector<BasicBlock*>* ordered_loop_blocks, bool include_pre_header,
    bool include_merge) const {
  CFG& cfg = *context_->cfg();


  ordered_loop_blocks->reserve(GetBlocks().size() + include_pre_header +
                               include_merge);

  if (include_pre_header && GetPreHeaderBlock())
    ordered_loop_blocks->push_back(loop_preheader_);

  bool is_shader =
      context_->get_feature_mgr()->HasCapability(spv::Capability::Shader);
  if (!is_shader) {
    cfg.ForEachBlockInReversePostOrder(
        loop_header_, [ordered_loop_blocks, this](BasicBlock* bb) {
          if (IsInsideLoop(bb)) ordered_loop_blocks->push_back(bb);
        });
  } else {



    std::list<BasicBlock*> order;
    cfg.ComputeStructuredOrder(loop_header_->GetParent(), loop_header_,
                               loop_merge_, &order);
    for (BasicBlock* bb : order) {
      if (bb == GetMergeBlock()) {
        break;
      }
      ordered_loop_blocks->push_back(bb);
    }
  }
  if (include_merge && GetMergeBlock())
    ordered_loop_blocks->push_back(loop_merge_);
}

LoopDescriptor::LoopDescriptor(IRContext* context, const Function* f)
    : loops_(), placeholder_top_loop_(nullptr) {
  PopulateList(context, f);
}

LoopDescriptor::~LoopDescriptor() { ClearLoops(); }

void LoopDescriptor::PopulateList(IRContext* context, const Function* f) {
  DominatorAnalysis* dom_analysis = context->GetDominatorAnalysis(f);

  ClearLoops();



  DominatorTree& dom_tree = dom_analysis->GetDomTree();
  for (DominatorTreeNode& node :
       make_range(dom_tree.post_begin(), dom_tree.post_end())) {
    Instruction* merge_inst = node.bb_->GetLoopMergeInst();
    if (merge_inst) {
      bool all_backedge_unreachable = true;
      for (uint32_t pid : context->cfg()->preds(node.bb_->id())) {
        if (dom_analysis->IsReachable(pid) &&
            dom_analysis->Dominates(node.bb_->id(), pid)) {
          all_backedge_unreachable = false;
          break;
        }
      }
      if (all_backedge_unreachable)
        continue;


      uint32_t merge_bb_id = merge_inst->GetSingleWordOperand(0);


      uint32_t continue_bb_id = merge_inst->GetSingleWordOperand(1);


      BasicBlock* merge_bb = context->cfg()->block(merge_bb_id);


      BasicBlock* continue_bb = context->cfg()->block(continue_bb_id);


      BasicBlock* header_bb = context->get_instr_block(merge_inst);


      Loop* current_loop =
          new Loop(context, dom_analysis, header_bb, continue_bb, merge_bb);
      loops_.push_back(current_loop);



      for (auto itr = loops_.rbegin() + 1; itr != loops_.rend(); ++itr) {
        Loop* previous_loop = *itr;


        if (previous_loop->HasParent()) continue;



        if (!dom_analysis->Dominates(header_bb,
                                     previous_loop->GetHeaderBlock()))
          continue;


        if (dom_analysis->Dominates(merge_bb, previous_loop->GetHeaderBlock()))
          continue;

        current_loop->AddNestedLoop(previous_loop);
      }
      DominatorTreeNode* dom_merge_node = dom_tree.GetTreeNode(merge_bb);
      for (DominatorTreeNode& loop_node :
           make_range(node.df_begin(), node.df_end())) {

        if (dom_tree.Dominates(dom_merge_node, &loop_node)) continue;
        current_loop->AddBasicBlock(loop_node.bb_);
        basic_block_to_loop_.insert(
            std::make_pair(loop_node.bb_->id(), current_loop));
      }
    }
  }
  for (Loop* loop : loops_) {
    if (!loop->HasParent()) placeholder_top_loop_.nested_loops_.push_back(loop);
  }
}

std::vector<Loop*> LoopDescriptor::GetLoopsInBinaryLayoutOrder() {
  std::vector<uint32_t> ids{};

  for (size_t i = 0; i < NumLoops(); ++i) {
    ids.push_back(GetLoopByIndex(i).GetHeaderBlock()->id());
  }

  std::vector<Loop*> loops{};
  if (!ids.empty()) {
    auto function = GetLoopByIndex(0).GetHeaderBlock()->GetParent();
    for (const auto& block : *function) {
      auto block_id = block.id();

      auto element = std::find(std::begin(ids), std::end(ids), block_id);
      if (element != std::end(ids)) {
        loops.push_back(&GetLoopByIndex(element - std::begin(ids)));
      }
    }
  }

  return loops;
}

BasicBlock* Loop::FindConditionBlock() const {
  if (!loop_merge_) {
    return nullptr;
  }
  BasicBlock* condition_block = nullptr;

  uint32_t in_loop_pred = 0;
  for (uint32_t p : context_->cfg()->preds(loop_merge_->id())) {
    if (IsInsideLoop(p)) {
      if (in_loop_pred) {

        return nullptr;
      }
      in_loop_pred = p;
    }
  }
  if (!in_loop_pred) {

    return nullptr;
  }

  BasicBlock* bb = context_->cfg()->block(in_loop_pred);

  if (!bb) return nullptr;

  const Instruction& branch = *bb->ctail();


  if (branch.opcode() != spv::Op::OpBranchConditional) return nullptr;


  if (branch.GetSingleWordInOperand(1) == loop_merge_->id() ||
      branch.GetSingleWordInOperand(2) == loop_merge_->id()) {
    condition_block = bb;
  }

  return condition_block;
}

bool Loop::FindNumberOfIterations(const Instruction* induction,
                                  const Instruction* branch_inst,
                                  size_t* iterations_out,
                                  int64_t* step_value_out,
                                  int64_t* init_value_out) const {

  analysis::DefUseManager* def_use_manager = context_->get_def_use_mgr();


  Instruction* condition =
      def_use_manager->GetDef(branch_inst->GetSingleWordOperand(0));

  assert(IsSupportedCondition(condition->opcode()));


  analysis::ConstantManager* const_manager = context_->get_constant_mgr();



  const analysis::Constant* upper_bound =
      const_manager->FindDeclaredConstant(condition->GetSingleWordOperand(3));
  if (!upper_bound) return false;


  const analysis::Integer* type = upper_bound->type()->AsInteger();

  if (!type || type->width() > 64) {
    return false;
  }

  int64_t condition_value = type->IsSigned()
                                ? upper_bound->GetSignExtendedValue()
                                : upper_bound->GetZeroExtendedValue();




  Instruction* step_inst = GetInductionStepOperation(induction);
  if (!step_inst) return false;


  const analysis::Constant* step_constant =
      const_manager->FindDeclaredConstant(step_inst->GetSingleWordOperand(3));
  if (!step_constant) return false;


  int64_t step_value = 0;

  const analysis::Integer* step_type =
      step_constant->AsIntConstant()->type()->AsInteger();

  if (step_type->IsSigned()) {
    step_value = step_constant->AsIntConstant()->GetS32BitValue();
  } else {
    step_value = step_constant->AsIntConstant()->GetU32BitValue();
  }


  if (step_inst->opcode() == spv::Op::OpISub) {
    step_value = -step_value;
  }


  int64_t init_value = 0;
  if (!GetInductionInitValue(induction, &init_value)) return false;


  int64_t num_itrs = GetIterations(condition->opcode(), condition_value,
                                   init_value, step_value);


  if (num_itrs <= 0) {
    return false;
  }

  if (iterations_out) {
    assert(static_cast<size_t>(num_itrs) <= std::numeric_limits<size_t>::max());
    *iterations_out = static_cast<size_t>(num_itrs);
  }

  if (step_value_out) {
    *step_value_out = step_value;
  }

  if (init_value_out) {
    *init_value_out = init_value;
  }

  return true;
}





int64_t Loop::GetIterations(spv::Op condition, int64_t condition_value,
                            int64_t init_value, int64_t step_value) const {
  if (step_value == 0) {
    return 0;
  }

  int64_t diff = 0;

  switch (condition) {
    case spv::Op::OpSLessThan:
    case spv::Op::OpULessThan: {

      if (!(init_value < condition_value)) return 0;

      diff = condition_value - init_value;




      if ((diff < 0 && step_value > 0) || (diff > 0 && step_value < 0)) {
        return 0;
      }

      break;
    }
    case spv::Op::OpSGreaterThan:
    case spv::Op::OpUGreaterThan: {

      if (!(init_value > condition_value)) return 0;

      diff = init_value - condition_value;




      if ((diff < 0 && step_value < 0) || (diff > 0 && step_value > 0)) {
        return 0;
      }

      break;
    }

    case spv::Op::OpSGreaterThanEqual:
    case spv::Op::OpUGreaterThanEqual: {

      if (!(init_value >= condition_value)) return 0;



      diff = init_value - (condition_value - 1);




      if ((diff > 0 && step_value > 0) || (diff < 0 && step_value < 0)) {
        return 0;
      }

      break;
    }

    case spv::Op::OpSLessThanEqual:
    case spv::Op::OpULessThanEqual: {

      if (!(init_value <= condition_value)) return 0;



      diff = (condition_value + 1) - init_value;




      if ((diff < 0 && step_value > 0) || (diff > 0 && step_value < 0)) {
        return 0;
      }

      break;
    }

    default:
      assert(false &&
             "Could not retrieve number of iterations from the loop condition. "
             "Condition is not supported.");
  }


  step_value = llabs(step_value);
  diff = llabs(diff);
  int64_t result = diff / step_value;

  if (diff % step_value != 0) {
    result += 1;
  }
  return result;
}


void Loop::GetInductionVariables(
    std::vector<Instruction*>& induction_variables) const {
  for (Instruction& inst : *loop_header_) {
    if (inst.opcode() == spv::Op::OpPhi) {
      induction_variables.push_back(&inst);
    }
  }
}

Instruction* Loop::FindConditionVariable(
    const BasicBlock* condition_block) const {

  const Instruction& branch_inst = *condition_block->ctail();

  Instruction* induction = nullptr;

  if (branch_inst.opcode() == spv::Op::OpBranchConditional) {

    analysis::DefUseManager* def_use_manager = context_->get_def_use_mgr();



    Instruction* condition =
        def_use_manager->GetDef(branch_inst.GetSingleWordOperand(0));


    if (condition && IsSupportedCondition(condition->opcode())) {

      Instruction* variable_inst =
          def_use_manager->GetDef(condition->GetSingleWordOperand(2));


      if (!variable_inst || variable_inst->opcode() != spv::Op::OpPhi)
        return nullptr;






      size_t max_supported_operands = 4;
      if (variable_inst->NumInOperands() == max_supported_operands) {

        uint32_t operand_label_1 = 1;


        uint32_t operand_label_2 = 3;


        if (!IsInsideLoop(
                variable_inst->GetSingleWordInOperand(operand_label_1)) &&
            !IsInsideLoop(
                variable_inst->GetSingleWordInOperand(operand_label_2))) {
          return nullptr;
        }


        if (variable_inst->GetSingleWordInOperand(operand_label_1) !=
                loop_latch_->id() &&
            variable_inst->GetSingleWordInOperand(operand_label_2) !=
                loop_latch_->id()) {
          return nullptr;
        }
      } else {
        return nullptr;
      }

      if (!FindNumberOfIterations(variable_inst, &branch_inst, nullptr))
        return nullptr;
      induction = variable_inst;
    }
  }

  return induction;
}

LoopDescriptor::Status LoopDescriptor::CreatePreHeaderBlocksIfMissing() {
  bool modified = false;

  for (auto& loop : *this) {
    if (!loop.GetPreHeaderBlock()) {
      if (!loop.GetOrCreatePreHeaderBlock()) {
        return Status::Failure;
      }
      modified = true;
    }
  }

  return modified ? Status::SuccessWithChange : Status::SuccessWithoutChange;
}



void LoopDescriptor::PostModificationCleanup() {
  LoopContainerType loops_to_remove_;
  for (Loop* loop : loops_) {
    if (loop->IsMarkedForRemoval()) {
      loops_to_remove_.push_back(loop);
      if (loop->HasParent()) {
        loop->GetParent()->RemoveChildLoop(loop);
      }
    }
  }

  for (Loop* loop : loops_to_remove_) {
    loops_.erase(std::find(loops_.begin(), loops_.end(), loop));
    delete loop;
  }

  for (auto& pair : loops_to_add_) {
    Loop* parent = pair.first;
    std::unique_ptr<Loop> loop = std::move(pair.second);

    if (parent) {
      loop->SetParent(nullptr);
      parent->AddNestedLoop(loop.get());

      for (uint32_t block_id : loop->GetBlocks()) {
        parent->AddBasicBlock(block_id);
      }
    }

    loops_.emplace_back(loop.release());
  }

  loops_to_add_.clear();
}

void LoopDescriptor::ClearLoops() {
  for (Loop* loop : loops_) {
    delete loop;
  }
  loops_.clear();
}


Loop* LoopDescriptor::AddLoopNest(std::unique_ptr<Loop> new_loop) {
  Loop* loop = new_loop.release();
  if (!loop->HasParent()) placeholder_top_loop_.nested_loops_.push_back(loop);


  for (Loop& current_loop :
       make_range(iterator::begin(loop), iterator::end(nullptr))) {
    loops_.push_back(&current_loop);
    for (uint32_t bb_id : current_loop.GetBlocks())
      basic_block_to_loop_.insert(std::make_pair(bb_id, &current_loop));
  }

  return loop;
}

void LoopDescriptor::RemoveLoop(Loop* loop) {
  Loop* parent = loop->GetParent() ? loop->GetParent() : &placeholder_top_loop_;
  parent->nested_loops_.erase(std::find(parent->nested_loops_.begin(),
                                        parent->nested_loops_.end(), loop));
  std::for_each(
      loop->nested_loops_.begin(), loop->nested_loops_.end(),
      [loop](Loop* sub_loop) { sub_loop->SetParent(loop->GetParent()); });
  parent->nested_loops_.insert(parent->nested_loops_.end(),
                               loop->nested_loops_.begin(),
                               loop->nested_loops_.end());
  for (uint32_t bb_id : loop->GetBlocks()) {
    Loop* l = FindLoopForBasicBlock(bb_id);
    if (l == loop) {
      SetBasicBlockToLoop(bb_id, l->GetParent());
    } else {
      ForgetBasicBlock(bb_id);
    }
  }

  LoopContainerType::iterator it =
      std::find(loops_.begin(), loops_.end(), loop);
  assert(it != loops_.end());
  delete loop;
  loops_.erase(it);
}

}
}
