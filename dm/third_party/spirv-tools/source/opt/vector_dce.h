// Copyright (c) 2018 Google LLC.
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

#ifndef SOURCE_OPT_VECTOR_DCE_H_
#define SOURCE_OPT_VECTOR_DCE_H_

#include <unordered_map>
#include <vector>

#include "source/opt/mem_pass.h"
#include "source/util/bit_vector.h"

namespace spvtools {
namespace opt {

class VectorDCE : public MemPass {
 private:
  using LiveComponentMap = std::unordered_map<uint32_t, utils::BitVector>;



  enum { kMaxVectorSize = 16 };

  struct WorkListItem {
    WorkListItem() : instruction(nullptr), components(kMaxVectorSize) {}

    Instruction* instruction;
    utils::BitVector components;
  };

 public:
  VectorDCE() : all_components_live_(kMaxVectorSize) {
    for (uint32_t i = 0; i < kMaxVectorSize; i++) {
      all_components_live_.Set(i);
    }
  }

  const char* name() const override { return "vector-dce"; }
  Status Process() override;

  IRContext::Analysis GetPreservedAnalyses() override {
    return IRContext::kAnalysisDefUse | IRContext::kAnalysisCFG |
           IRContext::kAnalysisInstrToBlockMapping |
           IRContext::kAnalysisLoopAnalysis | IRContext::kAnalysisDecorations |
           IRContext::kAnalysisDominatorAnalysis | IRContext::kAnalysisNameMap |
           IRContext::kAnalysisConstants | IRContext::kAnalysisTypes;
  }

 private:


  bool VectorDCEFunction(Function* function);



  void FindLiveComponents(Function* function,
                          LiveComponentMap* live_components);




  bool RewriteInstructions(Function* function,
                           const LiveComponentMap& live_components);



  void MarkDebugValueUsesAsDead(Instruction* composite,
                                std::vector<Instruction*>* dead_dbg_value);










  bool RewriteInsertInstruction(Instruction* current_inst,
                                const utils::BitVector& live_components,
                                std::vector<Instruction*>* dead_dbg_value);


  bool HasVectorOrScalarResult(const Instruction* inst) const;


  bool HasVectorResult(const Instruction* inst) const;


  bool HasScalarResult(const Instruction* inst) const;


  uint32_t GetVectorComponentCount(uint32_t type_id);




  void AddItemToWorkListIfNeeded(WorkListItem work_item,
                                 LiveComponentMap* live_components,
                                 std::vector<WorkListItem>* work_list);




  void MarkUsesAsLive(Instruction* current_inst,
                      const utils::BitVector& live_elements,
                      LiveComponentMap* live_components,
                      std::vector<WorkListItem>* work_list);





  void MarkVectorShuffleUsesAsLive(const WorkListItem& current_item,
                                   VectorDCE::LiveComponentMap* live_components,
                                   std::vector<WorkListItem>* work_list);





  void MarkInsertUsesAsLive(const WorkListItem& current_item,
                            LiveComponentMap* live_components,
                            std::vector<WorkListItem>* work_list);




  void MarkExtractUseAsLive(const Instruction* current_inst,
                            const utils::BitVector& live_elements,
                            LiveComponentMap* live_components,
                            std::vector<WorkListItem>* work_list);




  void MarkCompositeContructUsesAsLive(WorkListItem work_item,
                                       LiveComponentMap* live_components,
                                       std::vector<WorkListItem>* work_list);



  utils::BitVector all_components_live_;
};

}
}

#endif
