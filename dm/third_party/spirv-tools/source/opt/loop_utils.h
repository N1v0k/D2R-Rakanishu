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

#ifndef SOURCE_OPT_LOOP_UTILS_H_
#define SOURCE_OPT_LOOP_UTILS_H_

#include <list>
#include <memory>
#include <unordered_map>
#include <vector>

#include "source/opt/ir_context.h"
#include "source/opt/loop_descriptor.h"

namespace spvtools {

namespace opt {




struct CodeMetrics {
  void Analyze(const Loop& loop);


  std::unordered_map<uint32_t, size_t> block_sizes_;


  size_t roi_size_;
};




class LoopUtils {
 public:

  struct LoopCloningResult {
    using ValueMapTy = std::unordered_map<uint32_t, uint32_t>;
    using BlockMapTy = std::unordered_map<uint32_t, BasicBlock*>;
    using PtrMap = std::unordered_map<Instruction*, Instruction*>;

    PtrMap ptr_map_;


    ValueMapTy value_map_;

    BlockMapTy old_to_new_bb_;

    BlockMapTy new_to_old_bb_;

    std::vector<std::unique_ptr<BasicBlock>> cloned_bb_;
  };

  LoopUtils(IRContext* context, Loop* loop)
      : context_(context),
        loop_desc_(
            context->GetLoopDescriptor(loop->GetHeaderBlock()->GetParent())),
        loop_(loop),
        function_(*loop_->GetHeaderBlock()->GetParent()) {}


























  bool MakeLoopClosedSSA();






  bool CreateLoopDedicatedExits();












  Loop* CloneLoop(LoopCloningResult* cloning_result,
                  const std::vector<BasicBlock*>& ordered_loop_blocks) const;


  Loop* CloneLoop(LoopCloningResult* cloning_result) const;



  Loop* CloneAndAttachLoopToHeader(LoopCloningResult* cloning_result);




  bool PartiallyUnroll(size_t factor);


  bool FullyUnroll();
















  bool CanPerformUnroll();



  void Finalize();


  IRContext* GetContext() { return context_; }

  LoopDescriptor* GetLoopDescriptor() { return loop_desc_; }

  Loop* GetLoop() const { return loop_; }

  Function* GetFunction() const { return &function_; }

 private:
  IRContext* context_;
  LoopDescriptor* loop_desc_;
  Loop* loop_;
  Function& function_;


  void PopulateLoopNest(Loop* new_loop,
                        const LoopCloningResult& cloning_result) const;


  void PopulateLoopDesc(Loop* new_loop, Loop* old_loop,
                        const LoopCloningResult& cloning_result) const;
};

}
}

#endif
