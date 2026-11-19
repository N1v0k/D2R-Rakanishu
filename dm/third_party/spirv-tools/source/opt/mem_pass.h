// Copyright (c) 2017 The Khronos Group Inc.
// Copyright (c) 2017 Valve Corporation
// Copyright (c) 2017 LunarG Inc.
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

#ifndef SOURCE_OPT_MEM_PASS_H_
#define SOURCE_OPT_MEM_PASS_H_

#include <algorithm>
#include <list>
#include <map>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "source/opt/basic_block.h"
#include "source/opt/def_use_manager.h"
#include "source/opt/dominator_analysis.h"
#include "source/opt/module.h"
#include "source/opt/pass.h"

namespace spvtools {
namespace opt {



class MemPass : public Pass {
 public:
  virtual ~MemPass() override = default;


  uint32_t GetUndefVal(uint32_t var_id) {
    return Type2Undef(GetPointeeTypeId(get_def_use_mgr()->GetDef(var_id)));
  }




  Instruction* GetPtr(Instruction* ip, uint32_t* varId);











  bool IsTargetVar(uint32_t varId);




  void CollectTargetVars(Function* func);

 protected:
  MemPass();



  bool IsBaseTargetType(const Instruction* typeInst) const;




  bool IsTargetType(const Instruction* typeInst) const;


  bool IsNonPtrAccessChain(const spv::Op opcode) const;



  bool IsPtr(uint32_t ptrId);




  Instruction* GetPtr(uint32_t ptrId, uint32_t* varId);


  bool HasOnlyNamesAndDecorates(uint32_t id) const;



  void KillAllInsts(BasicBlock* bp, bool killLabel = true);


  bool HasLoads(uint32_t varId) const;



  bool IsLiveVar(uint32_t varId) const;


  void AddStores(uint32_t ptr_id, std::queue<Instruction*>* insts);




  void DCEInst(Instruction* inst, const std::function<void(Instruction*)>&);


  Status CFGCleanup(Function* func);


  inline bool IsNonTypeDecorate(spv::Op op) const {
    return (op == spv::Op::OpDecorate || op == spv::Op::OpDecorateId);
  }





  uint32_t Type2Undef(uint32_t type_id);


  std::unordered_set<uint32_t> seen_target_vars_;


  std::unordered_set<uint32_t> seen_non_target_vars_;

 private:





  bool HasOnlySupportedRefs(uint32_t varId);


  Status RemoveUnreachableBlocks(Function* func);



  void RemoveBlock(Function::iterator* bi);



  bool RemovePhiOperands(
      Instruction* phi,
      const std::unordered_set<BasicBlock*>& reachable_blocks);


  std::unordered_map<uint32_t, uint32_t> type2undefs_;
};

}
}

#endif
