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

#ifndef SOURCE_OPT_PRIVATE_TO_LOCAL_PASS_H_
#define SOURCE_OPT_PRIVATE_TO_LOCAL_PASS_H_

#include "source/opt/ir_context.h"
#include "source/opt/pass.h"

namespace spvtools {
namespace opt {





class PrivateToLocalPass : public Pass {
 public:
  const char* name() const override { return "private-to-local"; }
  Status Process() override;

  IRContext::Analysis GetPreservedAnalyses() override {
    return IRContext::kAnalysisDefUse |
           IRContext::kAnalysisInstrToBlockMapping |
           IRContext::kAnalysisDecorations | IRContext::kAnalysisCombinators |
           IRContext::kAnalysisCFG | IRContext::kAnalysisDominatorAnalysis |
           IRContext::kAnalysisNameMap | IRContext::kAnalysisConstants |
           IRContext::kAnalysisTypes;
  }

 private:


  bool MoveVariable(Instruction* variable, Function* function);





  Function* FindLocalFunction(const Instruction& inst) const;




  bool IsValidUse(const Instruction* inst, uint32_t private_variable_id) const;





  uint32_t GetNewType(uint32_t old_type_id);



  bool UpdateUse(Instruction* inst, Instruction* user);
  bool UpdateUses(Instruction* inst);
};

}
}

#endif
