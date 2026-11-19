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

#ifndef SOURCE_OPT_IR_CONTEXT_H_
#define SOURCE_OPT_IR_CONTEXT_H_

#include <algorithm>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "source/assembly_grammar.h"
#include "source/opt/cfg.h"
#include "source/opt/constants.h"
#include "source/opt/debug_info_manager.h"
#include "source/opt/decoration_manager.h"
#include "source/opt/def_use_manager.h"
#include "source/opt/dominator_analysis.h"
#include "source/opt/feature_manager.h"
#include "source/opt/fold.h"
#include "source/opt/liveness.h"
#include "source/opt/loop_descriptor.h"
#include "source/opt/module.h"
#include "source/opt/register_pressure.h"
#include "source/opt/scalar_analysis.h"
#include "source/opt/struct_cfg_analysis.h"
#include "source/opt/type_manager.h"
#include "source/opt/value_number_table.h"
#include "source/table2.h"
#include "source/util/make_unique.h"
#include "source/util/string_utils.h"

namespace spvtools {
namespace opt {

class IRContext {
 public:











  enum Analysis {
    kAnalysisNone = 0 << 0,
    kAnalysisBegin = 1 << 0,
    kAnalysisDefUse = kAnalysisBegin,
    kAnalysisInstrToBlockMapping = 1 << 1,
    kAnalysisDecorations = 1 << 2,
    kAnalysisCombinators = 1 << 3,
    kAnalysisCFG = 1 << 4,
    kAnalysisDominatorAnalysis = 1 << 5,
    kAnalysisLoopAnalysis = 1 << 6,
    kAnalysisNameMap = 1 << 7,
    kAnalysisScalarEvolution = 1 << 8,
    kAnalysisRegisterPressure = 1 << 9,
    kAnalysisValueNumberTable = 1 << 10,
    kAnalysisStructuredCFG = 1 << 11,
    kAnalysisBuiltinVarId = 1 << 12,
    kAnalysisIdToFuncMapping = 1 << 13,
    kAnalysisConstants = 1 << 14,
    kAnalysisTypes = 1 << 15,
    kAnalysisDebugInfo = 1 << 16,
    kAnalysisLiveness = 1 << 17,
    kAnalysisIdToGraphMapping = 1 << 18,
    kAnalysisEnd = 1 << 19
  };

  using ProcessFunction = std::function<bool(Function*)>;

  friend inline Analysis operator|(Analysis lhs, Analysis rhs);
  friend inline Analysis& operator|=(Analysis& lhs, Analysis rhs);
  friend inline Analysis operator<<(Analysis a, int shift);
  friend inline Analysis& operator<<=(Analysis& a, int shift);


  IRContext(spv_target_env env, MessageConsumer c)
      : syntax_context_(spvContextCreate(env)),
        grammar_(syntax_context_),
        unique_id_(0),
        module_(new Module()),
        consumer_(std::move(c)),
        def_use_mgr_(nullptr),
        feature_mgr_(nullptr),
        valid_analyses_(kAnalysisNone),
        constant_mgr_(nullptr),
        type_mgr_(nullptr),
        id_to_name_(nullptr),
        max_id_bound_(kDefaultMaxIdBound),
        preserve_bindings_(false),
        preserve_spec_constants_(false),
        id_overflow_(false) {
    SetContextMessageConsumer(syntax_context_, consumer_);
    module_->SetContext(this);
  }

  IRContext(spv_target_env env, std::unique_ptr<Module>&& m, MessageConsumer c)
      : syntax_context_(spvContextCreate(env)),
        grammar_(syntax_context_),
        unique_id_(0),
        module_(std::move(m)),
        consumer_(std::move(c)),
        def_use_mgr_(nullptr),
        feature_mgr_(nullptr),
        valid_analyses_(kAnalysisNone),
        type_mgr_(nullptr),
        id_to_name_(nullptr),
        max_id_bound_(kDefaultMaxIdBound),
        preserve_bindings_(false),
        preserve_spec_constants_(false),
        id_overflow_(false) {
    SetContextMessageConsumer(syntax_context_, consumer_);
    module_->SetContext(this);
    InitializeCombinators();
  }

  ~IRContext() { spvContextDestroy(syntax_context_); }

  Module* module() const { return module_.get(); }



  inline std::vector<Instruction*> GetConstants();
  inline std::vector<const Instruction*> GetConstants() const;


  inline Module::inst_iterator annotation_begin();
  inline Module::inst_iterator annotation_end();
  inline IteratorRange<Module::inst_iterator> annotations();
  inline IteratorRange<Module::const_inst_iterator> annotations() const;


  inline Module::inst_iterator capability_begin();
  inline Module::inst_iterator capability_end();
  inline IteratorRange<Module::inst_iterator> capabilities();
  inline IteratorRange<Module::const_inst_iterator> capabilities() const;


  inline Module::inst_iterator extension_begin();
  inline Module::inst_iterator extension_end();
  inline IteratorRange<Module::inst_iterator> extensions();
  inline IteratorRange<Module::const_inst_iterator> extensions() const;


  inline Module::inst_iterator types_values_begin();
  inline Module::inst_iterator types_values_end();
  inline IteratorRange<Module::inst_iterator> types_values();
  inline IteratorRange<Module::const_inst_iterator> types_values() const;


  inline Module::inst_iterator ext_inst_import_begin();
  inline Module::inst_iterator ext_inst_import_end();
  inline IteratorRange<Module::inst_iterator> ext_inst_imports();
  inline IteratorRange<Module::const_inst_iterator> ext_inst_imports() const;











  inline Module::inst_iterator debug1_begin();
  inline Module::inst_iterator debug1_end();
  inline IteratorRange<Module::inst_iterator> debugs1();
  inline IteratorRange<Module::const_inst_iterator> debugs1() const;



  inline Module::inst_iterator debug2_begin();
  inline Module::inst_iterator debug2_end();
  inline IteratorRange<Module::inst_iterator> debugs2();
  inline IteratorRange<Module::const_inst_iterator> debugs2() const;



  inline Module::inst_iterator debug3_begin();
  inline Module::inst_iterator debug3_end();
  inline IteratorRange<Module::inst_iterator> debugs3();
  inline IteratorRange<Module::const_inst_iterator> debugs3() const;





  inline Module::inst_iterator ext_inst_debuginfo_begin();
  inline Module::inst_iterator ext_inst_debuginfo_end();
  inline IteratorRange<Module::inst_iterator> ext_inst_debuginfo();
  inline IteratorRange<Module::const_inst_iterator> ext_inst_debuginfo() const;


  inline void AddCapability(spv::Capability capability);

  inline void AddCapability(std::unique_ptr<Instruction>&& c);


  bool RemoveCapability(spv::Capability capability);


  inline void AddExtension(const std::string& ext_name);
  inline void AddExtension(std::unique_ptr<Instruction>&& e);


  bool RemoveExtension(Extension extension);


  inline void AddExtInstImport(const std::string& name);
  inline void AddExtInstImport(std::unique_ptr<Instruction>&& e);

  inline void SetMemoryModel(std::unique_ptr<Instruction>&& m);

  inline const Instruction* GetMemoryModel() const;

  inline void AddEntryPoint(std::unique_ptr<Instruction>&& e);

  inline void AddExecutionMode(std::unique_ptr<Instruction>&& e);



  inline void AddDebug1Inst(std::unique_ptr<Instruction>&& d);



  inline void AddDebug2Inst(std::unique_ptr<Instruction>&& d);


  inline void AddDebug3Inst(std::unique_ptr<Instruction>&& d);

  inline void AddExtInstDebugInfo(std::unique_ptr<Instruction>&& d);

  inline void AddAnnotationInst(std::unique_ptr<Instruction>&& a);

  inline void AddType(std::unique_ptr<Instruction>&& t);

  inline void AddGlobalValue(std::unique_ptr<Instruction>&& v);

  inline void AddFunctionDeclaration(std::unique_ptr<Function>&& f);

  inline void AddFunction(std::unique_ptr<Function>&& f);



  analysis::DefUseManager* get_def_use_mgr() {
    if (!AreAnalysesValid(kAnalysisDefUse)) {
      BuildDefUseManager();
    }
    return def_use_mgr_.get();
  }



  analysis::LivenessManager* get_liveness_mgr() {
    if (!AreAnalysesValid(kAnalysisLiveness)) {
      BuildLivenessManager();
    }
    return liveness_mgr_.get();
  }



  ValueNumberTable* GetValueNumberTable() {
    if (!AreAnalysesValid(kAnalysisValueNumberTable)) {
      BuildValueNumberTable();
    }
    return vn_table_.get();
  }



  StructuredCFGAnalysis* GetStructuredCFGAnalysis() {
    if (!AreAnalysesValid(kAnalysisStructuredCFG)) {
      BuildStructuredCFGAnalysis();
    }
    return struct_cfg_analysis_.get();
  }



  LivenessAnalysis* GetLivenessAnalysis() {
    if (!AreAnalysesValid(kAnalysisRegisterPressure)) {
      BuildRegPressureAnalysis();
    }
    return reg_pressure_.get();
  }



  BasicBlock* get_instr_block(Instruction* instr) {
    if (!AreAnalysesValid(kAnalysisInstrToBlockMapping)) {
      BuildInstrToBlockMapping();
    }
    auto entry = instr_to_block_.find(instr);
    return (entry != instr_to_block_.end()) ? entry->second : nullptr;
  }





  BasicBlock* get_instr_block(uint32_t id) {
    Instruction* def = get_def_use_mgr()->GetDef(id);
    return get_instr_block(def);
  }



  void set_instr_block(Instruction* inst, BasicBlock* block) {
    if (AreAnalysesValid(kAnalysisInstrToBlockMapping)) {
      instr_to_block_[inst] = block;
    }
  }



  analysis::DecorationManager* get_decoration_mgr() {
    if (!AreAnalysesValid(kAnalysisDecorations)) {
      BuildDecorationManager();
    }
    return decoration_mgr_.get();
  }




  analysis::ConstantManager* get_constant_mgr() {
    if (!AreAnalysesValid(kAnalysisConstants)) {
      BuildConstantManager();
    }
    return constant_mgr_.get();
  }




  analysis::TypeManager* get_type_mgr() {
    if (!AreAnalysesValid(kAnalysisTypes)) {
      BuildTypeManager();
    }
    return type_mgr_.get();
  }





  analysis::DebugInfoManager* get_debug_info_mgr() {
    if (!AreAnalysesValid(kAnalysisDebugInfo)) {
      BuildDebugInfoManager();
    }
    return debug_info_mgr_.get();
  }



  ScalarEvolutionAnalysis* GetScalarEvolutionAnalysis() {
    if (!AreAnalysesValid(kAnalysisScalarEvolution)) {
      BuildScalarEvolutionAnalysis();
    }
    return scalar_evolution_analysis_.get();
  }



  inline void BuildIdToNameMap();



  inline IteratorRange<std::multimap<uint32_t, Instruction*>::iterator>
  GetNames(uint32_t id);






  inline Instruction* GetMemberName(uint32_t struct_type_id, uint32_t index);



  inline void CloneNames(const uint32_t old_id, const uint32_t new_id,
                         const uint32_t max_member_index = UINT32_MAX);



  void SetMessageConsumer(MessageConsumer c) { consumer_ = std::move(c); }


  const MessageConsumer& consumer() const { return consumer_; }


  void BuildInvalidAnalyses(Analysis set);


  void InvalidateAnalysesExceptFor(Analysis preserved_analyses);


  void InvalidateAnalyses(Analysis analyses_to_invalidate);







  bool KillDef(uint32_t id);














  Instruction* KillInst(Instruction* inst);







  bool KillInstructionIf(Module::inst_iterator begin, Module::inst_iterator end,
                         std::function<bool(Instruction*)> condition);



  void CollectNonSemanticTree(Instruction* inst,
                              std::unordered_set<Instruction*>* to_kill);


  void CollectCallTreeFromRoots(unsigned entryId,
                                std::unordered_set<uint32_t>* funcs);


  bool AreAnalysesValid(Analysis set) { return (set & valid_analyses_) == set; }







  bool ReplaceAllUsesWith(uint32_t before, uint32_t after);






  bool ReplaceAllUsesWithPredicate(
      uint32_t before, uint32_t after,
      const std::function<bool(Instruction*)>& predicate);



  bool IsConsistent();



  inline void AnalyzeDefUse(Instruction* inst);




  void ForgetUses(Instruction* inst);



  void AnalyzeUses(Instruction* inst);


  void KillNamesAndDecorates(uint32_t id);


  void KillNamesAndDecorates(Instruction* inst);


  void KillOperandFromDebugInstructions(Instruction* inst);


  void KillRelatedDebugScopes(Instruction* inst);


  inline uint32_t TakeNextUniqueId() {
    assert(unique_id_ != std::numeric_limits<uint32_t>::max());


    return ++unique_id_;
  }



  inline bool IsCombinatorInstruction(const Instruction* inst) {
    if (!AreAnalysesValid(kAnalysisCombinators)) {
      InitializeCombinators();
    }
    constexpr uint32_t kExtInstSetIdInIndx = 0;
    constexpr uint32_t kExtInstInstructionInIndx = 1;

    if (inst->opcode() != spv::Op::OpExtInst) {
      return combinator_ops_[0].count(uint32_t(inst->opcode())) != 0;
    } else {
      uint32_t set = inst->GetSingleWordInOperand(kExtInstSetIdInIndx);
      auto op = inst->GetSingleWordInOperand(kExtInstInstructionInIndx);
      return combinator_ops_[set].count(op) != 0;
    }
  }


  CFG* cfg() {
    if (!AreAnalysesValid(kAnalysisCFG)) {
      BuildCFG();
    }
    return cfg_.get();
  }


  LoopDescriptor* GetLoopDescriptor(const Function* f);


  DominatorAnalysis* GetDominatorAnalysis(const Function* f);


  PostDominatorAnalysis* GetPostDominatorAnalysis(const Function* f);


  inline void RemoveDominatorAnalysis(const Function* f) {
    dominator_trees_.erase(f);
  }


  inline void RemovePostDominatorAnalysis(const Function* f) {
    post_dominator_trees_.erase(f);
  }



  inline uint32_t TakeNextId() {
    uint32_t next_id = module()->TakeNextIdBound();
    if (next_id == 0) {
      id_overflow_ = true;
      if (consumer()) {
        std::string message = "ID overflow. Try running compact-ids.";
        consumer()(SPV_MSG_ERROR, "", {0, 0, 0}, message.c_str());
      }
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION









      exit(0);
#endif
    }
    return next_id;
  }



  bool id_overflow() const { return id_overflow_; }


  void clear_id_overflow() { id_overflow_ = false; }

  FeatureManager* get_feature_mgr() {
    if (!feature_mgr_.get()) {
      AnalyzeFeatures();
    }
    return feature_mgr_.get();
  }

  void ResetFeatureManager() { feature_mgr_.reset(nullptr); }


  const AssemblyGrammar& grammar() const { return grammar_; }



  inline void UpdateDefUse(Instruction* inst);

  const InstructionFolder& get_instruction_folder() {
    if (!inst_folder_) {
      inst_folder_ = MakeUnique<InstructionFolder>(this);
    }
    return *inst_folder_;
  }

  uint32_t max_id_bound() const { return max_id_bound_; }
  void set_max_id_bound(uint32_t new_bound) { max_id_bound_ = new_bound; }

  bool preserve_bindings() const { return preserve_bindings_; }
  void set_preserve_bindings(bool should_preserve_bindings) {
    preserve_bindings_ = should_preserve_bindings;
  }

  bool preserve_spec_constants() const { return preserve_spec_constants_; }
  void set_preserve_spec_constants(bool should_preserve_spec_constants) {
    preserve_spec_constants_ = should_preserve_spec_constants;
  }




  uint32_t GetBuiltinInputVarId(uint32_t builtin);



  Function* GetFunction(uint32_t id) {
    if (!AreAnalysesValid(kAnalysisIdToFuncMapping)) {
      BuildIdToFuncMapping();
    }
    auto entry = id_to_func_.find(id);
    return (entry != id_to_func_.end()) ? entry->second : nullptr;
  }

  Function* GetFunction(Instruction* inst) {
    if (inst->opcode() != spv::Op::OpFunction) {
      return nullptr;
    }
    return GetFunction(inst->result_id());
  }



  Graph* GetGraph(uint32_t id) {
    if (!AreAnalysesValid(kAnalysisIdToGraphMapping)) {
      BuildIdToGraphMapping();
    }
    auto entry = id_to_graph_.find(id);
    return (entry != id_to_graph_.end()) ? entry->second : nullptr;
  }

  Graph* GetGraph(Instruction* inst) {
    if (inst->opcode() != spv::Op::OpGraphARM) {
      return nullptr;
    }
    return GetGraph(inst->result_id());
  }


  void AddCalls(const Function* func, std::queue<uint32_t>* todo);




  bool ProcessEntryPointCallTree(ProcessFunction& pfn);




  bool ProcessReachableCallTree(ProcessFunction& pfn);





  bool ProcessCallTreeFromRoots(ProcessFunction& pfn,
                                std::queue<uint32_t>* roots);



  void EmitErrorMessage(std::string message, Instruction* inst);



  bool IsReachable(const opt::BasicBlock& bb);



  spv::ExecutionModel GetStage();



  bool IsTargetEnvAtLeast(spv_target_env env) {


    return syntax_context_->target_env >= env;
  }


  spv_target_env GetTargetEnv() const { return syntax_context_->target_env; }

 private:

  void BuildDefUseManager() {
    def_use_mgr_ = MakeUnique<analysis::DefUseManager>(module());
    valid_analyses_ = valid_analyses_ | kAnalysisDefUse;
  }


  void BuildLivenessManager() {
    liveness_mgr_ = MakeUnique<analysis::LivenessManager>(this);
    valid_analyses_ = valid_analyses_ | kAnalysisLiveness;
  }


  void BuildInstrToBlockMapping() {
    instr_to_block_.clear();
    for (auto& fn : *module_) {
      for (auto& block : fn) {
        block.ForEachInst([this, &block](Instruction* inst) {
          instr_to_block_[inst] = &block;
        });
      }
    }
    valid_analyses_ = valid_analyses_ | kAnalysisInstrToBlockMapping;
  }


  void BuildIdToFuncMapping() {
    id_to_func_.clear();
    for (auto& fn : *module_) {
      id_to_func_[fn.result_id()] = &fn;
    }
    valid_analyses_ = valid_analyses_ | kAnalysisIdToFuncMapping;
  }


  void BuildIdToGraphMapping() {
    id_to_graph_.clear();
    for (auto& g : module_->graphs()) {
      id_to_graph_[g->DefInst().result_id()] = g.get();
    }
    valid_analyses_ = valid_analyses_ | kAnalysisIdToGraphMapping;
  }

  void BuildDecorationManager() {
    decoration_mgr_ = MakeUnique<analysis::DecorationManager>(module());
    valid_analyses_ = valid_analyses_ | kAnalysisDecorations;
  }

  void BuildCFG() {
    cfg_ = MakeUnique<CFG>(module());
    valid_analyses_ = valid_analyses_ | kAnalysisCFG;
  }

  void BuildScalarEvolutionAnalysis() {
    scalar_evolution_analysis_ = MakeUnique<ScalarEvolutionAnalysis>(this);
    valid_analyses_ = valid_analyses_ | kAnalysisScalarEvolution;
  }


  void BuildRegPressureAnalysis() {
    reg_pressure_ = MakeUnique<LivenessAnalysis>(this);
    valid_analyses_ = valid_analyses_ | kAnalysisRegisterPressure;
  }



  void BuildValueNumberTable() {
    vn_table_ = MakeUnique<ValueNumberTable>(this);
    valid_analyses_ = valid_analyses_ | kAnalysisValueNumberTable;
  }



  void BuildStructuredCFGAnalysis() {
    struct_cfg_analysis_ = MakeUnique<StructuredCFGAnalysis>(this);
    valid_analyses_ = valid_analyses_ | kAnalysisStructuredCFG;
  }



  void BuildConstantManager() {
    constant_mgr_ = MakeUnique<analysis::ConstantManager>(this);
    valid_analyses_ = valid_analyses_ | kAnalysisConstants;
  }



  void BuildTypeManager() {
    type_mgr_ = MakeUnique<analysis::TypeManager>(consumer(), this);
    valid_analyses_ = valid_analyses_ | kAnalysisTypes;
  }



  void BuildDebugInfoManager() {
    debug_info_mgr_ = MakeUnique<analysis::DebugInfoManager>(this);
    valid_analyses_ = valid_analyses_ | kAnalysisDebugInfo;
  }



  void ResetDominatorAnalysis() {

    dominator_trees_.clear();
    post_dominator_trees_.clear();
    valid_analyses_ = valid_analyses_ | kAnalysisDominatorAnalysis;
  }


  void ResetLoopAnalysis() {

    loop_descriptors_.clear();
    valid_analyses_ = valid_analyses_ | kAnalysisLoopAnalysis;
  }


  void ResetBuiltinAnalysis() {

    builtin_var_id_map_.clear();
    valid_analyses_ = valid_analyses_ | kAnalysisBuiltinVarId;
  }


  void AnalyzeFeatures() {
    feature_mgr_ =
        std::unique_ptr<FeatureManager>(new FeatureManager(grammar_));
    feature_mgr_->Analyze(module());
  }



  void InitializeCombinators();


  void AddCombinatorsForCapability(uint32_t capability);


  void AddCombinatorsForExtension(Instruction* extension);


  void RemoveFromIdToName(const Instruction* inst);



  bool CheckCFG();



  uint32_t FindBuiltinInputVar(uint32_t builtin);


  void AddVarToEntryPoints(uint32_t var_id);



  spv_context syntax_context_;


  AssemblyGrammar grammar_;






  uint32_t unique_id_;


  std::unique_ptr<Module> module_;


  MessageConsumer consumer_;


  std::unique_ptr<analysis::DefUseManager> def_use_mgr_;


  std::unique_ptr<analysis::DecorationManager> decoration_mgr_;


  std::unique_ptr<FeatureManager> feature_mgr_;






  std::unordered_map<Instruction*, BasicBlock*> instr_to_block_;






  std::unordered_map<uint32_t, Function*> id_to_func_;






  std::unordered_map<uint32_t, Graph*> id_to_graph_;


  Analysis valid_analyses_;



  std::unordered_map<uint32_t, std::unordered_set<uint32_t>> combinator_ops_;



  std::unordered_map<uint32_t, uint32_t> builtin_var_id_map_;


  std::unique_ptr<CFG> cfg_;



  std::map<const Function*, DominatorAnalysis> dominator_trees_;
  std::map<const Function*, PostDominatorAnalysis> post_dominator_trees_;


  std::unordered_map<const Function*, LoopDescriptor> loop_descriptors_;


  std::unique_ptr<analysis::ConstantManager> constant_mgr_;


  std::unique_ptr<analysis::TypeManager> type_mgr_;


  std::unique_ptr<analysis::DebugInfoManager> debug_info_mgr_;


  std::unique_ptr<std::multimap<uint32_t, Instruction*>> id_to_name_;


  std::unique_ptr<ScalarEvolutionAnalysis> scalar_evolution_analysis_;


  std::unique_ptr<LivenessAnalysis> reg_pressure_;

  std::unique_ptr<ValueNumberTable> vn_table_;

  std::unique_ptr<InstructionFolder> inst_folder_;

  std::unique_ptr<StructuredCFGAnalysis> struct_cfg_analysis_;


  std::unique_ptr<analysis::LivenessManager> liveness_mgr_;


  uint32_t max_id_bound_;


  bool preserve_bindings_;



  bool preserve_spec_constants_;


  bool id_overflow_;
};

inline IRContext::Analysis operator|(IRContext::Analysis lhs,
                                     IRContext::Analysis rhs) {
  return static_cast<IRContext::Analysis>(static_cast<int>(lhs) |
                                          static_cast<int>(rhs));
}

inline IRContext::Analysis& operator|=(IRContext::Analysis& lhs,
                                       IRContext::Analysis rhs) {
  lhs = lhs | rhs;
  return lhs;
}

inline IRContext::Analysis operator<<(IRContext::Analysis a, int shift) {
  return static_cast<IRContext::Analysis>(static_cast<int>(a) << shift);
}

inline IRContext::Analysis& operator<<=(IRContext::Analysis& a, int shift) {
  a = static_cast<IRContext::Analysis>(static_cast<int>(a) << shift);
  return a;
}

std::vector<Instruction*> IRContext::GetConstants() {
  return module()->GetConstants();
}

std::vector<const Instruction*> IRContext::GetConstants() const {
  return ((const Module*)module())->GetConstants();
}

Module::inst_iterator IRContext::annotation_begin() {
  return module()->annotation_begin();
}

Module::inst_iterator IRContext::annotation_end() {
  return module()->annotation_end();
}

IteratorRange<Module::inst_iterator> IRContext::annotations() {
  return module_->annotations();
}

IteratorRange<Module::const_inst_iterator> IRContext::annotations() const {
  return ((const Module*)module_.get())->annotations();
}

Module::inst_iterator IRContext::capability_begin() {
  return module()->capability_begin();
}

Module::inst_iterator IRContext::capability_end() {
  return module()->capability_end();
}

IteratorRange<Module::inst_iterator> IRContext::capabilities() {
  return module()->capabilities();
}

IteratorRange<Module::const_inst_iterator> IRContext::capabilities() const {
  return ((const Module*)module())->capabilities();
}

Module::inst_iterator IRContext::extension_begin() {
  return module()->extension_begin();
}

Module::inst_iterator IRContext::extension_end() {
  return module()->extension_end();
}

IteratorRange<Module::inst_iterator> IRContext::extensions() {
  return module()->extensions();
}

IteratorRange<Module::const_inst_iterator> IRContext::extensions() const {
  return ((const Module*)module())->extensions();
}

Module::inst_iterator IRContext::types_values_begin() {
  return module()->types_values_begin();
}

Module::inst_iterator IRContext::types_values_end() {
  return module()->types_values_end();
}

IteratorRange<Module::inst_iterator> IRContext::types_values() {
  return module()->types_values();
}

IteratorRange<Module::const_inst_iterator> IRContext::types_values() const {
  return ((const Module*)module_.get())->types_values();
}

Module::inst_iterator IRContext::ext_inst_import_begin() {
  return module()->ext_inst_import_begin();
}

Module::inst_iterator IRContext::ext_inst_import_end() {
  return module()->ext_inst_import_end();
}

IteratorRange<Module::inst_iterator> IRContext::ext_inst_imports() {
  return module()->ext_inst_imports();
}

IteratorRange<Module::const_inst_iterator> IRContext::ext_inst_imports() const {
  return ((const Module*)module_.get())->ext_inst_imports();
}

Module::inst_iterator IRContext::debug1_begin() {
  return module()->debug1_begin();
}

Module::inst_iterator IRContext::debug1_end() { return module()->debug1_end(); }

IteratorRange<Module::inst_iterator> IRContext::debugs1() {
  return module()->debugs1();
}

IteratorRange<Module::const_inst_iterator> IRContext::debugs1() const {
  return ((const Module*)module_.get())->debugs1();
}

Module::inst_iterator IRContext::debug2_begin() {
  return module()->debug2_begin();
}
Module::inst_iterator IRContext::debug2_end() { return module()->debug2_end(); }

IteratorRange<Module::inst_iterator> IRContext::debugs2() {
  return module()->debugs2();
}

IteratorRange<Module::const_inst_iterator> IRContext::debugs2() const {
  return ((const Module*)module_.get())->debugs2();
}

Module::inst_iterator IRContext::debug3_begin() {
  return module()->debug3_begin();
}

Module::inst_iterator IRContext::debug3_end() { return module()->debug3_end(); }

IteratorRange<Module::inst_iterator> IRContext::debugs3() {
  return module()->debugs3();
}

IteratorRange<Module::const_inst_iterator> IRContext::debugs3() const {
  return ((const Module*)module_.get())->debugs3();
}

Module::inst_iterator IRContext::ext_inst_debuginfo_begin() {
  return module()->ext_inst_debuginfo_begin();
}

Module::inst_iterator IRContext::ext_inst_debuginfo_end() {
  return module()->ext_inst_debuginfo_end();
}

IteratorRange<Module::inst_iterator> IRContext::ext_inst_debuginfo() {
  return module()->ext_inst_debuginfo();
}

IteratorRange<Module::const_inst_iterator> IRContext::ext_inst_debuginfo()
    const {
  return ((const Module*)module_.get())->ext_inst_debuginfo();
}

void IRContext::AddCapability(spv::Capability capability) {
  if (!get_feature_mgr()->HasCapability(capability)) {
    std::unique_ptr<Instruction> capability_inst(new Instruction(
        this, spv::Op::OpCapability, 0, 0,
        {{SPV_OPERAND_TYPE_CAPABILITY, {static_cast<uint32_t>(capability)}}}));
    AddCapability(std::move(capability_inst));
  }
}

void IRContext::AddCapability(std::unique_ptr<Instruction>&& c) {
  AddCombinatorsForCapability(c->GetSingleWordInOperand(0));
  if (feature_mgr_ != nullptr) {
    feature_mgr_->AddCapability(
        static_cast<spv::Capability>(c->GetSingleWordInOperand(0)));
  }
  if (AreAnalysesValid(kAnalysisDefUse)) {
    get_def_use_mgr()->AnalyzeInstDefUse(c.get());
  }
  module()->AddCapability(std::move(c));
}

void IRContext::AddExtension(const std::string& ext_name) {
  std::vector<uint32_t> ext_words = spvtools::utils::MakeVector(ext_name);
  AddExtension(std::unique_ptr<Instruction>(
      new Instruction(this, spv::Op::OpExtension, 0u, 0u,
                      {{SPV_OPERAND_TYPE_LITERAL_STRING, ext_words}})));
}

void IRContext::AddExtension(std::unique_ptr<Instruction>&& e) {
  if (AreAnalysesValid(kAnalysisDefUse)) {
    get_def_use_mgr()->AnalyzeInstDefUse(e.get());
  }
  if (feature_mgr_ != nullptr) {
    feature_mgr_->AddExtension(&*e);
  }
  module()->AddExtension(std::move(e));
}

void IRContext::AddExtInstImport(const std::string& name) {
  std::vector<uint32_t> ext_words = spvtools::utils::MakeVector(name);
  AddExtInstImport(std::unique_ptr<Instruction>(
      new Instruction(this, spv::Op::OpExtInstImport, 0u, TakeNextId(),
                      {{SPV_OPERAND_TYPE_LITERAL_STRING, ext_words}})));
}

void IRContext::AddExtInstImport(std::unique_ptr<Instruction>&& e) {
  AddCombinatorsForExtension(e.get());
  if (AreAnalysesValid(kAnalysisDefUse)) {
    get_def_use_mgr()->AnalyzeInstDefUse(e.get());
  }
  module()->AddExtInstImport(std::move(e));
  if (feature_mgr_ != nullptr) {
    feature_mgr_->AddExtInstImportIds(module());
  }
}

void IRContext::SetMemoryModel(std::unique_ptr<Instruction>&& m) {
  module()->SetMemoryModel(std::move(m));
}

const Instruction* IRContext::GetMemoryModel() const {
  return module()->GetMemoryModel();
}

void IRContext::AddEntryPoint(std::unique_ptr<Instruction>&& e) {
  module()->AddEntryPoint(std::move(e));
}

void IRContext::AddExecutionMode(std::unique_ptr<Instruction>&& e) {
  module()->AddExecutionMode(std::move(e));
}

void IRContext::AddDebug1Inst(std::unique_ptr<Instruction>&& d) {
  module()->AddDebug1Inst(std::move(d));
}

void IRContext::AddDebug2Inst(std::unique_ptr<Instruction>&& d) {
  if (AreAnalysesValid(kAnalysisNameMap)) {
    if (d->opcode() == spv::Op::OpName ||
        d->opcode() == spv::Op::OpMemberName) {


      id_to_name_->insert({d->GetSingleWordInOperand(0), d.get()});
    }
  }
  if (AreAnalysesValid(kAnalysisDefUse)) {
    get_def_use_mgr()->AnalyzeInstDefUse(d.get());
  }
  module()->AddDebug2Inst(std::move(d));
}

void IRContext::AddDebug3Inst(std::unique_ptr<Instruction>&& d) {
  module()->AddDebug3Inst(std::move(d));
}

void IRContext::AddExtInstDebugInfo(std::unique_ptr<Instruction>&& d) {
  module()->AddExtInstDebugInfo(std::move(d));
}

void IRContext::AddAnnotationInst(std::unique_ptr<Instruction>&& a) {
  if (AreAnalysesValid(kAnalysisDecorations)) {
    get_decoration_mgr()->AddDecoration(a.get());
  }
  if (AreAnalysesValid(kAnalysisDefUse)) {
    get_def_use_mgr()->AnalyzeInstDefUse(a.get());
  }
  module()->AddAnnotationInst(std::move(a));
}

void IRContext::AddType(std::unique_ptr<Instruction>&& t) {
  module()->AddType(std::move(t));
  if (AreAnalysesValid(kAnalysisDefUse)) {
    get_def_use_mgr()->AnalyzeInstDefUse(&*(--types_values_end()));
  }
}

void IRContext::AddGlobalValue(std::unique_ptr<Instruction>&& v) {
  if (AreAnalysesValid(kAnalysisDefUse)) {
    get_def_use_mgr()->AnalyzeInstDefUse(&*v);
  }
  module()->AddGlobalValue(std::move(v));
}

void IRContext::AddFunctionDeclaration(std::unique_ptr<Function>&& f) {
  module()->AddFunctionDeclaration(std::move(f));
}

void IRContext::AddFunction(std::unique_ptr<Function>&& f) {
  module()->AddFunction(std::move(f));
}

void IRContext::AnalyzeDefUse(Instruction* inst) {
  if (AreAnalysesValid(kAnalysisDefUse)) {
    get_def_use_mgr()->AnalyzeInstDefUse(inst);
  }
}

void IRContext::UpdateDefUse(Instruction* inst) {
  if (AreAnalysesValid(kAnalysisDefUse)) {
    get_def_use_mgr()->UpdateDefUse(inst);
  }
}

void IRContext::BuildIdToNameMap() {
  id_to_name_ = MakeUnique<std::multimap<uint32_t, Instruction*>>();
  for (Instruction& debug_inst : debugs2()) {
    if (debug_inst.opcode() == spv::Op::OpMemberName ||
        debug_inst.opcode() == spv::Op::OpName) {
      id_to_name_->insert({debug_inst.GetSingleWordInOperand(0), &debug_inst});
    }
  }
  valid_analyses_ = valid_analyses_ | kAnalysisNameMap;
}

IteratorRange<std::multimap<uint32_t, Instruction*>::iterator>
IRContext::GetNames(uint32_t id) {
  if (!AreAnalysesValid(kAnalysisNameMap)) {
    BuildIdToNameMap();
  }
  auto result = id_to_name_->equal_range(id);
  return make_range(std::move(result.first), std::move(result.second));
}

Instruction* IRContext::GetMemberName(uint32_t struct_type_id, uint32_t index) {
  if (!AreAnalysesValid(kAnalysisNameMap)) {
    BuildIdToNameMap();
  }
  auto result = id_to_name_->equal_range(struct_type_id);
  for (auto i = result.first; i != result.second; ++i) {
    auto* name_instr = i->second;
    if (name_instr->opcode() == spv::Op::OpMemberName &&
        name_instr->GetSingleWordInOperand(1) == index) {
      return name_instr;
    }
  }
  return nullptr;
}

void IRContext::CloneNames(const uint32_t old_id, const uint32_t new_id,
                           const uint32_t max_member_index) {
  std::vector<std::unique_ptr<Instruction>> names_to_add;
  auto names = GetNames(old_id);
  for (auto n : names) {
    Instruction* old_name_inst = n.second;
    if (old_name_inst->opcode() == spv::Op::OpMemberName) {
      auto midx = old_name_inst->GetSingleWordInOperand(1);
      if (midx >= max_member_index) continue;
    }
    std::unique_ptr<Instruction> new_name_inst(old_name_inst->Clone(this));
    new_name_inst->SetInOperand(0, {new_id});
    names_to_add.push_back(std::move(new_name_inst));
  }


  for (auto& new_name : names_to_add) AddDebug2Inst(std::move(new_name));
}

}
}

#endif
