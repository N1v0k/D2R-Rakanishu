// Copyright (c) 2025 Google LLC
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

#ifndef LIBSPIRV_OPT_SPLIT_COMBINED_IMAGE_SAMPLER_PASS_H_
#define LIBSPIRV_OPT_SPLIT_COMBINED_IMAGE_SAMPLER_PASS_H_

#include <unordered_map>
#include <utility>
#include <vector>

#include "source/diagnostic.h"
#include "source/opt/decoration_manager.h"
#include "source/opt/def_use_manager.h"
#include "source/opt/pass.h"
#include "source/opt/type_manager.h"

namespace spvtools {
namespace opt {





class SplitCombinedImageSamplerPass : public Pass {
 public:
  virtual ~SplitCombinedImageSamplerPass() override = default;
  const char* name() const override { return "split-combined-image-sampler"; }
  IRContext::Analysis GetPreservedAnalyses() override;
  Status Process() override;

 private:



  spvtools::DiagnosticStream Fail();



  void FindCombinedTextureSamplers();



  Instruction* GetSamplerType();





  spv_result_t RemapFunctions();


  spv_result_t RemapVars();


  spv_result_t RemapVar(Instruction* combined_var);





  spv_result_t RemapUses(Instruction* combined, Instruction* image_part,
                         Instruction* sampler_part);

  spv_result_t RemoveDeadTypes();





  Instruction* MakeUniformConstantPointer(Instruction* pointee);


  uint32_t PointeeTypeId(Instruction* ptr_value) {
    auto* ptr_ty = def_use_mgr_->GetDef(ptr_value->type_id());
    assert(ptr_ty->opcode() == spv::Op::OpTypePointer);
    return ptr_ty->GetSingleWordInOperand(1);
  }




  void AddOpName(uint32_t id, const std::string& name);


  analysis::DefUseManager* def_use_mgr_ = nullptr;

  analysis::TypeManager* type_mgr_ = nullptr;


  bool modified_ = false;
  Pass::Status Ok() {
    return modified_ ? Pass::Status::SuccessWithChange
                     : Pass::Status::SuccessWithoutChange;
  }


  Instruction* first_sampled_image_type_ = nullptr;

  Instruction* sampler_type_ = nullptr;



  std::unordered_set<uint32_t> known_globals_;
  bool IsKnownGlobal(uint32_t id) const {
    return known_globals_.find(id) != known_globals_.end();
  }
  void RegisterGlobal(uint32_t id) { known_globals_.insert(id); }
  void RegisterNewGlobal(uint32_t id) {
    modified_ = true;
    RegisterGlobal(id);
  }



  void KillInst(Instruction* inst);



  std::unordered_set<uint32_t> combined_types_;


  std::vector<uint32_t> combined_types_to_remove_;


  bool sampled_image_used_as_param_ = false;



  struct TypeRemapInfo {


    Instruction* combined_kind_type;


    Instruction* image_kind_type;


    Instruction* sampler_kind_type;
  };


  std::unordered_map<uint32_t, TypeRemapInfo> type_remap_;







  std::pair<Instruction*, Instruction*> SplitType(
      Instruction& combined_kind_type);


  std::vector<Instruction*> ordered_vars_;
};
}
}
#endif
