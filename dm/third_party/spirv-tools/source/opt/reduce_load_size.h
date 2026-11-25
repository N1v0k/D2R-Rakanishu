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

#ifndef SOURCE_OPT_REDUCE_LOAD_SIZE_H_
#define SOURCE_OPT_REDUCE_LOAD_SIZE_H_

#include <unordered_map>

#include "source/opt/ir_context.h"
#include "source/opt/module.h"
#include "source/opt/pass.h"

namespace spvtools {
namespace opt {


class ReduceLoadSize : public Pass {
 public:
  explicit ReduceLoadSize(double replacement_threshold)
      : replacement_threshold_(replacement_threshold) {}

  const char* name() const override { return "reduce-load-size"; }
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





  bool ReplaceExtract(Instruction* inst);





  bool ShouldReplaceExtract(Instruction* inst);




  double replacement_threshold_;



  std::unordered_map<uint32_t, bool> should_replace_cache_;
};

}
}

#endif
