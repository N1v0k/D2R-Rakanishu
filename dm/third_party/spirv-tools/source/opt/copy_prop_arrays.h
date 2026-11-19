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

#ifndef SOURCE_OPT_COPY_PROP_ARRAYS_H_
#define SOURCE_OPT_COPY_PROP_ARRAYS_H_

#include <memory>
#include <vector>

#include "source/opt/mem_pass.h"

namespace spvtools {
namespace opt {
















class CopyPropagateArrays : public MemPass {
 public:
  const char* name() const override { return "copy-propagate-arrays"; }
  Status Process() override;

  IRContext::Analysis GetPreservedAnalyses() override {
    return IRContext::kAnalysisDefUse | IRContext::kAnalysisCFG |
           IRContext::kAnalysisInstrToBlockMapping |
           IRContext::kAnalysisLoopAnalysis | IRContext::kAnalysisDecorations |
           IRContext::kAnalysisDominatorAnalysis | IRContext::kAnalysisNameMap |
           IRContext::kAnalysisConstants | IRContext::kAnalysisTypes;
  }

 private:




  struct AccessChainEntry {
    bool is_result_id;
    union {
      uint32_t result_id;
      uint32_t immediate;
    };

    bool operator!=(const AccessChainEntry& other) const {
      return other.is_result_id != is_result_id || other.result_id != result_id;
    }
  };





  class MemoryObject {
   public:





    template <class iterator>
    MemoryObject(Instruction* var_inst, iterator begin, iterator end);





    void PushIndirection(const std::vector<AccessChainEntry>& access_chain);




    void PopIndirection() {
      assert(IsMember());
      access_chain_.pop_back();
    }



    bool IsMember() const { return !access_chain_.empty(); }




    uint32_t GetNumberOfMembers();


    Instruction* GetVariable() const { return variable_inst_; }





    const std::vector<AccessChainEntry>& AccessChain() const {
      return access_chain_;
    }




    bool BuildConstants();



    uint32_t GetPointerTypeId(const CopyPropagateArrays* pass) const {
      analysis::DefUseManager* def_use_mgr =
          GetVariable()->context()->get_def_use_mgr();
      analysis::TypeManager* type_mgr =
          GetVariable()->context()->get_type_mgr();

      Instruction* var_pointer_inst =
          def_use_mgr->GetDef(GetVariable()->type_id());

      uint32_t member_type_id = pass->GetMemberTypeId(
          var_pointer_inst->GetSingleWordInOperand(1), GetAccessIds());

      uint32_t member_pointer_type_id = type_mgr->FindPointerToType(
          member_type_id, static_cast<spv::StorageClass>(
                              var_pointer_inst->GetSingleWordInOperand(0)));
      return member_pointer_type_id;
    }


    spv::StorageClass GetStorageClass() const {
      analysis::TypeManager* type_mgr =
          GetVariable()->context()->get_type_mgr();
      const analysis::Pointer* pointer_type =
          type_mgr->GetType(GetVariable()->type_id())->AsPointer();
      return pointer_type->storage_class();
    }



    bool Contains(MemoryObject* other);

   private:

    Instruction* variable_inst_;




    std::vector<AccessChainEntry> access_chain_;
    std::vector<uint32_t> GetAccessIds() const;
  };






  std::unique_ptr<CopyPropagateArrays::MemoryObject> FindSourceObjectIfPossible(
      Instruction* var_inst, Instruction* store_inst);





  bool PropagateObject(Instruction* var_inst, MemoryObject* source,
                       Instruction* insertion_pos);



  bool HasValidReferencesOnly(Instruction* ptr_inst, Instruction* store_inst);



  std::unique_ptr<MemoryObject> GetSourceObjectIfAny(uint32_t result);




  std::unique_ptr<MemoryObject> BuildMemoryObjectFromLoad(
      Instruction* load_inst);





  std::unique_ptr<MemoryObject> BuildMemoryObjectFromExtract(
      Instruction* extract_inst);





  std::unique_ptr<MemoryObject> BuildMemoryObjectFromCompositeConstruct(
      Instruction* conststruct_inst);







  std::unique_ptr<MemoryObject> BuildMemoryObjectFromInsert(
      Instruction* insert_inst);


  bool IsAccessChainIndexValidAndEqualTo(const AccessChainEntry& entry,
                                         uint32_t value) const;


  bool IsPointerToArrayType(uint32_t type_id);



  bool IsInterpolationInstruction(Instruction* inst);



  bool HasNoStores(Instruction* ptr_inst);





  Instruction* BuildNewAccessChain(Instruction* insertion_point,
                                   MemoryObject* source) const;





  bool UpdateUses(Instruction* original_ptr_inst,
                  Instruction* new_pointer_inst);



  bool CanUpdateUses(Instruction* original_ptr_inst, uint32_t type_id);




  Instruction* FindStoreInstruction(const Instruction* var_inst) const;




  uint32_t GetMemberTypeId(uint32_t id,
                           const std::vector<uint32_t>& access_chain) const;



  void AddUsesToWorklist(Instruction* inst);



  std::queue<Instruction*> worklist_;
};

}
}

#endif
