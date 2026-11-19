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

#include "source/opt/loop_unswitch_pass.h"

#include <functional>
#include <list>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "source/opt/basic_block.h"
#include "source/opt/dominator_tree.h"
#include "source/opt/fold.h"
#include "source/opt/function.h"
#include "source/opt/instruction.h"
#include "source/opt/ir_builder.h"
#include "source/opt/ir_context.h"
#include "source/opt/loop_descriptor.h"
#include "source/opt/loop_utils.h"

namespace spvtools {
namespace opt {
namespace {
constexpr uint32_t kTypePointerStorageClassInIdx = 0;





class LoopUnswitch {
 public:
  LoopUnswitch(IRContext* context, Function* function, Loop* loop,
               LoopDescriptor* loop_desc)
      : function_(function),
        loop_(loop),
        loop_desc_(*loop_desc),
        context_(context),
        switch_block_(nullptr) {}







  bool CanUnswitchLoop() {
    if (switch_block_) return true;
    if (loop_->IsSafeToClone()) return false;

    CFG& cfg = *context_->cfg();

    for (uint32_t bb_id : loop_->GetBlocks()) {
      BasicBlock* bb = cfg.block(bb_id);
      if (loop_->GetLatchBlock() == bb) {
        continue;
      }

      if (bb->terminator()->IsBranch() &&
          bb->terminator()->opcode() != spv::Op::OpBranch) {
        if (IsConditionNonConstantLoopInvariant(bb->terminator())) {
          switch_block_ = bb;
          break;
        }
      }
    }

    return switch_block_;
  }


  Function::iterator FindBasicBlockPosition(BasicBlock* bb_to_find) {
    Function::iterator it = function_->FindBlock(bb_to_find->id());
    assert(it != function_->end() && "Basic Block not found");
    return it;
  }




  BasicBlock* CreateBasicBlock(Function::iterator ip) {
    uint32_t new_label_id = TakeNextId();
    if (new_label_id == 0) {
      return nullptr;
    }

    analysis::DefUseManager* def_use_mgr = context_->get_def_use_mgr();

    BasicBlock* bb = &*ip.InsertBefore(std::unique_ptr<BasicBlock>(
        new BasicBlock(std::unique_ptr<Instruction>(new Instruction(
            context_, spv::Op::OpLabel, 0, new_label_id, {})))));
    bb->SetParent(function_);
    def_use_mgr->AnalyzeInstDef(bb->GetLabelInst());
    context_->set_instr_block(bb->GetLabelInst(), bb);

    return bb;
  }

  Instruction* GetValueForDefaultPathForSwitch(Instruction* switch_inst) {
    assert(switch_inst->opcode() == spv::Op::OpSwitch &&
           "The given instructoin must be an OpSwitch.");





    std::vector<uint32_t> existing_values;
    for (uint32_t i = 2; i < switch_inst->NumInOperands(); i += 2) {
      existing_values.push_back(switch_inst->GetSingleWordInOperand(i));
    }
    std::sort(existing_values.begin(), existing_values.end());
    uint32_t value_for_default_path = 0;
    if (existing_values.size() < std::numeric_limits<uint32_t>::max()) {
      for (value_for_default_path = 0;
           value_for_default_path < existing_values.size();
           value_for_default_path++) {
        if (existing_values[value_for_default_path] != value_for_default_path) {
          break;
        }
      }
    }
    InstructionBuilder builder(
        context_, static_cast<Instruction*>(nullptr),
        IRContext::kAnalysisDefUse | IRContext::kAnalysisInstrToBlockMapping);
    return builder.GetUintConstant(value_for_default_path);
  }


  bool PerformUnswitch() {
    assert(CanUnswitchLoop() &&
           "Cannot unswitch if there is not constant condition");
    assert(loop_->GetPreHeaderBlock() && "This loop has no pre-header block");
    assert(loop_->IsLCSSA() && "This loop is not in LCSSA form");

    CFG& cfg = *context_->cfg();
    DominatorTree* dom_tree =
        &context_->GetDominatorAnalysis(function_)->GetDomTree();
    analysis::DefUseManager* def_use_mgr = context_->get_def_use_mgr();
    LoopUtils loop_utils(context_, loop_);











    BasicBlock* if_merge_block = loop_->GetMergeBlock();



    BasicBlock* loop_merge_block =
        if_merge_block
            ? CreateBasicBlock(FindBasicBlockPosition(if_merge_block))
            : nullptr;
    if (if_merge_block && !loop_merge_block) {
      return false;
    }
    if (loop_merge_block) {

      InstructionBuilder builder(
          context_, loop_merge_block,
          IRContext::kAnalysisDefUse | IRContext::kAnalysisInstrToBlockMapping);
      builder.AddBranch(if_merge_block->id());
      builder.SetInsertPoint(&*loop_merge_block->begin());
      cfg.RegisterBlock(loop_merge_block);
      def_use_mgr->AnalyzeInstDef(loop_merge_block->GetLabelInst());
      bool ok = true;
      if_merge_block->ForEachPhiInst(
          [loop_merge_block, &ok, &builder, this](Instruction* phi) -> bool {
            Instruction* cloned = phi->Clone(context_);
            uint32_t new_id = TakeNextId();
            if (new_id == 0) {
              ok = false;
              return false;
            }
            cloned->SetResultId(new_id);
            builder.AddInstruction(std::unique_ptr<Instruction>(cloned));
            phi->SetInOperand(0, {cloned->result_id()});
            phi->SetInOperand(1, {loop_merge_block->id()});
            for (uint32_t j = phi->NumInOperands() - 1; j > 1; j--)
              phi->RemoveInOperand(j);
            return true;
          });
      if (!ok) return false;

      std::vector<uint32_t> preds = cfg.preds(if_merge_block->id());
      for (uint32_t pid : preds) {
        if (pid == loop_merge_block->id()) continue;
        BasicBlock* p_bb = cfg.block(pid);
        p_bb->ForEachSuccessorLabel(
            [if_merge_block, loop_merge_block](uint32_t* id) {
              if (*id == if_merge_block->id()) *id = loop_merge_block->id();
            });
        cfg.AddEdge(pid, loop_merge_block->id());
      }
      cfg.RemoveNonExistingEdges(if_merge_block->id());

      if (Loop* ploop = loop_->GetParent()) {
        ploop->AddBasicBlock(loop_merge_block);
        loop_desc_.SetBasicBlockToLoop(loop_merge_block->id(), ploop);
      }

      DominatorTreeNode* loop_merge_dtn =
          dom_tree->GetOrInsertNode(loop_merge_block);
      DominatorTreeNode* if_merge_block_dtn =
          dom_tree->GetOrInsertNode(if_merge_block);
      loop_merge_dtn->parent_ = if_merge_block_dtn->parent_;
      loop_merge_dtn->children_.push_back(if_merge_block_dtn);
      loop_merge_dtn->parent_->children_.push_back(loop_merge_dtn);
      if_merge_block_dtn->parent_->children_.erase(std::find(
          if_merge_block_dtn->parent_->children_.begin(),
          if_merge_block_dtn->parent_->children_.end(), if_merge_block_dtn));

      loop_->SetMergeBlock(loop_merge_block);
    }






    BasicBlock* if_block = loop_->GetPreHeaderBlock();


    BasicBlock* loop_pre_header =
        CreateBasicBlock(++FindBasicBlockPosition(if_block));
    if (!loop_pre_header) {
      return false;
    }
    InstructionBuilder(
        context_, loop_pre_header,
        IRContext::kAnalysisDefUse | IRContext::kAnalysisInstrToBlockMapping)
        .AddBranch(loop_->GetHeaderBlock()->id());

    if_block->tail()->SetInOperand(0, {loop_pre_header->id()});


    if (Loop* ploop = loop_desc_[if_block]) {
      ploop->AddBasicBlock(loop_pre_header);
      loop_desc_.SetBasicBlockToLoop(loop_pre_header->id(), ploop);
    }


    cfg.RegisterBlock(loop_pre_header);
    def_use_mgr->AnalyzeInstDef(loop_pre_header->GetLabelInst());
    cfg.AddEdge(if_block->id(), loop_pre_header->id());
    cfg.RemoveNonExistingEdges(loop_->GetHeaderBlock()->id());

    loop_->GetHeaderBlock()->ForEachPhiInst(
        [loop_pre_header, if_block](Instruction* phi) {
          phi->ForEachInId([loop_pre_header, if_block](uint32_t* id) {
            if (*id == if_block->id()) {
              *id = loop_pre_header->id();
            }
          });
        });
    loop_->SetPreHeaderBlock(loop_pre_header);


    DominatorTreeNode* loop_pre_header_dtn =
        dom_tree->GetOrInsertNode(loop_pre_header);
    DominatorTreeNode* if_block_dtn = dom_tree->GetTreeNode(if_block);
    loop_pre_header_dtn->parent_ = if_block_dtn;
    assert(
        if_block_dtn->children_.size() == 1 &&
        "A loop preheader should only have the header block as a child in the "
        "dominator tree");
    loop_pre_header_dtn->children_.push_back(if_block_dtn->children_[0]);
    if_block_dtn->children_.clear();
    if_block_dtn->children_.push_back(loop_pre_header_dtn);


    dom_tree->ResetDFNumbering();



    loop_->ComputeLoopStructuredOrder(&ordered_loop_blocks_, true, true);








    Instruction* iv_condition = &*switch_block_->tail();
    spv::Op iv_opcode = iv_condition->opcode();
    Instruction* condition =
        def_use_mgr->GetDef(iv_condition->GetOperand(0).words[0]);

    analysis::ConstantManager* cst_mgr = context_->get_constant_mgr();
    const analysis::Type* cond_type =
        context_->get_type_mgr()->GetType(condition->type_id());



    std::vector<std::pair<Instruction*, BasicBlock*>> constant_branch;

    Instruction* original_loop_constant_value;
    if (iv_opcode == spv::Op::OpBranchConditional) {
      constant_branch.emplace_back(
          cst_mgr->GetDefiningInstruction(cst_mgr->GetConstant(cond_type, {0})),
          nullptr);
      original_loop_constant_value =
          cst_mgr->GetDefiningInstruction(cst_mgr->GetConstant(cond_type, {1}));
    } else {


      original_loop_constant_value =
          GetValueForDefaultPathForSwitch(iv_condition);
      if (!original_loop_constant_value) {
        return false;
      }
      if (!original_loop_constant_value) {
        return false;
      }

      for (uint32_t i = 2; i < iv_condition->NumInOperands(); i += 2) {
        constant_branch.emplace_back(
            cst_mgr->GetDefiningInstruction(cst_mgr->GetConstant(
                cond_type, iv_condition->GetInOperand(i).words)),
            nullptr);
      }
    }


    std::unordered_set<uint32_t> if_merging_blocks;
    std::function<bool(uint32_t)> is_from_original_loop;
    if (loop_->GetHeaderBlock()->GetLoopMergeInst()) {
      if_merging_blocks.insert(if_merge_block->id());
      is_from_original_loop = [this](uint32_t id) {
        return loop_->IsInsideLoop(id) || loop_->GetMergeBlock()->id() == id;
      };
    } else {
      loop_->GetExitBlocks(&if_merging_blocks);
      is_from_original_loop = [this](uint32_t id) {
        return loop_->IsInsideLoop(id);
      };
    }

    for (auto& specialisation_pair : constant_branch) {
      Instruction* specialisation_value = specialisation_pair.first;



      LoopUtils::LoopCloningResult clone_result;

      Loop* cloned_loop =
          loop_utils.CloneLoop(&clone_result, ordered_loop_blocks_);
      if (!cloned_loop) {
        return false;
      }
      specialisation_pair.second = cloned_loop->GetPreHeaderBlock();





      {
        SpecializeLoop(cloned_loop, condition, specialisation_value);





        for (uint32_t merge_bb_id : if_merging_blocks) {
          BasicBlock* merge = context_->cfg()->block(merge_bb_id);

          merge->ForEachPhiInst(
              [is_from_original_loop, &clone_result](Instruction* phi) {
                uint32_t num_in_operands = phi->NumInOperands();
                for (uint32_t i = 0; i < num_in_operands; i += 2) {
                  uint32_t pred = phi->GetSingleWordInOperand(i + 1);
                  if (is_from_original_loop(pred)) {
                    pred = clone_result.value_map_.at(pred);
                    uint32_t incoming_value_id = phi->GetSingleWordInOperand(i);

                    ValueMapTy::iterator new_value =
                        clone_result.value_map_.find(incoming_value_id);
                    if (new_value != clone_result.value_map_.end()) {
                      incoming_value_id = new_value->second;
                    }
                    phi->AddOperand({SPV_OPERAND_TYPE_ID, {incoming_value_id}});
                    phi->AddOperand({SPV_OPERAND_TYPE_ID, {pred}});
                  }
                }
              });
        }
      }
      function_->AddBasicBlocks(clone_result.cloned_bb_.begin(),
                                clone_result.cloned_bb_.end(),
                                ++FindBasicBlockPosition(if_block));
    }


    SpecializeLoop(loop_, condition, original_loop_constant_value);
    BasicBlock* original_loop_target = loop_->GetPreHeaderBlock();






    context_->KillInst(&*if_block->tail());
    InstructionBuilder builder(context_, if_block);
    if (iv_opcode == spv::Op::OpBranchConditional) {
      assert(constant_branch.size() == 1);
      builder.AddConditionalBranch(
          condition->result_id(), original_loop_target->id(),
          constant_branch[0].second->id(),
          if_merge_block ? if_merge_block->id() : kInvalidId);
    } else {
      std::vector<std::pair<Operand::OperandData, uint32_t>> targets;
      for (auto& t : constant_branch) {
        targets.emplace_back(t.first->GetInOperand(0).words, t.second->id());
      }

      builder.AddSwitch(condition->result_id(), original_loop_target->id(),
                        targets,
                        if_merge_block ? if_merge_block->id() : kInvalidId);
    }

    switch_block_ = nullptr;
    ordered_loop_blocks_.clear();

    context_->InvalidateAnalysesExceptFor(
        IRContext::Analysis::kAnalysisLoopAnalysis);
    return true;
  }

 private:
  using ValueMapTy = std::unordered_map<uint32_t, uint32_t>;
  using BlockMapTy = std::unordered_map<uint32_t, BasicBlock*>;

  Function* function_;
  Loop* loop_;
  LoopDescriptor& loop_desc_;
  IRContext* context_;

  BasicBlock* switch_block_;

  std::unordered_map<uint32_t, bool> dynamically_uniform_;

  std::vector<BasicBlock*> ordered_loop_blocks_;


  uint32_t TakeNextId() { return context_->TakeNextId(); }











  void SpecializeLoop(Loop* loop, Instruction* to_version_insn,
                      Instruction* cst_value) {
    analysis::DefUseManager* def_use_mgr = context_->get_def_use_mgr();

    std::function<bool(uint32_t)> ignore_node;
    ignore_node = [loop](uint32_t bb_id) { return !loop->IsInsideLoop(bb_id); };

    std::vector<std::pair<Instruction*, uint32_t>> use_list;
    def_use_mgr->ForEachUse(to_version_insn,
                            [&use_list, &ignore_node, this](
                                Instruction* inst, uint32_t operand_index) {
                              BasicBlock* bb = context_->get_instr_block(inst);

                              if (!bb || ignore_node(bb->id())) {


                                return;
                              }
                              use_list.emplace_back(inst, operand_index);
                            });



    for (auto use : use_list) {
      Instruction* inst = use.first;
      uint32_t operand_index = use.second;





      assert(cst_value && "We do not have a value to use.");
      inst->SetOperand(operand_index, {cst_value->result_id()});
      def_use_mgr->AnalyzeInstUse(inst);
    }
  }



  bool IsDynamicallyUniform(Instruction* var, const BasicBlock* entry,
                            const DominatorTree& post_dom_tree) {
    assert(post_dom_tree.IsPostDominator());
    analysis::DefUseManager* def_use_mgr = context_->get_def_use_mgr();

    auto it = dynamically_uniform_.find(var->result_id());

    if (it != dynamically_uniform_.end()) return it->second;

    analysis::DecorationManager* dec_mgr = context_->get_decoration_mgr();

    bool& is_uniform = dynamically_uniform_[var->result_id()];
    is_uniform = false;

    dec_mgr->WhileEachDecoration(var->result_id(),
                                 uint32_t(spv::Decoration::Uniform),
                                 [&is_uniform](const Instruction&) {
                                   is_uniform = true;
                                   return false;
                                 });
    if (is_uniform) {
      return is_uniform;
    }

    BasicBlock* parent = context_->get_instr_block(var);
    if (!parent) {
      return is_uniform = true;
    }

    if (!post_dom_tree.Dominates(parent->id(), entry->id())) {
      return is_uniform = false;
    }
    if (var->opcode() == spv::Op::OpLoad) {
      const uint32_t PtrTypeId =
          def_use_mgr->GetDef(var->GetSingleWordInOperand(0))->type_id();
      const Instruction* PtrTypeInst = def_use_mgr->GetDef(PtrTypeId);
      auto storage_class = spv::StorageClass(
          PtrTypeInst->GetSingleWordInOperand(kTypePointerStorageClassInIdx));
      if (storage_class != spv::StorageClass::Uniform &&
          storage_class != spv::StorageClass::UniformConstant) {
        return is_uniform = false;
      }
    } else {
      if (!context_->IsCombinatorInstruction(var)) {
        return is_uniform = false;
      }
    }

    return is_uniform = var->WhileEachInId([entry, &post_dom_tree,
                                            this](const uint32_t* id) {
      return IsDynamicallyUniform(context_->get_def_use_mgr()->GetDef(*id),
                                  entry, post_dom_tree);
    });
  }



  bool IsConditionNonConstantLoopInvariant(Instruction* insn) {
    assert(insn->IsBranch());
    assert(insn->opcode() != spv::Op::OpBranch);
    analysis::DefUseManager* def_use_mgr = context_->get_def_use_mgr();

    Instruction* condition = def_use_mgr->GetDef(insn->GetOperand(0).words[0]);
    if (condition->IsConstant()) {
      return false;
    }

    if (loop_->IsInsideLoop(condition)) {
      return false;
    }

    return IsDynamicallyUniform(
        condition, function_->entry().get(),
        context_->GetPostDominatorAnalysis(function_)->GetDomTree());
  }
};

}

Pass::Status LoopUnswitchPass::Process() {
  bool modified = false;
  Module* module = context()->module();


  for (Function& f : *module) {
    Pass::Status status = ProcessFunction(&f);
    if (status == Status::Failure) return Status::Failure;
    if (status == Status::SuccessWithChange) modified = true;
  }

  return modified ? Status::SuccessWithChange : Status::SuccessWithoutChange;
}

Pass::Status LoopUnswitchPass::ProcessFunction(Function* f) {
  bool modified = false;
  std::unordered_set<Loop*> processed_loop;

  LoopDescriptor& loop_descriptor = *context()->GetLoopDescriptor(f);

  bool loop_changed = true;
  while (loop_changed) {
    loop_changed = false;
    for (Loop& loop : make_range(
             ++TreeDFIterator<Loop>(loop_descriptor.GetPlaceholderRootLoop()),
             TreeDFIterator<Loop>())) {
      if (processed_loop.count(&loop)) continue;
      processed_loop.insert(&loop);

      LoopUnswitch unswitcher(context(), f, &loop, &loop_descriptor);
      while (unswitcher.CanUnswitchLoop()) {
        if (!loop.IsLCSSA()) {
          if (!LoopUtils(context(), &loop).MakeLoopClosedSSA()) {
            return Status::Failure;
          }
        }
        if (!unswitcher.PerformUnswitch()) {
          return Status::Failure;
        }
        modified = true;
        loop_changed = true;
      }
      if (loop_changed) break;
    }
  }

  return modified ? Status::SuccessWithChange : Status::SuccessWithoutChange;
}

}
}
