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

#ifndef SOURCE_OPT_ELIMINATE_DEAD_OUTPUT_STORES_H_
#define SOURCE_OPT_ELIMINATE_DEAD_OUTPUT_STORES_H_

#include <unordered_set>

#include "source/opt/ir_context.h"
#include "source/opt/module.h"
#include "source/opt/pass.h"

namespace spvtools {
namespace opt {


class EliminateDeadOutputStoresPass : public Pass {
 public:
  explicit EliminateDeadOutputStoresPass(
      std::unordered_set<uint32_t>* live_locs,
      std::unordered_set<uint32_t>* live_builtins)
      : live_locs_(live_locs), live_builtins_(live_builtins) {}

  const char* name() const override { return "eliminate-dead-output-stores"; }
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

  void InitializeElimination();


  Status DoDeadOutputStoreElimination();


  void KillAllStoresOfRef(Instruction* ref);


  void KillAllDeadStoresOfLocRef(Instruction* user, Instruction* var);


  void KillAllDeadStoresOfBuiltinRef(Instruction* user, Instruction* var);



  bool AnyLocsAreLive(uint32_t start, uint32_t count);


  bool IsLiveBuiltin(uint32_t bi);

  std::unordered_set<uint32_t>* live_locs_;
  std::unordered_set<uint32_t>* live_builtins_;

  std::vector<Instruction*> kill_list_;
};

}
}

#endif
