// Copyright (c) 2021 Google LLC
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

#ifndef SOURCE_OPT_CONVERT_TO_SAMPLED_IMAGE_PASS_H_
#define SOURCE_OPT_CONVERT_TO_SAMPLED_IMAGE_PASS_H_

#include <memory>
#include <unordered_set>
#include <utility>

#include "source/opt/pass.h"
#include "source/opt/types.h"

namespace spvtools {
namespace opt {


struct DescriptorSetAndBinding {
  uint32_t descriptor_set;
  uint32_t binding;

  bool operator==(const DescriptorSetAndBinding& descriptor_set_binding) const {
    return descriptor_set_binding.descriptor_set == descriptor_set &&
           descriptor_set_binding.binding == binding;
  }
};


class ConvertToSampledImagePass : public Pass {
 public:

  struct DescriptorSetAndBindingHash {
    size_t operator()(
        const DescriptorSetAndBinding& descriptor_set_binding) const {
      return std::hash<uint32_t>()(descriptor_set_binding.descriptor_set) ^
             std::hash<uint32_t>()(descriptor_set_binding.binding);
    }
  };

  using SetOfDescriptorSetAndBindingPairs =
      std::unordered_set<DescriptorSetAndBinding, DescriptorSetAndBindingHash>;
  using DescriptorSetBindingToInstruction =
      std::unordered_map<DescriptorSetAndBinding, Instruction*,
                         DescriptorSetAndBindingHash>;

  explicit ConvertToSampledImagePass(
      const std::vector<DescriptorSetAndBinding>& descriptor_set_binding_pairs)
      : descriptor_set_binding_pairs_(descriptor_set_binding_pairs.begin(),
                                      descriptor_set_binding_pairs.end()) {}

  const char* name() const override { return "convert-to-sampled-image"; }
  Status Process() override;


























  static std::unique_ptr<std::vector<DescriptorSetAndBinding>>
  ParseDescriptorSetBindingPairsString(const char* str);

 private:





  bool CollectResourcesToConvert(
      DescriptorSetBindingToInstruction* descriptor_set_binding_pair_to_sampler,
      DescriptorSetBindingToInstruction* descriptor_set_binding_pair_to_image)
      const;





  bool GetDescriptorSetBinding(
      const Instruction& inst,
      DescriptorSetAndBinding* descriptor_set_binding) const;




  bool ShouldResourceBeConverted(
      const DescriptorSetAndBinding& descriptor_set_binding) const;



  const analysis::Type* GetVariableType(const Instruction& variable) const;


  spv::StorageClass GetStorageClass(const Instruction& variable) const;




  void FindUses(const Instruction* inst, std::vector<Instruction*>* uses,
                spv::Op user_opcode) const;



  void FindUsesOfImage(const Instruction* image,
                       std::vector<Instruction*>* uses) const;



  Instruction* CreateImageExtraction(Instruction* sampled_image);









  Pass::Status UpdateImageVariableToSampledImage(
      Instruction* image_variable,
      const DescriptorSetAndBinding& descriptor_set_binding);



  uint32_t GetSampledImageTypeForImage(Instruction* image_variable);


  void MoveInstructionNextToType(Instruction* inst, uint32_t type_id);




  bool ConvertImageVariableToSampledImage(Instruction* image_variable,
                                          uint32_t sampled_image_type_id);




  Instruction* UpdateImageUses(Instruction* sampled_image_load);



  bool IsSamplerOfSampledImageDecoratedByDescriptorSetBinding(
      Instruction* sampled_image_inst,
      const DescriptorSetAndBinding& descriptor_set_binding);





  void UpdateSampledImageUses(
      Instruction* image_load, Instruction* image_extraction,
      const DescriptorSetAndBinding& image_descriptor_set_binding);





  Pass::Status CheckUsesOfSamplerVariable(
      const Instruction* sampler_variable,
      Instruction* image_to_be_combined_with);



  bool DoesSampledImageReferenceImage(Instruction* sampled_image_inst,
                                      Instruction* image_variable);





  const SetOfDescriptorSetAndBindingPairs descriptor_set_binding_pairs_;
};

}
}

#endif
