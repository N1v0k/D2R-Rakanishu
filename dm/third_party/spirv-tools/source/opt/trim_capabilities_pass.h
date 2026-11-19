// Copyright (c) 2023 Google Inc.
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

#ifndef SOURCE_OPT_TRIM_CAPABILITIES_PASS_H_
#define SOURCE_OPT_TRIM_CAPABILITIES_PASS_H_

#include <algorithm>
#include <array>
#include <functional>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include "source/enum_set.h"
#include "source/extensions.h"
#include "source/opt/ir_context.h"
#include "source/opt/module.h"
#include "source/opt/pass.h"
#include "source/spirv_target_env.h"
#include "source/table2.h"

namespace spvtools {
namespace opt {



struct ClassEnumHash {
  std::size_t operator()(spv::Capability value) const {
    using StoringType = typename std::underlying_type_t<spv::Capability>;
    return std::hash<StoringType>{}(static_cast<StoringType>(value));
  }

  std::size_t operator()(spv::Op value) const {
    using StoringType = typename std::underlying_type_t<spv::Op>;
    return std::hash<StoringType>{}(static_cast<StoringType>(value));
  }
};










using OpcodeHandler =
    std::optional<spv::Capability> (*)(const Instruction* instruction);











class TrimCapabilitiesPass : public Pass {
 private:


  static constexpr std::array kSupportedCapabilities{

      spv::Capability::ComputeDerivativeGroupLinearKHR,
      spv::Capability::ComputeDerivativeGroupQuadsKHR,
      spv::Capability::Float16,
      spv::Capability::Float64,
      spv::Capability::FragmentShaderPixelInterlockEXT,
      spv::Capability::FragmentShaderSampleInterlockEXT,
      spv::Capability::FragmentShaderShadingRateInterlockEXT,
      spv::Capability::Geometry,
      spv::Capability::GroupNonUniform,
      spv::Capability::GroupNonUniformArithmetic,
      spv::Capability::GroupNonUniformClustered,
      spv::Capability::GroupNonUniformPartitionedNV,
      spv::Capability::GroupNonUniformVote,
      spv::Capability::Groups,
      spv::Capability::ImageMSArray,
      spv::Capability::Int16,
      spv::Capability::Int64,
      spv::Capability::InterpolationFunction,
      spv::Capability::Linkage,
      spv::Capability::MinLod,
      spv::Capability::PhysicalStorageBufferAddresses,
      spv::Capability::RayQueryKHR,
      spv::Capability::RayTracingKHR,
      spv::Capability::RayTraversalPrimitiveCullingKHR,
      spv::Capability::Shader,
      spv::Capability::ShaderClockKHR,
      spv::Capability::StorageBuffer16BitAccess,
      spv::Capability::StorageImageReadWithoutFormat,
      spv::Capability::StorageImageWriteWithoutFormat,
      spv::Capability::StorageInputOutput16,
      spv::Capability::StoragePushConstant16,
      spv::Capability::StorageUniform16,
      spv::Capability::StorageUniformBufferBlock16,
      spv::Capability::VulkanMemoryModelDeviceScope,
      spv::Capability::QuadControlKHR,

  };


  static constexpr std::array kForbiddenCapabilities{
      spv::Capability::Linkage,
  };



  static constexpr std::array kUntouchableCapabilities{
      spv::Capability::Shader,
  };

 public:
  TrimCapabilitiesPass();
  TrimCapabilitiesPass(const TrimCapabilitiesPass&) = delete;
  TrimCapabilitiesPass(TrimCapabilitiesPass&&) = delete;

 private:


  template <typename Descriptor>
  void addSupportedCapabilitiesToSet(const Descriptor* const descriptor,
                                     CapabilitySet* output) const {
    for (auto capability : descriptor->capabilities()) {
      if (supportedCapabilities_.contains(capability)) {
        output->insert(capability);
      }
    }
  }




  template <class Descriptor>
  inline void addSupportedExtensionsToSet(const Descriptor* const descriptor,
                                          ExtensionSet* output) const {
    if (descriptor->minVersion <=
        spvVersionForTargetEnv(context()->GetTargetEnv())) {
      return;
    }
    output->insert(descriptor->extensions().begin(),
                   descriptor->extensions().end());
  }

  void addInstructionRequirementsForOpcode(spv::Op opcode,
                                           CapabilitySet* capabilities,
                                           ExtensionSet* extensions) const;
  void addInstructionRequirementsForOperand(const Operand& operand,
                                            CapabilitySet* capabilities,
                                            ExtensionSet* extensions) const;

  void addInstructionRequirementsForExtInst(Instruction* instruction,
                                            CapabilitySet* capabilities) const;




  void addInstructionRequirements(Instruction* instruction,
                                  CapabilitySet* capabilities,
                                  ExtensionSet* extensions) const;



  void AddExtensionsForOperand(const spv_operand_type_t type,
                               const uint32_t value,
                               ExtensionSet* extensions) const;



  std::pair<CapabilitySet, ExtensionSet>
  DetermineRequiredCapabilitiesAndExtensions() const;



  Pass::Status TrimUnrequiredCapabilities(
      const CapabilitySet& required_capabilities) const;




  Pass::Status TrimUnrequiredExtensions(
      const ExtensionSet& required_extensions) const;


  bool HasForbiddenCapabilities() const;

 public:
  const char* name() const override { return "trim-capabilities"; }
  Status Process() override;

 private:
  const CapabilitySet supportedCapabilities_;
  const CapabilitySet forbiddenCapabilities_;
  const CapabilitySet untouchableCapabilities_;
  const std::unordered_multimap<spv::Op, OpcodeHandler, ClassEnumHash>
      opcodeHandlers_;
};

}
}
#endif
