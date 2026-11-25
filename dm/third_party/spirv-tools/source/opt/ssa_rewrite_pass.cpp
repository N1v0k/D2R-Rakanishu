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




























#include "source/opt/ssa_rewrite_pass.h"

#include <memory>
#include <sstream>

#include "source/opcode.h"
#include "source/opt/cfg.h"
#include "source/opt/mem_pass.h"
#include "source/opt/types.h"






#ifdef SSA_REWRITE_DEBUGGING_LEVEL
#include <ostream>
#else
#define SSA_REWRITE_DEBUGGING_LEVEL 0
#endif

namespace spvtools {
namespace opt {
namespace {
constexpr uint32_t kStoreValIdInIdx = 1;
constexpr uint32_t kVariableInitIdInIdx = 1;
}

std::string SSARewriter::PhiCandidate::PrettyPrint(const CFG* cfg) const {
  std::ostringstream str;
  str << "%" << result_id_ << " = Phi[%" << var_id_ << ", BB %" << bb_->id()
      << "](";
  if (phi_args_.size() > 0) {
    uint32_t arg_ix = 0;
    for (uint32_t pred_label : cfg->preds(bb_->id())) {
      uint32_t arg_id = phi_args_[arg_ix++];
      str << "[%" << arg_id << ", bb(%" << pred_label << ")] ";
    }
  }
  str << ")";
  if (copy_of_ != 0) {
    str << "  [COPY OF " << copy_of_ << "]";
  }
  str << ((is_complete_) ? "  [COMPLETE]" : "  [INCOMPLETE]");

  return str.str();
}

SSARewriter::PhiCandidate* SSARewriter::CreatePhiCandidate(uint32_t var_id,
                                                           BasicBlock* bb) {
  uint32_t phi_result_id = pass_->context()->TakeNextId();
  if (phi_result_id == 0) {
    return nullptr;
  }
  auto result = phi_candidates_.emplace(
      phi_result_id, PhiCandidate(var_id, phi_result_id, bb));
  PhiCandidate* phi_candidate = &result.first->second;
  return phi_candidate;
}

void SSARewriter::ReplacePhiUsersWith(const PhiCandidate& phi_to_remove,
                                      uint32_t repl_id) {
  for (uint32_t user_id : phi_to_remove.users()) {
    PhiCandidate* user_phi = GetPhiCandidate(user_id);
    BasicBlock* bb = pass_->context()->get_instr_block(user_id);
    if (user_phi) {


      for (uint32_t& arg : user_phi->phi_args()) {
        if (arg == phi_to_remove.result_id()) {
          arg = repl_id;
        }
      }
    } else if (bb->id() == user_id) {


      WriteVariable(phi_to_remove.var_id(), bb, repl_id);
    } else {


      for (auto& it : load_replacement_) {
        if (it.second == phi_to_remove.result_id()) {
          it.second = repl_id;
        }
      }
    }
  }
}

uint32_t SSARewriter::TryRemoveTrivialPhi(PhiCandidate* phi_candidate) {
  uint32_t same_id = 0;
  for (uint32_t arg_id : phi_candidate->phi_args()) {
    if (arg_id == same_id || arg_id == phi_candidate->result_id()) {

      continue;
    }
    if (same_id != 0) {


      assert(phi_candidate->copy_of() == 0 &&
             "Phi candidate transitioning from copy to non-copy.");
      return phi_candidate->result_id();
    }
    same_id = arg_id;
  }









  phi_candidate->MarkCopyOf(same_id);

  assert(same_id != 0 && "Completed Phis cannot have %0 in their arguments");



  ReplacePhiUsersWith(*phi_candidate, same_id);

  return same_id;
}

uint32_t SSARewriter::AddPhiOperands(PhiCandidate* phi_candidate) {
  assert(phi_candidate->phi_args().size() == 0 &&
         "Phi candidate already has arguments");

  bool found_0_arg = false;
  for (uint32_t pred : pass_->cfg()->preds(phi_candidate->bb()->id())) {
    BasicBlock* pred_bb = pass_->cfg()->block(pred);


































    uint32_t arg_id = IsBlockSealed(pred_bb)
                          ? GetReachingDef(phi_candidate->var_id(), pred_bb)
                          : 0;
    phi_candidate->phi_args().push_back(arg_id);

    if (arg_id == 0) {
      found_0_arg = true;
    } else {


      PhiCandidate* defining_phi = GetPhiCandidate(arg_id);
      if (defining_phi && defining_phi != phi_candidate) {
        defining_phi->AddUser(phi_candidate->result_id());
      }
    }
  }



  if (found_0_arg) {
    phi_candidate->MarkIncomplete();
    incomplete_phis_.push(phi_candidate);
    return phi_candidate->result_id();
  }


  uint32_t repl_id = TryRemoveTrivialPhi(phi_candidate);
  if (repl_id == phi_candidate->result_id()) {


    phi_candidate->MarkComplete();
    phis_to_generate_.push_back(phi_candidate);
  }

  return repl_id;
}

uint32_t SSARewriter::GetValueAtBlock(uint32_t var_id, BasicBlock* bb) {
  assert(bb != nullptr);
  const auto& bb_it = defs_at_block_.find(bb);
  if (bb_it != defs_at_block_.end()) {
    const auto& current_defs = bb_it->second;
    const auto& var_it = current_defs.find(var_id);
    if (var_it != current_defs.end()) {
      return var_it->second;
    }
  }
  return 0;
}

uint32_t SSARewriter::GetReachingDef(uint32_t var_id, BasicBlock* bb) {

  uint32_t val_id = GetValueAtBlock(var_id, bb);
  if (val_id != 0) return val_id;


  auto& predecessors = pass_->cfg()->preds(bb->id());
  if (predecessors.size() == 1) {


    val_id = GetReachingDef(var_id, pass_->cfg()->block(predecessors[0]));
  } else if (predecessors.size() > 1) {



    PhiCandidate* phi_candidate = CreatePhiCandidate(var_id, bb);
    if (phi_candidate == nullptr) return 0;


    WriteVariable(var_id, bb, phi_candidate->result_id());
    val_id = AddPhiOperands(phi_candidate);
  }



  if (val_id == 0) {
    val_id = pass_->GetUndefVal(var_id);
    if (val_id == 0) {
      return 0;
    }
  }

  WriteVariable(var_id, bb, val_id);

  return val_id;
}

void SSARewriter::SealBlock(BasicBlock* bb) {
  auto result = sealed_blocks_.insert(bb);
  (void)result;
  assert(result.second == true &&
         "Tried to seal the same basic block more than once.");
}

void SSARewriter::ProcessStore(Instruction* inst, BasicBlock* bb) {
  auto opcode = inst->opcode();
  assert((opcode == spv::Op::OpStore || opcode == spv::Op::OpVariable) &&
         "Expecting a store or a variable definition instruction.");

  uint32_t var_id = 0;
  uint32_t val_id = 0;
  if (opcode == spv::Op::OpStore) {
    (void)pass_->GetPtr(inst, &var_id);
    val_id = inst->GetSingleWordInOperand(kStoreValIdInIdx);
  } else if (inst->NumInOperands() >= 2) {
    var_id = inst->result_id();
    val_id = inst->GetSingleWordInOperand(kVariableInitIdInIdx);
  }
  if (pass_->IsTargetVar(var_id)) {
    WriteVariable(var_id, bb, val_id);
    pass_->context()->get_debug_info_mgr()->AddDebugValueForVariable(
        inst, var_id, val_id, inst);

#if SSA_REWRITE_DEBUGGING_LEVEL > 1
    std::cerr << "\tFound store '%" << var_id << " = %" << val_id << "': "
              << inst->PrettyPrint(SPV_BINARY_TO_TEXT_OPTION_FRIENDLY_NAMES)
              << "\n";
#endif
  }
}

bool SSARewriter::ProcessLoad(Instruction* inst, BasicBlock* bb) {

  uint32_t var_id = 0;
  (void)pass_->GetPtr(inst, &var_id);



































  analysis::DefUseManager* def_use_mgr = pass_->context()->get_def_use_mgr();
  analysis::TypeManager* type_mgr = pass_->context()->get_type_mgr();
  analysis::Type* load_type = type_mgr->GetType(inst->type_id());
  uint32_t val_id = 0;
  bool found_reaching_def = false;
  while (!found_reaching_def) {
    if (!pass_->IsTargetVar(var_id)) {


      return true;
    }

    val_id = GetReachingDef(var_id, bb);
    if (val_id == 0) {
      return false;
    }








    Instruction* reaching_def_inst = def_use_mgr->GetDef(val_id);
    if (reaching_def_inst &&
        !type_mgr->GetType(reaching_def_inst->type_id())->IsSame(load_type)) {
      var_id = val_id;
    } else {
      found_reaching_def = true;
    }
  }




  uint32_t load_id = inst->result_id();
  assert(load_replacement_.count(load_id) == 0);
  load_replacement_[load_id] = val_id;
  PhiCandidate* defining_phi = GetPhiCandidate(val_id);
  if (defining_phi) {
    defining_phi->AddUser(load_id);
  }

#if SSA_REWRITE_DEBUGGING_LEVEL > 1
  std::cerr << "\tFound load: "
            << inst->PrettyPrint(SPV_BINARY_TO_TEXT_OPTION_FRIENDLY_NAMES)
            << " (replacement for %" << load_id << " is %" << val_id << ")\n";
#endif

  return true;
}

void SSARewriter::PrintPhiCandidates() const {
  std::cerr << "\nPhi candidates:\n";
  for (const auto& phi_it : phi_candidates_) {
    std::cerr << "\tBB %" << phi_it.second.bb()->id() << ": "
              << phi_it.second.PrettyPrint(pass_->cfg()) << "\n";
  }
  std::cerr << "\n";
}

void SSARewriter::PrintReplacementTable() const {
  std::cerr << "\nLoad replacement table\n";
  for (const auto& it : load_replacement_) {
    std::cerr << "\t%" << it.first << " -> %" << it.second << "\n";
  }
  std::cerr << "\n";
}

bool SSARewriter::GenerateSSAReplacements(BasicBlock* bb) {
#if SSA_REWRITE_DEBUGGING_LEVEL > 1
  std::cerr << "Generating SSA replacements for block: " << bb->id() << "\n";
  std::cerr << bb->PrettyPrint(SPV_BINARY_TO_TEXT_OPTION_FRIENDLY_NAMES)
            << "\n";
#endif

  for (auto& inst : *bb) {
    auto opcode = inst.opcode();
    if (opcode == spv::Op::OpStore || opcode == spv::Op::OpVariable) {
      ProcessStore(&inst, bb);
    } else if (inst.opcode() == spv::Op::OpLoad) {
      if (!ProcessLoad(&inst, bb)) {
        return false;
      }
    }
  }



  SealBlock(bb);

#if SSA_REWRITE_DEBUGGING_LEVEL > 1
  PrintPhiCandidates();
  PrintReplacementTable();
  std::cerr << "\n\n";
#endif
  return true;
}

uint32_t SSARewriter::GetReplacement(std::pair<uint32_t, uint32_t> repl) {
  uint32_t val_id = repl.second;
  auto it = load_replacement_.find(val_id);
  while (it != load_replacement_.end()) {
    val_id = it->second;
    it = load_replacement_.find(val_id);
  }
  return val_id;
}

uint32_t SSARewriter::GetPhiArgument(const PhiCandidate* phi_candidate,
                                     uint32_t ix) {
  assert(phi_candidate->IsReady() &&
         "Tried to get the final argument from an incomplete/trivial Phi");

  uint32_t arg_id = phi_candidate->phi_args()[ix];
  while (arg_id != 0) {
    PhiCandidate* phi_user = GetPhiCandidate(arg_id);
    if (phi_user == nullptr || phi_user->IsReady()) {


      return arg_id;
    }
    arg_id = phi_user->copy_of();
  }

  assert(false &&
         "No Phi candidates in the copy-of chain are ready to be generated");

  return 0;
}

bool SSARewriter::ApplyReplacements() {
  bool modified = false;

#if SSA_REWRITE_DEBUGGING_LEVEL > 2
  std::cerr << "\n\nApplying replacement decisions to IR\n\n";
  PrintPhiCandidates();
  PrintReplacementTable();
  std::cerr << "\n\n";
#endif


  std::vector<Instruction*> generated_phis;
  for (const PhiCandidate* phi_candidate : phis_to_generate_) {
#if SSA_REWRITE_DEBUGGING_LEVEL > 2
    std::cerr << "Phi candidate: " << phi_candidate->PrettyPrint(pass_->cfg())
              << "\n";
#endif

    assert(phi_candidate->is_complete() &&
           "Tried to instantiate a Phi instruction from an incomplete Phi "
           "candidate");

    auto* local_var = pass_->get_def_use_mgr()->GetDef(phi_candidate->var_id());


    uint32_t type_id = pass_->GetPointeeTypeId(local_var);
    std::vector<Operand> phi_operands;
    uint32_t arg_ix = 0;
    std::unordered_map<uint32_t, uint32_t> already_seen;
    for (uint32_t pred_label : pass_->cfg()->preds(phi_candidate->bb()->id())) {
      uint32_t op_val_id = GetPhiArgument(phi_candidate, arg_ix++);
      if (already_seen.count(pred_label) == 0) {
        phi_operands.push_back(
            {spv_operand_type_t::SPV_OPERAND_TYPE_ID, {op_val_id}});
        phi_operands.push_back(
            {spv_operand_type_t::SPV_OPERAND_TYPE_ID, {pred_label}});
        already_seen[pred_label] = op_val_id;
      } else {



        assert(already_seen[pred_label] == op_val_id &&
               "Inconsistent value for duplicate edges.");
      }
    }



    std::unique_ptr<Instruction> phi_inst(
        new Instruction(pass_->context(), spv::Op::OpPhi, type_id,
                        phi_candidate->result_id(), phi_operands));
    generated_phis.push_back(phi_inst.get());
    pass_->get_def_use_mgr()->AnalyzeInstDef(&*phi_inst);
    pass_->context()->set_instr_block(&*phi_inst, phi_candidate->bb());
    auto insert_it = phi_candidate->bb()->begin();
    insert_it = insert_it.InsertBefore(std::move(phi_inst));
    pass_->context()->get_decoration_mgr()->CloneDecorations(
        phi_candidate->var_id(), phi_candidate->result_id(),
        {spv::Decoration::RelaxedPrecision});


    insert_it->SetDebugScope(local_var->GetDebugScope());
    pass_->context()->get_debug_info_mgr()->AddDebugValueForVariable(
        &*insert_it, phi_candidate->var_id(), phi_candidate->result_id(),
        &*insert_it);

    modified = true;
  }




  for (Instruction* phi_inst : generated_phis) {
    pass_->get_def_use_mgr()->AnalyzeInstUse(&*phi_inst);
  }

#if SSA_REWRITE_DEBUGGING_LEVEL > 1
  std::cerr << "\n\nReplacing the result of load instructions with the "
               "corresponding SSA id\n\n";
#endif


  for (auto& repl : load_replacement_) {
    uint32_t load_id = repl.first;
    uint32_t val_id = GetReplacement(repl);
    Instruction* load_inst =
        pass_->context()->get_def_use_mgr()->GetDef(load_id);

#if SSA_REWRITE_DEBUGGING_LEVEL > 2
    std::cerr << "\t"
              << load_inst->PrettyPrint(
                     SPV_BINARY_TO_TEXT_OPTION_FRIENDLY_NAMES)
              << "  (%" << load_id << " -> %" << val_id << ")\n";
#endif





    pass_->context()->KillNamesAndDecorates(load_id);
    pass_->context()->ReplaceAllUsesWith(load_id, val_id);
    pass_->context()->KillInst(load_inst);
    modified = true;
  }

  return modified;
}

void SSARewriter::FinalizePhiCandidate(PhiCandidate* phi_candidate) {
  assert(phi_candidate->phi_args().size() > 0 &&
         "Phi candidate should have arguments");

  uint32_t ix = 0;
  for (uint32_t pred : pass_->cfg()->preds(phi_candidate->bb()->id())) {
    BasicBlock* pred_bb = pass_->cfg()->block(pred);
    uint32_t& arg_id = phi_candidate->phi_args()[ix++];
    if (arg_id == 0) {


      arg_id = IsBlockSealed(pred_bb)
                   ? GetReachingDef(phi_candidate->var_id(), pred_bb)
                   : pass_->GetUndefVal(phi_candidate->var_id());
    }
  }


  phi_candidate->MarkComplete();



  if (TryRemoveTrivialPhi(phi_candidate) == phi_candidate->result_id()) {


    assert(!phi_candidate->copy_of() && "A completed Phi cannot be trivial.");
    phis_to_generate_.push_back(phi_candidate);
  }
}

void SSARewriter::FinalizePhiCandidates() {
#if SSA_REWRITE_DEBUGGING_LEVEL > 1
  std::cerr << "Finalizing Phi candidates:\n\n";
  PrintPhiCandidates();
  std::cerr << "\n";
#endif


  while (incomplete_phis_.size() > 0) {
    PhiCandidate* phi_candidate = incomplete_phis_.front();
    incomplete_phis_.pop();
    FinalizePhiCandidate(phi_candidate);
  }
}

Pass::Status SSARewriter::RewriteFunctionIntoSSA(Function* fp) {
#if SSA_REWRITE_DEBUGGING_LEVEL > 0
  std::cerr << "Function before SSA rewrite:\n"
            << fp->PrettyPrint(0) << "\n\n\n";
#endif


  pass_->CollectTargetVars(fp);



  bool succeeded = pass_->cfg()->WhileEachBlockInReversePostOrder(
      fp->entry().get(), [this](BasicBlock* bb) {
        if (!GenerateSSAReplacements(bb)) {
          return false;
        }
        return true;
      });

  if (!succeeded) {
    return Pass::Status::Failure;
  }


  FinalizePhiCandidates();


  bool modified = ApplyReplacements();

#if SSA_REWRITE_DEBUGGING_LEVEL > 0
  std::cerr << "\n\n\nFunction after SSA rewrite:\n"
            << fp->PrettyPrint(0) << "\n";
#endif

  return modified ? Pass::Status::SuccessWithChange
                  : Pass::Status::SuccessWithoutChange;
}

Pass::Status SSARewritePass::Process() {
  Status status = Status::SuccessWithoutChange;
  for (auto& fn : *get_module()) {
    if (fn.IsDeclaration()) {
      continue;
    }
    status =
        CombineStatus(status, SSARewriter(this).RewriteFunctionIntoSSA(&fn));

    for (auto var_id : seen_target_vars_) {
      context()->get_debug_info_mgr()->KillDebugDeclares(var_id);
    }
    if (status == Status::Failure) {
      break;
    }
  }
  return status;
}

}
}
