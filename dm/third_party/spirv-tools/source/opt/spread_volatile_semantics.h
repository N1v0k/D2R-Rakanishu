// Copyright (c) 2022 Google LLC
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

#ifndef SOURCE_OPT_SPREAD_VOLATILE_SEMANTICS_H_
#define SOURCE_OPT_SPREAD_VOLATILE_SEMANTICS_H_

#include "source/opt/pass.h"

namespace spvtools {
namespace opt {


class SpreadVolatileSemantics : public Pass {
 public:
  SpreadVolatileSemantics() {}

  const char* name() const override { return "spread-volatile-semantics"; }

  Status Process() override;

  IRContext::Analysis GetPreservedAnalyses() override {
    return IRContext::kAnalysisDefUse | IRContext::kAnalysisDecorations |
           IRContext::kAnalysisInstrToBlockMapping;
  }

 private:


  bool HasNoExecutionModel() {
    return get_module()->entry_points().empty() &&
           context()->get_feature_mgr()->HasCapability(
               spv::Capability::Linkage);
  }



  Pass::Status SpreadVolatileSemanticsToVariables(
      const bool is_vk_memory_model_enabled);





  bool IsTargetForVolatileSemantics(uint32_t var_id,
                                    spv::ExecutionModel execution_model);




  void CollectTargetsForVolatileSemantics(
      const bool is_vk_memory_model_enabled);




  bool HasInterfaceInConflictOfVolatileSemantics();




  bool IsTargetUsedByNonVolatileLoadInEntryPoint(uint32_t var_id,
                                                 Instruction* entry_point);







  bool VisitLoadsOfPointersToVariableInEntries(
      uint32_t var_id, const std::function<bool(Instruction*)>& handle_load,
      const std::unordered_set<uint32_t>& function_ids);




  void SetVolatileForLoadsInEntries(
      Instruction* var, const std::unordered_set<uint32_t>& entry_function_ids);


  void DecorateVarWithVolatile(Instruction* var);



  std::unordered_set<uint32_t> EntryFunctionsToSpreadVolatileSemanticsForVar(
      uint32_t var_id) {
    auto itr = var_ids_to_entry_fn_for_volatile_semantics_.find(var_id);
    if (itr == var_ids_to_entry_fn_for_volatile_semantics_.end()) return {};
    return itr->second;
  }



  void MarkVolatileSemanticsForVariable(uint32_t var_id,
                                        Instruction* entry_point);



  std::unordered_map<uint32_t, std::unordered_set<uint32_t>>
      var_ids_to_entry_fn_for_volatile_semantics_;
};

}
}

#endif
