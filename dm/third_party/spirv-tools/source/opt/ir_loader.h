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

#ifndef SOURCE_OPT_IR_LOADER_H_
#define SOURCE_OPT_IR_LOADER_H_

#include <memory>
#include <string>
#include <vector>

#include "source/opt/basic_block.h"
#include "source/opt/graph.h"
#include "source/opt/instruction.h"
#include "source/opt/module.h"
#include "spirv-tools/libspirv.hpp"

namespace spvtools {
namespace opt {









class IrLoader {
 public:




  IrLoader(const MessageConsumer& consumer, Module* m);


  void SetSource(const std::string& src) { source_ = src; }

  Module* module() const { return module_; }


  void SetModuleHeader(uint32_t magic, uint32_t version, uint32_t generator,
                       uint32_t bound, uint32_t reserved) {
    module_->SetHeader({magic, version, generator, bound, reserved});
  }



  bool AddInstruction(const spv_parsed_instruction_t* inst);




  void EndModule();



  void SetExtraLineTracking(bool flag) { extra_line_tracking_ = flag; }

 private:

  const MessageConsumer& consumer_;

  Module* module_;

  std::string source_;

  uint32_t inst_index_;

  std::unique_ptr<Function> function_;

  std::unique_ptr<BasicBlock> block_;

  std::unique_ptr<Graph> graph_;

  std::vector<Instruction> dbg_line_info_;


  std::unique_ptr<Instruction> last_line_inst_;


  DebugScope last_dbg_scope_;




  bool extra_line_tracking_ = true;
};

}
}

#endif
