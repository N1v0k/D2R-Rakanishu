// Copyright (c) 2020 Google LLC
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

#ifndef SOURCE_OPT_DEBUG_INFO_MANAGER_H_
#define SOURCE_OPT_DEBUG_INFO_MANAGER_H_

#include <set>
#include <unordered_map>
#include <unordered_set>

#include "source/opt/instruction.h"
#include "source/opt/module.h"

namespace spvtools {
namespace opt {
namespace analysis {







class DebugInlinedAtContext {
 public:
  explicit DebugInlinedAtContext(Instruction* call_inst)
      : call_inst_line_(call_inst->dbg_line_inst()),
        call_inst_scope_(call_inst->GetDebugScope()) {}

  const Instruction* GetLineOfCallInstruction() { return call_inst_line_; }
  const DebugScope& GetScopeOfCallInstruction() { return call_inst_scope_; }



  void SetDebugInlinedAtChain(uint32_t callee_instr_inlined_at,
                              uint32_t chain_head_id) {
    callee_inlined_at2chain_[callee_instr_inlined_at] = chain_head_id;
  }

  uint32_t GetDebugInlinedAtChain(uint32_t callee_instr_inlined_at) {
    auto chain_itr = callee_inlined_at2chain_.find(callee_instr_inlined_at);
    if (chain_itr != callee_inlined_at2chain_.end()) return chain_itr->second;
    return kNoInlinedAt;
  }

 private:


  const Instruction* call_inst_line_;



  const DebugScope call_inst_scope_;



  std::unordered_map<uint32_t, uint32_t> callee_inlined_at2chain_;
};



class DebugInfoManager {
 public:

  DebugInfoManager(IRContext* context);

  DebugInfoManager(const DebugInfoManager&) = delete;
  DebugInfoManager(DebugInfoManager&&) = delete;
  DebugInfoManager& operator=(const DebugInfoManager&) = delete;
  DebugInfoManager& operator=(DebugInfoManager&&) = delete;

  friend bool operator==(const DebugInfoManager&, const DebugInfoManager&);
  friend bool operator!=(const DebugInfoManager& lhs,
                         const DebugInfoManager& rhs) {
    return !(lhs == rhs);
  }


  void AnalyzeDebugInst(Instruction* dbg_inst);





  uint32_t CreateDebugInlinedAt(const Instruction* line,
                                const DebugScope& scope);



  Instruction* DerefDebugExpression(Instruction* dbg_expr);


  Instruction* GetDebugInfoNone();



  Instruction* GetDebugInlinedAt(uint32_t dbg_inlined_at_id);



  Instruction* GetDebugFunction(uint32_t fn_id) {
    auto dbg_fn_it = fn_id_to_dbg_fn_.find(fn_id);
    return dbg_fn_it == fn_id_to_dbg_fn_.end() ? nullptr : dbg_fn_it->second;
  }






  Instruction* CloneDebugInlinedAt(uint32_t clone_inlined_at_id,
                                   Instruction* insert_before = nullptr);




  DebugScope BuildDebugScope(const DebugScope& callee_instr_scope,
                             DebugInlinedAtContext* inlined_at_ctx);




  uint32_t BuildDebugInlinedAtChain(uint32_t callee_inlined_at,
                                    DebugInlinedAtContext* inlined_at_ctx);



  bool IsVariableDebugDeclared(uint32_t variable_id);



  bool KillDebugDeclares(uint32_t variable_id);





  bool AddDebugValueForVariable(Instruction* line, uint32_t variable_id,
                                uint32_t value_id, Instruction* insert_pos);







  Instruction* AddDebugValueForDecl(Instruction* dbg_decl, uint32_t value_id,
                                    Instruction* insert_before,
                                    Instruction* line);


  void ClearDebugInfo(Instruction* instr);


  uint32_t GetVulkanDebugOperation(Instruction* inst);




  uint32_t GetVariableIdOfDebugValueUsedForDeclare(Instruction* inst);



  void ConvertDebugGlobalToLocalVariable(Instruction* dbg_global_var,
                                         Instruction* local_var);


  bool IsDebugDeclare(Instruction* instr);



  void ReplaceAllUsesInDebugScopeWithPredicate(
      uint32_t before, uint32_t after,
      const std::function<bool(Instruction*)>& predicate);



  void ClearDebugScopeAndInlinedAtUses(Instruction* inst);

 private:
  IRContext* context() { return context_; }



  void AnalyzeDebugInsts(Module& module);


  uint32_t GetDbgSetImportId();



  Instruction* GetDbgInst(uint32_t id);


  Instruction* GetDebugOperationWithDeref();



  void RegisterDbgInst(Instruction* inst);



  void RegisterDbgFunction(Instruction* inst);




  void RegisterDbgDeclare(uint32_t var_id, Instruction* dbg_declare);


  Instruction* GetEmptyDebugExpression();



  bool IsAncestorOfScope(uint32_t scope, uint32_t ancestor);



  bool IsDeclareVisibleToInstr(Instruction* dbg_declare, Instruction* scope);


  uint32_t GetParentScope(uint32_t child_scope);

  IRContext* context_;



  std::unordered_map<uint32_t, Instruction*> id_to_dbg_inst_;



  std::unordered_map<uint32_t, Instruction*> fn_id_to_dbg_fn_;



  typedef Instruction* InstPtr;
  struct InstPtrLess {
    bool operator()(const InstPtr& lhs, const InstPtr& rhs) const {
      return lhs->unique_id() < rhs->unique_id();
    }
  };



  std::unordered_map<uint32_t, std::set<InstPtr, InstPtrLess>>
      var_id_to_dbg_decl_;


  std::unordered_map<uint32_t, std::unordered_set<Instruction*>>
      scope_id_to_users_;


  std::unordered_map<uint32_t, std::unordered_set<Instruction*>>
      inlinedat_id_to_users_;


  Instruction* deref_operation_;



  Instruction* debug_info_none_inst_;




  Instruction* empty_debug_expr_inst_;
};

}
}
}

#endif
