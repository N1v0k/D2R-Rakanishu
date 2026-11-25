// Copyright (c) 2021 Google LLC
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

#ifndef SOURCE_OPT_REPLACE_DESC_VAR_INDEX_ACCESS_H_
#define SOURCE_OPT_REPLACE_DESC_VAR_INDEX_ACCESS_H_

#include <cstdio>
#include <memory>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "source/opt/function.h"
#include "source/opt/pass.h"
#include "source/opt/type_manager.h"

namespace spvtools {
namespace opt {


class ReplaceDescArrayAccessUsingVarIndex : public Pass {
 public:
  ReplaceDescArrayAccessUsingVarIndex() {}

  const char* name() const override {
    return "replace-desc-array-access-using-var-index";
  }

  Status Process() override;

  IRContext::Analysis GetPreservedAnalyses() override {
    return IRContext::kAnalysisDefUse |
           IRContext::kAnalysisInstrToBlockMapping |
           IRContext::kAnalysisConstants | IRContext::kAnalysisTypes;
  }

 private:



  Status ReplaceVariableAccessesWithConstantElements(Instruction* var) const;




  bool ReplaceAccessChain(Instruction* var, Instruction* use) const;




  void UseConstIndexForAccessChain(Instruction* access_chain,
                                   uint32_t const_element_idx) const;





  bool ReplaceUsersOfAccessChain(Instruction* access_chain,
                                 uint32_t number_of_elements) const;



  void CollectRecursiveUsersWithConcreteType(
      Instruction* access_chain, std::vector<Instruction*>* final_users) const;





  std::deque<Instruction*> CollectRequiredImageAndAccessInsts(
      Instruction* user) const;



  bool HasImageOrImagePtrType(const Instruction* inst) const;



  bool IsImageOrImagePtrType(const Instruction* type_inst) const;


  bool IsConcreteType(uint32_t type_id) const;









  bool ReplaceNonUniformAccessWithSwitchCase(
      Instruction* access_chain_final_user, Instruction* access_chain,
      uint32_t number_of_elements,
      const std::deque<Instruction*>& non_uniform_accesses_to_clone) const;




  BasicBlock* SeparateInstructionsIntoNewBlock(
      BasicBlock* block, Instruction* separation_begin_inst) const;


  BasicBlock* CreateNewBlock() const;



  uint32_t GetFirstIndexOfAccessChain(Instruction* access_chain) const;





  bool AddConstElementAccessToCaseBlock(
      BasicBlock* case_block, Instruction* access_chain,
      uint32_t const_element_idx,
      std::unordered_map<uint32_t, uint32_t>* old_ids_to_new_ids) const;




  bool CloneInstsToBlock(
      BasicBlock* block, Instruction* inst_to_skip_cloning,
      const std::deque<Instruction*>& insts_to_be_cloned,
      std::unordered_map<uint32_t, uint32_t>* old_ids_to_new_ids) const;


  void AddBranchToBlock(BasicBlock* parent_block,
                        uint32_t branch_destination) const;




  void UseNewIdsInBlock(
      BasicBlock* block,
      const std::unordered_map<uint32_t, uint32_t>& old_ids_to_new_ids) const;






  BasicBlock* CreateCaseBlock(
      Instruction* access_chain, uint32_t element_index,
      const std::deque<Instruction*>& insts_to_be_cloned,
      uint32_t branch_target_id,
      std::unordered_map<uint32_t, uint32_t>* old_ids_to_new_ids) const;






  BasicBlock* CreateDefaultBlock(bool null_const_for_phi_is_needed,
                                 std::vector<uint32_t>* phi_operands,
                                 uint32_t merge_block_id) const;







  void AddSwitchForAccessChain(
      BasicBlock* parent_block, uint32_t access_chain_index_var_id,
      uint32_t default_id, uint32_t merge_id,
      const std::vector<uint32_t>& case_block_ids) const;







  uint32_t CreatePhiInstruction(BasicBlock* parent_block,
                                const std::vector<uint32_t>& phi_operands,
                                const std::vector<uint32_t>& case_block_ids,
                                uint32_t default_block_id) const;




  void ReplacePhiIncomingBlock(uint32_t old_incoming_block_id,
                               uint32_t new_incoming_block_id) const;


  Instruction* GetConstNull(uint32_t type_id) const;
};

}
}

#endif
