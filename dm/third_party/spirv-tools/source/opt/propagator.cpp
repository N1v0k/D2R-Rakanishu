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

#include "source/opt/propagator.h"

namespace spvtools {
namespace opt {

void SSAPropagator::AddControlEdge(const Edge& edge) {
  BasicBlock* dest_bb = edge.dest;


  if (dest_bb == ctx_->cfg()->pseudo_exit_block()) {
    return;
  }



  if (!MarkEdgeExecutable(edge)) {
    return;
  }



  blocks_.push(dest_bb);
}

void SSAPropagator::AddSSAEdges(Instruction* instr) {

  if (instr->result_id() == 0) {
    return;
  }

  get_def_use_mgr()->ForEachUser(
      instr->result_id(), [this](Instruction* use_instr) {



        if (!BlockHasBeenSimulated(ctx_->get_instr_block(use_instr))) {
          return;
        }

        if (ShouldSimulateAgain(use_instr)) {
          ssa_edge_uses_.push(use_instr);
        }
      });
}

bool SSAPropagator::IsPhiArgExecutable(Instruction* phi, uint32_t i) const {
  BasicBlock* phi_bb = ctx_->get_instr_block(phi);

  uint32_t in_label_id = phi->GetSingleWordOperand(i + 1);
  Instruction* in_label_instr = get_def_use_mgr()->GetDef(in_label_id);
  BasicBlock* in_bb = ctx_->get_instr_block(in_label_instr);

  return IsEdgeExecutable(Edge(in_bb, phi_bb));
}

bool SSAPropagator::SetStatus(Instruction* inst, PropStatus status) {
  bool has_old_status = false;
  PropStatus old_status = kVarying;
  if (HasStatus(inst)) {
    has_old_status = true;
    old_status = Status(inst);
  }

  assert((!has_old_status || old_status <= status) &&
         "Invalid lattice transition");

  bool status_changed = !has_old_status || (old_status != status);
  if (status_changed) statuses_[inst] = status;

  return status_changed;
}

Pass::Status SSAPropagator::Simulate(Instruction* instr) {
  bool changed = false;


  if (!ShouldSimulateAgain(instr)) {
    return Pass::Status::SuccessWithoutChange;
  }

  BasicBlock* dest_bb = nullptr;
  PropStatus status = visit_fn_(instr, &dest_bb);
  if (status == SSAPropagator::kFailed) {
    return Pass::Status::Failure;
  }
  bool status_changed = SetStatus(instr, status);

  if (status == kVarying) {


    DontSimulateAgain(instr);
    if (status_changed) {
      AddSSAEdges(instr);
    }



    if (instr->IsBlockTerminator()) {
      BasicBlock* block = ctx_->get_instr_block(instr);
      for (const auto& e : bb_succs_.at(block)) {
        AddControlEdge(e);
      }
    }
    return Pass::Status::SuccessWithoutChange;
  } else if (status == kInteresting) {


    if (status_changed) {
      AddSSAEdges(instr);
    }



    if (dest_bb) {
      AddControlEdge(Edge(ctx_->get_instr_block(instr), dest_bb));
    }
    changed = true;
  }






  bool has_operands_to_simulate = false;
  if (instr->opcode() == spv::Op::OpPhi) {



    for (uint32_t i = 2; i < instr->NumOperands(); i += 2) {


      assert(i % 2 == 0 && i < instr->NumOperands() - 1 &&
             "malformed Phi arguments");

      uint32_t arg_id = instr->GetSingleWordOperand(i);
      Instruction* arg_def_instr = get_def_use_mgr()->GetDef(arg_id);
      if (!IsPhiArgExecutable(instr, i) || ShouldSimulateAgain(arg_def_instr)) {
        has_operands_to_simulate = true;
        break;
      }
    }
  } else {



    has_operands_to_simulate =
        !instr->WhileEachInId([this](const uint32_t* use) {
          Instruction* def_instr = get_def_use_mgr()->GetDef(*use);
          if (ShouldSimulateAgain(def_instr)) {
            return false;
          }
          return true;
        });
  }

  if (!has_operands_to_simulate) {
    DontSimulateAgain(instr);
  }

  return changed ? Pass::Status::SuccessWithChange
                 : Pass::Status::SuccessWithoutChange;
}

Pass::Status SSAPropagator::Simulate(BasicBlock* block) {
  if (block == ctx_->cfg()->pseudo_exit_block()) {
    return Pass::Status::SuccessWithoutChange;
  }





  bool changed = false;
  bool succeeded =
      block->WhileEachPhiInst([&changed, this](Instruction* instr) {
        auto Status = Simulate(instr);
        if (Status == Pass::Status::Failure) return false;
        changed |= Status == Pass::Status::SuccessWithChange;
        return true;
      });
  if (!succeeded) {
    return Pass::Status::Failure;
  }



  if (!BlockHasBeenSimulated(block)) {
    succeeded = block->WhileEachInst([&changed, this](Instruction* instr) {
      auto Status = Simulate(instr);
      if (Status == Pass::Status::Failure) return false;
      changed |= Status == Pass::Status::SuccessWithChange;
      return true;
    });
    if (!succeeded) {
      return Pass::Status::Failure;
    }

    MarkBlockSimulated(block);



    if (bb_succs_.at(block).size() == 1) {
      AddControlEdge(bb_succs_.at(block).at(0));
    }
  }

  return changed ? Pass::Status::SuccessWithChange
                 : Pass::Status::SuccessWithoutChange;
}

void SSAPropagator::Initialize(Function* fn) {



  bb_succs_[ctx_->cfg()->pseudo_entry_block()].push_back(
      Edge(ctx_->cfg()->pseudo_entry_block(), fn->entry().get()));

  for (auto& block : *fn) {
    const auto& const_block = block;
    const_block.ForEachSuccessorLabel([this, &block](const uint32_t label_id) {
      BasicBlock* succ_bb =
          ctx_->get_instr_block(get_def_use_mgr()->GetDef(label_id));
      bb_succs_[&block].push_back(Edge(&block, succ_bb));
      bb_preds_[succ_bb].push_back(Edge(succ_bb, &block));
    });
    if (block.IsReturnOrAbort()) {
      bb_succs_[&block].push_back(
          Edge(&block, ctx_->cfg()->pseudo_exit_block()));
      bb_preds_[ctx_->cfg()->pseudo_exit_block()].push_back(
          Edge(ctx_->cfg()->pseudo_exit_block(), &block));
    }
  }


  const auto& entry_succs = bb_succs_[ctx_->cfg()->pseudo_entry_block()];
  for (const auto& e : entry_succs) {
    AddControlEdge(e);
  }
}

bool SSAPropagator::Run(Function* fn) {
  Initialize(fn);

  bool changed = false;
  while (!blocks_.empty() || !ssa_edge_uses_.empty()) {


    if (!blocks_.empty()) {
      auto block = blocks_.front();
      Pass::Status status = Simulate(block);
      if (status == Pass::Status::Failure) {
        return false;
      }
      changed |= status == Pass::Status::SuccessWithChange;
      blocks_.pop();
      continue;
    }


    if (!ssa_edge_uses_.empty()) {
      Instruction* instr = ssa_edge_uses_.front();
      Pass::Status status = Simulate(instr);
      if (status == Pass::Status::Failure) {
        return changed;
      }
      changed |= status == Pass::Status::SuccessWithChange;
      ssa_edge_uses_.pop();
    }
  }

#ifndef NDEBUG


  fn->ForEachInst([this](Instruction* inst) {
    assert(
        (!HasStatus(inst) || Status(inst) != SSAPropagator::kNotInteresting) &&
        "Unsettled value");
  });
#endif

  return changed;
}

std::ostream& operator<<(std::ostream& str,
                         const SSAPropagator::PropStatus& status) {
  switch (status) {
    case SSAPropagator::kVarying:
      str << "Varying";
      break;
    case SSAPropagator::kInteresting:
      str << "Interesting";
      break;
    default:
      str << "Not interesting";
      break;
  }
  return str;
}

}
}
