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

#ifndef SOURCE_OPT_FEATURE_MANAGER_H_
#define SOURCE_OPT_FEATURE_MANAGER_H_

#include "source/assembly_grammar.h"
#include "source/extensions.h"
#include "source/opt/module.h"

namespace spvtools {
namespace opt {


class FeatureManager {
 public:

  bool HasExtension(Extension ext) const { return extensions_.contains(ext); }


  bool HasCapability(spv::Capability cap) const {
    return capabilities_.contains(cap);
  }


  inline const CapabilitySet& GetCapabilities() const { return capabilities_; }


  inline const ExtensionSet& GetExtensions() const { return extensions_; }

  uint32_t GetExtInstImportId_GLSLstd450() const {
    return extinst_importid_GLSLstd450_;
  }

  uint32_t GetExtInstImportId_OpenCL100DebugInfo() const {
    return extinst_importid_OpenCL100DebugInfo_;
  }

  uint32_t GetExtInstImportId_ShaderDebugInfo() const {
    return extinst_importid_ShaderDebugInfo_;
  }

  friend bool operator==(const FeatureManager& a, const FeatureManager& b);
  friend bool operator!=(const FeatureManager& a, const FeatureManager& b) {
    return !(a == b);
  }

 private:
  explicit FeatureManager(const AssemblyGrammar& grammar) : grammar_(grammar) {}


  void Analyze(Module* module);


  void AddExtension(Instruction* ext);


  void AddExtensions(Module* module);


  void RemoveExtension(Extension extension);



  void AddCapability(spv::Capability capability);


  void AddCapabilities(Module* module);


  void RemoveCapability(spv::Capability capability);


  void AddExtInstImportIds(Module* module);


  const AssemblyGrammar& grammar_;


  ExtensionSet extensions_;


  CapabilitySet capabilities_;


  uint32_t extinst_importid_GLSLstd450_ = 0;



  uint32_t extinst_importid_OpenCL100DebugInfo_ = 0;



  uint32_t extinst_importid_ShaderDebugInfo_ = 0;

  friend class IRContext;
};

}
}

#endif
