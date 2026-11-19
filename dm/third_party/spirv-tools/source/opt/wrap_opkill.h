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

#ifndef SOURCE_OPT_WRAP_OPKILL_H_
#define SOURCE_OPT_WRAP_OPKILL_H_

#include "source/opt/pass.h"

namespace spvtools {
namespace opt {


class WrapOpKill : public Pass {
 public:
  WrapOpKill() : void_type_id_(0) {}

  const char* name() const override { return "wrap-opkill"; }

  Status Process() override;

  IRContext::Analysis GetPreservedAnalyses() override {
    return IRContext::kAnalysisDefUse |
           IRContext::kAnalysisInstrToBlockMapping |
           IRContext::kAnalysisDecorations | IRContext::kAnalysisCombinators |
           IRContext::kAnalysisNameMap | IRContext::kAnalysisBuiltinVarId |
           IRContext::kAnalysisConstants | IRContext::kAnalysisTypes;
  }

 private:




  bool ReplaceWithFunctionCall(Instruction* inst);


  uint32_t GetVoidTypeId();


  uint32_t GetVoidFunctionTypeId();




  uint32_t GetKillingFuncId(spv::Op opcode);



  uint32_t GetOwningFunctionsReturnType(Instruction* inst);



  uint32_t void_type_id_;




  std::unique_ptr<Function> opkill_function_;




  std::unique_ptr<Function> opterminateinvocation_function_;
};

}
}

#endif
