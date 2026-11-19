// Copyright (c) 2016 Google Inc.
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

#ifndef SOURCE_OPT_FOLD_SPEC_CONSTANT_OP_AND_COMPOSITE_PASS_H_
#define SOURCE_OPT_FOLD_SPEC_CONSTANT_OP_AND_COMPOSITE_PASS_H_

#include <memory>
#include <unordered_map>
#include <vector>

#include "source/diagnostic.h"
#include "source/opt/constants.h"
#include "source/opt/def_use_manager.h"
#include "source/opt/ir_context.h"
#include "source/opt/module.h"
#include "source/opt/pass.h"
#include "source/opt/type_manager.h"

namespace spvtools {
namespace opt {


class FoldSpecConstantOpAndCompositePass : public Pass {
 public:
  FoldSpecConstantOpAndCompositePass() = default;

  const char* name() const override { return "fold-spec-const-op-composite"; }





  Status Process() override;

 private:









  Status ProcessOpSpecConstantOp(Module::inst_iterator* pos);



  Instruction* FoldWithInstructionFolder(Module::inst_iterator* inst_iter_ptr);






  Instruction* DoComponentWiseOperation(Module::inst_iterator* inst_iter_ptr);


  uint32_t TakeNextId() {
    uint32_t next_id = context()->TakeNextId();
    if (next_id == 0) {
      Fail() << "ID overflow. Try running compact-ids.";
    }
    return next_id;
  }



  spvtools::DiagnosticStream Fail() {
    return spvtools::DiagnosticStream({}, context()->consumer(), "",
                                      SPV_ERROR_INTERNAL);
  }
};

}
}

#endif
