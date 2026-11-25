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

#ifndef SOURCE_OPT_CCP_PASS_H_
#define SOURCE_OPT_CCP_PASS_H_

#include <memory>
#include <unordered_map>

#include "source/opt/constants.h"
#include "source/opt/function.h"
#include "source/opt/ir_context.h"
#include "source/opt/mem_pass.h"
#include "source/opt/module.h"
#include "source/opt/propagator.h"

namespace spvtools {
namespace opt {

class CCPPass : public MemPass {
 public:
  CCPPass() = default;

  const char* name() const override { return "ccp"; }
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

  void Initialize();



  bool PropagateConstants(Function* fp);




  SSAPropagator::PropStatus VisitInstruction(Instruction* instr,
                                             BasicBlock** dest_bb);




  SSAPropagator::PropStatus VisitPhi(Instruction* phi);




  SSAPropagator::PropStatus VisitAssignment(Instruction* instr);





  SSAPropagator::PropStatus VisitBranch(Instruction* instr,
                                        BasicBlock** dest_bb) const;




  bool ReplaceValues();



  SSAPropagator::PropStatus MarkInstructionVarying(Instruction* instr);



  bool IsVaryingValue(uint32_t id) const;



  analysis::ConstantManager* const_mgr_;















  uint32_t ComputeLatticeMeet(Instruction* instr, uint32_t val2);










  std::unordered_map<uint32_t, uint32_t> values_;


  std::unique_ptr<SSAPropagator> propagator_;



  uint32_t original_id_bound_;
};

}
}

#endif
