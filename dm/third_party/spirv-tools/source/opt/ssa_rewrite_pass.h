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

#ifndef SOURCE_OPT_SSA_REWRITE_PASS_H_
#define SOURCE_OPT_SSA_REWRITE_PASS_H_

#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "source/opt/basic_block.h"
#include "source/opt/ir_context.h"
#include "source/opt/mem_pass.h"

namespace spvtools {
namespace opt {









class SSARewriter {
 public:
  SSARewriter(MemPass* pass) : pass_(pass) {}







  Pass::Status RewriteFunctionIntoSSA(Function* fp);

 private:
  class PhiCandidate {
   public:
    explicit PhiCandidate(uint32_t var, uint32_t result, BasicBlock* block)
        : var_id_(var),
          result_id_(result),
          bb_(block),
          phi_args_(),
          copy_of_(0),
          is_complete_(false),
          users_() {}

    uint32_t var_id() const { return var_id_; }
    uint32_t result_id() const { return result_id_; }
    BasicBlock* bb() const { return bb_; }
    std::vector<uint32_t>& phi_args() { return phi_args_; }
    const std::vector<uint32_t>& phi_args() const { return phi_args_; }
    uint32_t copy_of() const { return copy_of_; }
    bool is_complete() const { return is_complete_; }
    std::vector<uint32_t>& users() { return users_; }
    const std::vector<uint32_t>& users() const { return users_; }


    void MarkCopyOf(uint32_t orig_id) { copy_of_ = orig_id; }


    void MarkIncomplete() { is_complete_ = false; }


    void MarkComplete() { is_complete_ = true; }


    bool IsReady() const { return is_complete() && copy_of() == 0; }



    std::string PrettyPrint(const CFG* cfg) const;


    void AddUser(uint32_t operand_id) { users_.push_back(operand_id); }

   private:

    uint32_t var_id_;



    uint32_t result_id_;


    BasicBlock* bb_;




    std::vector<uint32_t> phi_args_;



    uint32_t copy_of_;



    bool is_complete_;




    std::vector<uint32_t> users_;
  };


  typedef std::unordered_map<BasicBlock*,
                             std::unordered_map<uint32_t, uint32_t>>
      BlockDefsMap;




  bool GenerateSSAReplacements(BasicBlock* bb);



  void SealBlock(BasicBlock* bb);


  bool IsBlockSealed(BasicBlock* bb) { return sealed_blocks_.count(bb) != 0; }



  PhiCandidate* GetPhiCandidate(uint32_t id) {
    auto it = phi_candidates_.find(id);
    return (it != phi_candidates_.end()) ? &it->second : nullptr;
  }



  void ReplacePhiUsersWith(const PhiCandidate& phi_cand, uint32_t repl_id);





















  uint32_t GetReplacement(std::pair<uint32_t, uint32_t> repl);







  uint32_t GetPhiArgument(const PhiCandidate* phi_candidate, uint32_t ix);




  bool ApplyReplacements();



  void WriteVariable(uint32_t var_id, BasicBlock* bb, uint32_t val_id) {
    defs_at_block_[bb][var_id] = val_id;
    if (auto* pc = GetPhiCandidate(val_id)) {
      pc->AddUser(bb->id());
    }
  }



  uint32_t GetValueAtBlock(uint32_t var_id, BasicBlock* bb);





  void ProcessStore(Instruction* inst, BasicBlock* bb);





  bool ProcessLoad(Instruction* inst, BasicBlock* bb);







  uint32_t GetReachingDef(uint32_t var_id, BasicBlock* bb);






  uint32_t AddPhiOperands(PhiCandidate* phi_candidate);










  PhiCandidate* CreatePhiCandidate(uint32_t var_id, BasicBlock* bb);








  uint32_t TryRemoveTrivialPhi(PhiCandidate* phi_cand);



  void FinalizePhiCandidate(PhiCandidate* phi_candidate);





  void FinalizePhiCandidates();


  void PrintPhiCandidates() const;


  void PrintReplacementTable() const;





  BlockDefsMap defs_at_block_;




  std::unordered_map<uint32_t, PhiCandidate> phi_candidates_;




  std::queue<PhiCandidate*> incomplete_phis_;



  std::vector<PhiCandidate*> phis_to_generate_;





  std::unordered_map<uint32_t, uint32_t> load_replacement_;


  std::unordered_set<BasicBlock*> sealed_blocks_;


  MemPass* pass_;
};

class SSARewritePass : public MemPass {
 public:
  SSARewritePass() = default;

  const char* name() const override { return "ssa-rewrite"; }
  Status Process() override;
};

}
}

#endif
