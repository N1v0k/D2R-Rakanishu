// Copyright (c) 2025 LunarG Inc.
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

#include <algorithm>
#include <map>
#include <set>
#include <vector>

#include "source/opt/pass.h"

namespace spvtools {
namespace opt {





















class CanonicalizeIdsPass : public Pass {
 public:
  CanonicalizeIdsPass() = default;
  virtual ~CanonicalizeIdsPass() = default;

  Pass::Status Process() override;

  const char* name() const override { return "canonicalize-ids"; }

 private:

  static constexpr spv::Id unmapped_{spv::Id(-10000)};
  static constexpr spv::Id unused_{spv::Id(-10001)};


  void ScanIds();


  void CanonicalizeTypeAndConst();
  spv::Id HashTypeAndConst(
      spv::Id const id) const;
  void CanonicalizeNames();
  void CanonicalizeFunctions();
  spv::Id HashOpCode(Instruction const* const inst)
      const;
  void CanonicalizeRemainders();


  bool ApplyMap();


  spv::Id GetBound() const;
  void UpdateBound();


  spv::Id GetNewId(spv::Id const old_id) const { return new_id_[old_id]; }
  spv::Id SetNewId(spv::Id const old_id, spv::Id new_id);


  spv::Id ClaimNewId(spv::Id new_id);
  bool IsNewIdClaimed(spv::Id const new_id) const {
    return claimed_new_ids_.find(new_id) != claimed_new_ids_.end();
  }


  bool IsOldIdUnmapped(spv::Id const old_id) const {
    return GetNewId(old_id) == unmapped_;
  }
  bool IsOldIdUnused(spv::Id const old_id) const {
    return GetNewId(old_id) == unused_;
  }


  std::vector<spv::Id> new_id_;



  std::set<spv::Id> claimed_new_ids_;


  std::string IdAsString(spv::Id const id) const;
  void PrintNewIds() const;


  std::vector<spv::Id> type_and_const_ids_;
  std::map<std::string, spv::Id> name_ids_;
  std::vector<spv::Id> function_ids_;
};

}
}
