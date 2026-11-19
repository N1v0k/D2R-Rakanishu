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

#include "source/opt/loop_fission.h"

#include <set>

#include "source/opt/register_pressure.h"




































namespace spvtools {
namespace opt {

class LoopFissionImpl {
 public:
  LoopFissionImpl(IRContext* context, Loop* loop)
      : context_(context), loop_(loop), load_used_in_condition_(false) {}





  bool GroupInstructionsByUseDef();



  bool CanPerformSplit();


  Loop* SplitLoop();



  bool MovableInstruction(const Instruction& inst) const;

 private:


  void TraverseUseDef(Instruction* inst, std::set<Instruction*>* returned_set,
                      bool ignore_phi_users = false, bool report_loads = false);





  std::set<Instruction*> cloned_loop_instructions_;
  std::set<Instruction*> original_loop_instructions_;




  std::set<Instruction*> seen_instructions_;


  std::map<Instruction*, size_t> instruction_order_;

  IRContext* context_;

  Loop* loop_;




  bool load_used_in_condition_;
};

bool LoopFissionImpl::MovableInstruction(const Instruction& inst) const {
  return inst.opcode() == spv::Op::OpLoad ||
         inst.opcode() == spv::Op::OpStore ||
         inst.opcode() == spv::Op::OpSelectionMerge ||
         inst.opcode() == spv::Op::OpPhi || inst.IsOpcodeCodeMotionSafe();
}

void LoopFissionImpl::TraverseUseDef(Instruction* inst,
                                     std::set<Instruction*>* returned_set,
                                     bool ignore_phi_users, bool report_loads) {
  assert(returned_set && "Set to be returned cannot be null.");

  analysis::DefUseManager* def_use = context_->get_def_use_mgr();
  std::set<Instruction*>& inst_set = *returned_set;




  std::function<void(Instruction*)> traverser_functor;
  traverser_functor = [this, def_use, &inst_set, &traverser_functor,
                       ignore_phi_users, report_loads](Instruction* user) {


    if (!user || seen_instructions_.count(user) != 0 ||
        !context_->get_instr_block(user) ||
        !loop_->IsInsideLoop(context_->get_instr_block(user))) {
      return;
    }






    if (user->opcode() == spv::Op::OpLoopMerge ||
        user->opcode() == spv::Op::OpLabel)
      return;




    if (user->opcode() == spv::Op::OpLoad && report_loads) {
      load_used_in_condition_ = true;
    }



    seen_instructions_.insert(user);

    inst_set.insert(user);


    auto traverse_operand = [&traverser_functor, def_use](const uint32_t* id) {
      traverser_functor(def_use->GetDef(*id));
    };
    user->ForEachInOperand(traverse_operand);


    if (ignore_phi_users && user->opcode() == spv::Op::OpPhi) return;


    def_use->ForEachUser(user, traverser_functor);


    auto traverse_use = [&traverser_functor](Instruction* use, uint32_t) {
      traverser_functor(use);
    };
    def_use->ForEachUse(user, traverse_use);

  };



  traverser_functor(inst);
}

bool LoopFissionImpl::GroupInstructionsByUseDef() {
  std::vector<std::set<Instruction*>> sets{};



  BasicBlock* condition_block = loop_->FindConditionBlock();

  if (!condition_block) return false;
  Instruction* condition = &*condition_block->tail();




  Function& function = *loop_->GetHeaderBlock()->GetParent();




  std::set<Instruction*> instructions_to_ignore{};
  TraverseUseDef(condition, &instructions_to_ignore, true, true);




  for (BasicBlock& block : function) {
    if (!loop_->IsInsideLoop(block.id())) continue;

    for (Instruction& inst : block) {

      if (inst.opcode() == spv::Op::OpSelectionMerge || inst.IsBranch()) {
        TraverseUseDef(&inst, &instructions_to_ignore, true, true);
      }
    }
  }



  for (BasicBlock& block : function) {
    if (!loop_->IsInsideLoop(block.id()) ||
        loop_->GetHeaderBlock()->id() == block.id())
      continue;

    for (Instruction& inst : block) {

      if (inst.opcode() == spv::Op::OpLoad ||
          inst.opcode() == spv::Op::OpStore) {
        instruction_order_[&inst] = instruction_order_.size();
      }


      if (seen_instructions_.count(&inst) != 0) {
        continue;
      }


      std::set<Instruction*> inst_set{};
      TraverseUseDef(&inst, &inst_set);
      if (!inst_set.empty()) sets.push_back(std::move(inst_set));
    }
  }




  if (sets.size() < 2) {
    return false;
  }




  for (size_t index = 0; index < sets.size() / 2; ++index) {
    cloned_loop_instructions_.insert(sets[index].begin(), sets[index].end());
  }
  for (size_t index = sets.size() / 2; index < sets.size(); ++index) {
    original_loop_instructions_.insert(sets[index].begin(), sets[index].end());
  }

  return true;
}

bool LoopFissionImpl::CanPerformSplit() {


  if (load_used_in_condition_) {
    return false;
  }



  std::vector<const Loop*> loops;
  Loop* parent_loop = loop_;
  while (parent_loop) {
    loops.push_back(parent_loop);
    parent_loop = parent_loop->GetParent();
  }

  LoopDependenceAnalysis analysis{context_, loops};


  std::vector<Instruction*> set_one_stores{};


  std::vector<Instruction*> set_one_loads{};


  for (Instruction* inst : cloned_loop_instructions_) {
    if (inst->opcode() == spv::Op::OpStore) {
      set_one_stores.push_back(inst);
    } else if (inst->opcode() == spv::Op::OpLoad) {
      set_one_loads.push_back(inst);
    }



    if (!MovableInstruction(*inst)) return false;
  }



  const size_t loop_depth = loop_->GetDepth();



  for (Instruction* inst : original_loop_instructions_) {


    if (!MovableInstruction(*inst)) return false;



    if (inst->opcode() == spv::Op::OpLoad) {
      for (Instruction* store : set_one_stores) {
        DistanceVector vec{loop_depth};



        if (instruction_order_[store] > instruction_order_[inst]) {
          return false;
        }

        if (!analysis.GetDependence(store, inst, &vec)) {
          for (DistanceEntry& entry : vec.GetEntries()) {


            if (entry.distance > 0) return false;
          }
        }
      }
    } else if (inst->opcode() == spv::Op::OpStore) {
      for (Instruction* load : set_one_loads) {
        DistanceVector vec{loop_depth};


        if (instruction_order_[load] > instruction_order_[inst]) {
          return false;
        }


        if (!analysis.GetDependence(inst, load, &vec)) {
          for (DistanceEntry& entry : vec.GetEntries()) {


            if (entry.distance < 0) return false;
          }
        }
      }
    }
  }
  return true;
}

Loop* LoopFissionImpl::SplitLoop() {

  LoopUtils util{context_, loop_};
  LoopUtils::LoopCloningResult clone_results;
  Loop* cloned_loop = util.CloneAndAttachLoopToHeader(&clone_results);
  if (!cloned_loop) {
    return nullptr;
  }


  cloned_loop->UpdateLoopMergeInst();


  BasicBlock* pre_header = loop_->GetOrCreatePreHeaderBlock();
  if (!pre_header) {
    return nullptr;
  }
  Function::iterator it = util.GetFunction()->FindBlock(pre_header->id());
  util.GetFunction()->AddBasicBlocks(clone_results.cloned_bb_.begin(),
                                     clone_results.cloned_bb_.end(), ++it);
  loop_->SetPreHeaderBlock(cloned_loop->GetMergeBlock());

  std::vector<Instruction*> instructions_to_kill{};



  for (uint32_t id : loop_->GetBlocks()) {
    BasicBlock* block = context_->cfg()->block(id);

    for (Instruction& inst : *block) {


      if (cloned_loop_instructions_.count(&inst) == 1 &&
          original_loop_instructions_.count(&inst) == 0) {
        instructions_to_kill.push_back(&inst);
        if (inst.opcode() == spv::Op::OpPhi) {
          context_->ReplaceAllUsesWith(
              inst.result_id(), clone_results.value_map_[inst.result_id()]);
        }
      }
    }
  }



  for (uint32_t id : cloned_loop->GetBlocks()) {
    BasicBlock* block = context_->cfg()->block(id);
    for (Instruction& inst : *block) {
      Instruction* old_inst = clone_results.ptr_map_[&inst];


      if (cloned_loop_instructions_.count(old_inst) == 0 &&
          original_loop_instructions_.count(old_inst) == 1) {
        instructions_to_kill.push_back(&inst);
      }
    }
  }

  for (Instruction* i : instructions_to_kill) {
    context_->KillInst(i);
  }

  return cloned_loop;
}

LoopFissionPass::LoopFissionPass(const size_t register_threshold_to_split,
                                 bool split_multiple_times)
    : split_multiple_times_(split_multiple_times) {


  split_criteria_ =
      [register_threshold_to_split](
          const RegisterLiveness::RegionRegisterLiveness& liveness) {
        return liveness.used_registers_ > register_threshold_to_split;
      };
}

LoopFissionPass::LoopFissionPass() : split_multiple_times_(false) {

  split_criteria_ = [](const RegisterLiveness::RegionRegisterLiveness&) {
    return true;
  };
}

bool LoopFissionPass::ShouldSplitLoop(const Loop& loop, IRContext* c) {
  LivenessAnalysis* analysis = c->GetLivenessAnalysis();

  RegisterLiveness::RegionRegisterLiveness liveness{};

  Function* function = loop.GetHeaderBlock()->GetParent();
  analysis->Get(function)->ComputeLoopRegisterPressure(loop, &liveness);

  return split_criteria_(liveness);
}

Pass::Status LoopFissionPass::Process() {
  bool changed = false;

  for (Function& f : *context()->module()) {



    std::vector<Loop*> inner_most_loops{};
    LoopDescriptor& loop_descriptor = *context()->GetLoopDescriptor(&f);
    for (Loop& loop : loop_descriptor) {
      if (!loop.HasChildren() && ShouldSplitLoop(loop, context())) {
        inner_most_loops.push_back(&loop);
      }
    }


    std::vector<Loop*> new_loops_to_split{};

    while (!inner_most_loops.empty()) {
      for (Loop* loop : inner_most_loops) {
        LoopFissionImpl impl{context(), loop};




        if (!impl.GroupInstructionsByUseDef()) {
          continue;
        }

        if (impl.CanPerformSplit()) {
          Loop* second_loop = impl.SplitLoop();
          if (!second_loop) {
            return Status::Failure;
          }
          changed = true;
          context()->InvalidateAnalysesExceptFor(
              IRContext::kAnalysisLoopAnalysis);



          if (ShouldSplitLoop(*second_loop, context()))
            new_loops_to_split.push_back(second_loop);



          if (ShouldSplitLoop(*loop, context()))
            new_loops_to_split.push_back(loop);
        }
      }




      if (split_multiple_times_) {
        inner_most_loops = std::move(new_loops_to_split);
        new_loops_to_split = {};
      } else {
        break;
      }
    }
  }

  return changed ? Pass::Status::SuccessWithChange
                 : Pass::Status::SuccessWithoutChange;
}

}
}
