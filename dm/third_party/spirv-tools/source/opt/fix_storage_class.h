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

#ifndef SOURCE_OPT_FIX_STORAGE_CLASS_H_
#define SOURCE_OPT_FIX_STORAGE_CLASS_H_

#include <unordered_map>

#include "source/opt/ir_context.h"
#include "source/opt/module.h"
#include "source/opt/pass.h"

namespace spvtools {
namespace opt {





class FixStorageClass : public Pass {
 public:
  const char* name() const override { return "fix-storage-class"; }
  Status Process() override;


  IRContext::Analysis GetPreservedAnalyses() override {
    return IRContext::kAnalysisDefUse |
           IRContext::kAnalysisInstrToBlockMapping |
           IRContext::kAnalysisCombinators | IRContext::kAnalysisCFG |
           IRContext::kAnalysisDominatorAnalysis |
           IRContext::kAnalysisLoopAnalysis | IRContext::kAnalysisNameMap |
           IRContext::kAnalysisConstants | IRContext::kAnalysisTypes;
  }

 private:




  bool PropagateStorageClass(Instruction* inst, spv::StorageClass storage_class,
                             std::set<uint32_t>* seen);







  void FixInstructionStorageClass(Instruction* inst,
                                  spv::StorageClass storage_class,
                                  std::set<uint32_t>* seen);



  void ChangeResultStorageClass(Instruction* inst,
                                spv::StorageClass storage_class) const;


  bool IsPointerResultType(Instruction* inst);



  bool IsPointerToStorageClass(Instruction* inst,
                               spv::StorageClass storage_class);



  bool PropagateType(Instruction* inst, uint32_t type_id, uint32_t op_idx,
                     std::set<uint32_t>* seen);


  bool ChangeResultType(Instruction* inst, uint32_t new_type_id);



  uint32_t WalkAccessChainType(Instruction* inst, uint32_t id);
};

}
}

#endif
