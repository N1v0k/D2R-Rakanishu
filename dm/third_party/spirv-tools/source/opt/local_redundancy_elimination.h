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

#ifndef SOURCE_OPT_LOCAL_REDUNDANCY_ELIMINATION_H_
#define SOURCE_OPT_LOCAL_REDUNDANCY_ELIMINATION_H_

#include <map>

#include "source/opt/ir_context.h"
#include "source/opt/pass.h"
#include "source/opt/value_number_table.h"

namespace spvtools {
namespace opt {








class LocalRedundancyEliminationPass : public Pass {
 public:
  const char* name() const override { return "local-redundancy-elimination"; }
  Status Process() override;

  IRContext::Analysis GetPreservedAnalyses() override {
    return IRContext::kAnalysisDefUse |
           IRContext::kAnalysisInstrToBlockMapping |
           IRContext::kAnalysisDecorations | IRContext::kAnalysisCombinators |
           IRContext::kAnalysisCFG | IRContext::kAnalysisDominatorAnalysis |
           IRContext::kAnalysisNameMap | IRContext::kAnalysisConstants |
           IRContext::kAnalysisTypes;
  }

 protected:











  bool EliminateRedundanciesInBB(BasicBlock* block,
                                 const ValueNumberTable& vnTable,
                                 std::map<uint32_t, uint32_t>* value_to_ids);
};

}
}

#endif
