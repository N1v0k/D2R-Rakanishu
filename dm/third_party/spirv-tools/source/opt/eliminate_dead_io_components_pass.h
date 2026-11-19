// Copyright (c) 2022 The Khronos Group Inc.
// Copyright (c) 2022 LunarG Inc.
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

#ifndef SOURCE_OPT_ELIMINATE_DEAD_INPUT_COMPONENTS_H_
#define SOURCE_OPT_ELIMINATE_DEAD_INPUT_COMPONENTS_H_

#include <unordered_map>

#include "source/opt/ir_context.h"
#include "source/opt/module.h"
#include "source/opt/pass.h"

namespace spvtools {
namespace opt {


class EliminateDeadIOComponentsPass : public Pass {
 public:
  explicit EliminateDeadIOComponentsPass(spv::StorageClass elim_sclass,
                                         bool safe_mode = true)
      : elim_sclass_(elim_sclass), safe_mode_(safe_mode) {}

  const char* name() const override {
    return "eliminate-dead-input-components";
  }
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



  unsigned FindMaxIndex(const Instruction& var, const unsigned original_max,
                        const bool skip_first_index = false);


  void ChangeArrayLength(Instruction& inst, unsigned length);



  void ChangeIOVarStructLength(Instruction& io_var, unsigned length);


  spv::StorageClass elim_sclass_;



  bool safe_mode_;
};

}
}

#endif
