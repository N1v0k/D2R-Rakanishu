// Copyright (c) 2019 Google LLC
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

#ifndef SOURCE_OPT_DESC_SROA_H_
#define SOURCE_OPT_DESC_SROA_H_

#include <cstdio>
#include <memory>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "source/opt/function.h"
#include "source/opt/pass.h"
#include "source/opt/type_manager.h"

namespace spvtools {
namespace opt {


class DescriptorScalarReplacement : public Pass {
 public:
  DescriptorScalarReplacement(bool flatten_composites, bool flatten_arrays)
      : flatten_composites_(flatten_composites),
        flatten_arrays_(flatten_arrays) {}

  const char* name() const override {
    if (flatten_composites_ && flatten_arrays_)
      return "descriptor-scalar-replacement";
    if (flatten_composites_) return "descriptor-compososite-scalar-replacement";
    return "descriptor-array-scalar-replacement";
  }

  Status Process() override;

  IRContext::Analysis GetPreservedAnalyses() override {
    return IRContext::kAnalysisDefUse |
           IRContext::kAnalysisInstrToBlockMapping |
           IRContext::kAnalysisCombinators | IRContext::kAnalysisCFG |
           IRContext::kAnalysisConstants | IRContext::kAnalysisTypes;
  }

 private:



  bool ReplaceCandidate(Instruction* var);





  bool ReplaceAccessChain(Instruction* var, Instruction* use);






  bool ReplaceLoadedValue(Instruction* var, Instruction* value);




  bool ReplaceEntryPoint(Instruction* var, Instruction* use);






  bool ReplaceCompositeExtract(Instruction* var, Instruction* extract);




  uint32_t GetReplacementVariable(Instruction* var, uint32_t idx);



  uint32_t CreateReplacementVariable(Instruction* var, uint32_t idx);








  uint32_t GetNumBindingsUsedByType(uint32_t type_id);








  void CopyDecorationsForNewVariable(Instruction* old_var, uint32_t index,
                                     uint32_t new_var_id,
                                     uint32_t new_var_ptr_type_id,
                                     const bool is_old_var_array,
                                     const bool is_old_var_struct,
                                     Instruction* old_var_type);







  uint32_t GetNewBindingForElement(uint32_t old_binding, uint32_t index,
                                   uint32_t ptr_elem_type_id,
                                   const bool is_old_var_array,
                                   const bool is_old_var_struct,
                                   Instruction* old_var_type);






  void CreateNewDecorationForNewVariable(Instruction* old_decoration,
                                         uint32_t new_var_id,
                                         uint32_t new_binding);




  void CreateNewDecorationForMemberDecorate(Instruction* old_decoration,
                                            uint32_t new_var_id);






  std::map<Instruction*, std::vector<uint32_t>> replacement_variables_;

  bool flatten_composites_;
  bool flatten_arrays_;
};

}
}

#endif
