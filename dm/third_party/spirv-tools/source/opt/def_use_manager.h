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

#ifndef SOURCE_OPT_DEF_USE_MANAGER_H_
#define SOURCE_OPT_DEF_USE_MANAGER_H_

#include <set>
#include <unordered_map>
#include <vector>

#include "source/opt/instruction.h"
#include "source/opt/module.h"
#include "spirv-tools/libspirv.hpp"

namespace spvtools {
namespace opt {
namespace analysis {




struct UserEntry {
  Instruction* def;
  Instruction* user;
};

inline bool operator==(const UserEntry& lhs, const UserEntry& rhs) {
  return lhs.def == rhs.def && lhs.user == rhs.user;
}










struct UserEntryLess {
  bool operator()(const UserEntry& lhs, const UserEntry& rhs) const {


    if (!lhs.def && rhs.def) return true;
    if (lhs.def && !rhs.def) return false;


    if (lhs.def && rhs.def) {
      if (lhs.def->unique_id() < rhs.def->unique_id()) return true;
      if (rhs.def->unique_id() < lhs.def->unique_id()) return false;
    }


    if (!lhs.user && !rhs.user) return false;
    if (!lhs.user) return true;
    if (!rhs.user) return false;


    return lhs.user->unique_id() < rhs.user->unique_id();
  }
};


class DefUseManager {
 public:
  using IdToDefMap = std::unordered_map<uint32_t, Instruction*>;





  DefUseManager(Module* module) { AnalyzeDefUse(module); }

  DefUseManager(const DefUseManager&) = delete;
  DefUseManager(DefUseManager&&) = delete;
  DefUseManager& operator=(const DefUseManager&) = delete;
  DefUseManager& operator=(DefUseManager&&) = delete;


  void AnalyzeInstDef(Instruction* inst);




  void AnalyzeInstUse(Instruction* inst);


  void AnalyzeInstDefUse(Instruction* inst);



  Instruction* GetDef(uint32_t id);
  const Instruction* GetDef(uint32_t id) const;








  void ForEachUser(const Instruction* def,
                   const std::function<void(Instruction*)>& f) const;
  void ForEachUser(uint32_t id,
                   const std::function<void(Instruction*)>& f) const;









  bool WhileEachUser(const Instruction* def,
                     const std::function<bool(Instruction*)>& f) const;
  bool WhileEachUser(uint32_t id,
                     const std::function<bool(Instruction*)>& f) const;








  void ForEachUse(
      const Instruction* def,
      const std::function<void(Instruction*, uint32_t operand_index)>& f) const;
  void ForEachUse(
      uint32_t id,
      const std::function<void(Instruction*, uint32_t operand_index)>& f) const;









  bool WhileEachUse(
      const Instruction* def,
      const std::function<bool(Instruction*, uint32_t operand_index)>& f) const;
  bool WhileEachUse(
      uint32_t id,
      const std::function<bool(Instruction*, uint32_t operand_index)>& f) const;


  uint32_t NumUsers(const Instruction* def) const;
  uint32_t NumUsers(uint32_t id) const;


  uint32_t NumUses(const Instruction* def) const;
  uint32_t NumUses(uint32_t id) const;






  std::vector<Instruction*> GetAnnotations(uint32_t id) const;


  const IdToDefMap& id_to_defs() const { return id_to_def_; }






  void ClearInst(Instruction* inst);


  void EraseUseRecordsOfOperandIds(const Instruction* inst);

  friend bool CompareAndPrintDifferences(const DefUseManager&,
                                         const DefUseManager&);



  void UpdateDefUse(Instruction* inst);

 private:
  using IdToUsersMap = std::set<UserEntry, UserEntryLess>;
  using InstToUsedIdsMap =
      std::unordered_map<const Instruction*, std::vector<uint32_t>>;



  IdToUsersMap::const_iterator UsersBegin(const Instruction* def) const;







  bool UsersNotEnd(const IdToUsersMap::const_iterator& iter,
                   const Instruction* def) const;
  bool UsersNotEnd(const IdToUsersMap::const_iterator& iter,
                   const IdToUsersMap::const_iterator& cached_end,
                   const Instruction* def) const;



  void AnalyzeDefUse(Module* module);

  IdToDefMap id_to_def_;
  IdToUsersMap id_to_users_;

  InstToUsedIdsMap inst_to_used_ids_;
};

}
}
}

#endif
