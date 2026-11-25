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

#ifndef SOURCE_OPT_PASS_H_
#define SOURCE_OPT_PASS_H_

#include <algorithm>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "source/opt/basic_block.h"
#include "source/opt/def_use_manager.h"
#include "source/opt/ir_context.h"
#include "source/opt/module.h"
#include "source/util/status.h"
#include "spirv-tools/libspirv.hpp"
#include "types.h"


#ifndef NDEBUG
#define USE_ASSERT(x) assert(x)
#else
#define USE_ASSERT(x) ((void)(x))
#endif

namespace spvtools {
namespace opt {



class Pass {
 public:




  using Status = utils::Status;

  using ProcessFunction = std::function<bool(Function*)>;


  virtual ~Pass() = default;







  virtual const char* name() const = 0;



  void SetMessageConsumer(MessageConsumer c) { consumer_ = std::move(c); }


  const MessageConsumer& consumer() const { return consumer_; }



  analysis::DefUseManager* get_def_use_mgr() const {
    return context()->get_def_use_mgr();
  }

  analysis::DecorationManager* get_decoration_mgr() const {
    return context()->get_decoration_mgr();
  }

  FeatureManager* get_feature_mgr() const {
    return context()->get_feature_mgr();
  }


  Module* get_module() const { return context_->module(); }


  void SetContextForTesting(IRContext* ctx) { context_ = ctx; }


  IRContext* context() const { return context_; }


  CFG* cfg() const { return context()->cfg(); }









  Status Run(IRContext* ctx);


  virtual IRContext::Analysis GetPreservedAnalyses() {
    return IRContext::kAnalysisNone;
  }


  uint32_t GetPointeeTypeId(const Instruction* ptrInst) const;


  Instruction* GetBaseType(uint32_t ty_id);



  bool IsFloat(uint32_t ty_id, uint32_t width);


  uint32_t GetNullId(uint32_t type_id);

 protected:





  Pass();




  virtual Status Process() = 0;



  uint32_t TakeNextId() { return context_->TakeNextId(); }





  uint32_t GenerateCopy(Instruction* object_to_copy, uint32_t new_type_id,
                        Instruction* insertion_position);

 private:
  MessageConsumer consumer_;


  IRContext* context_;




  bool already_run_;
};

inline Pass::Status CombineStatus(Pass::Status a, Pass::Status b) {
  return std::min(a, b);
}

}
}

#endif
