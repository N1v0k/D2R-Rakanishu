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

#ifndef SOURCE_OPT_LICM_PASS_H_
#define SOURCE_OPT_LICM_PASS_H_

#include <queue>
#include <vector>

#include "source/opt/basic_block.h"
#include "source/opt/instruction.h"
#include "source/opt/loop_descriptor.h"
#include "source/opt/pass.h"

namespace spvtools {
namespace opt {

class LICMPass : public Pass {
 public:
  LICMPass() {}

  const char* name() const override { return "loop-invariant-code-motion"; }
  Status Process() override;

 private:




  Pass::Status ProcessIRContext();




  Pass::Status ProcessFunction(Function* f);





  Pass::Status ProcessLoop(Loop* loop, Function* f);





  Pass::Status AnalyseAndHoistFromBB(Loop* loop, Function* f, BasicBlock* bb,
                                     std::vector<BasicBlock*>* loop_bbs);


  bool IsImmediatelyContainedInLoop(Loop* loop, Function* f, BasicBlock* bb);



  bool HoistInstruction(Loop* loop, Instruction* inst);
};

}
}

#endif
