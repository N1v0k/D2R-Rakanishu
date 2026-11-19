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

#ifndef SOURCE_OPT_ELIMINATE_DEAD_MEMBERS_PASS_H_
#define SOURCE_OPT_ELIMINATE_DEAD_MEMBERS_PASS_H_

#include "source/opt/def_use_manager.h"
#include "source/opt/function.h"
#include "source/opt/mem_pass.h"
#include "source/opt/module.h"

namespace spvtools {
namespace opt {



class EliminateDeadMembersPass : public MemPass {
 public:
  const char* name() const override { return "eliminate-dead-members"; }
  Status Process() override;

  IRContext::Analysis GetPreservedAnalyses() override {
    return IRContext::kAnalysisDefUse |
           IRContext::kAnalysisInstrToBlockMapping |
           IRContext::kAnalysisCombinators | IRContext::kAnalysisCFG |
           IRContext::kAnalysisDominatorAnalysis |
           IRContext::kAnalysisLoopAnalysis |
           IRContext::kAnalysisScalarEvolution |
           IRContext::kAnalysisRegisterPressure |
           IRContext::kAnalysisValueNumberTable |
           IRContext::kAnalysisStructuredCFG |
           IRContext::kAnalysisBuiltinVarId |
           IRContext::kAnalysisIdToFuncMapping;
  }

 private:


  void FindLiveMembers();



  void FindLiveMembers(const Function& function);

  void FindLiveMembers(const Instruction* inst);



  void MarkMembersAsLiveForStore(const Instruction* inst);



  void MarkMembersAsLiveForCopyMemory(const Instruction* inst);



  void MarkMembersAsLiveForExtract(const Instruction* inst);



  void MarkMembersAsLiveForAccessChain(const Instruction* inst);



  void MarkMembersAsLiveForArrayLength(const Instruction* inst);



  bool RemoveDeadMembers();




  bool UpdateOpMemberNameOrDecorate(Instruction* inst);




  bool UpdateOpGroupMemberDecorate(Instruction* inst);



  bool UpdateOpTypeStruct(Instruction* inst);




  bool UpdateConstantComposite(Instruction* inst);





  bool UpdateAccessChain(Instruction* inst);





  bool UpdateCompsiteExtract(Instruction* inst);





  bool UpdateCompositeInsert(Instruction* inst);




  bool UpdateOpArrayLength(Instruction* inst);



  void MarkTypeAsFullyUsed(uint32_t type_id);



  void MarkOperandTypeAsFullyUsed(const Instruction* inst, uint32_t in_idx);



  uint32_t GetNewMemberIndex(uint32_t type_id, uint32_t member_idx);



  std::unordered_map<uint32_t, std::set<uint32_t>> used_members_;
  void MarkStructOperandsAsFullyUsed(const Instruction* inst);
  void MarkPointeeTypeAsFullUsed(uint32_t ptr_type_id);
};

}
}

#endif
