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

#ifndef SOURCE_OPT_SCALAR_REPLACEMENT_PASS_H_
#define SOURCE_OPT_SCALAR_REPLACEMENT_PASS_H_

#include <cassert>
#include <cstdio>
#include <memory>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "source/opt/function.h"
#include "source/opt/mem_pass.h"
#include "source/opt/type_manager.h"

namespace spvtools {
namespace opt {


class ScalarReplacementPass : public MemPass {
 private:
  static constexpr uint32_t kDefaultLimit = 0;

 public:
  ScalarReplacementPass(uint32_t limit = kDefaultLimit)
      : max_num_elements_(limit) {
    const auto num_to_write = snprintf(
        name_, sizeof(name_), "scalar-replacement=%u", max_num_elements_);
    assert(size_t(num_to_write) < sizeof(name_));
    (void)num_to_write;

#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION




    max_num_elements_ =
        (max_num_elements_ > 0 && max_num_elements_ < 100 ? max_num_elements_
                                                          : 100);
#endif
  }

  const char* name() const override { return name_; }



  Status Process() override;

  IRContext::Analysis GetPreservedAnalyses() override {
    return IRContext::kAnalysisDefUse |
           IRContext::kAnalysisInstrToBlockMapping |
           IRContext::kAnalysisDecorations | IRContext::kAnalysisCombinators |
           IRContext::kAnalysisCFG | IRContext::kAnalysisNameMap |
           IRContext::kAnalysisConstants | IRContext::kAnalysisTypes;
  }

 private:



  struct VariableStats {
    uint32_t num_partial_accesses;
    uint32_t num_full_accesses;
  };



  Status ProcessFunction(Function* function);





  bool CanReplaceVariable(const Instruction* varInst) const;





  bool CheckType(const Instruction* typeInst) const;



  bool CheckAnnotations(const Instruction* varInst) const;



  bool CheckTypeAnnotations(const Instruction* typeInst) const;








  bool CheckUses(const Instruction* inst) const;





  bool CheckUses(const Instruction* inst, VariableStats* stats) const;


  bool CheckUsesRelaxed(const Instruction* inst) const;


  void TransferAnnotations(const Instruction* source,
                           std::vector<Instruction*>* replacements);











  Pass::Status ReplaceVariable(Instruction* inst,
                               std::queue<Instruction*>* worklist);





  Instruction* GetStorageType(const Instruction* inst) const;





  bool CheckLoad(const Instruction* inst, uint32_t index) const;





  bool CheckStore(const Instruction* inst, uint32_t index) const;


  bool CheckDebugDeclare(uint32_t index) const;



  bool CheckImageTexelPointer(uint32_t index) const;




  void CreateVariable(uint32_t typeId, Instruction* varInst, uint32_t index,
                      std::vector<Instruction*>* replacements);









  bool CreateReplacementVariables(Instruction* inst,
                                  std::vector<Instruction*>* replacements);


  uint64_t GetArrayLength(const Instruction* arrayInst) const;




  uint64_t GetNumElements(const Instruction* type) const;




  bool IsSpecConstant(uint32_t id) const;


  uint32_t GetOrCreatePointerType(uint32_t id);








  bool GetOrCreateInitialValue(Instruction* source, uint32_t index,
                               Instruction* newVar);







  bool ReplaceWholeLoad(Instruction* load,
                        const std::vector<Instruction*>& replacements);





  bool ReplaceWholeStore(Instruction* store,
                         const std::vector<Instruction*>& replacements);






  bool ReplaceWholeDebugDeclare(Instruction* dbg_decl,
                                const std::vector<Instruction*>& replacements);





  bool ReplaceWholeDebugValue(Instruction* dbg_value,
                              const std::vector<Instruction*>& replacements);





  bool ReplaceAccessChain(Instruction* chain,
                          const std::vector<Instruction*>& replacements);




  std::unique_ptr<std::unordered_set<int64_t>> GetUsedComponents(
      Instruction* inst);


  Instruction* GetUndef(uint32_t type_id);


  std::unordered_map<uint32_t, uint32_t> pointee_to_pointer_;


  std::unordered_map<uint32_t, uint32_t> type_to_null_;


  uint64_t GetMaxLegalIndex(const Instruction* var_inst) const;



  bool IsLargerThanSizeLimit(uint64_t length) const;





  void CopyDecorationsToVariable(Instruction* from, Instruction* to,
                                 uint32_t member_index);


  void CopyPointerDecorationsToVariable(Instruction* from, Instruction* to);



  void CopyNecessaryMemberDecorationsToVariable(Instruction* from,
                                                Instruction* to,
                                                uint32_t member_index);



  uint32_t max_num_elements_;




  char name_[30];
};

}
}

#endif
