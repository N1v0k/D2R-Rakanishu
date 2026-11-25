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

#ifndef SOURCE_OPT_GRAPHICS_ROBUST_ACCESS_PASS_H_
#define SOURCE_OPT_GRAPHICS_ROBUST_ACCESS_PASS_H_

#include <map>
#include <unordered_map>

#include "constants.h"
#include "def_use_manager.h"
#include "instruction.h"
#include "module.h"
#include "pass.h"
#include "source/diagnostic.h"
#include "type_manager.h"

namespace spvtools {
namespace opt {


class GraphicsRobustAccessPass : public Pass {
 public:
  GraphicsRobustAccessPass();
  const char* name() const override { return "graphics-robust-access"; }
  Status Process() override;

  IRContext::Analysis GetPreservedAnalyses() override {
    return IRContext::kAnalysisDefUse |
           IRContext::kAnalysisInstrToBlockMapping |
           IRContext::kAnalysisConstants | IRContext::kAnalysisTypes |
           IRContext::kAnalysisIdToFuncMapping;
  }

 private:



  spvtools::DiagnosticStream Fail();




  spv_result_t IsCompatibleModule();




  spv_result_t ProcessCurrentModule();



  bool ProcessAFunction(opt::Function*);




  void ClampIndicesForAccessChain(Instruction* access_chain);





  uint32_t GetGlslInsts();



  Instruction* GetValueForType(uint64_t value, const analysis::Integer* type);






  Instruction* WidenInteger(bool sign_extend, uint32_t bit_width,
                            Instruction* value, Instruction* before_inst);








  opt::Instruction* MakeUMinInst(const analysis::TypeManager& tm,
                                 Instruction* x, Instruction* y,
                                 Instruction* where);










  opt::Instruction* MakeSClampInst(const analysis::TypeManager& tm,
                                   Instruction* x, Instruction* min,
                                   Instruction* max, Instruction* where);





  opt::Instruction* MakeRuntimeArrayLengthInst(Instruction* access_chain,
                                               uint32_t operand_index);






  spv_result_t ClampCoordinateForImageTexelPointer(
      opt::Instruction* image_texel_pointer);


  opt::Instruction* GetDef(uint32_t id) {
    return context()->get_def_use_mgr()->GetDef(id);
  }




  opt::Instruction* InsertInst(opt::Instruction* where_inst, spv::Op opcode,
                               uint32_t type_id, uint32_t result_id,
                               const Instruction::OperandList& operands);


  struct PerModuleState {

    bool modified = false;


    bool failed = false;


    uint32_t glsl_insts_id = 0;
  } module_status_;
};

}
}

#endif
