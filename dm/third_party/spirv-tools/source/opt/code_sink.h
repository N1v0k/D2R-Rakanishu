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

#ifndef SOURCE_OPT_CODE_SINK_H_
#define SOURCE_OPT_CODE_SINK_H_

#include <unordered_map>

#include "source/opt/ir_context.h"
#include "source/opt/module.h"
#include "source/opt/pass.h"

namespace spvtools {
namespace opt {

















class CodeSinkingPass : public Pass {
 public:
  const char* name() const override { return "code-sink"; }
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


  bool SinkInstructionsInBB(BasicBlock* bb);



  bool SinkInstruction(Instruction* inst);





  BasicBlock* FindNewBasicBlockFor(Instruction* inst);



  bool ReferencesMutableMemory(Instruction* inst);





  bool HasUniformMemorySync();


  bool HasPossibleStore(Instruction* var_inst);



  bool IntersectsPath(uint32_t start, uint32_t end,
                      const std::unordered_set<uint32_t>& set);




  bool IsSyncOnUniform(uint32_t mem_semantics_id) const;


  bool checked_for_uniform_sync_;



  bool has_uniform_sync_;
};

}
}

#endif
