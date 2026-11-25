// Copyright (c) 2019 Valve Corporation
// Copyright (c) 2019 LunarG Inc.
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

#ifndef LIBSPIRV_OPT_CONVERT_TO_HALF_PASS_H_
#define LIBSPIRV_OPT_CONVERT_TO_HALF_PASS_H_

#include "source/opt/ir_builder.h"
#include "source/opt/pass.h"

namespace spvtools {
namespace opt {

class ConvertToHalfPass : public Pass {
 public:
  ConvertToHalfPass() : Pass() {}

  ~ConvertToHalfPass() override = default;

  IRContext::Analysis GetPreservedAnalyses() override {
    return IRContext::kAnalysisDefUse | IRContext::kAnalysisInstrToBlockMapping;
  }


  Status Process() override;

  const char* name() const override { return "convert-to-half-pass"; }

 private:


  bool IsArithmetic(Instruction* inst);



  bool IsFloat(Instruction* inst, uint32_t width);
  bool IsStruct(Instruction* inst);


  bool IsDecoratedRelaxed(Instruction* inst);


  bool IsRelaxed(uint32_t id);


  void AddRelaxed(uint32_t id);


  bool CanRelaxOpOperands(Instruction* inst);


  analysis::Type* FloatScalarType(uint32_t width);


  analysis::Type* FloatVectorType(uint32_t v_len, uint32_t width);



  analysis::Type* FloatMatrixType(uint32_t v_cnt, uint32_t vty_id,
                                  uint32_t width);


  uint32_t EquivFloatTypeId(uint32_t ty_id, uint32_t width);



  void GenConvert(uint32_t* val_idp, uint32_t width, Instruction* inst);


  bool RemoveRelaxedDecoration(uint32_t id);




  bool CloseRelaxInst(Instruction* inst);







  bool GenHalfInst(Instruction* inst);


  bool GenHalfArith(Instruction* inst);


  bool ProcessPhi(Instruction* inst, uint32_t from_width, uint32_t to_width);


  bool ProcessConvert(Instruction* inst);


  bool ProcessImageRef(Instruction* inst);


  bool ProcessDefault(Instruction* inst);





  bool MatConvertCleanup(Instruction* inst);




  bool ProcessFunction(Function* func);

  Pass::Status ProcessImpl();


  void Initialize();

  struct hasher {
    size_t operator()(const spv::Op& op) const noexcept {
      return std::hash<uint32_t>()(uint32_t(op));
    }
  };


  Pass::Status status_;


  std::unordered_set<spv::Op, hasher> target_ops_core_;


  std::unordered_set<uint32_t> target_ops_450_;


  std::unordered_set<spv::Op, hasher> image_ops_;


  std::unordered_set<spv::Op, hasher> dref_image_ops_;


  std::unordered_set<spv::Op, hasher> coordinate_image_ops_;


  std::unordered_set<spv::Op, hasher> closure_ops_;


  std::unordered_set<uint32_t> relaxed_ids_set_;


  std::unordered_set<uint32_t> converted_ids_;
};

}
}

#endif
