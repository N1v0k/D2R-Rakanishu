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

#ifndef SOURCE_OPT_STRENGTH_REDUCTION_PASS_H_
#define SOURCE_OPT_STRENGTH_REDUCTION_PASS_H_

#include "source/opt/def_use_manager.h"
#include "source/opt/ir_context.h"
#include "source/opt/module.h"
#include "source/opt/pass.h"

namespace spvtools {
namespace opt {


class StrengthReductionPass : public Pass {
 public:
  const char* name() const override { return "strength-reduction"; }
  Status Process() override;

 private:


  Status ReplaceMultiplyByPowerOf2(BasicBlock::iterator*);






  void FindIntTypesAndConstants();



  uint32_t GetConstantId(uint32_t);



  Status ScanFunctions();


  uint32_t int32_type_id_;
  uint32_t uint32_type_id_;




  uint32_t constant_ids_[33];
};

}
}

#endif
