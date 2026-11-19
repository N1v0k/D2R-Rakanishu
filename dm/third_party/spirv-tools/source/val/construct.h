// Copyright (c) 2015-2016 The Khronos Group Inc.
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

#ifndef SOURCE_VAL_CONSTRUCT_H_
#define SOURCE_VAL_CONSTRUCT_H_

#include <cstdint>
#include <set>
#include <vector>

#include "source/val/basic_block.h"

namespace spvtools {
namespace val {
class ValidationState_t;


struct less_than_id {
  bool operator()(const BasicBlock* lhs, const BasicBlock* rhs) const {
    return lhs->id() < rhs->id();
  }
};

enum class ConstructType : int {
  kNone = 0,


  kSelection,


  kContinue,



  kLoop,




  kCase
};

class Function;


class Construct {
 public:
  Construct(ConstructType type, BasicBlock* dominator,
            BasicBlock* exit = nullptr,
            std::vector<Construct*> constructs = std::vector<Construct*>());


  ConstructType type() const;

  const std::vector<Construct*>& corresponding_constructs() const;
  std::vector<Construct*>& corresponding_constructs();
  void set_corresponding_constructs(std::vector<Construct*> constructs);




  const BasicBlock* entry_block() const;




  BasicBlock* entry_block();







  const BasicBlock* exit_block() const;







  BasicBlock* exit_block();



  void set_exit(BasicBlock* exit_block);



  bool ExitBlockIsMergeBlock() const {
    return type_ == ConstructType::kLoop || type_ == ConstructType::kSelection;
  }

  using ConstructBlockSet = std::set<BasicBlock*, less_than_id>;




  ConstructBlockSet blocks(Function* function) const;

















  bool IsStructuredExit(ValidationState_t& _, BasicBlock* dest) const;

 private:

  ConstructType type_;
















  std::vector<Construct*> corresponding_constructs_;






  BasicBlock* entry_block_;






  BasicBlock* exit_block_;
};

}
}

#endif
