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

#ifndef SOURCE_OPT_LOOP_FUSION_H_
#define SOURCE_OPT_LOOP_FUSION_H_

#include <map>
#include <set>
#include <utility>
#include <vector>

#include "source/opt/ir_context.h"
#include "source/opt/loop_descriptor.h"
#include "source/opt/loop_utils.h"
#include "source/opt/scalar_analysis.h"

namespace spvtools {
namespace opt {

class LoopFusion {
 public:
  LoopFusion(IRContext* context, Loop* loop_0, Loop* loop_1)
      : context_(context),
        loop_0_(loop_0),
        loop_1_(loop_1),
        containing_function_(loop_0->GetHeaderBlock()->GetParent()) {}












  bool AreCompatible();





  bool IsLegal();



  void Fuse();

 private:

  bool CheckInit();


  bool CheckCondition();


  bool CheckStep();



  bool UsedInContinueOrConditionBlock(Instruction* instruction, Loop* loop);



  void RemoveIfNotUsedContinueOrConditionBlock(
      std::vector<Instruction*>* instructions, Loop* loop);


  bool IsUsedInLoop(Instruction* instruction, Loop* loop);


  bool ContainsBarriersOrFunctionCalls(Loop* loop);



  std::pair<std::vector<Instruction*>, std::vector<Instruction*>>
  GetLoadsAndStoresInLoop(Loop* loop);



  std::map<Instruction*, std::vector<Instruction*>> LocationToMemOps(
      const std::vector<Instruction*>& mem_ops);

  IRContext* context_;


  Loop* loop_0_;
  Loop* loop_1_;


  Function* containing_function_ = nullptr;


  Instruction* induction_0_ = nullptr;
  Instruction* induction_1_ = nullptr;
};

}
}

#endif
