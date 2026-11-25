// Copyright (c) 2022 Google LLC
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

#ifndef SOURCE_OPT_INTERFACE_VAR_SROA_H_
#define SOURCE_OPT_INTERFACE_VAR_SROA_H_

#include <optional>
#include <unordered_set>

#include "source/opt/pass.h"

namespace spvtools {
namespace opt {






class InterfaceVariableScalarReplacement : public Pass {
 public:
  InterfaceVariableScalarReplacement() {}

  const char* name() const override {
    return "interface-variable-scalar-replacement";
  }
  Status Process() override;

  IRContext::Analysis GetPreservedAnalyses() override {
    return IRContext::kAnalysisDecorations | IRContext::kAnalysisDefUse |
           IRContext::kAnalysisConstants | IRContext::kAnalysisTypes;
  }

 private:







  struct NestedCompositeComponents {
    NestedCompositeComponents() : component_variable(nullptr) {}

    bool HasMultipleComponents() const {
      return !nested_composite_components.empty();
    }

    const std::vector<NestedCompositeComponents>& GetComponents() const {
      return nested_composite_components;
    }

    void AddComponent(const NestedCompositeComponents& component) {
      nested_composite_components.push_back(component);
    }

    Instruction* GetComponentVariable() const { return component_variable; }

    void SetSingleComponentVariable(Instruction* var) {
      component_variable = var;
    }

   private:
    std::vector<NestedCompositeComponents> nested_composite_components;
    Instruction* component_variable;
  };


  std::vector<Instruction*> CollectInterfaceVariables(Instruction& entry_point);



  bool HasExtraArrayness(Instruction& entry_point, Instruction* var);



  bool GetVariableLocation(Instruction* var, uint32_t* location);



  bool GetVariableComponent(Instruction* var, uint32_t* component);


  Instruction* GetTypeOfVariable(Instruction* var);








  Status ReplaceInterfaceVariableWithScalars(Instruction* interface_var,
                                             Instruction* interface_var_type,
                                             uint32_t location,
                                             uint32_t component,
                                             uint32_t extra_array_length);





  std::optional<NestedCompositeComponents>
  CreateScalarInterfaceVarsForReplacement(Instruction* interface_var_type,
                                          spv::StorageClass storage_class,
                                          uint32_t extra_array_length);





  std::optional<NestedCompositeComponents> CreateScalarInterfaceVarsForArray(
      Instruction* interface_var_type, spv::StorageClass storage_class,
      uint32_t extra_array_length);





  std::optional<NestedCompositeComponents> CreateScalarInterfaceVarsForMatrix(
      Instruction* interface_var_type, spv::StorageClass storage_class,
      uint32_t extra_array_length);




  void AddLocationAndComponentDecorations(const NestedCompositeComponents& vars,
                                          uint32_t* location,
                                          uint32_t component);






  Status ReplaceInterfaceVarWith(
      Instruction* interface_var, uint32_t extra_arrayness,
      const NestedCompositeComponents& scalar_interface_vars);










  Status ReplaceComponentsOfInterfaceVarWith(
      Instruction* interface_var,
      const std::vector<Instruction*>& interface_var_users,
      const NestedCompositeComponents& scalar_interface_vars,
      std::vector<uint32_t>& interface_var_component_indices,
      const uint32_t* extra_array_index,
      std::unordered_map<Instruction*, Instruction*>* loads_to_composites,
      std::unordered_map<Instruction*, Instruction*>*
          loads_for_access_chain_to_composites);










  Status ReplaceMultipleComponentsOfInterfaceVarWith(
      Instruction* interface_var,
      const std::vector<Instruction*>& interface_var_users,
      const std::vector<NestedCompositeComponents>& components,
      std::vector<uint32_t>& interface_var_component_indices,
      const uint32_t* extra_array_index,
      std::unordered_map<Instruction*, Instruction*>* loads_to_composites,
      std::unordered_map<Instruction*, Instruction*>*
          loads_for_access_chain_to_composites);









  Status ReplaceComponentOfInterfaceVarWith(
      Instruction* interface_var, Instruction* interface_var_user,
      Instruction* scalar_var,
      const std::vector<uint32_t>& interface_var_component_indices,
      const uint32_t* extra_array_index,
      std::unordered_map<Instruction*, Instruction*>* loads_to_component_values,
      std::unordered_map<Instruction*, Instruction*>*
          loads_for_access_chain_to_component_values);





  Instruction* LoadScalarVar(Instruction* scalar_var,
                             const uint32_t* extra_array_index,
                             Instruction* insert_before);




  Instruction* LoadAccessChainToVar(Instruction* var,
                                    const std::vector<uint32_t>& indexes,
                                    Instruction* insert_before);







  void StoreComponentOfValueToAccessChainToScalarVar(
      uint32_t value_id, const std::vector<uint32_t>& component_indices,
      Instruction* scalar_var,
      const std::vector<uint32_t>& access_chain_indices,
      Instruction* insert_before);





  void StoreComponentOfValueToScalarVar(
      uint32_t value_id, const std::vector<uint32_t>& component_indices,
      Instruction* scalar_var, const uint32_t* extra_array_index,
      Instruction* insert_before);






  void StoreComponentOfValueTo(uint32_t component_type_id, uint32_t value_id,
                               const std::vector<uint32_t>& component_indices,
                               Instruction* ptr,
                               const uint32_t* extra_array_index,
                               Instruction* insert_before);





  Instruction* CreateCompositeExtract(uint32_t type_id, uint32_t composite_id,
                                      const std::vector<uint32_t>& indexes,
                                      const uint32_t* extra_first_index);



  Instruction* CreateLoad(uint32_t type_id, Instruction* ptr,
                          Instruction* insert_before);



  void CloneAnnotationForVariable(Instruction* annotation_inst,
                                  uint32_t var_id);





  bool ReplaceInterfaceVarInEntryPoint(Instruction* interface_var,
                                       Instruction* entry_point,
                                       uint32_t scalar_var_id);





  Instruction* CreateAccessChainWithIndex(uint32_t component_type_id,
                                          Instruction* var, uint32_t index,
                                          Instruction* insert_before);


  uint32_t GetPointeeTypeIdOfVar(Instruction* var);







  void ReplaceAccessChainWith(
      Instruction* access_chain,
      const std::vector<uint32_t>& interface_var_component_indices,
      Instruction* scalar_var,
      std::unordered_map<Instruction*, Instruction*>*
          loads_to_component_values);





  void UseBaseAccessChainForAccessChain(Instruction* access_chain,
                                        Instruction* base_access_chain);









  void AddComponentsToCompositesForLoads(
      const std::unordered_map<Instruction*, Instruction*>&
          loads_to_component_values,
      std::unordered_map<Instruction*, Instruction*>* loads_to_composites,
      uint32_t depth_to_component);




  Instruction* CreateCompositeConstructForComponentOfLoad(
      Instruction* load, uint32_t depth_to_component);






  Instruction* CreateAccessChainToVar(uint32_t var_type_id, Instruction* var,
                                      const std::vector<uint32_t>& index_ids,
                                      Instruction* insert_before,
                                      uint32_t* component_type_id);



  uint32_t GetArrayType(uint32_t elem_type_id, uint32_t array_length);



  uint32_t GetPointerType(uint32_t type_id, spv::StorageClass storage_class);


  void KillInstructionAndUsers(Instruction* inst);


  void KillInstructionsAndUsers(const std::vector<Instruction*>& insts);



  void KillLocationAndComponentDecorations(uint32_t var_id);



  bool ReportErrorIfHasExtraArraynessForOtherEntry(Instruction* var);



  bool ReportErrorIfHasNoExtraArraynessForOtherEntry(Instruction* var);





  bool CheckExtraArraynessConflictBetweenEntries(Instruction* interface_var,
                                                 bool has_extra_arrayness);



  Pass::Status ReplaceInterfaceVarsWithScalars(Instruction& entry_point);



  std::unordered_set<uint32_t>
      interface_vars_removed_from_entry_point_operands_;




  std::unordered_map<uint32_t, uint32_t> composite_ids_to_component_depths;



  std::unordered_set<Instruction*> vars_with_extra_arrayness;



  std::unordered_set<Instruction*> vars_without_extra_arrayness;


  uint32_t TakeNextId() { return context()->TakeNextId(); }
};

}
}

#endif
