// Copyright (c) 2022 Advanced Micro Devices, Inc.
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

#ifndef _VAR_FUNC_CALL_PASS_H
#define _VAR_FUNC_CALL_PASS_H

#include "source/opt/pass.h"

namespace spvtools {
namespace opt {
class FixFuncCallArgumentsPass : public Pass {
 public:
  FixFuncCallArgumentsPass() {}
  const char* name() const override { return "fix-for-funcall-param"; }
  Status Process() override;

  bool ModuleHasASingleFunction();





  uint32_t ReplaceAccessChainFuncCallArguments(Instruction* func_call_inst,
                                               Instruction* operand_inst);


  bool FixFuncCallArguments(Instruction* func_call_inst);

  IRContext::Analysis GetPreservedAnalyses() override {
    return IRContext::kAnalysisTypes;
  }
};
}
}

#endif