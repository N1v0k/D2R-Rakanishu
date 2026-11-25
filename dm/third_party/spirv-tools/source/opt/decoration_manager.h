// Copyright (c) 2017 Pierre Moreau
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

#ifndef SOURCE_OPT_DECORATION_MANAGER_H_
#define SOURCE_OPT_DECORATION_MANAGER_H_

#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "source/opt/instruction.h"
#include "source/opt/module.h"

namespace spvtools {
namespace opt {
namespace analysis {


class DecorationManager {
 public:

  explicit DecorationManager(Module* module) : module_(module) {
    AnalyzeDecorations();
  }
  DecorationManager() = delete;


















  bool RemoveDecorationsFrom(
      uint32_t id, std::function<bool(const Instruction&)> pred =
                       [](const Instruction&) { return true; });





  void RemoveDecoration(Instruction* inst);





  std::vector<Instruction*> GetDecorationsFor(uint32_t id,
                                              bool include_linkage);
  std::vector<const Instruction*> GetDecorationsFor(uint32_t id,
                                                    bool include_linkage) const;



  bool HaveTheSameDecorations(uint32_t id1, uint32_t id2) const;




  bool HaveSubsetOfDecorations(uint32_t id1, uint32_t id2) const;







  bool AreDecorationsTheSame(const Instruction* inst1, const Instruction* inst2,
                             bool ignore_target) const;



  bool HasDecoration(uint32_t id, uint32_t decoration) const;
  bool HasDecoration(uint32_t id, spv::Decoration decoration) const;




  void ForEachDecoration(uint32_t id, uint32_t decoration,
                         std::function<void(const Instruction&)> f) const;





  bool WhileEachDecoration(uint32_t id, uint32_t decoration,
                           std::function<bool(const Instruction&)> f) const;





  bool FindDecoration(uint32_t id, uint32_t decoration,
                      std::function<bool(const Instruction&)> f);





  void CloneDecorations(uint32_t from, uint32_t to);




  void CloneDecorations(
      uint32_t from, uint32_t to,
      const std::vector<spv::Decoration>& decorations_to_copy);


  void AddDecoration(Instruction* inst);


  void AddDecoration(spv::Op opcode, const std::vector<Operand> opnds);


  void AddDecoration(uint32_t inst_id, uint32_t decoration);


  void AddDecorationVal(uint32_t inst_id, uint32_t decoration,
                        uint32_t decoration_value);


  void AddMemberDecoration(uint32_t inst_id, uint32_t member,
                           uint32_t decoration, uint32_t decoration_value);

  friend bool operator==(const DecorationManager&, const DecorationManager&);
  friend bool operator!=(const DecorationManager& lhs,
                         const DecorationManager& rhs) {
    return !(lhs == rhs);
  }

 private:


  void AnalyzeDecorations();

  template <typename T>
  std::vector<T> InternalGetDecorationsFor(uint32_t id, bool include_linkage);


  struct TargetData {
    std::vector<Instruction*> direct_decorations;


    std::vector<Instruction*> indirect_decorations;


    std::vector<Instruction*> decorate_insts;






  };

  friend bool operator==(const TargetData& lhs, const TargetData& rhs) {
    if (!std::is_permutation(lhs.direct_decorations.begin(),
                             lhs.direct_decorations.end(),
                             rhs.direct_decorations.begin())) {
      return false;
    }
    if (!std::is_permutation(lhs.indirect_decorations.begin(),
                             lhs.indirect_decorations.end(),
                             rhs.indirect_decorations.begin())) {
      return false;
    }
    if (!std::is_permutation(lhs.decorate_insts.begin(),
                             lhs.decorate_insts.end(),
                             rhs.decorate_insts.begin())) {
      return false;
    }
    return true;
  }






  std::unordered_map<uint32_t, TargetData> id_to_decoration_insts_;

  Module* module_;
};

}
}
}

#endif
