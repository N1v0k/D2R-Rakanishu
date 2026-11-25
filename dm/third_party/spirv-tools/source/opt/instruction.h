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

#ifndef SOURCE_OPT_INSTRUCTION_H_
#define SOURCE_OPT_INSTRUCTION_H_

#include <cassert>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "OpenCLDebugInfo100.h"
#include "source/binary.h"
#include "source/common_debug_info.h"
#include "source/latest_version_glsl_std_450_header.h"
#include "source/latest_version_spirv_header.h"
#include "source/opcode.h"
#include "source/operand.h"
#include "source/opt/reflect.h"
#include "source/util/ilist_node.h"
#include "source/util/small_vector.h"
#include "source/util/string_utils.h"
#include "spirv-tools/libspirv.h"
#include "spirv/unified1/NonSemanticShaderDebugInfo.h"

constexpr uint32_t kNoDebugScope = 0;
constexpr uint32_t kNoInlinedAt = 0;

namespace spvtools {
namespace opt {

class Function;
class IRContext;
class Module;
class InstructionList;































struct Operand {
  using OperandData = utils::SmallVector<uint32_t, 2>;
  Operand(spv_operand_type_t t, OperandData&& w)
      : type(t), words(std::move(w)) {}

  Operand(spv_operand_type_t t, const OperandData& w) : type(t), words(w) {}

  template <class InputIt>
  Operand(spv_operand_type_t t, InputIt firstOperandData,
          InputIt lastOperandData)
      : type(t), words(firstOperandData, lastOperandData) {}

  spv_operand_type_t type;
  OperandData words;

  uint32_t AsId() const {
    assert(spvIsIdType(type));
    assert(words.size() == 1);
    return words[0];
  }


  std::string AsString() const {
    assert(type == SPV_OPERAND_TYPE_LITERAL_STRING);
    return spvtools::utils::MakeString(words);
  }


  uint64_t AsLiteralUint64() const {
    assert(type == SPV_OPERAND_TYPE_LITERAL_INTEGER ||
           type == SPV_OPERAND_TYPE_TYPED_LITERAL_NUMBER ||
           type == SPV_OPERAND_TYPE_OPTIONAL_LITERAL_INTEGER ||
           type == SPV_OPERAND_TYPE_OPTIONAL_TYPED_LITERAL_INTEGER);
    assert(1 <= words.size());
    assert(words.size() <= 2);
    uint64_t result = 0;
    if (words.size() > 0) {
      uint32_t low = words[0];
      result = uint64_t(low);
    }
    if (words.size() > 1) {
      uint32_t high = words[1];
      result = result | (uint64_t(high) << 32);
    }
    return result;
  }

  friend bool operator==(const Operand& o1, const Operand& o2) {
    return o1.type == o2.type && o1.words == o2.words;
  }


};

inline bool operator!=(const Operand& o1, const Operand& o2) {
  return !(o1 == o2);
}







class DebugScope {
 public:
  DebugScope(uint32_t lexical_scope, uint32_t inlined_at)
      : lexical_scope_(lexical_scope), inlined_at_(inlined_at) {}

  inline bool operator!=(const DebugScope& d) const {
    return lexical_scope_ != d.lexical_scope_ || inlined_at_ != d.inlined_at_;
  }


  uint32_t GetLexicalScope() const { return lexical_scope_; }
  void SetLexicalScope(uint32_t scope) { lexical_scope_ = scope; }


  uint32_t GetInlinedAt() const { return inlined_at_; }
  void SetInlinedAt(uint32_t at) { inlined_at_ = at; }



  void ToBinary(uint32_t type_id, uint32_t result_id, uint32_t ext_set,
                std::vector<uint32_t>* binary) const;

 private:


  uint32_t lexical_scope_;



  uint32_t inlined_at_;
};







class Instruction : public utils::IntrusiveNodeBase<Instruction> {
 public:
  using OperandList = std::vector<Operand>;
  using iterator = OperandList::iterator;
  using const_iterator = OperandList::const_iterator;



  Instruction()
      : utils::IntrusiveNodeBase<Instruction>(),
        context_(nullptr),
        opcode_(spv::Op::OpNop),
        has_type_id_(false),
        has_result_id_(false),
        unique_id_(0),
        dbg_scope_(kNoDebugScope, kNoInlinedAt) {}


  Instruction(IRContext*);


  Instruction(IRContext*, spv::Op);




  Instruction(IRContext* c, const spv_parsed_instruction_t& inst,
              std::vector<Instruction>&& dbg_line = {});

  Instruction(IRContext* c, const spv_parsed_instruction_t& inst,
              const DebugScope& dbg_scope);



  Instruction(IRContext* c, spv::Op op, uint32_t ty_id, uint32_t res_id,
              const OperandList& in_operands);



  Instruction(const Instruction&) = default;
  Instruction& operator=(const Instruction&) = default;

  Instruction(Instruction&&);
  Instruction& operator=(Instruction&&);

  ~Instruction() override = default;






  Instruction* Clone(IRContext* c) const;

  IRContext* context() const { return context_; }

  spv::Op opcode() const { return opcode_; }




  void SetOpcode(spv::Op op) { opcode_ = op; }
  uint32_t type_id() const {
    return has_type_id_ ? GetSingleWordOperand(0) : 0;
  }
  uint32_t result_id() const {
    return has_result_id_ ? GetSingleWordOperand(has_type_id_ ? 1 : 0) : 0;
  }
  uint32_t unique_id() const {
    assert(unique_id_ != 0);
    return unique_id_;
  }


  std::vector<Instruction>& dbg_line_insts() { return dbg_line_insts_; }
  const std::vector<Instruction>& dbg_line_insts() const {
    return dbg_line_insts_;
  }

  const Instruction* dbg_line_inst() const {
    return dbg_line_insts_.empty() ? nullptr : &dbg_line_insts_[0];
  }


  void clear_dbg_line_insts() { dbg_line_insts_.clear(); }







  iterator begin() { return operands_.begin(); }
  iterator end() { return operands_.end(); }
  const_iterator begin() const { return operands_.cbegin(); }
  const_iterator end() const { return operands_.cend(); }

  const_iterator cbegin() const { return operands_.cbegin(); }
  const_iterator cend() const { return operands_.cend(); }


  uint32_t NumOperands() const {
    return static_cast<uint32_t>(operands_.size());
  }

  uint32_t NumOperandWords() const {
    return NumInOperandWords() + TypeResultIdCount();
  }

  inline Operand& GetOperand(uint32_t index);
  inline const Operand& GetOperand(uint32_t index) const;



  inline void AddOperand(Operand&& operand);

  inline void AddOperand(const Operand& operand);



  uint32_t GetSingleWordOperand(uint32_t index) const;

  inline void SetInOperand(uint32_t index, Operand::OperandData&& data);



  inline void SetOperand(uint32_t index, Operand::OperandData&& data);

  inline void SetInOperands(OperandList&& new_operands);

  inline void SetResultType(uint32_t ty_id);
  inline bool HasResultType() const { return has_type_id_; }

  inline void SetResultId(uint32_t res_id);
  inline bool HasResultId() const { return has_result_id_; }

  inline void SetDebugScope(const DebugScope& scope);
  inline const DebugScope& GetDebugScope() const { return dbg_scope_; }

  bool AddDebugLine(const Instruction* inst);

  void UpdateDebugInlinedAt(uint32_t new_inlined_at);


  void ClearDbgLineInsts();

  bool IsDebugLineInst() const;

  bool IsLineInst() const;

  bool IsLine() const;

  bool IsNoLine() const;
  inline uint32_t GetDebugInlinedAt() const {
    return dbg_scope_.GetInlinedAt();
  }

  void UpdateLexicalScope(uint32_t scope);

  bool UpdateDebugInfoFrom(const Instruction* from,
                           const Instruction* line = nullptr);

  void RemoveOperand(uint32_t index) {
    operands_.erase(operands_.begin() + index);
  }

  void InsertOperand(uint32_t index, Operand&& operand) {
    operands_.insert(operands_.begin() + index, operand);
  }


  uint32_t NumInOperands() const {
    return static_cast<uint32_t>(operands_.size() - TypeResultIdCount());
  }
  uint32_t NumInOperandWords() const;
  Operand& GetInOperand(uint32_t index) {
    return GetOperand(index + TypeResultIdCount());
  }
  const Operand& GetInOperand(uint32_t index) const {
    return GetOperand(index + TypeResultIdCount());
  }
  uint32_t GetSingleWordInOperand(uint32_t index) const {
    return GetSingleWordOperand(index + TypeResultIdCount());
  }
  void RemoveInOperand(uint32_t index) {
    operands_.erase(operands_.begin() + index + TypeResultIdCount());
  }


  inline bool IsNop() const;


  inline void ToNop();




  inline void ForEachInst(const std::function<void(Instruction*)>& f,
                          bool run_on_debug_line_insts = false);
  inline void ForEachInst(const std::function<void(const Instruction*)>& f,
                          bool run_on_debug_line_insts = false) const;





  inline bool WhileEachInst(const std::function<bool(Instruction*)>& f,
                            bool run_on_debug_line_insts = false);
  inline bool WhileEachInst(const std::function<bool(const Instruction*)>& f,
                            bool run_on_debug_line_insts = false) const;




  inline void ForEachId(const std::function<void(uint32_t*)>& f);
  inline void ForEachId(const std::function<void(const uint32_t*)>& f) const;


  inline void ForEachInId(const std::function<void(uint32_t*)>& f);
  inline void ForEachInId(const std::function<void(const uint32_t*)>& f) const;



  inline bool WhileEachInId(const std::function<bool(uint32_t*)>& f);
  inline bool WhileEachInId(
      const std::function<bool(const uint32_t*)>& f) const;


  inline void ForEachInOperand(const std::function<void(uint32_t*)>& f);
  inline void ForEachInOperand(
      const std::function<void(const uint32_t*)>& f) const;



  inline bool WhileEachInOperand(const std::function<bool(uint32_t*)>& f);
  inline bool WhileEachInOperand(
      const std::function<bool(const uint32_t*)>& f) const;



  bool HasBranchWeights() const;


  inline bool HasLabels() const;


  void ToBinaryWithoutAttachedDebugInsts(std::vector<uint32_t>* binary) const;




  void ReplaceOperands(const OperandList& new_operands);


  inline bool IsDecoration() const;



  bool IsReadOnlyLoad() const;








  Instruction* GetBaseAddress() const;




  inline bool IsLoad() const;








  bool IsReadOnlyPointer() const;






  bool IsVulkanStorageImage() const;



  bool IsVulkanSampledImage() const;



  bool IsVulkanStorageTexelBuffer() const;



  bool IsVulkanStorageBuffer() const;



  bool IsVulkanStorageBufferVariable() const;



  bool IsVulkanUniformBuffer() const;



  inline bool IsAtomicWithLoad() const;


  inline bool IsAtomicOp() const;



  bool IsBranch() const { return spvOpcodeIsBranch(opcode()); }



  bool IsReturn() const { return spvOpcodeIsReturn(opcode()); }


  bool IsReturnOrAbort() const { return spvOpcodeIsReturnOrAbort(opcode()); }


  bool IsBlockTerminator() const {
    return spvOpcodeIsBlockTerminator(opcode());
  }




  bool IsOpaqueType() const;



  bool IsFoldable() const;



  bool IsFoldableByFoldScalar() const;



  bool IsFoldableByFoldVector() const;




  bool IsFloatingPointFoldingAllowed() const;

  inline bool operator==(const Instruction&) const;
  inline bool operator!=(const Instruction&) const;
  inline bool operator<(const Instruction&) const;



  Instruction* InsertBefore(std::unique_ptr<Instruction>&& i);



  Instruction* InsertBefore(std::vector<std::unique_ptr<Instruction>>&& list);
  using utils::IntrusiveNodeBase<Instruction>::InsertBefore;



  inline bool IsConstant() const;


  bool IsOpcodeCodeMotionSafe() const;








  std::string PrettyPrint(uint32_t options = 0u) const;



  bool IsScalarizable() const;


  bool IsOpcodeSafeToDelete() const;





  bool IsValidBasePointer() const;




  OpenCLDebugInfo100Instructions GetOpenCL100DebugOpcode() const;




  NonSemanticShaderDebugInfoInstructions GetShaderDebugOpcode() const;




  CommonDebugInfoInstructions GetCommonDebugOpcode() const;


  bool IsOpenCL100DebugInstr() const {
    return GetOpenCL100DebugOpcode() != OpenCLDebugInfo100InstructionsMax;
  }


  bool IsShaderDebugInstr() const {
    return GetShaderDebugOpcode() != NonSemanticShaderDebugInfoInstructionsMax;
  }
  bool IsCommonDebugInstr() const {
    return GetCommonDebugOpcode() != CommonDebugInfoInstructionsMax;
  }


  bool IsNonSemanticInstruction() const;



  void Dump() const;

 private:

  uint32_t TypeResultIdCount() const {
    if (has_type_id_ && has_result_id_) return 2;
    if (has_type_id_ || has_result_id_) return 1;
    return 0;
  }





  bool IsReadOnlyPointerShaders() const;
  bool IsReadOnlyPointerKernel() const;



  bool IsValidBaseImage() const;

  IRContext* context_;
  spv::Op opcode_;
  bool has_type_id_;
  bool has_result_id_;
  uint32_t unique_id_;

  OperandList operands_;



  std::vector<Instruction> dbg_line_insts_;


  DebugScope dbg_scope_;

  friend InstructionList;
};







std::ostream& operator<<(std::ostream& str, const Instruction& inst);

inline bool Instruction::operator==(const Instruction& other) const {
  return unique_id() == other.unique_id();
}

inline bool Instruction::operator!=(const Instruction& other) const {
  return !(*this == other);
}

inline bool Instruction::operator<(const Instruction& other) const {
  return unique_id() < other.unique_id();
}

inline Operand& Instruction::GetOperand(uint32_t index) {
  assert(index < operands_.size() && "operand index out of bound");
  return operands_[index];
}

inline const Operand& Instruction::GetOperand(uint32_t index) const {
  assert(index < operands_.size() && "operand index out of bound");
  return operands_[index];
}

inline void Instruction::AddOperand(Operand&& operand) {
  operands_.push_back(std::move(operand));
}

inline void Instruction::AddOperand(const Operand& operand) {
  operands_.push_back(operand);
}

inline void Instruction::SetInOperand(uint32_t index,
                                      Operand::OperandData&& data) {
  SetOperand(index + TypeResultIdCount(), std::move(data));
}

inline void Instruction::SetOperand(uint32_t index,
                                    Operand::OperandData&& data) {
  assert(index < operands_.size() && "operand index out of bound");
  assert(index >= TypeResultIdCount() && "operand is not a in-operand");
  operands_[index].words = std::move(data);
}

inline void Instruction::SetInOperands(OperandList&& new_operands) {

  operands_.erase(operands_.begin() + TypeResultIdCount(), operands_.end());

  operands_.insert(operands_.end(), new_operands.begin(), new_operands.end());
}

inline void Instruction::SetResultId(uint32_t res_id) {



  assert(has_result_id_);




  assert(res_id != 0);

  auto ridx = has_type_id_ ? 1 : 0;
  operands_[ridx].words = {res_id};
}

inline void Instruction::SetDebugScope(const DebugScope& scope) {
  dbg_scope_ = scope;
  for (auto& i : dbg_line_insts_) {
    i.dbg_scope_ = scope;
  }
}

inline void Instruction::SetResultType(uint32_t ty_id) {



  assert(has_type_id_);




  assert(ty_id != 0);

  operands_.front().words = {ty_id};
}

inline bool Instruction::IsNop() const {
  return opcode_ == spv::Op::OpNop && !has_type_id_ && !has_result_id_ &&
         operands_.empty();
}

inline void Instruction::ToNop() {
  opcode_ = spv::Op::OpNop;
  has_type_id_ = false;
  has_result_id_ = false;
  operands_.clear();
}

inline bool Instruction::WhileEachInst(
    const std::function<bool(Instruction*)>& f, bool run_on_debug_line_insts) {
  if (run_on_debug_line_insts) {
    for (auto& dbg_line : dbg_line_insts_) {
      if (!f(&dbg_line)) return false;
    }
  }
  return f(this);
}

inline bool Instruction::WhileEachInst(
    const std::function<bool(const Instruction*)>& f,
    bool run_on_debug_line_insts) const {
  if (run_on_debug_line_insts) {
    for (auto& dbg_line : dbg_line_insts_) {
      if (!f(&dbg_line)) return false;
    }
  }
  return f(this);
}

inline void Instruction::ForEachInst(const std::function<void(Instruction*)>& f,
                                     bool run_on_debug_line_insts) {
  WhileEachInst(
      [&f](Instruction* inst) {
        f(inst);
        return true;
      },
      run_on_debug_line_insts);
}

inline void Instruction::ForEachInst(
    const std::function<void(const Instruction*)>& f,
    bool run_on_debug_line_insts) const {
  WhileEachInst(
      [&f](const Instruction* inst) {
        f(inst);
        return true;
      },
      run_on_debug_line_insts);
}

inline void Instruction::ForEachId(const std::function<void(uint32_t*)>& f) {
  for (auto& operand : operands_)
    if (spvIsIdType(operand.type)) f(&operand.words[0]);
}

inline void Instruction::ForEachId(
    const std::function<void(const uint32_t*)>& f) const {
  for (const auto& operand : operands_)
    if (spvIsIdType(operand.type)) f(&operand.words[0]);
}

inline bool Instruction::WhileEachInId(
    const std::function<bool(uint32_t*)>& f) {
  for (auto& operand : operands_) {
    if (spvIsInIdType(operand.type) && !f(&operand.words[0])) {
      return false;
    }
  }
  return true;
}

inline bool Instruction::WhileEachInId(
    const std::function<bool(const uint32_t*)>& f) const {
  for (const auto& operand : operands_) {
    if (spvIsInIdType(operand.type) && !f(&operand.words[0])) {
      return false;
    }
  }
  return true;
}

inline void Instruction::ForEachInId(const std::function<void(uint32_t*)>& f) {
  WhileEachInId([&f](uint32_t* id) {
    f(id);
    return true;
  });
}

inline void Instruction::ForEachInId(
    const std::function<void(const uint32_t*)>& f) const {
  WhileEachInId([&f](const uint32_t* id) {
    f(id);
    return true;
  });
}

inline bool Instruction::WhileEachInOperand(
    const std::function<bool(uint32_t*)>& f) {
  for (auto& operand : operands_) {
    switch (operand.type) {
      case SPV_OPERAND_TYPE_RESULT_ID:
      case SPV_OPERAND_TYPE_TYPE_ID:
        break;
      default:
        if (!f(&operand.words[0])) return false;
        break;
    }
  }
  return true;
}

inline bool Instruction::WhileEachInOperand(
    const std::function<bool(const uint32_t*)>& f) const {
  for (const auto& operand : operands_) {
    switch (operand.type) {
      case SPV_OPERAND_TYPE_RESULT_ID:
      case SPV_OPERAND_TYPE_TYPE_ID:
        break;
      default:
        if (!f(&operand.words[0])) return false;
        break;
    }
  }
  return true;
}

inline void Instruction::ForEachInOperand(
    const std::function<void(uint32_t*)>& f) {
  WhileEachInOperand([&f](uint32_t* operand) {
    f(operand);
    return true;
  });
}

inline void Instruction::ForEachInOperand(
    const std::function<void(const uint32_t*)>& f) const {
  WhileEachInOperand([&f](const uint32_t* operand) {
    f(operand);
    return true;
  });
}

inline bool Instruction::HasLabels() const {
  switch (opcode_) {
    case spv::Op::OpSelectionMerge:
    case spv::Op::OpBranch:
    case spv::Op::OpLoopMerge:
    case spv::Op::OpBranchConditional:
    case spv::Op::OpSwitch:
    case spv::Op::OpPhi:
      return true;
      break;
    default:
      break;
  }
  return false;
}

bool Instruction::IsDecoration() const {
  return spvOpcodeIsDecoration(opcode());
}

bool Instruction::IsLoad() const { return spvOpcodeIsLoad(opcode()); }

bool Instruction::IsAtomicWithLoad() const {
  return spvOpcodeIsAtomicWithLoad(opcode());
}

bool Instruction::IsAtomicOp() const { return spvOpcodeIsAtomicOp(opcode()); }

bool Instruction::IsConstant() const {
  return IsConstantInst(opcode()) && !IsSpecConstantInst(opcode());
}
}
}

#endif
