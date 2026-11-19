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

#include "source/opt/inline_exhaustive_pass.h"

#include <iterator>
#include <utility>

namespace spvtools {
namespace opt {

Pass::Status InlineExhaustivePass::InlineExhaustive(Function* func) {
  bool modified = false;

  for (auto bi = func->begin(); bi != func->end(); ++bi) {
    for (auto ii = bi->begin(); ii != bi->end();) {
      if (IsInlinableFunctionCall(&*ii)) {

        Instruction* prev_inst =
            (ii == bi->begin()) ? nullptr : &*std::prev(ii);


        std::vector<std::unique_ptr<BasicBlock>> newBlocks;
        std::vector<std::unique_ptr<Instruction>> newVars;
        if (!GenInlineCode(&newBlocks, &newVars, ii, bi)) {
          return Status::Failure;
        }


        if (newBlocks.size() > 1) UpdateSucceedingPhis(newBlocks);


        bi = bi.Erase();

        for (auto& bb : newBlocks) {
          bb->SetParent(func);
        }
        bi = bi.InsertBefore(&newBlocks);

        if (newVars.size() > 0)
          func->begin()->begin().InsertBefore(std::move(newVars));

        ii = prev_inst ? ++InstructionList::iterator(prev_inst) : bi->begin();
        modified = true;
      } else {
        ++ii;
      }
    }
  }

  if (modified) {
    FixDebugDeclares(func);
  }

  return (modified ? Status::SuccessWithChange : Status::SuccessWithoutChange);
}

Pass::Status InlineExhaustivePass::ProcessImpl() {
  Status status = Status::SuccessWithoutChange;

  ProcessFunction pfn = [&status, this](Function* fp) {
    status = CombineStatus(status, InlineExhaustive(fp));
    return false;
  };
  context()->ProcessReachableCallTree(pfn);
  return status;
}

InlineExhaustivePass::InlineExhaustivePass() = default;

Pass::Status InlineExhaustivePass::Process() {
  InitializeInline();
  return ProcessImpl();
}

}
}
