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

#include "source/opt/loop_peeling.h"

#include <functional>
#include <memory>
#include <unordered_set>
#include <vector>

#include "source/opt/ir_builder.h"
#include "source/opt/ir_context.h"
#include "source/opt/loop_descriptor.h"
#include "source/opt/loop_utils.h"
#include "source/opt/scalar_analysis.h"
#include "source/opt/scalar_analysis_nodes.h"

namespace spvtools {
namespace opt {
namespace {

void GetBlocksInPath(uint32_t block, uint32_t entry,
                     std::unordered_set<uint32_t>* blocks_in_path,
                     const CFG& cfg) {
  for (uint32_t pid : cfg.preds(block)) {
    if (blocks_in_path->insert(pid).second) {
      if (pid != entry) {
        GetBlocksInPath(pid, entry, blocks_in_path, cfg);
      }
    }
  }
}
}

size_t LoopPeelingPass::code_grow_threshold_ = 1000;

bool LoopPeeling::DuplicateAndConnectLoop(
    LoopUtils::LoopCloningResult* clone_results) {
  CFG& cfg = *context_->cfg();
  analysis::DefUseManager* def_use_mgr = context_->get_def_use_mgr();

  assert(CanPeelLoop() && "Cannot peel loop!");

  std::vector<BasicBlock*> ordered_loop_blocks;
  BasicBlock* pre_header = loop_->GetOrCreatePreHeaderBlock();
  if (!pre_header) {
    return false;
  }

  loop_->ComputeLoopStructuredOrder(&ordered_loop_blocks);

  cloned_loop_ = loop_utils_.CloneLoop(clone_results, ordered_loop_blocks);
  if (!cloned_loop_) {
    return false;
  }


  Function::iterator it =
      loop_utils_.GetFunction()->FindBlock(pre_header->id());
  assert(it != loop_utils_.GetFunction()->end() &&
         "Pre-header not found in the function.");
  loop_utils_.GetFunction()->AddBasicBlocks(
      clone_results->cloned_bb_.begin(), clone_results->cloned_bb_.end(), ++it);


  BasicBlock* cloned_header = cloned_loop_->GetHeaderBlock();
  pre_header->ForEachSuccessorLabel(
      [cloned_header](uint32_t* succ) { *succ = cloned_header->id(); });


  cfg.RemoveEdge(pre_header->id(), loop_->GetHeaderBlock()->id());
  cloned_loop_->SetPreHeaderBlock(pre_header);
  loop_->SetPreHeaderBlock(nullptr);






  uint32_t cloned_loop_exit = 0;
  for (uint32_t pred_id : cfg.preds(loop_->GetMergeBlock()->id())) {
    if (loop_->IsInsideLoop(pred_id)) continue;
    BasicBlock* bb = cfg.block(pred_id);
    assert(cloned_loop_exit == 0 && "The loop has multiple exits.");
    cloned_loop_exit = bb->id();
    bb->ForEachSuccessorLabel([this](uint32_t* succ) {
      if (*succ == loop_->GetMergeBlock()->id())
        *succ = loop_->GetHeaderBlock()->id();
    });
  }


  cfg.RemoveNonExistingEdges(loop_->GetMergeBlock()->id());
  cfg.AddEdge(cloned_loop_exit, loop_->GetHeaderBlock()->id());































  loop_->GetHeaderBlock()->ForEachPhiInst([cloned_loop_exit, def_use_mgr,
                                           clone_results,
                                           this](Instruction* phi) {
    for (uint32_t i = 0; i < phi->NumInOperands(); i += 2) {
      if (!loop_->IsInsideLoop(phi->GetSingleWordInOperand(i + 1))) {
        phi->SetInOperand(i,
                          {clone_results->value_map_.at(
                              exit_value_.at(phi->result_id())->result_id())});
        phi->SetInOperand(i + 1, {cloned_loop_exit});
        def_use_mgr->AnalyzeInstUse(phi);
        return;
      }
    }
  });



  BasicBlock* new_pre_header = loop_->GetOrCreatePreHeaderBlock();
  if (!new_pre_header) {
    return false;
  }
  cloned_loop_->SetMergeBlock(new_pre_header);
  return true;
}

bool LoopPeeling::InsertCanonicalInductionVariable(
    LoopUtils::LoopCloningResult* clone_results) {
  if (original_loop_canonical_induction_variable_) {
    canonical_induction_variable_ =
        context_->get_def_use_mgr()->GetDef(clone_results->value_map_.at(
            original_loop_canonical_induction_variable_->result_id()));
    return true;
  }

  BasicBlock::iterator insert_point = GetClonedLoop()->GetLatchBlock()->tail();
  if (GetClonedLoop()->GetLatchBlock()->GetMergeInst()) {
    --insert_point;
  }
  InstructionBuilder builder(
      context_, &*insert_point,
      IRContext::kAnalysisDefUse | IRContext::kAnalysisInstrToBlockMapping);
  Instruction* uint_1_cst =
      builder.GetIntConstant<uint32_t>(1, int_type_->IsSigned());
  if (!uint_1_cst) return false;



  Instruction* iv_inc = builder.AddIAdd(
      uint_1_cst->type_id(), uint_1_cst->result_id(), uint_1_cst->result_id());
  if (!iv_inc) return false;

  builder.SetInsertPoint(&*GetClonedLoop()->GetHeaderBlock()->begin());

  Instruction* initial_value =
      builder.GetIntConstant<uint32_t>(0, int_type_->IsSigned());
  if (!initial_value) return false;

  canonical_induction_variable_ = builder.AddPhi(
      uint_1_cst->type_id(),
      {initial_value->result_id(), GetClonedLoop()->GetPreHeaderBlock()->id(),
       iv_inc->result_id(), GetClonedLoop()->GetLatchBlock()->id()});
  if (!canonical_induction_variable_) return false;

  iv_inc->SetInOperand(0, {canonical_induction_variable_->result_id()});


  context_->get_def_use_mgr()->AnalyzeInstUse(iv_inc);


  if (do_while_form_) {
    canonical_induction_variable_ = iv_inc;
  }
  return true;
}

void LoopPeeling::GetIteratorUpdateOperations(
    const Loop* loop, Instruction* iterator,
    std::unordered_set<Instruction*>* operations) {
  analysis::DefUseManager* def_use_mgr = context_->get_def_use_mgr();
  operations->insert(iterator);
  iterator->ForEachInId([def_use_mgr, loop, operations, this](uint32_t* id) {
    Instruction* insn = def_use_mgr->GetDef(*id);
    if (insn->opcode() == spv::Op::OpLabel) {
      return;
    }
    if (operations->count(insn)) {
      return;
    }
    if (!loop->IsInsideLoop(insn)) {
      return;
    }
    GetIteratorUpdateOperations(loop, insn, operations);
  });
}

bool LoopPeeling::IsConditionCheckSideEffectFree() const {
  CFG& cfg = *context_->cfg();



  if (!do_while_form_) {
    uint32_t condition_block_id = cfg.preds(loop_->GetMergeBlock()->id())[0];

    std::unordered_set<uint32_t> blocks_in_path;

    blocks_in_path.insert(condition_block_id);
    GetBlocksInPath(condition_block_id, loop_->GetHeaderBlock()->id(),
                    &blocks_in_path, cfg);

    for (uint32_t bb_id : blocks_in_path) {
      BasicBlock* bb = cfg.block(bb_id);
      if (!bb->WhileEachInst([this](Instruction* insn) {
            if (insn->IsBranch()) return true;
            switch (insn->opcode()) {
              case spv::Op::OpLabel:
              case spv::Op::OpSelectionMerge:
              case spv::Op::OpLoopMerge:
                return true;
              default:
                break;
            }
            return context_->IsCombinatorInstruction(insn);
          })) {
        return false;
      }
    }
  }

  return true;
}

void LoopPeeling::GetIteratingExitValues() {
  CFG& cfg = *context_->cfg();

  loop_->GetHeaderBlock()->ForEachPhiInst(
      [this](Instruction* phi) { exit_value_[phi->result_id()] = nullptr; });

  if (!loop_->GetMergeBlock()) {
    return;
  }
  if (cfg.preds(loop_->GetMergeBlock()->id()).size() != 1) {
    return;
  }
  analysis::DefUseManager* def_use_mgr = context_->get_def_use_mgr();

  uint32_t condition_block_id = cfg.preds(loop_->GetMergeBlock()->id())[0];

  auto& header_pred = cfg.preds(loop_->GetHeaderBlock()->id());
  do_while_form_ = std::find(header_pred.begin(), header_pred.end(),
                             condition_block_id) != header_pred.end();
  if (do_while_form_) {
    loop_->GetHeaderBlock()->ForEachPhiInst(
        [condition_block_id, def_use_mgr, this](Instruction* phi) {
          std::unordered_set<Instruction*> operations;

          for (uint32_t i = 0; i < phi->NumInOperands(); i += 2) {
            if (condition_block_id == phi->GetSingleWordInOperand(i + 1)) {
              exit_value_[phi->result_id()] =
                  def_use_mgr->GetDef(phi->GetSingleWordInOperand(i));
            }
          }
        });
  } else {
    DominatorTree* dom_tree =
        &context_->GetDominatorAnalysis(loop_utils_.GetFunction())
             ->GetDomTree();
    BasicBlock* condition_block = cfg.block(condition_block_id);

    loop_->GetHeaderBlock()->ForEachPhiInst(
        [dom_tree, condition_block, this](Instruction* phi) {
          std::unordered_set<Instruction*> operations;



          GetIteratorUpdateOperations(loop_, phi, &operations);

          for (Instruction* insn : operations) {
            if (insn == phi) {
              continue;
            }
            if (dom_tree->Dominates(context_->get_instr_block(insn),
                                    condition_block)) {
              return;
            }
          }
          exit_value_[phi->result_id()] = phi;
        });
  }
}

bool LoopPeeling::FixExitCondition(
    const std::function<uint32_t(Instruction*)>& condition_builder) {
  CFG& cfg = *context_->cfg();

  uint32_t condition_block_id = 0;
  for (uint32_t id : cfg.preds(GetClonedLoop()->GetMergeBlock()->id())) {
    if (GetClonedLoop()->IsInsideLoop(id)) {
      condition_block_id = id;
      break;
    }
  }
  assert(condition_block_id != 0 && "2nd loop in improperly connected");

  BasicBlock* condition_block = cfg.block(condition_block_id);
  Instruction* exit_condition = condition_block->terminator();
  assert(exit_condition->opcode() == spv::Op::OpBranchConditional);
  BasicBlock::iterator insert_point = condition_block->tail();
  if (condition_block->GetMergeInst()) {
    --insert_point;
  }

  uint32_t new_cond_id = condition_builder(&*insert_point);
  if (new_cond_id == 0) {
    return false;
  }
  exit_condition->SetInOperand(0, {new_cond_id});

  uint32_t to_continue_block_idx =
      GetClonedLoop()->IsInsideLoop(exit_condition->GetSingleWordInOperand(1))
          ? 1
          : 2;
  exit_condition->SetInOperand(
      1, {exit_condition->GetSingleWordInOperand(to_continue_block_idx)});
  exit_condition->SetInOperand(2, {GetClonedLoop()->GetMergeBlock()->id()});


  context_->get_def_use_mgr()->AnalyzeInstUse(exit_condition);
  return true;
}

BasicBlock* LoopPeeling::CreateBlockBefore(BasicBlock* bb) {
  analysis::DefUseManager* def_use_mgr = context_->get_def_use_mgr();
  CFG& cfg = *context_->cfg();
  assert(cfg.preds(bb->id()).size() == 1 && "More than one predecessor");

  uint32_t new_id = context_->TakeNextId();
  if (new_id == 0) {
    return nullptr;
  }
  std::unique_ptr<BasicBlock> new_bb =
      MakeUnique<BasicBlock>(std::unique_ptr<Instruction>(
          new Instruction(context_, spv::Op::OpLabel, 0, new_id, {})));

  Loop* in_loop = (*loop_utils_.GetLoopDescriptor())[bb];
  if (in_loop) {
    in_loop->AddBasicBlock(new_bb.get());
    loop_utils_.GetLoopDescriptor()->SetBasicBlockToLoop(new_bb->id(), in_loop);
  }

  context_->set_instr_block(new_bb->GetLabelInst(), new_bb.get());
  def_use_mgr->AnalyzeInstDefUse(new_bb->GetLabelInst());

  BasicBlock* bb_pred = cfg.block(cfg.preds(bb->id())[0]);
  bb_pred->tail()->ForEachInId([bb, &new_bb](uint32_t* id) {
    if (*id == bb->id()) {
      *id = new_bb->id();
    }
  });
  cfg.RemoveEdge(bb_pred->id(), bb->id());
  cfg.AddEdge(bb_pred->id(), new_bb->id());
  def_use_mgr->AnalyzeInstUse(&*bb_pred->tail());


  bb->ForEachPhiInst([&new_bb, def_use_mgr](Instruction* phi) {
    phi->SetInOperand(1, {new_bb->id()});
    def_use_mgr->AnalyzeInstUse(phi);
  });
  InstructionBuilder(
      context_, new_bb.get(),
      IRContext::kAnalysisDefUse | IRContext::kAnalysisInstrToBlockMapping)
      .AddBranch(bb->id());
  cfg.RegisterBlock(new_bb.get());


  Function::iterator it = loop_utils_.GetFunction()->FindBlock(bb->id());
  assert(it != loop_utils_.GetFunction()->end() &&
         "Basic block not found in the function.");
  BasicBlock* ret = new_bb.get();
  loop_utils_.GetFunction()->AddBasicBlock(std::move(new_bb), it);
  return ret;
}

BasicBlock* LoopPeeling::ProtectLoop(Loop* loop, Instruction* condition,
                                     BasicBlock* if_merge) {
  BasicBlock* if_block = loop->GetOrCreatePreHeaderBlock();
  if (!if_block) {
    return nullptr;
  }

  loop->SetPreHeaderBlock(nullptr);

  context_->KillInst(&*if_block->tail());

  InstructionBuilder builder(
      context_, if_block,
      IRContext::kAnalysisDefUse | IRContext::kAnalysisInstrToBlockMapping);
  builder.AddConditionalBranch(condition->result_id(),
                               loop->GetHeaderBlock()->id(), if_merge->id(),
                               if_merge->id());

  return if_block;
}

bool LoopPeeling::PeelBefore(uint32_t peel_factor) {
  assert(CanPeelLoop() && "Cannot peel loop");
  LoopUtils::LoopCloningResult clone_results;


  if (!DuplicateAndConnectLoop(&clone_results)) {
    return false;
  }


  if (!InsertCanonicalInductionVariable(&clone_results)) {
    return false;
  }

  InstructionBuilder builder(
      context_, &*cloned_loop_->GetPreHeaderBlock()->tail(),
      IRContext::kAnalysisDefUse | IRContext::kAnalysisInstrToBlockMapping);
  Instruction* factor =
      builder.GetIntConstant(peel_factor, int_type_->IsSigned());
  if (!factor) return false;

  Instruction* has_remaining_iteration = builder.AddLessThan(
      factor->result_id(), loop_iteration_count_->result_id());
  if (!has_remaining_iteration) return false;
  Instruction* max_iteration = builder.AddSelect(
      factor->type_id(), has_remaining_iteration->result_id(),
      factor->result_id(), loop_iteration_count_->result_id());
  if (!max_iteration) return false;




  if (!FixExitCondition(
          [max_iteration, this](Instruction* insert_before_point) {
            Instruction* new_cond =
                InstructionBuilder(context_, insert_before_point,
                                   IRContext::kAnalysisDefUse |
                                       IRContext::kAnalysisInstrToBlockMapping)
                    .AddLessThan(canonical_induction_variable_->result_id(),
                                 max_iteration->result_id());
            return new_cond ? new_cond->result_id() : 0;
          })) {
    return false;
  }



  BasicBlock* if_merge_block = loop_->GetMergeBlock();
  BasicBlock* new_merge_block = CreateBlockBefore(loop_->GetMergeBlock());
  if (!new_merge_block) return false;
  loop_->SetMergeBlock(new_merge_block);


  BasicBlock* if_block =
      ProtectLoop(loop_, has_remaining_iteration, if_merge_block);
  if (!if_block) return false;


  if_merge_block->ForEachPhiInst(
      [&clone_results, if_block, this](Instruction* phi) {

        uint32_t incoming_value = phi->GetSingleWordInOperand(0);
        auto def_in_loop = clone_results.value_map_.find(incoming_value);
        if (def_in_loop != clone_results.value_map_.end())
          incoming_value = def_in_loop->second;
        phi->AddOperand(
            {spv_operand_type_t::SPV_OPERAND_TYPE_ID, {incoming_value}});
        phi->AddOperand(
            {spv_operand_type_t::SPV_OPERAND_TYPE_ID, {if_block->id()}});
        context_->get_def_use_mgr()->AnalyzeInstUse(phi);
      });

  context_->InvalidateAnalysesExceptFor(
      IRContext::kAnalysisDefUse | IRContext::kAnalysisInstrToBlockMapping |
      IRContext::kAnalysisLoopAnalysis | IRContext::kAnalysisCFG);
  return true;
}

bool LoopPeeling::PeelAfter(uint32_t peel_factor) {
  assert(CanPeelLoop() && "Cannot peel loop");
  LoopUtils::LoopCloningResult clone_results;


  if (!DuplicateAndConnectLoop(&clone_results)) {
    return false;
  }


  InsertCanonicalInductionVariable(&clone_results);

  InstructionBuilder builder(
      context_, &*cloned_loop_->GetPreHeaderBlock()->tail(),
      IRContext::kAnalysisDefUse | IRContext::kAnalysisInstrToBlockMapping);
  Instruction* factor =
      builder.GetIntConstant(peel_factor, int_type_->IsSigned());
  if (!factor) return false;

  Instruction* has_remaining_iteration = builder.AddLessThan(
      factor->result_id(), loop_iteration_count_->result_id());
  if (!has_remaining_iteration) return false;




  if (!FixExitCondition([factor,
                         this](Instruction* insert_before_point) -> uint32_t {
        InstructionBuilder cond_builder(
            context_, insert_before_point,
            IRContext::kAnalysisDefUse |
                IRContext::kAnalysisInstrToBlockMapping);


        Instruction* add = cond_builder.AddIAdd(
            canonical_induction_variable_->type_id(),
            canonical_induction_variable_->result_id(), factor->result_id());
        if (!add) return 0;
        Instruction* new_cond = cond_builder.AddLessThan(
            add->result_id(), loop_iteration_count_->result_id());
        return new_cond ? new_cond->result_id() : 0;
      })) {
    return false;
  }





  BasicBlock* pre_header = GetOriginalLoop()->GetPreHeaderBlock();
  if (!pre_header) {
    return false;
  }
  BasicBlock* new_merge_block = CreateBlockBefore(pre_header);
  if (!new_merge_block) {
    return false;
  }
  GetClonedLoop()->SetMergeBlock(new_merge_block);



  BasicBlock* if_block = ProtectLoop(cloned_loop_, has_remaining_iteration,
                                     GetOriginalLoop()->GetPreHeaderBlock());
  if (!if_block) {
    return false;
  }







  bool ok = true;
  GetOriginalLoop()->GetHeaderBlock()->ForEachPhiInst(
      [&clone_results, if_block, &ok, this](Instruction* phi) {
        if (!ok) return;
        analysis::DefUseManager* def_use_mgr = context_->get_def_use_mgr();

        auto find_value_idx = [](Instruction* phi_inst, Loop* loop) {
          uint32_t preheader_value_idx =
              !loop->IsInsideLoop(phi_inst->GetSingleWordInOperand(1)) ? 0 : 2;
          return preheader_value_idx;
        };

        Instruction* cloned_phi =
            def_use_mgr->GetDef(clone_results.value_map_.at(phi->result_id()));
        uint32_t cloned_preheader_value = cloned_phi->GetSingleWordInOperand(
            find_value_idx(cloned_phi, GetClonedLoop()));

        Instruction* new_phi =
            InstructionBuilder(context_,
                               &*GetOriginalLoop()->GetPreHeaderBlock()->tail(),
                               IRContext::kAnalysisDefUse |
                                   IRContext::kAnalysisInstrToBlockMapping)
                .AddPhi(phi->type_id(),
                        {phi->GetSingleWordInOperand(
                             find_value_idx(phi, GetOriginalLoop())),
                         GetClonedLoop()->GetMergeBlock()->id(),
                         cloned_preheader_value, if_block->id()});
        if (!new_phi) {
          ok = false;
          return;
        }

        phi->SetInOperand(find_value_idx(phi, GetOriginalLoop()),
                          {new_phi->result_id()});
        def_use_mgr->AnalyzeInstUse(phi);
      });
  if (!ok) return false;

  context_->InvalidateAnalysesExceptFor(
      IRContext::kAnalysisDefUse | IRContext::kAnalysisInstrToBlockMapping |
      IRContext::kAnalysisLoopAnalysis | IRContext::kAnalysisCFG);
  return true;
}

Pass::Status LoopPeelingPass::Process() {
  bool modified = false;
  Module* module = context()->module();


  for (Function& f : *module) {
    Pass::Status status = ProcessFunction(&f);
    if (status == Status::Failure) {
      return Status::Failure;
    }
    if (status == Status::SuccessWithChange) {
      modified = true;
    }
  }

  return modified ? Status::SuccessWithChange : Status::SuccessWithoutChange;
}

Pass::Status LoopPeelingPass::ProcessFunction(Function* f) {
  bool modified = false;
  LoopDescriptor& loop_descriptor = *context()->GetLoopDescriptor(f);

  std::vector<Loop*> to_process_loop;
  to_process_loop.reserve(loop_descriptor.NumLoops());
  for (Loop& l : loop_descriptor) {
    to_process_loop.push_back(&l);
  }

  ScalarEvolutionAnalysis scev_analysis(context());

  for (Loop* loop : to_process_loop) {
    CodeMetrics loop_size;
    loop_size.Analyze(*loop);

    auto try_peel = [&loop_size, &modified, this](
                        Loop* loop_to_peel) -> std::pair<Pass::Status, Loop*> {
      if (!loop_to_peel->IsLCSSA()) {
        if (!LoopUtils(context(), loop_to_peel).MakeLoopClosedSSA()) {
          return {Pass::Status::Failure, nullptr};
        }
      }

      Pass::Status status;
      Loop* still_peelable_loop;
      std::tie(status, still_peelable_loop) =
          ProcessLoop(loop_to_peel, &loop_size);

      if (status == Pass::Status::SuccessWithChange) {
        modified = true;
      }

      return {status, still_peelable_loop};
    };

    Pass::Status status;
    Loop* still_peelable_loop;
    std::tie(status, still_peelable_loop) = try_peel(loop);

    if (status == Pass::Status::Failure) {
      return Pass::Status::Failure;
    }




    if (still_peelable_loop) {
      std::tie(status, still_peelable_loop) = try_peel(still_peelable_loop);
      if (status == Pass::Status::Failure) {
        return Pass::Status::Failure;
      }
    }
  }

  return modified ? Pass::Status::SuccessWithChange
                  : Pass::Status::SuccessWithoutChange;
}

std::tuple<Pass::Status, Loop*> LoopPeelingPass::ProcessLoop(
    Loop* loop, CodeMetrics* loop_size) {
  ScalarEvolutionAnalysis* scev_analysis =
      context()->GetScalarEvolutionAnalysis();

  std::tuple<Pass::Status, Loop*> bail_out{Pass::Status::SuccessWithoutChange,
                                           nullptr};

  BasicBlock* exit_block = loop->FindConditionBlock();
  if (!exit_block) {
    return bail_out;
  }

  Instruction* exiting_iv = loop->FindConditionVariable(exit_block);
  if (!exiting_iv) {
    return bail_out;
  }
  size_t iterations = 0;
  if (!loop->FindNumberOfIterations(exiting_iv, &*exit_block->tail(),
                                    &iterations)) {
    return bail_out;
  }
  if (!iterations) {
    return bail_out;
  }

  Instruction* canonical_induction_variable = nullptr;

  loop->GetHeaderBlock()->WhileEachPhiInst([&canonical_induction_variable,
                                            scev_analysis,
                                            this](Instruction* insn) {
    if (const SERecurrentNode* iv =
            scev_analysis->AnalyzeInstruction(insn)->AsSERecurrentNode()) {
      const SEConstantNode* offset = iv->GetOffset()->AsSEConstantNode();
      const SEConstantNode* coeff = iv->GetCoefficient()->AsSEConstantNode();
      if (offset && coeff && offset->FoldToSingleValue() == 0 &&
          coeff->FoldToSingleValue() == 1) {
        if (context()->get_type_mgr()->GetType(insn->type_id())->AsInteger()) {
          canonical_induction_variable = insn;
          return false;
        }
      }
    }
    return true;
  });

  bool is_signed = canonical_induction_variable
                       ? context()
                             ->get_type_mgr()
                             ->GetType(canonical_induction_variable->type_id())
                             ->AsInteger()
                             ->IsSigned()
                       : false;

  LoopPeeling peeler(
      loop,
      InstructionBuilder(
          context(), loop->GetHeaderBlock(),
          IRContext::kAnalysisDefUse | IRContext::kAnalysisInstrToBlockMapping)
          .GetIntConstant<uint32_t>(static_cast<uint32_t>(iterations),
                                    is_signed),
      canonical_induction_variable);

  if (!peeler.CanPeelLoop()) {
    return bail_out;
  }



  LoopPeelingInfo peel_info(loop, iterations, scev_analysis);

  uint32_t peel_before_factor = 0;
  uint32_t peel_after_factor = 0;

  for (uint32_t block : loop->GetBlocks()) {
    if (block == exit_block->id()) {
      continue;
    }
    BasicBlock* bb = cfg()->block(block);
    PeelDirection direction;
    uint32_t factor;
    std::tie(direction, factor) = peel_info.GetPeelingInfo(bb);

    if (direction == PeelDirection::kNone) {
      continue;
    }
    if (direction == PeelDirection::kBefore) {
      peel_before_factor = std::max(peel_before_factor, factor);
    } else {
      assert(direction == PeelDirection::kAfter);
      peel_after_factor = std::max(peel_after_factor, factor);
    }
  }
  PeelDirection direction = PeelDirection::kNone;
  uint32_t factor = 0;


  if (peel_before_factor) {
    factor = peel_before_factor;
    direction = PeelDirection::kBefore;
  }
  if (peel_after_factor) {
    if (peel_before_factor < peel_after_factor) {

      factor = peel_after_factor;
      direction = PeelDirection::kAfter;
    }
  }


  if (direction == PeelDirection::kNone) return bail_out;



  if (factor * loop_size->roi_size_ > code_grow_threshold_) {
    return bail_out;
  }
  loop_size->roi_size_ *= factor;


  Loop* extra_opportunity = nullptr;

  if (direction == PeelDirection::kBefore) {
    if (!peeler.PeelBefore(factor)) {
      return {Pass::Status::Failure, nullptr};
    }
    if (stats_) {
      stats_->peeled_loops_.emplace_back(loop, PeelDirection::kBefore, factor);
    }
    if (peel_after_factor) {

      extra_opportunity = peeler.GetOriginalLoop();
    }
  } else {
    if (!peeler.PeelAfter(factor)) {
      return {Pass::Status::Failure, nullptr};
    }
    if (stats_) {
      stats_->peeled_loops_.emplace_back(loop, PeelDirection::kAfter, factor);
    }
    if (peel_before_factor) {

      extra_opportunity = peeler.GetClonedLoop();
    }
  }

  return {Pass::Status::SuccessWithChange, extra_opportunity};
}

uint32_t LoopPeelingPass::LoopPeelingInfo::GetFirstLoopInvariantOperand(
    Instruction* condition) const {
  for (uint32_t i = 0; i < condition->NumInOperands(); i++) {
    BasicBlock* bb =
        context_->get_instr_block(condition->GetSingleWordInOperand(i));
    if (bb && loop_->IsInsideLoop(bb)) {
      return condition->GetSingleWordInOperand(i);
    }
  }

  return 0;
}

uint32_t LoopPeelingPass::LoopPeelingInfo::GetFirstNonLoopInvariantOperand(
    Instruction* condition) const {
  for (uint32_t i = 0; i < condition->NumInOperands(); i++) {
    BasicBlock* bb =
        context_->get_instr_block(condition->GetSingleWordInOperand(i));
    if (!bb || !loop_->IsInsideLoop(bb)) {
      return condition->GetSingleWordInOperand(i);
    }
  }

  return 0;
}

static bool IsHandledCondition(spv::Op opcode) {
  switch (opcode) {
    case spv::Op::OpIEqual:
    case spv::Op::OpINotEqual:
    case spv::Op::OpUGreaterThan:
    case spv::Op::OpSGreaterThan:
    case spv::Op::OpUGreaterThanEqual:
    case spv::Op::OpSGreaterThanEqual:
    case spv::Op::OpULessThan:
    case spv::Op::OpSLessThan:
    case spv::Op::OpULessThanEqual:
    case spv::Op::OpSLessThanEqual:
      return true;
    default:
      return false;
  }
}

LoopPeelingPass::LoopPeelingInfo::Direction
LoopPeelingPass::LoopPeelingInfo::GetPeelingInfo(BasicBlock* bb) const {
  if (bb->terminator()->opcode() != spv::Op::OpBranchConditional) {
    return GetNoneDirection();
  }

  analysis::DefUseManager* def_use_mgr = context_->get_def_use_mgr();

  Instruction* condition =
      def_use_mgr->GetDef(bb->terminator()->GetSingleWordInOperand(0));

  if (!IsHandledCondition(condition->opcode())) {
    return GetNoneDirection();
  }

  if (!GetFirstLoopInvariantOperand(condition)) {

    return GetNoneDirection();
  }
  if (!GetFirstNonLoopInvariantOperand(condition)) {

    return GetNoneDirection();
  }


  SExpression lhs = scev_analysis_->AnalyzeInstruction(
      def_use_mgr->GetDef(condition->GetSingleWordInOperand(0)));
  if (lhs->GetType() == SENode::CanNotCompute) {

    return GetNoneDirection();
  }


  SExpression rhs = scev_analysis_->AnalyzeInstruction(
      def_use_mgr->GetDef(condition->GetSingleWordInOperand(1)));
  if (rhs->GetType() == SENode::CanNotCompute) {

    return GetNoneDirection();
  }


  bool is_lhs_rec = !scev_analysis_->IsLoopInvariant(loop_, lhs);
  bool is_rhs_rec = !scev_analysis_->IsLoopInvariant(loop_, rhs);

  if ((is_lhs_rec && is_rhs_rec) || (!is_lhs_rec && !is_rhs_rec)) {
    return GetNoneDirection();
  }

  if (is_lhs_rec) {
    if (!lhs->AsSERecurrentNode() ||
        lhs->AsSERecurrentNode()->GetLoop() != loop_) {
      return GetNoneDirection();
    }
  }
  if (is_rhs_rec) {
    if (!rhs->AsSERecurrentNode() ||
        rhs->AsSERecurrentNode()->GetLoop() != loop_) {
      return GetNoneDirection();
    }
  }













  CmpOperator cmp_operator;
  switch (condition->opcode()) {
    default:
      return GetNoneDirection();
    case spv::Op::OpIEqual:
    case spv::Op::OpINotEqual:
      return HandleEquality(lhs, rhs);
    case spv::Op::OpUGreaterThan:
    case spv::Op::OpSGreaterThan: {
      cmp_operator = CmpOperator::kGT;
      break;
    }
    case spv::Op::OpULessThan:
    case spv::Op::OpSLessThan: {
      cmp_operator = CmpOperator::kLT;
      break;
    }

    case spv::Op::OpUGreaterThanEqual:
    case spv::Op::OpSGreaterThanEqual: {
      cmp_operator = CmpOperator::kGE;
      break;
    }
    case spv::Op::OpULessThanEqual:
    case spv::Op::OpSLessThanEqual: {
      cmp_operator = CmpOperator::kLE;
      break;
    }
  }


  if (is_lhs_rec) {
    std::swap(lhs, rhs);
    switch (cmp_operator) {
      case CmpOperator::kLT: {
        cmp_operator = CmpOperator::kGT;
        break;
      }
      case CmpOperator::kGT: {
        cmp_operator = CmpOperator::kLT;
        break;
      }
      case CmpOperator::kLE: {
        cmp_operator = CmpOperator::kGE;
        break;
      }
      case CmpOperator::kGE: {
        cmp_operator = CmpOperator::kLE;
        break;
      }
    }
  }
  return HandleInequality(cmp_operator, lhs, rhs->AsSERecurrentNode());
}

SExpression LoopPeelingPass::LoopPeelingInfo::GetValueAtFirstIteration(
    SERecurrentNode* rec) const {
  return rec->GetOffset();
}

SExpression LoopPeelingPass::LoopPeelingInfo::GetValueAtIteration(
    SERecurrentNode* rec, int64_t iteration) const {
  SExpression coeff = rec->GetCoefficient();
  SExpression offset = rec->GetOffset();

  return (coeff * iteration) + offset;
}

SExpression LoopPeelingPass::LoopPeelingInfo::GetValueAtLastIteration(
    SERecurrentNode* rec) const {
  return GetValueAtIteration(rec, loop_max_iterations_ - 1);
}

bool LoopPeelingPass::LoopPeelingInfo::EvalOperator(CmpOperator cmp_op,
                                                    SExpression lhs,
                                                    SExpression rhs,
                                                    bool* result) const {
  assert(scev_analysis_->IsLoopInvariant(loop_, lhs));
  assert(scev_analysis_->IsLoopInvariant(loop_, rhs));


  switch (cmp_op) {
    case CmpOperator::kLT: {
      return scev_analysis_->IsAlwaysGreaterThanZero(rhs - lhs, result);
    }
    case CmpOperator::kGT: {
      return scev_analysis_->IsAlwaysGreaterThanZero(lhs - rhs, result);
    }
    case CmpOperator::kLE: {
      return scev_analysis_->IsAlwaysGreaterOrEqualToZero(rhs - lhs, result);
    }
    case CmpOperator::kGE: {
      return scev_analysis_->IsAlwaysGreaterOrEqualToZero(lhs - rhs, result);
    }
  }
  return false;
}

LoopPeelingPass::LoopPeelingInfo::Direction
LoopPeelingPass::LoopPeelingInfo::HandleEquality(SExpression lhs,
                                                 SExpression rhs) const {
  {

    SExpression lhs_cst = lhs;
    if (SERecurrentNode* rec_node = lhs->AsSERecurrentNode()) {
      lhs_cst = rec_node->GetOffset();
    }
    SExpression rhs_cst = rhs;
    if (SERecurrentNode* rec_node = rhs->AsSERecurrentNode()) {
      rhs_cst = rec_node->GetOffset();
    }

    if (lhs_cst == rhs_cst) {
      return Direction{LoopPeelingPass::PeelDirection::kBefore, 1};
    }
  }

  {

    SExpression lhs_cst = lhs;
    if (SERecurrentNode* rec_node = lhs->AsSERecurrentNode()) {


      lhs_cst = GetValueAtLastIteration(rec_node);
    }
    SExpression rhs_cst = rhs;
    if (SERecurrentNode* rec_node = rhs->AsSERecurrentNode()) {


      rhs_cst = GetValueAtLastIteration(rec_node);
    }

    if (lhs_cst == rhs_cst) {
      return Direction{LoopPeelingPass::PeelDirection::kAfter, 1};
    }
  }

  return GetNoneDirection();
}

LoopPeelingPass::LoopPeelingInfo::Direction
LoopPeelingPass::LoopPeelingInfo::HandleInequality(CmpOperator cmp_op,
                                                   SExpression lhs,
                                                   SERecurrentNode* rhs) const {
  SExpression offset = rhs->GetOffset();
  SExpression coefficient = rhs->GetCoefficient();

  std::pair<SExpression, int64_t> flip_iteration = (lhs - offset) / coefficient;
  if (!flip_iteration.first->AsSEConstantNode()) {
    return GetNoneDirection();
  }

  int64_t iteration =
      flip_iteration.first->AsSEConstantNode()->FoldToSingleValue() +
      !!flip_iteration.second;
  if (iteration <= 0 ||
      loop_max_iterations_ <= static_cast<uint64_t>(iteration)) {

    return GetNoneDirection();
  }




  if (!flip_iteration.second &&
      (cmp_op == CmpOperator::kLE || cmp_op == CmpOperator::kGE)) {
    bool first_iteration;
    bool current_iteration;
    if (!EvalOperator(cmp_op, lhs, offset, &first_iteration) ||
        !EvalOperator(cmp_op, lhs, GetValueAtIteration(rhs, iteration),
                      &current_iteration)) {
      return GetNoneDirection();
    }

    if (first_iteration == current_iteration) {
      iteration++;
    }
  }

  uint32_t cast_iteration = 0;

  if (static_cast<uint64_t>(iteration) < std::numeric_limits<uint32_t>::max()) {
    cast_iteration = static_cast<uint32_t>(iteration);
  }

  if (cast_iteration) {

    if (loop_max_iterations_ / 2 > cast_iteration) {
      return Direction{LoopPeelingPass::PeelDirection::kBefore, cast_iteration};
    } else {
      return Direction{
          LoopPeelingPass::PeelDirection::kAfter,
          static_cast<uint32_t>(loop_max_iterations_ - cast_iteration)};
    }
  }

  return GetNoneDirection();
}

}
}
