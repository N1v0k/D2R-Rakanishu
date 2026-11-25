// Copyright (c) 2026 Google LLC
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

#ifndef SOURCE_OPT_LEGALIZE_MULTIDIM_ARRAY_PASS_H_
#define SOURCE_OPT_LEGALIZE_MULTIDIM_ARRAY_PASS_H_

#include "source/opt/pass.h"

namespace spvtools {
namespace opt {



class LegalizeMultidimArrayPass : public Pass {
 public:
  const char* name() const override { return "legalize-multidim-array"; }
  Status Process() override;

 private:

  bool IsMultidimArrayOfResources(Instruction* var);



  uint32_t FlattenArrayType(Instruction* var);


  bool RewriteAccessChains(Instruction* var, uint32_t old_ptr_type_id);


  bool CanLegalize(Instruction* var);


  bool CheckUse(Instruction* inst, uint32_t max_depth);


  void GetArrayDimensions(uint32_t type_id, std::vector<uint32_t>* dims,
                          uint32_t* element_type_id);
};

}
}

#endif
