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

#ifndef SOURCE_OPT_IF_CONVERSION_H_
#define SOURCE_OPT_IF_CONVERSION_H_

#include "source/opt/basic_block.h"
#include "source/opt/ir_builder.h"
#include "source/opt/pass.h"
#include "source/opt/types.h"

namespace spvtools {
namespace opt {


class IfConversion : public Pass {
 public:
  const char* name() const override { return "if-conversion"; }
  Status Process() override;

  IRContext::Analysis GetPreservedAnalyses() override {
    return IRContext::kAnalysisDefUse | IRContext::kAnalysisDominatorAnalysis |
           IRContext::kAnalysisInstrToBlockMapping | IRContext::kAnalysisCFG |
           IRContext::kAnalysisNameMap | IRContext::kAnalysisConstants |
           IRContext::kAnalysisTypes;
  }

 private:


  bool CheckType(uint32_t id);


  BasicBlock* GetBlock(uint32_t id);



  BasicBlock* GetIncomingBlock(Instruction* phi, uint32_t predecessor);


  Instruction* GetIncomingValue(Instruction* phi, uint32_t predecessor);






  uint32_t SplatCondition(analysis::Vector* vec_data_ty, uint32_t cond,
                          InstructionBuilder* builder);


  bool CheckPhiUsers(Instruction* phi, BasicBlock* block);





  bool CheckBlock(BasicBlock* block, DominatorAnalysis* dominators,
                  BasicBlock** common);






  void HoistInstruction(Instruction* inst, BasicBlock* target_block,
                        DominatorAnalysis* dominators);



  bool CanHoistInstruction(Instruction* inst, BasicBlock* target_block,
                           DominatorAnalysis* dominators);
};

}
}

#endif
