// Copyright (c) 2025 Google LLC
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

#include "source/opt/resolve_binding_conflicts_pass.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "source/opt/decoration_manager.h"
#include "source/opt/def_use_manager.h"
#include "source/opt/instruction.h"
#include "source/opt/ir_builder.h"
#include "source/opt/ir_context.h"
#include "spirv/unified1/spirv.h"

namespace spvtools {
namespace opt {








struct VarBindingInfo {
  const Instruction* const var;
  const uint32_t descriptor_set;
  Instruction* const binding_decoration;


  uint32_t binding() const {
    return binding_decoration->GetSingleWordInOperand(2);
  }

  void updateBinding(uint32_t b) { binding_decoration->SetOperand(2, {b}); }
};


using BindingList = std::vector<VarBindingInfo*>;


using DescriptorSets = std::unordered_map<uint32_t, BindingList>;

IRContext::Analysis ResolveBindingConflictsPass::GetPreservedAnalyses() {


  return IRContext::kAnalysisDefUse | IRContext::kAnalysisInstrToBlockMapping |
         IRContext::kAnalysisDecorations | IRContext::kAnalysisCombinators |
         IRContext::kAnalysisCFG | IRContext::kAnalysisDominatorAnalysis |
         IRContext::kAnalysisLoopAnalysis | IRContext::kAnalysisNameMap |
         IRContext::kAnalysisScalarEvolution |
         IRContext::kAnalysisRegisterPressure |
         IRContext::kAnalysisValueNumberTable |
         IRContext::kAnalysisStructuredCFG | IRContext::kAnalysisBuiltinVarId |
         IRContext::kAnalysisIdToFuncMapping | IRContext::kAnalysisConstants |
         IRContext::kAnalysisTypes | IRContext::kAnalysisDebugInfo |
         IRContext::kAnalysisLiveness;
}







bool Less(const VarBindingInfo* const lhs, const VarBindingInfo* const rhs) {
  if (lhs->binding() < rhs->binding()) return true;
  if (lhs->binding() > rhs->binding()) return false;








  auto* def_use_mgr = lhs->var->context()->get_def_use_mgr();



  auto unwrap = [&def_use_mgr](Instruction* ty) {
    bool keep_going = true;
    do {
      switch (ty->opcode()) {
        case spv::Op::OpTypePointer:
          ty = def_use_mgr->GetDef(ty->GetSingleWordInOperand(1));
          break;
        case spv::Op::OpTypeArray:
        case spv::Op::OpTypeRuntimeArray:
          ty = def_use_mgr->GetDef(ty->GetSingleWordInOperand(0));
          break;
        default:
          keep_going = false;
          break;
      }
    } while (keep_going);
    return ty;
  };

  auto* lhs_ty = unwrap(def_use_mgr->GetDef(lhs->var->type_id()));
  auto* rhs_ty = unwrap(def_use_mgr->GetDef(rhs->var->type_id()));
  if (lhs_ty->opcode() == rhs_ty->opcode()) {

    return lhs->var->result_id() < rhs->var->result_id();
  }

  if (lhs_ty->opcode() == spv::Op::OpTypeSampler) {
    return false;
  }
  if (rhs_ty->opcode() == spv::Op::OpTypeSampler) {
    return true;
  }

  return lhs->var->result_id() < rhs->var->result_id();
}


class CallGraph {
 public:


  const std::vector<uint32_t>& CalleesBeforeCallers() const {
    return visit_order_;
  }

  const std::unordered_set<uint32_t>& Callees(uint32_t caller) {
    return calls_[caller];
  }

  CallGraph(IRContext& context) {

    std::queue<uint32_t> callee_queue;
    for (const auto& fn : *context.module()) {
      auto& callees = calls_[fn.result_id()];
      context.AddCalls(&fn, &callee_queue);
      while (!callee_queue.empty()) {
        callees.insert(callee_queue.front());
        callee_queue.pop();
      }
    }



    for (const auto& ep : context.module()->entry_points()) {
      Visit(ep.GetSingleWordInOperand(1));
    }
  }

 private:


  void Visit(uint32_t func_id) {
    if (visited_.count(func_id)) {
      return;
    }
    visited_.insert(func_id);
    for (auto callee_id : calls_[func_id]) {
      Visit(callee_id);
    }
    visit_order_.push_back(func_id);
  }


  std::unordered_map<uint32_t, std::unordered_set<uint32_t>> calls_;


  std::unordered_set<uint32_t> visited_;

  std::vector<uint32_t> visit_order_;
};


auto GetVarBindings(IRContext& context) {
  std::vector<VarBindingInfo> vars;
  auto* deco_mgr = context.get_decoration_mgr();
  for (auto& inst : context.module()->types_values()) {
    if (inst.opcode() == spv::Op::OpVariable) {
      Instruction* descriptor_set_deco = nullptr;
      Instruction* binding_deco = nullptr;
      for (auto* deco : deco_mgr->GetDecorationsFor(inst.result_id(), false)) {
        switch (static_cast<spv::Decoration>(deco->GetSingleWordInOperand(1))) {
          case spv::Decoration::DescriptorSet:
            assert(!descriptor_set_deco);
            descriptor_set_deco = deco;
            break;
          case spv::Decoration::Binding:
            assert(!binding_deco);
            binding_deco = deco;
            break;
          default:
            break;
        }
      }
      if (descriptor_set_deco && binding_deco) {
        vars.push_back({&inst, descriptor_set_deco->GetSingleWordInOperand(2),
                        binding_deco});
      }
    }
  }
  return vars;
}



void Merge(DescriptorSets& sink, const DescriptorSets& source) {
  for (auto index_and_bindings : source) {
    const uint32_t index = index_and_bindings.first;
    const BindingList& src1 = index_and_bindings.second;
    const BindingList& src2 = sink[index];
    BindingList merged;
    merged.resize(src1.size() + src2.size());
    auto merged_end = std::merge(src1.begin(), src1.end(), src2.begin(),
                                 src2.end(), merged.begin(), Less);
    auto unique_end = std::unique(merged.begin(), merged_end);
    merged.resize(unique_end - merged.begin());
    sink[index] = std::move(merged);
  }
}





bool ResolveConflicts(BindingList& bl) {
  bool changed = false;
  for (size_t i = 1; i < bl.size(); i++) {
    const auto prev_num = bl[i - 1]->binding();
    if (prev_num >= bl[i]->binding()) {
      bl[i]->updateBinding(prev_num + 1);
      changed = true;
    }
  }
  return changed;
}

Pass::Status ResolveBindingConflictsPass::Process() {




  auto vars = GetVarBindings(*context());




  std::unordered_map<uint32_t, DescriptorSets> used_vars;


  auto* def_use_mgr = context()->get_def_use_mgr();
  for (auto& var : vars) {
    std::unordered_set<uint32_t> visited_functions_for_var;
    def_use_mgr->ForEachUser(var.var, [&](Instruction* user) {
      if (auto* block = context()->get_instr_block(user)) {
        auto* fn = block->GetParent();
        assert(fn);
        const auto fn_id = fn->result_id();
        if (visited_functions_for_var.insert(fn_id).second) {
          used_vars[fn_id][var.descriptor_set].push_back(&var);
        }
      }
    });
  }


  for (auto& sets_for_fn : used_vars) {
    for (auto& ds : sets_for_fn.second) {
      BindingList& bindings = ds.second;
      std::stable_sort(bindings.begin(), bindings.end(), Less);
    }
  }


  CallGraph call_graph(*context());
  for (const uint32_t caller : call_graph.CalleesBeforeCallers()) {
    DescriptorSets& caller_ds = used_vars[caller];
    for (const uint32_t callee : call_graph.Callees(caller)) {
      Merge(caller_ds, used_vars[callee]);
    }
  }











  std::vector<BindingList*> ep_bindings;

  for (auto& ep : context()->module()->entry_points()) {
    for (auto& ds : used_vars[ep.GetSingleWordInOperand(1)]) {
      BindingList& bindings = ds.second;
      ep_bindings.push_back(&bindings);
    }
  }
  bool modified = false;
  bool found_conflict;
  do {
    found_conflict = false;
    for (BindingList* bl : ep_bindings) {
      found_conflict |= ResolveConflicts(*bl);
    }
    modified |= found_conflict;
  } while (found_conflict);

  return modified ? Pass::Status::SuccessWithChange
                  : Pass::Status::SuccessWithoutChange;
}

}
}
