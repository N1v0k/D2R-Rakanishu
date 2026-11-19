// Copyright (c) 2018 Google LLC
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

#ifndef LIBSPIRV_OPT_UPGRADE_MEMORY_MODEL_H_
#define LIBSPIRV_OPT_UPGRADE_MEMORY_MODEL_H_

#include <functional>
#include <tuple>

#include "pass.h"

namespace spvtools {
namespace opt {


struct CacheHash {
  size_t operator()(
      const std::pair<uint32_t, std::vector<uint32_t>>& item) const {
    std::u32string to_hash;
    to_hash.push_back(item.first);
    for (auto i : item.second) to_hash.push_back(i);
    return std::hash<std::u32string>()(to_hash);
  }
};







class UpgradeMemoryModel : public Pass {
 public:
  const char* name() const override { return "upgrade-memory-model"; }
  Status Process() override;

 private:


  enum OperationType { kVisibility, kAvailability };


  enum InstructionType { kMemory, kImage };



  void UpgradeMemoryModelInstruction();






  void UpgradeInstructions();


  void UpgradeMemoryAndImages();


  void UpgradeAtomics();


  std::tuple<bool, bool, spv::Scope> GetInstructionAttributes(uint32_t id);



  std::pair<bool, bool> TraceInstruction(Instruction* inst,
                                         std::vector<uint32_t> indices,
                                         std::unordered_set<uint32_t>* visited);





  bool HasDecoration(const Instruction* inst, uint32_t value,
                     spv::Decoration decoration);



  std::pair<bool, bool> CheckType(uint32_t type_id,
                                  const std::vector<uint32_t>& indices);


  std::pair<bool, bool> CheckAllTypes(const Instruction* inst);





  void UpgradeFlags(Instruction* inst, uint32_t in_operand, bool is_coherent,
                    bool is_volatile, OperationType operation_type,
                    InstructionType inst_type);



  void UpgradeSemantics(Instruction* inst, uint32_t in_operand,
                        bool is_volatile);


  uint32_t GetScopeConstant(spv::Scope scope);



  uint64_t GetIndexValue(Instruction* index_inst);


  void CleanupDecorations();




  void UpgradeBarriers();




  void UpgradeMemoryScope();


  bool IsDeviceScope(uint32_t scope_id);




  void UpgradeExtInst(Instruction* modf);



  uint32_t MemoryAccessNumWords(uint32_t mask);



  std::unordered_map<std::pair<uint32_t, std::vector<uint32_t>>,
                     std::pair<bool, bool>, CacheHash>
      cache_;
};
}
}
#endif
