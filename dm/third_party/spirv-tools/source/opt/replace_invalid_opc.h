// Copyright (c) 2018 Google LLC
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

#ifndef SOURCE_OPT_REPLACE_INVALID_OPC_H_
#define SOURCE_OPT_REPLACE_INVALID_OPC_H_

#include <string>

#include "source/opt/pass.h"

namespace spvtools {
namespace opt {





class ReplaceInvalidOpcodePass : public Pass {
 public:
  const char* name() const override { return "replace-invalid-opcode"; }
  Status Process() override;

 private:



  spv::ExecutionModel GetExecutionModel();




  bool RewriteFunction(Function* function, spv::ExecutionModel mode);


  bool IsFragmentShaderOnlyInstruction(Instruction* inst);





  void ReplaceInstruction(Instruction* inst, const char* source,
                          uint32_t line_number, uint32_t column_number);






  uint32_t GetSpecialConstant(uint32_t type_id);
  std::string BuildWarningMessage(spv::Op opcode);
};

}
}

#endif
