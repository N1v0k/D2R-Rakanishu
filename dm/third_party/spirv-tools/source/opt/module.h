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

#ifndef SOURCE_OPT_MODULE_H_
#define SOURCE_OPT_MODULE_H_

#include <functional>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "source/opt/function.h"
#include "source/opt/graph.h"
#include "source/opt/instruction.h"
#include "source/opt/iterator.h"

namespace spvtools {
namespace opt {

class IRContext;


struct ModuleHeader {
  uint32_t magic_number;
  uint32_t version;
  uint32_t generator;
  uint32_t bound;
  uint32_t schema;
};



class Module {
 public:
  using iterator = UptrVectorIterator<Function>;
  using const_iterator = UptrVectorIterator<Function, true>;
  using inst_iterator = InstructionList::iterator;
  using const_inst_iterator = InstructionList::const_iterator;
  using graph_iterator = UptrVectorIterator<Graph>;
  using const_graph_iterator = UptrVectorIterator<Graph, true>;


  Module() : header_({}), contains_debug_info_(false) {}


  void SetHeader(const ModuleHeader& header) { header_ = header; }


  void SetIdBound(uint32_t bound) {
    assert(bound != 0);
    header_.bound = bound;
  }


  uint32_t IdBound() const { return header_.bound; }







  uint32_t TakeNextIdBound();


  inline void AddCapability(std::unique_ptr<Instruction> c);


  inline void AddExtension(std::unique_ptr<Instruction> e);


  inline void AddExtInstImport(std::unique_ptr<Instruction> e);


  inline void SetMemoryModel(std::unique_ptr<Instruction> m);


  inline void SetSampledImageAddressMode(std::unique_ptr<Instruction> m);


  inline void AddEntryPoint(std::unique_ptr<Instruction> e);


  inline void AddGraphEntryPoint(std::unique_ptr<Instruction> e);


  inline void AddExecutionMode(std::unique_ptr<Instruction> e);




  inline void AddDebug1Inst(std::unique_ptr<Instruction> d);




  inline void AddDebug2Inst(std::unique_ptr<Instruction> d);



  inline void AddDebug3Inst(std::unique_ptr<Instruction> d);



  inline void AddExtInstDebugInfo(std::unique_ptr<Instruction> d);


  inline void AddAnnotationInst(std::unique_ptr<Instruction> a);


  inline void AddType(std::unique_ptr<Instruction> t);


  inline void AddGlobalValue(std::unique_ptr<Instruction> v);


  inline void AddFunctionDeclaration(std::unique_ptr<Function> f);


  inline void AddFunction(std::unique_ptr<Function> f);


  inline void AddGraph(std::unique_ptr<Graph> g);


  inline void SetContainsDebugInfo();
  inline bool ContainsDebugInfo() { return contains_debug_info_; }



  std::vector<Instruction*> GetTypes();
  std::vector<const Instruction*> GetTypes() const;


  std::vector<Instruction*> GetConstants();
  std::vector<const Instruction*> GetConstants() const;


  uint32_t GetGlobalValue(spv::Op opcode) const;


  void AddGlobalValue(spv::Op opcode, uint32_t result_id, uint32_t type_id);

  inline uint32_t id_bound() const { return header_.bound; }

  inline uint32_t version() const { return header_.version; }
  inline uint32_t generator() const { return header_.generator; }
  inline uint32_t schema() const { return header_.schema; }

  inline void set_version(uint32_t v) { header_.version = v; }


  inline inst_iterator capability_begin();
  inline inst_iterator capability_end();
  inline IteratorRange<inst_iterator> capabilities();
  inline IteratorRange<const_inst_iterator> capabilities() const;


  inline inst_iterator ext_inst_import_begin();
  inline inst_iterator ext_inst_import_end();
  inline IteratorRange<inst_iterator> ext_inst_imports();
  inline IteratorRange<const_inst_iterator> ext_inst_imports() const;


  inline Instruction* GetMemoryModel() { return memory_model_.get(); }
  inline const Instruction* GetMemoryModel() const {
    return memory_model_.get();
  }


  inline Instruction* GetSampledImageAddressMode() {
    return sampled_image_address_mode_.get();
  }
  inline const Instruction* GetSampledImageAddressMode() const {
    return sampled_image_address_mode_.get();
  }











  inline inst_iterator debug1_begin();
  inline inst_iterator debug1_end();
  inline IteratorRange<inst_iterator> debugs1();
  inline IteratorRange<const_inst_iterator> debugs1() const;



  inline inst_iterator debug2_begin();
  inline inst_iterator debug2_end();
  inline IteratorRange<inst_iterator> debugs2();
  inline IteratorRange<const_inst_iterator> debugs2() const;



  inline inst_iterator debug3_begin();
  inline inst_iterator debug3_end();
  inline IteratorRange<inst_iterator> debugs3();
  inline IteratorRange<const_inst_iterator> debugs3() const;




  inline inst_iterator ext_inst_debuginfo_begin();
  inline inst_iterator ext_inst_debuginfo_end();
  inline IteratorRange<inst_iterator> ext_inst_debuginfo();
  inline IteratorRange<const_inst_iterator> ext_inst_debuginfo() const;


  inline IteratorRange<inst_iterator> entry_points();
  inline IteratorRange<const_inst_iterator> entry_points() const;


  inline IteratorRange<inst_iterator> graph_entry_points();
  inline IteratorRange<const_inst_iterator> graph_entry_points() const;


  inline inst_iterator execution_mode_begin();
  inline inst_iterator execution_mode_end();
  inline IteratorRange<inst_iterator> execution_modes();
  inline IteratorRange<const_inst_iterator> execution_modes() const;


  inline inst_iterator annotation_begin();
  inline inst_iterator annotation_end();
  IteratorRange<inst_iterator> annotations();
  IteratorRange<const_inst_iterator> annotations() const;


  inline inst_iterator extension_begin();
  inline inst_iterator extension_end();
  IteratorRange<inst_iterator> extensions();
  IteratorRange<const_inst_iterator> extensions() const;


  inline inst_iterator types_values_begin();
  inline inst_iterator types_values_end();
  inline IteratorRange<inst_iterator> types_values();
  inline IteratorRange<const_inst_iterator> types_values() const;


  iterator begin() { return iterator(&functions_, functions_.begin()); }
  iterator end() { return iterator(&functions_, functions_.end()); }
  const_iterator begin() const { return cbegin(); }
  const_iterator end() const { return cend(); }
  inline const_iterator cbegin() const;
  inline const_iterator cend() const;


  inline const std::vector<std::unique_ptr<Graph>>& graphs() const;



  void ForEachInst(const std::function<void(Instruction*)>& f,
                   bool run_on_debug_line_insts = false);
  void ForEachInst(const std::function<void(const Instruction*)>& f,
                   bool run_on_debug_line_insts = false) const;



  void ToBinary(std::vector<uint32_t>* binary, bool skip_nop) const;


  uint32_t ComputeIdBound() const;


  bool HasExplicitCapability(uint32_t cap);



  uint32_t GetExtInstImportId(const char* extstr);


  void SetContext(IRContext* c) { context_ = c; }


  IRContext* context() const { return context_; }


  void SetTrailingDbgLineInfo(std::vector<Instruction>&& dbg_line_info) {
    trailing_dbg_line_info_ = std::move(dbg_line_info);
  }

  std::vector<Instruction>& trailing_dbg_line_info() {
    return trailing_dbg_line_info_;
  }

  const std::vector<Instruction>& trailing_dbg_line_info() const {
    return trailing_dbg_line_info_;
  }

 private:
  ModuleHeader header_;



  IRContext* context_;
  InstructionList capabilities_;
  InstructionList extensions_;
  InstructionList ext_inst_imports_;

  std::unique_ptr<Instruction> memory_model_;

  std::unique_ptr<Instruction> sampled_image_address_mode_;
  InstructionList entry_points_;
  InstructionList graph_entry_points_;
  InstructionList execution_modes_;
  InstructionList debugs1_;
  InstructionList debugs2_;
  InstructionList debugs3_;
  InstructionList ext_inst_debuginfo_;
  InstructionList annotations_;

  InstructionList types_values_;
  std::vector<std::unique_ptr<Function>> functions_;
  std::vector<std::unique_ptr<Graph>> graphs_;



  std::vector<Instruction> trailing_dbg_line_info_;


  bool contains_debug_info_;
};


std::ostream& operator<<(std::ostream& str, const Module& module);

inline void Module::AddCapability(std::unique_ptr<Instruction> c) {
  capabilities_.push_back(std::move(c));
}

inline void Module::AddExtension(std::unique_ptr<Instruction> e) {
  extensions_.push_back(std::move(e));
}

inline void Module::AddExtInstImport(std::unique_ptr<Instruction> e) {
  ext_inst_imports_.push_back(std::move(e));
}

inline void Module::SetMemoryModel(std::unique_ptr<Instruction> m) {
  memory_model_ = std::move(m);
}

inline void Module::SetSampledImageAddressMode(std::unique_ptr<Instruction> m) {
  sampled_image_address_mode_ = std::move(m);
}

inline void Module::AddEntryPoint(std::unique_ptr<Instruction> e) {
  entry_points_.push_back(std::move(e));
}

inline void Module::AddGraphEntryPoint(std::unique_ptr<Instruction> e) {
  graph_entry_points_.push_back(std::move(e));
}

inline void Module::AddExecutionMode(std::unique_ptr<Instruction> e) {
  execution_modes_.push_back(std::move(e));
}

inline void Module::AddDebug1Inst(std::unique_ptr<Instruction> d) {
  debugs1_.push_back(std::move(d));
}

inline void Module::AddDebug2Inst(std::unique_ptr<Instruction> d) {
  debugs2_.push_back(std::move(d));
}

inline void Module::AddDebug3Inst(std::unique_ptr<Instruction> d) {
  debugs3_.push_back(std::move(d));
}

inline void Module::AddExtInstDebugInfo(std::unique_ptr<Instruction> d) {
  ext_inst_debuginfo_.push_back(std::move(d));
}

inline void Module::AddAnnotationInst(std::unique_ptr<Instruction> a) {
  annotations_.push_back(std::move(a));
}

inline void Module::AddType(std::unique_ptr<Instruction> t) {
  types_values_.push_back(std::move(t));
}

inline void Module::AddGlobalValue(std::unique_ptr<Instruction> v) {
  types_values_.push_back(std::move(v));
}

inline void Module::AddFunctionDeclaration(std::unique_ptr<Function> f) {

  functions_.emplace(functions_.begin(), std::move(f));
}

inline void Module::AddFunction(std::unique_ptr<Function> f) {
  functions_.emplace_back(std::move(f));
}

inline void Module::AddGraph(std::unique_ptr<Graph> g) {
  graphs_.emplace_back(std::move(g));
}

inline void Module::SetContainsDebugInfo() { contains_debug_info_ = true; }

inline Module::inst_iterator Module::capability_begin() {
  return capabilities_.begin();
}
inline Module::inst_iterator Module::capability_end() {
  return capabilities_.end();
}

inline IteratorRange<Module::inst_iterator> Module::capabilities() {
  return make_range(capabilities_.begin(), capabilities_.end());
}

inline IteratorRange<Module::const_inst_iterator> Module::capabilities() const {
  return make_range(capabilities_.begin(), capabilities_.end());
}

inline Module::inst_iterator Module::ext_inst_import_begin() {
  return ext_inst_imports_.begin();
}
inline Module::inst_iterator Module::ext_inst_import_end() {
  return ext_inst_imports_.end();
}

inline IteratorRange<Module::inst_iterator> Module::ext_inst_imports() {
  return make_range(ext_inst_imports_.begin(), ext_inst_imports_.end());
}

inline IteratorRange<Module::const_inst_iterator> Module::ext_inst_imports()
    const {
  return make_range(ext_inst_imports_.begin(), ext_inst_imports_.end());
}

inline Module::inst_iterator Module::debug1_begin() { return debugs1_.begin(); }
inline Module::inst_iterator Module::debug1_end() { return debugs1_.end(); }

inline IteratorRange<Module::inst_iterator> Module::debugs1() {
  return make_range(debugs1_.begin(), debugs1_.end());
}

inline IteratorRange<Module::const_inst_iterator> Module::debugs1() const {
  return make_range(debugs1_.begin(), debugs1_.end());
}

inline Module::inst_iterator Module::debug2_begin() { return debugs2_.begin(); }
inline Module::inst_iterator Module::debug2_end() { return debugs2_.end(); }

inline IteratorRange<Module::inst_iterator> Module::debugs2() {
  return make_range(debugs2_.begin(), debugs2_.end());
}

inline IteratorRange<Module::const_inst_iterator> Module::debugs2() const {
  return make_range(debugs2_.begin(), debugs2_.end());
}

inline Module::inst_iterator Module::debug3_begin() { return debugs3_.begin(); }
inline Module::inst_iterator Module::debug3_end() { return debugs3_.end(); }

inline IteratorRange<Module::inst_iterator> Module::debugs3() {
  return make_range(debugs3_.begin(), debugs3_.end());
}

inline IteratorRange<Module::const_inst_iterator> Module::debugs3() const {
  return make_range(debugs3_.begin(), debugs3_.end());
}

inline Module::inst_iterator Module::ext_inst_debuginfo_begin() {
  return ext_inst_debuginfo_.begin();
}
inline Module::inst_iterator Module::ext_inst_debuginfo_end() {
  return ext_inst_debuginfo_.end();
}

inline IteratorRange<Module::inst_iterator> Module::ext_inst_debuginfo() {
  return make_range(ext_inst_debuginfo_.begin(), ext_inst_debuginfo_.end());
}

inline IteratorRange<Module::const_inst_iterator> Module::ext_inst_debuginfo()
    const {
  return make_range(ext_inst_debuginfo_.begin(), ext_inst_debuginfo_.end());
}

inline IteratorRange<Module::inst_iterator> Module::entry_points() {
  return make_range(entry_points_.begin(), entry_points_.end());
}

inline IteratorRange<Module::const_inst_iterator> Module::entry_points() const {
  return make_range(entry_points_.begin(), entry_points_.end());
}

inline IteratorRange<Module::inst_iterator> Module::graph_entry_points() {
  return make_range(graph_entry_points_.begin(), graph_entry_points_.end());
}

inline IteratorRange<Module::const_inst_iterator> Module::graph_entry_points()
    const {
  return make_range(graph_entry_points_.begin(), graph_entry_points_.end());
}

inline Module::inst_iterator Module::execution_mode_begin() {
  return execution_modes_.begin();
}
inline Module::inst_iterator Module::execution_mode_end() {
  return execution_modes_.end();
}

inline IteratorRange<Module::inst_iterator> Module::execution_modes() {
  return make_range(execution_modes_.begin(), execution_modes_.end());
}

inline IteratorRange<Module::const_inst_iterator> Module::execution_modes()
    const {
  return make_range(execution_modes_.begin(), execution_modes_.end());
}

inline Module::inst_iterator Module::annotation_begin() {
  return annotations_.begin();
}
inline Module::inst_iterator Module::annotation_end() {
  return annotations_.end();
}

inline IteratorRange<Module::inst_iterator> Module::annotations() {
  return make_range(annotations_.begin(), annotations_.end());
}

inline IteratorRange<Module::const_inst_iterator> Module::annotations() const {
  return make_range(annotations_.begin(), annotations_.end());
}

inline Module::inst_iterator Module::extension_begin() {
  return extensions_.begin();
}
inline Module::inst_iterator Module::extension_end() {
  return extensions_.end();
}

inline IteratorRange<Module::inst_iterator> Module::extensions() {
  return make_range(extensions_.begin(), extensions_.end());
}

inline IteratorRange<Module::const_inst_iterator> Module::extensions() const {
  return make_range(extensions_.begin(), extensions_.end());
}

inline Module::inst_iterator Module::types_values_begin() {
  return types_values_.begin();
}

inline Module::inst_iterator Module::types_values_end() {
  return types_values_.end();
}

inline IteratorRange<Module::inst_iterator> Module::types_values() {
  return make_range(types_values_.begin(), types_values_.end());
}

inline IteratorRange<Module::const_inst_iterator> Module::types_values() const {
  return make_range(types_values_.begin(), types_values_.end());
}

inline const std::vector<std::unique_ptr<Graph>>& Module::graphs() const {
  return graphs_;
}

inline Module::const_iterator Module::cbegin() const {
  return const_iterator(&functions_, functions_.cbegin());
}

inline Module::const_iterator Module::cend() const {
  return const_iterator(&functions_, functions_.cend());
}

}
}

#endif
