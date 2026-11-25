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

#include "source/opt/folding_rules.h"

#include <limits>
#include <memory>
#include <optional>
#include <utility>

#include "ir_builder.h"
#include "source/latest_version_glsl_std_450_header.h"
#include "source/opt/ir_context.h"

namespace spvtools {
namespace opt {
namespace {

constexpr uint32_t kExtractCompositeIdInIdx = 0;
constexpr uint32_t kInsertObjectIdInIdx = 0;
constexpr uint32_t kInsertCompositeIdInIdx = 1;
constexpr uint32_t kExtInstSetIdInIdx = 0;
constexpr uint32_t kExtInstInstructionInIdx = 1;
constexpr uint32_t kFMixXIdInIdx = 2;
constexpr uint32_t kFMixYIdInIdx = 3;
constexpr uint32_t kFMixAIdInIdx = 4;
constexpr uint32_t kStoreObjectInIdx = 1;




int32_t ImageOperandsMaskInOperandIndex(Instruction* inst) {
  const auto opcode = inst->opcode();
  switch (opcode) {
    case spv::Op::OpImageSampleImplicitLod:
    case spv::Op::OpImageSampleExplicitLod:
    case spv::Op::OpImageSampleProjImplicitLod:
    case spv::Op::OpImageSampleProjExplicitLod:
    case spv::Op::OpImageFetch:
    case spv::Op::OpImageRead:
    case spv::Op::OpImageSparseSampleImplicitLod:
    case spv::Op::OpImageSparseSampleExplicitLod:
    case spv::Op::OpImageSparseSampleProjImplicitLod:
    case spv::Op::OpImageSparseSampleProjExplicitLod:
    case spv::Op::OpImageSparseFetch:
    case spv::Op::OpImageSparseRead:
      return inst->NumOperands() > 4 ? 2 : -1;
    case spv::Op::OpImageSampleDrefImplicitLod:
    case spv::Op::OpImageSampleDrefExplicitLod:
    case spv::Op::OpImageSampleProjDrefImplicitLod:
    case spv::Op::OpImageSampleProjDrefExplicitLod:
    case spv::Op::OpImageGather:
    case spv::Op::OpImageDrefGather:
    case spv::Op::OpImageSparseSampleDrefImplicitLod:
    case spv::Op::OpImageSparseSampleDrefExplicitLod:
    case spv::Op::OpImageSparseSampleProjDrefImplicitLod:
    case spv::Op::OpImageSparseSampleProjDrefExplicitLod:
    case spv::Op::OpImageSparseGather:
    case spv::Op::OpImageSparseDrefGather:
      return inst->NumOperands() > 5 ? 3 : -1;
    case spv::Op::OpImageWrite:
      return inst->NumOperands() > 3 ? 3 : -1;
    default:
      return -1;
  }
}


uint32_t ElementWidth(const analysis::Type* type) {
  if (const analysis::CooperativeVectorNV* coopvec_type =
          type->AsCooperativeVectorNV()) {
    return ElementWidth(coopvec_type->component_type());
  } else if (const analysis::Vector* vec_type = type->AsVector()) {
    return ElementWidth(vec_type->element_type());
  } else if (const analysis::Float* float_type = type->AsFloat()) {
    return float_type->width();
  } else {
    assert(type->AsInteger());
    return type->AsInteger()->width();
  }
}


bool HasFloatingPoint(const analysis::Type* type) {
  if (type->AsFloat()) {
    return true;
  } else if (const analysis::Vector* vec_type = type->AsVector()) {
    return vec_type->element_type()->AsFloat() != nullptr;
  }

  return false;
}


template <typename T>
bool IsValidResult(T val) {
  int classified = std::fpclassify(val);
  switch (classified) {
    case FP_NAN:
    case FP_INFINITE:
    case FP_SUBNORMAL:
      return false;
    default:
      return true;
  }
}

const analysis::Constant* ConstInput(
    const std::vector<const analysis::Constant*>& constants) {
  return constants[0] ? constants[0] : constants[1];
}

Instruction* NonConstInput(IRContext* context, const analysis::Constant* c,
                           Instruction* inst) {
  uint32_t in_op = c ? 1u : 0u;
  return context->get_def_use_mgr()->GetDef(
      inst->GetSingleWordInOperand(in_op));
}

std::vector<uint32_t> ExtractInts(uint64_t val) {
  std::vector<uint32_t> words;
  words.push_back(static_cast<uint32_t>(val));
  words.push_back(static_cast<uint32_t>(val >> 32));
  return words;
}

std::vector<uint32_t> GetWordsFromScalarIntConstant(
    const analysis::IntConstant* c) {
  assert(c != nullptr);
  uint32_t width = c->type()->AsInteger()->width();
  assert(width == 8 || width == 16 || width == 32 || width == 64);
  if (width == 64) {
    uint64_t uval = static_cast<uint64_t>(c->GetU64());
    return ExtractInts(uval);
  }


  return {c->GetU32BitValue()};
}

std::vector<uint32_t> GetWordsFromScalarFloatConstant(
    const analysis::FloatConstant* c) {
  assert(c != nullptr);
  uint32_t width = c->type()->AsFloat()->width();
  assert(width == 16 || width == 32 || width == 64);
  if (width == 64) {
    utils::FloatProxy<double> result(c->GetDouble());
    return result.GetWords();
  }


  return {c->GetU32BitValue()};
}

std::vector<uint32_t> GetWordsFromNumericScalarOrVectorConstant(
    analysis::ConstantManager* const_mgr, const analysis::Constant* c) {
  if (const auto* float_constant = c->AsFloatConstant()) {
    return GetWordsFromScalarFloatConstant(float_constant);
  } else if (const auto* int_constant = c->AsIntConstant()) {
    return GetWordsFromScalarIntConstant(int_constant);
  } else if (const auto* vec_constant = c->AsVectorConstant()) {
    std::vector<uint32_t> words;

    for (const auto* comp : vec_constant->GetComponents()) {
      auto comp_in_words =
          GetWordsFromNumericScalarOrVectorConstant(const_mgr, comp);
      words.insert(words.end(), comp_in_words.begin(), comp_in_words.end());
    }

    if (ElementWidth(c->type()) >= 32) {
      return words;
    }

    if (ElementWidth(c->type()) == 8) {
      assert(words.size() <= 4);


      uint32_t compacted_word = 0;
      for (int32_t i = static_cast<int32_t>(words.size()) - 1; i >= 0; --i) {
        compacted_word <<= 8;
        compacted_word |= (words[i] & 0xFF);
      }
      return {compacted_word};
    } else if (ElementWidth(c->type()) == 16) {
      assert(words.size() <= 4);
      std::vector<uint32_t> compacted_words;


      for (uint32_t i = 0; i < words.size(); i += 2) {
        uint32_t word1 = words[i];
        uint32_t word2 = (i + 1 < words.size()) ? words[i + 1] : 0;
        uint32_t compacted_word = (word2 << 16) | (word1 & 0xFFFF);
        compacted_words.push_back(compacted_word);
      }
      return compacted_words;
    }
    assert(false && "Unhandled element width");
  } else if (c->AsNullConstant()) {
    uint32_t num_elements = 1;

    if (const auto* vec_type = c->type()->AsVector()) {
      num_elements = vec_type->element_count();
    }



    uint32_t element_width = ElementWidth(c->type());
    if (element_width < 32) {
      num_elements = (num_elements + 1) / 2;
    } else if (element_width == 64) {
      num_elements = num_elements * 2;
    }
    return std::vector<uint32_t>(num_elements, 0);
  }
  return {};
}

const analysis::Constant* ConvertWordsToNumericScalarOrVectorConstant(
    analysis::ConstantManager* const_mgr, const std::vector<uint32_t>& words,
    const analysis::Type* type) {
  const spvtools::opt::analysis::Integer* int_type = type->AsInteger();

  if (int_type && int_type->width() <= 32) {
    assert(words.size() == 1);
    return const_mgr->GenerateIntegerConstant(int_type, words[0]);
  }

  if (int_type || type->AsFloat()) return const_mgr->GetConstant(type, words);
  if (const auto* vec_type = type->AsVector())
    return const_mgr->GetNumericVectorConstantWithWords(vec_type, words);
  return nullptr;
}



uint32_t NegateFloatingPointConstant(analysis::ConstantManager* const_mgr,
                                     const analysis::Constant* c) {
  assert(c);
  assert(c->type()->AsFloat());
  uint32_t width = c->type()->AsFloat()->width();
  assert(width == 32 || width == 64);
  std::vector<uint32_t> words;
  if (width == 64) {
    utils::FloatProxy<double> result(c->GetDouble() * -1.0);
    words = result.GetWords();
  } else {
    utils::FloatProxy<float> result(c->GetFloat() * -1.0f);
    words = result.GetWords();
  }

  const analysis::Constant* negated_const =
      const_mgr->GetConstant(c->type(), std::move(words));
  return const_mgr->GetDefiningInstruction(negated_const)->result_id();
}


uint32_t NegateIntegerConstant(analysis::ConstantManager* const_mgr,
                               const analysis::Constant* c) {
  assert(c);
  assert(c->type()->AsInteger());
  uint32_t width = c->type()->AsInteger()->width();
  assert(width == 32 || width == 64);
  std::vector<uint32_t> words;
  if (width == 64) {
    uint64_t uval = static_cast<uint64_t>(0 - c->GetU64());
    words = ExtractInts(uval);
  } else {
    words.push_back(static_cast<uint32_t>(0 - c->GetU32()));
  }

  const analysis::Constant* negated_const =
      const_mgr->GetConstant(c->type(), std::move(words));
  return const_mgr->GetDefiningInstruction(negated_const)->result_id();
}


uint32_t NegateVectorConstant(analysis::ConstantManager* const_mgr,
                              const analysis::Constant* c) {
  assert(const_mgr && c);
  assert(c->type()->AsVector());
  if (c->AsNullConstant()) {

    return const_mgr->GetDefiningInstruction(c)->result_id();
  } else {
    const analysis::Type* component_type =
        c->AsVectorConstant()->component_type();
    std::vector<uint32_t> words;
    for (auto& comp : c->AsVectorConstant()->GetComponents()) {
      if (component_type->AsFloat()) {
        words.push_back(NegateFloatingPointConstant(const_mgr, comp));
      } else {
        assert(component_type->AsInteger());
        words.push_back(NegateIntegerConstant(const_mgr, comp));
      }
    }

    const analysis::Constant* negated_const =
        const_mgr->GetConstant(c->type(), std::move(words));
    return const_mgr->GetDefiningInstruction(negated_const)->result_id();
  }
}


uint32_t NegateConstant(analysis::ConstantManager* const_mgr,
                        const analysis::Constant* c) {
  if (c->type()->AsVector()) {
    return NegateVectorConstant(const_mgr, c);
  } else if (c->type()->AsFloat()) {
    return NegateFloatingPointConstant(const_mgr, c);
  } else {
    assert(c->type()->AsInteger());
    return NegateIntegerConstant(const_mgr, c);
  }
}



uint32_t Reciprocal(analysis::ConstantManager* const_mgr,
                    const analysis::Constant* c) {
  assert(const_mgr && c);
  assert(c->type()->AsFloat());

  uint32_t width = c->type()->AsFloat()->width();
  assert(width == 32 || width == 64);
  std::vector<uint32_t> words;

  if (c->IsZero()) {
    return 0;
  }

  if (width == 64) {
    spvtools::utils::FloatProxy<double> result(1.0 / c->GetDouble());
    if (!IsValidResult(result.getAsFloat())) return 0;
    words = result.GetWords();
  } else {
    spvtools::utils::FloatProxy<float> result(1.0f / c->GetFloat());
    if (!IsValidResult(result.getAsFloat())) return 0;
    words = result.GetWords();
  }

  const analysis::Constant* negated_const =
      const_mgr->GetConstant(c->type(), std::move(words));
  return const_mgr->GetDefiningInstruction(negated_const)->result_id();
}


FoldingRule ReciprocalFDiv() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(inst->opcode() == spv::Op::OpFDiv);
    analysis::ConstantManager* const_mgr = context->get_constant_mgr();
    const analysis::Type* type =
        context->get_type_mgr()->GetType(inst->type_id());

    if (type->IsCooperativeMatrix()) {
      return false;
    }

    if (!inst->IsFloatingPointFoldingAllowed()) return false;

    uint32_t width = ElementWidth(type);
    if (width != 32 && width != 64) return false;

    if (constants[1] != nullptr) {
      uint32_t id = 0;
      if (const analysis::VectorConstant* vector_const =
              constants[1]->AsVectorConstant()) {
        std::vector<uint32_t> neg_ids;
        for (auto& comp : vector_const->GetComponents()) {
          id = Reciprocal(const_mgr, comp);
          if (id == 0) return false;
          neg_ids.push_back(id);
        }
        const analysis::Constant* negated_const =
            const_mgr->GetConstant(constants[1]->type(), std::move(neg_ids));
        id = const_mgr->GetDefiningInstruction(negated_const)->result_id();
      } else if (constants[1]->AsFloatConstant()) {
        id = Reciprocal(const_mgr, constants[1]);
        if (id == 0) return false;
      } else {

        return false;
      }
      inst->SetOpcode(spv::Op::OpFMul);
      inst->SetInOperands(
          {{SPV_OPERAND_TYPE_ID, {inst->GetSingleWordInOperand(0u)}},
           {SPV_OPERAND_TYPE_ID, {id}}});
      return true;
    }

    return false;
  };
}


FoldingRule MergeNegateArithmetic() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(inst->opcode() == spv::Op::OpFNegate ||
           inst->opcode() == spv::Op::OpSNegate);
    (void)constants;
    const analysis::Type* type =
        context->get_type_mgr()->GetType(inst->type_id());
    if (HasFloatingPoint(type) && !inst->IsFloatingPointFoldingAllowed())
      return false;

    Instruction* op_inst =
        context->get_def_use_mgr()->GetDef(inst->GetSingleWordInOperand(0u));
    if (HasFloatingPoint(type) && !op_inst->IsFloatingPointFoldingAllowed())
      return false;

    if (op_inst->opcode() == inst->opcode()) {

      inst->SetOpcode(spv::Op::OpCopyObject);
      inst->SetInOperands(
          {{SPV_OPERAND_TYPE_ID, {op_inst->GetSingleWordInOperand(0u)}}});
      return true;
    }

    return false;
  };
}








FoldingRule MergeNegateMulDivArithmetic() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(inst->opcode() == spv::Op::OpFNegate ||
           inst->opcode() == spv::Op::OpSNegate);
    (void)constants;
    analysis::ConstantManager* const_mgr = context->get_constant_mgr();
    const analysis::Type* type =
        context->get_type_mgr()->GetType(inst->type_id());

    if (type->IsCooperativeMatrix()) {
      return false;
    }

    if (HasFloatingPoint(type) && !inst->IsFloatingPointFoldingAllowed())
      return false;

    Instruction* op_inst =
        context->get_def_use_mgr()->GetDef(inst->GetSingleWordInOperand(0u));
    if (HasFloatingPoint(type) && !op_inst->IsFloatingPointFoldingAllowed())
      return false;

    uint32_t width = ElementWidth(type);
    if (width != 32 && width != 64) return false;

    spv::Op opcode = op_inst->opcode();
    if (opcode != spv::Op::OpFMul && opcode != spv::Op::OpFDiv &&
        opcode != spv::Op::OpIMul && opcode != spv::Op::OpSDiv) {
      return false;
    }

    std::vector<const analysis::Constant*> op_constants =
        const_mgr->GetOperandConstants(op_inst);

    if (op_constants[0] == nullptr && op_constants[1] == nullptr) {
      return false;
    }

    bool zero_is_variable = op_constants[0] == nullptr;
    const analysis::Constant* c = ConstInput(op_constants);
    uint32_t neg_id = NegateConstant(const_mgr, c);
    uint32_t non_const_id = zero_is_variable
                                ? op_inst->GetSingleWordInOperand(0u)
                                : op_inst->GetSingleWordInOperand(1u);

    inst->SetOpcode(op_inst->opcode());
    if (opcode == spv::Op::OpFDiv || opcode == spv::Op::OpUDiv ||
        opcode == spv::Op::OpSDiv) {
      uint32_t op0 = zero_is_variable ? non_const_id : neg_id;
      uint32_t op1 = zero_is_variable ? neg_id : non_const_id;
      inst->SetInOperands(
          {{SPV_OPERAND_TYPE_ID, {op0}}, {SPV_OPERAND_TYPE_ID, {op1}}});
    } else {
      inst->SetInOperands({{SPV_OPERAND_TYPE_ID, {non_const_id}},
                           {SPV_OPERAND_TYPE_ID, {neg_id}}});
    }
    return true;
  };
}








FoldingRule MergeNegateAddSubArithmetic() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(inst->opcode() == spv::Op::OpFNegate ||
           inst->opcode() == spv::Op::OpSNegate);
    (void)constants;
    analysis::ConstantManager* const_mgr = context->get_constant_mgr();
    const analysis::Type* type =
        context->get_type_mgr()->GetType(inst->type_id());

    if (type->IsCooperativeMatrix()) {
      return false;
    }

    if (HasFloatingPoint(type) && !inst->IsFloatingPointFoldingAllowed())
      return false;

    Instruction* op_inst =
        context->get_def_use_mgr()->GetDef(inst->GetSingleWordInOperand(0u));
    if (HasFloatingPoint(type) && !op_inst->IsFloatingPointFoldingAllowed())
      return false;

    uint32_t width = ElementWidth(type);
    if (width != 32 && width != 64) return false;

    if (op_inst->opcode() == spv::Op::OpFAdd ||
        op_inst->opcode() == spv::Op::OpFSub ||
        op_inst->opcode() == spv::Op::OpIAdd ||
        op_inst->opcode() == spv::Op::OpISub) {
      std::vector<const analysis::Constant*> op_constants =
          const_mgr->GetOperandConstants(op_inst);
      if (op_constants[0] || op_constants[1]) {
        bool zero_is_variable = op_constants[0] == nullptr;
        bool is_add = (op_inst->opcode() == spv::Op::OpFAdd) ||
                      (op_inst->opcode() == spv::Op::OpIAdd);
        bool swap_operands = !is_add || zero_is_variable;
        bool negate_const = is_add;
        const analysis::Constant* c = ConstInput(op_constants);
        uint32_t const_id = 0;
        if (negate_const) {
          const_id = NegateConstant(const_mgr, c);
        } else {
          const_id = zero_is_variable ? op_inst->GetSingleWordInOperand(1u)
                                      : op_inst->GetSingleWordInOperand(0u);
        }


        uint32_t op0 =
            zero_is_variable ? op_inst->GetSingleWordInOperand(0u) : const_id;
        uint32_t op1 =
            zero_is_variable ? const_id : op_inst->GetSingleWordInOperand(1u);
        if (swap_operands) std::swap(op0, op1);
        inst->SetOpcode(HasFloatingPoint(type) ? spv::Op::OpFSub
                                               : spv::Op::OpISub);
        inst->SetInOperands(
            {{SPV_OPERAND_TYPE_ID, {op0}}, {SPV_OPERAND_TYPE_ID, {op1}}});
        return true;
      }
    }

    return false;
  };
}


bool HasZero(const analysis::Constant* c) {
  if (c->AsNullConstant()) {
    return true;
  }
  if (const analysis::VectorConstant* vec_const = c->AsVectorConstant()) {
    for (auto& comp : vec_const->GetComponents())
      if (HasZero(comp)) return true;
  } else {
    assert(c->AsScalarConstant());
    return c->AsScalarConstant()->IsZero();
  }

  return false;
}




uint32_t PerformFloatingPointOperation(analysis::ConstantManager* const_mgr,
                                       spv::Op opcode,
                                       const analysis::Constant* input1,
                                       const analysis::Constant* input2) {
  const analysis::Type* type = input1->type();
  assert(type->AsFloat());
  uint32_t width = type->AsFloat()->width();
  assert(width == 32 || width == 64);
  std::vector<uint32_t> words;
#define FOLD_OP(op)                                                          \
  if (width == 64) {                                                         \
    utils::FloatProxy<double> val =                                          \
        input1->GetDouble() op input2->GetDouble();                          \
    double dval = val.getAsFloat();                                          \
    if (!IsValidResult(dval)) return 0;                                      \
    words = val.GetWords();                                                  \
  } else {                                                                   \
    utils::FloatProxy<float> val = input1->GetFloat() op input2->GetFloat(); \
    float fval = val.getAsFloat();                                           \
    if (!IsValidResult(fval)) return 0;                                      \
    words = val.GetWords();                                                  \
  }                                                                          \
  static_assert(true, "require extra semicolon")
  switch (opcode) {
    case spv::Op::OpFMul:
      FOLD_OP(*);
      break;
    case spv::Op::OpFDiv:
      if (HasZero(input2)) return 0;
      FOLD_OP(/);
      break;
    case spv::Op::OpFAdd:
      FOLD_OP(+);
      break;
    case spv::Op::OpFSub:
      FOLD_OP(-);
      break;
    default:
      assert(false && "Unexpected operation");
      break;
  }
#undef FOLD_OP
  const analysis::Constant* merged_const = const_mgr->GetConstant(type, words);
  return const_mgr->GetDefiningInstruction(merged_const)->result_id();
}




uint32_t PerformIntegerOperation(analysis::ConstantManager* const_mgr,
                                 spv::Op opcode,
                                 const analysis::Constant* input1,
                                 const analysis::Constant* input2) {
  assert(input1->type()->AsInteger());
  const analysis::Integer* type = input1->type()->AsInteger();
  uint32_t width = type->AsInteger()->width();
  assert(width == 32 || width == 64);
  std::vector<uint32_t> words;




#define FOLD_OP(op)                                      \
  if (width == 64) {                                     \
    uint64_t val = input1->GetU64() op input2->GetU64(); \
    words = ExtractInts(val);                            \
  } else {                                               \
    uint32_t val = input1->GetU32() op input2->GetU32(); \
    words.push_back(val);                                \
  }                                                      \
  static_assert(true, "require extra semicolon")
  switch (opcode) {
    case spv::Op::OpIMul:
      FOLD_OP(*);
      break;
    case spv::Op::OpSDiv:
    case spv::Op::OpUDiv:
      assert(false && "Should not merge integer division");
      break;
    case spv::Op::OpIAdd:
      FOLD_OP(+);
      break;
    case spv::Op::OpISub:
      FOLD_OP(-);
      break;
    case spv::Op::OpBitwiseXor:
      FOLD_OP(^);
      break;
    case spv::Op::OpBitwiseOr:
      FOLD_OP(|);
      break;
    case spv::Op::OpBitwiseAnd:
      FOLD_OP(&);
      break;
    default:
      assert(false && "Unexpected operation");
      break;
  }
#undef FOLD_OP
  const analysis::Constant* merged_const = const_mgr->GetConstant(type, words);
  return const_mgr->GetDefiningInstruction(merged_const)->result_id();
}




uint32_t PerformOperation(analysis::ConstantManager* const_mgr, spv::Op opcode,
                          const analysis::Constant* input1,
                          const analysis::Constant* input2) {
  assert(input1 && input2);
  const analysis::Type* type = input1->type();
  std::vector<uint32_t> words;
  if (const analysis::Vector* vector_type = type->AsVector()) {
    const analysis::Type* ele_type = vector_type->element_type();
    for (uint32_t i = 0; i != vector_type->element_count(); ++i) {
      uint32_t id = 0;

      const analysis::Constant* input1_comp = nullptr;
      if (const analysis::VectorConstant* input1_vector =
              input1->AsVectorConstant()) {
        input1_comp = input1_vector->GetComponents()[i];
      } else {
        assert(input1->AsNullConstant());
        input1_comp = const_mgr->GetConstant(ele_type, {});
      }

      const analysis::Constant* input2_comp = nullptr;
      if (const analysis::VectorConstant* input2_vector =
              input2->AsVectorConstant()) {
        input2_comp = input2_vector->GetComponents()[i];
      } else {
        assert(input2->AsNullConstant());
        input2_comp = const_mgr->GetConstant(ele_type, {});
      }

      if (ele_type->AsFloat()) {
        id = PerformFloatingPointOperation(const_mgr, opcode, input1_comp,
                                           input2_comp);
      } else {
        assert(ele_type->AsInteger());
        id = PerformIntegerOperation(const_mgr, opcode, input1_comp,
                                     input2_comp);
      }
      if (id == 0) return 0;
      words.push_back(id);
    }
    const analysis::Constant* merged_const =
        const_mgr->GetConstant(type, words);
    return const_mgr->GetDefiningInstruction(merged_const)->result_id();
  } else if (type->AsFloat()) {
    return PerformFloatingPointOperation(const_mgr, opcode, input1, input2);
  } else {
    assert(type->AsInteger());
    return PerformIntegerOperation(const_mgr, opcode, input1, input2);
  }
}







FoldingRule MergeMulMulArithmetic() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(inst->opcode() == spv::Op::OpFMul ||
           inst->opcode() == spv::Op::OpIMul);
    analysis::ConstantManager* const_mgr = context->get_constant_mgr();
    const analysis::Type* type =
        context->get_type_mgr()->GetType(inst->type_id());

    if (type->IsCooperativeMatrix()) {
      return false;
    }

    if (HasFloatingPoint(type) && !inst->IsFloatingPointFoldingAllowed())
      return false;

    uint32_t width = ElementWidth(type);
    if (width != 32 && width != 64) return false;


    const analysis::Constant* const_input1 = ConstInput(constants);
    if (!const_input1) return false;
    Instruction* other_inst = NonConstInput(context, constants[0], inst);
    if (HasFloatingPoint(type) && !other_inst->IsFloatingPointFoldingAllowed())
      return false;

    if (other_inst->opcode() == inst->opcode()) {
      std::vector<const analysis::Constant*> other_constants =
          const_mgr->GetOperandConstants(other_inst);
      const analysis::Constant* const_input2 = ConstInput(other_constants);
      if (!const_input2) return false;

      bool other_first_is_variable = other_constants[0] == nullptr;
      uint32_t merged_id = PerformOperation(const_mgr, inst->opcode(),
                                            const_input1, const_input2);
      if (merged_id == 0) return false;

      uint32_t non_const_id = other_first_is_variable
                                  ? other_inst->GetSingleWordInOperand(0u)
                                  : other_inst->GetSingleWordInOperand(1u);
      inst->SetInOperands({{SPV_OPERAND_TYPE_ID, {non_const_id}},
                           {SPV_OPERAND_TYPE_ID, {merged_id}}});
      return true;
    }

    return false;
  };
}










FoldingRule MergeMulDivArithmetic() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(inst->opcode() == spv::Op::OpFMul);
    analysis::ConstantManager* const_mgr = context->get_constant_mgr();
    analysis::DefUseManager* def_use_mgr = context->get_def_use_mgr();

    const analysis::Type* type =
        context->get_type_mgr()->GetType(inst->type_id());

    if (type->IsCooperativeMatrix()) {
      return false;
    }

    if (!inst->IsFloatingPointFoldingAllowed()) return false;

    uint32_t width = ElementWidth(type);
    if (width != 32 && width != 64) return false;

    for (uint32_t i = 0; i < 2; i++) {
      uint32_t op_id = inst->GetSingleWordInOperand(i);
      Instruction* op_inst = def_use_mgr->GetDef(op_id);
      if (op_inst->opcode() == spv::Op::OpFDiv) {
        if (op_inst->GetSingleWordInOperand(1) ==
            inst->GetSingleWordInOperand(1 - i)) {
          inst->SetOpcode(spv::Op::OpCopyObject);
          inst->SetInOperands(
              {{SPV_OPERAND_TYPE_ID, {op_inst->GetSingleWordInOperand(0)}}});
          return true;
        }
      }
    }

    const analysis::Constant* const_input1 = ConstInput(constants);
    if (!const_input1) return false;
    Instruction* other_inst = NonConstInput(context, constants[0], inst);
    if (!other_inst->IsFloatingPointFoldingAllowed()) return false;

    if (other_inst->opcode() == spv::Op::OpFDiv) {
      std::vector<const analysis::Constant*> other_constants =
          const_mgr->GetOperandConstants(other_inst);
      const analysis::Constant* const_input2 = ConstInput(other_constants);
      if (!const_input2 || HasZero(const_input2)) return false;

      bool other_first_is_variable = other_constants[0] == nullptr;


      uint32_t merged_id = PerformOperation(
          const_mgr,
          other_first_is_variable ? other_inst->opcode() : inst->opcode(),
          const_input1, const_input2);
      if (merged_id == 0) return false;

      uint32_t non_const_id = other_first_is_variable
                                  ? other_inst->GetSingleWordInOperand(0u)
                                  : other_inst->GetSingleWordInOperand(1u);



      inst->SetOpcode(other_first_is_variable ? inst->opcode()
                                              : other_inst->opcode());
      if (other_first_is_variable) {
        inst->SetInOperands({{SPV_OPERAND_TYPE_ID, {non_const_id}},
                             {SPV_OPERAND_TYPE_ID, {merged_id}}});
      } else {
        inst->SetInOperands({{SPV_OPERAND_TYPE_ID, {merged_id}},
                             {SPV_OPERAND_TYPE_ID, {non_const_id}}});
      }
      return true;
    }

    return false;
  };
}





FoldingRule MergeMulNegateArithmetic() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(inst->opcode() == spv::Op::OpFMul ||
           inst->opcode() == spv::Op::OpIMul);
    analysis::ConstantManager* const_mgr = context->get_constant_mgr();
    const analysis::Type* type =
        context->get_type_mgr()->GetType(inst->type_id());

    if (type->IsCooperativeMatrix()) {
      return false;
    }

    bool uses_float = HasFloatingPoint(type);
    if (uses_float && !inst->IsFloatingPointFoldingAllowed()) return false;

    uint32_t width = ElementWidth(type);
    if (width != 32 && width != 64) return false;

    const analysis::Constant* const_input1 = ConstInput(constants);
    if (!const_input1) return false;
    Instruction* other_inst = NonConstInput(context, constants[0], inst);
    if (uses_float && !other_inst->IsFloatingPointFoldingAllowed())
      return false;

    if (other_inst->opcode() == spv::Op::OpFNegate ||
        other_inst->opcode() == spv::Op::OpSNegate) {
      uint32_t neg_id = NegateConstant(const_mgr, const_input1);

      inst->SetInOperands(
          {{SPV_OPERAND_TYPE_ID, {other_inst->GetSingleWordInOperand(0u)}},
           {SPV_OPERAND_TYPE_ID, {neg_id}}});
      return true;
    }

    return false;
  };
}


static bool IsFoldableNegation(const Instruction* inst) {
  return (inst->opcode() == spv::Op::OpSNegate ||
          (inst->opcode() == spv::Op::OpFNegate &&
           inst->IsFloatingPointFoldingAllowed()));
}





FoldingRule MergeDivMulDoubleNegative() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>&) {
    assert(inst->opcode() == spv::Op::OpFMul ||
           inst->opcode() == spv::Op::OpVectorTimesScalar ||
           inst->opcode() == spv::Op::OpFDiv ||
           inst->opcode() == spv::Op::OpIMul ||
           inst->opcode() == spv::Op::OpSDiv);

    const analysis::Type* type =
        context->get_type_mgr()->GetType(inst->type_id());

    bool uses_float = HasFloatingPoint(type);
    if (uses_float && !inst->IsFloatingPointFoldingAllowed()) return false;

    analysis::DefUseManager* def_use_mgr = context->get_def_use_mgr();
    Instruction* lhs = def_use_mgr->GetDef(inst->GetSingleWordInOperand(0));
    Instruction* rhs = def_use_mgr->GetDef(inst->GetSingleWordInOperand(1));

    if (IsFoldableNegation(lhs) && IsFoldableNegation(rhs)) {
      inst->SetInOperands(
          {{SPV_OPERAND_TYPE_ID, {lhs->GetSingleWordInOperand(0u)}},
           {SPV_OPERAND_TYPE_ID, {rhs->GetSingleWordInOperand(0u)}}});
      return true;
    }
    return false;
  };
}








FoldingRule MergeDivDivArithmetic() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(inst->opcode() == spv::Op::OpFDiv);
    analysis::ConstantManager* const_mgr = context->get_constant_mgr();
    const analysis::Type* type =
        context->get_type_mgr()->GetType(inst->type_id());

    if (type->IsCooperativeMatrix()) {
      return false;
    }

    if (!inst->IsFloatingPointFoldingAllowed()) return false;

    uint32_t width = ElementWidth(type);
    if (width != 32 && width != 64) return false;

    const analysis::Constant* const_input1 = ConstInput(constants);
    if (!const_input1 || HasZero(const_input1)) return false;
    Instruction* other_inst = NonConstInput(context, constants[0], inst);
    if (!other_inst->IsFloatingPointFoldingAllowed()) return false;

    bool first_is_variable = constants[0] == nullptr;
    if (other_inst->opcode() == inst->opcode()) {
      std::vector<const analysis::Constant*> other_constants =
          const_mgr->GetOperandConstants(other_inst);
      const analysis::Constant* const_input2 = ConstInput(other_constants);
      if (!const_input2 || HasZero(const_input2)) return false;

      bool other_first_is_variable = other_constants[0] == nullptr;

      spv::Op merge_op = inst->opcode();
      if (other_first_is_variable) {

        merge_op = spv::Op::OpFMul;
      }



      if (first_is_variable) std::swap(const_input1, const_input2);
      uint32_t merged_id =
          PerformOperation(const_mgr, merge_op, const_input1, const_input2);
      if (merged_id == 0) return false;

      uint32_t non_const_id = other_first_is_variable
                                  ? other_inst->GetSingleWordInOperand(0u)
                                  : other_inst->GetSingleWordInOperand(1u);

      spv::Op op = inst->opcode();
      if (!first_is_variable && !other_first_is_variable) {

        op = spv::Op::OpFMul;
      }

      uint32_t op1 = merged_id;
      uint32_t op2 = non_const_id;
      if (first_is_variable && other_first_is_variable) std::swap(op1, op2);
      inst->SetOpcode(op);
      inst->SetInOperands(
          {{SPV_OPERAND_TYPE_ID, {op1}}, {SPV_OPERAND_TYPE_ID, {op2}}});
      return true;
    }

    return false;
  };
}










FoldingRule MergeDivMulArithmetic() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(inst->opcode() == spv::Op::OpFDiv);
    analysis::DefUseManager* def_use_mgr = context->get_def_use_mgr();
    analysis::ConstantManager* const_mgr = context->get_constant_mgr();

    const analysis::Type* type =
        context->get_type_mgr()->GetType(inst->type_id());

    if (type->IsCooperativeMatrix()) {
      return false;
    }

    if (!inst->IsFloatingPointFoldingAllowed()) return false;

    uint32_t width = ElementWidth(type);
    if (width != 32 && width != 64) return false;

    uint32_t op_id = inst->GetSingleWordInOperand(0);
    Instruction* op_inst = def_use_mgr->GetDef(op_id);

    if (op_inst->opcode() == spv::Op::OpFMul) {
      for (uint32_t i = 0; i < 2; i++) {
        if (op_inst->GetSingleWordInOperand(i) ==
            inst->GetSingleWordInOperand(1)) {
          inst->SetOpcode(spv::Op::OpCopyObject);
          inst->SetInOperands({{SPV_OPERAND_TYPE_ID,
                                {op_inst->GetSingleWordInOperand(1 - i)}}});
          return true;
        }
      }
    }

    const analysis::Constant* const_input1 = ConstInput(constants);
    if (!const_input1 || HasZero(const_input1)) return false;
    Instruction* other_inst = NonConstInput(context, constants[0], inst);
    if (!other_inst->IsFloatingPointFoldingAllowed()) return false;

    bool first_is_variable = constants[0] == nullptr;
    if (other_inst->opcode() == spv::Op::OpFMul) {
      std::vector<const analysis::Constant*> other_constants =
          const_mgr->GetOperandConstants(other_inst);
      const analysis::Constant* const_input2 = ConstInput(other_constants);
      if (!const_input2) return false;

      bool other_first_is_variable = other_constants[0] == nullptr;


      if (first_is_variable) std::swap(const_input1, const_input2);
      uint32_t merged_id = PerformOperation(const_mgr, inst->opcode(),
                                            const_input1, const_input2);
      if (merged_id == 0) return false;

      uint32_t non_const_id = other_first_is_variable
                                  ? other_inst->GetSingleWordInOperand(0u)
                                  : other_inst->GetSingleWordInOperand(1u);

      uint32_t op1 = merged_id;
      uint32_t op2 = non_const_id;
      if (first_is_variable) std::swap(op1, op2);


      if (first_is_variable) inst->SetOpcode(other_inst->opcode());
      inst->SetInOperands(
          {{SPV_OPERAND_TYPE_ID, {op1}}, {SPV_OPERAND_TYPE_ID, {op2}}});
      return true;
    }

    return false;
  };
}





FoldingRule MergeDivNegateArithmetic() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(inst->opcode() == spv::Op::OpFDiv);
    analysis::ConstantManager* const_mgr = context->get_constant_mgr();
    if (!inst->IsFloatingPointFoldingAllowed()) return false;

    const analysis::Constant* const_input1 = ConstInput(constants);
    if (!const_input1) return false;
    Instruction* other_inst = NonConstInput(context, constants[0], inst);
    if (!other_inst->IsFloatingPointFoldingAllowed()) return false;

    bool first_is_variable = constants[0] == nullptr;
    if (other_inst->opcode() == spv::Op::OpFNegate) {
      uint32_t neg_id = NegateConstant(const_mgr, const_input1);

      if (first_is_variable) {
        inst->SetInOperands(
            {{SPV_OPERAND_TYPE_ID, {other_inst->GetSingleWordInOperand(0u)}},
             {SPV_OPERAND_TYPE_ID, {neg_id}}});
      } else {
        inst->SetInOperands(
            {{SPV_OPERAND_TYPE_ID, {neg_id}},
             {SPV_OPERAND_TYPE_ID, {other_inst->GetSingleWordInOperand(0u)}}});
      }
      return true;
    }

    return false;
  };
}




FoldingRule MergeAddNegateArithmetic() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>&) {
    assert(inst->opcode() == spv::Op::OpFAdd ||
           inst->opcode() == spv::Op::OpIAdd);
    const analysis::Type* type =
        context->get_type_mgr()->GetType(inst->type_id());
    bool uses_float = HasFloatingPoint(type);
    if (uses_float && !inst->IsFloatingPointFoldingAllowed()) return false;

    analysis::DefUseManager* def_use_mgr = context->get_def_use_mgr();
    Instruction* lhs = def_use_mgr->GetDef(inst->GetSingleWordInOperand(0));
    Instruction* rhs = def_use_mgr->GetDef(inst->GetSingleWordInOperand(1));

    auto TrySubstitute = [inst, uses_float](Instruction* first,
                                            Instruction* second) {
      if (IsFoldableNegation(first)) {
        inst->SetOpcode(uses_float ? spv::Op::OpFSub : spv::Op::OpISub);
        inst->SetInOperands(
            {{SPV_OPERAND_TYPE_ID, {second->result_id()}},
             {SPV_OPERAND_TYPE_ID, {first->GetSingleWordInOperand(0u)}}});
        return true;
      }
      return false;
    };

    return TrySubstitute(lhs, rhs) || TrySubstitute(rhs, lhs);
  };
}





FoldingRule MergeSubNegateArithmetic() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(inst->opcode() == spv::Op::OpFSub ||
           inst->opcode() == spv::Op::OpISub);
    const analysis::Type* type =
        context->get_type_mgr()->GetType(inst->type_id());

    bool uses_float = HasFloatingPoint(type);
    if (uses_float && !inst->IsFloatingPointFoldingAllowed()) return false;

    analysis::DefUseManager* def_use_mgr = context->get_def_use_mgr();
    Instruction* lhs = def_use_mgr->GetDef(inst->GetSingleWordInOperand(0));
    Instruction* rhs = def_use_mgr->GetDef(inst->GetSingleWordInOperand(1));

    if (IsFoldableNegation(rhs)) {
      inst->SetOpcode(uses_float ? spv::Op::OpFAdd : spv::Op::OpIAdd);
      inst->SetInOperands(
          {{SPV_OPERAND_TYPE_ID, {lhs->result_id()}},
           {SPV_OPERAND_TYPE_ID, {rhs->GetSingleWordInOperand(0)}}});
      return true;
    }

    if (type->IsCooperativeMatrix()) {
      return false;
    }

    uint32_t width = ElementWidth(type);
    if (width != 32 && width != 64) return false;

    if (constants[1] && IsFoldableNegation(lhs)) {
      inst->SetInOperands(
          {{SPV_OPERAND_TYPE_ID,
            {NegateConstant(context->get_constant_mgr(), constants[1])}},
           {SPV_OPERAND_TYPE_ID, {lhs->GetSingleWordInOperand(0)}}});
      return true;
    }
    return false;
  };
}







FoldingRule MergeAddAddArithmetic() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(inst->opcode() == spv::Op::OpFAdd ||
           inst->opcode() == spv::Op::OpIAdd);
    const analysis::Type* type =
        context->get_type_mgr()->GetType(inst->type_id());

    if (type->IsCooperativeMatrix()) {
      return false;
    }

    analysis::ConstantManager* const_mgr = context->get_constant_mgr();
    bool uses_float = HasFloatingPoint(type);
    if (uses_float && !inst->IsFloatingPointFoldingAllowed()) return false;

    uint32_t width = ElementWidth(type);
    if (width != 32 && width != 64) return false;

    const analysis::Constant* const_input1 = ConstInput(constants);
    if (!const_input1) return false;
    Instruction* other_inst = NonConstInput(context, constants[0], inst);
    if (uses_float && !other_inst->IsFloatingPointFoldingAllowed())
      return false;

    if (other_inst->opcode() == spv::Op::OpFAdd ||
        other_inst->opcode() == spv::Op::OpIAdd) {
      std::vector<const analysis::Constant*> other_constants =
          const_mgr->GetOperandConstants(other_inst);
      const analysis::Constant* const_input2 = ConstInput(other_constants);
      if (!const_input2) return false;

      Instruction* non_const_input =
          NonConstInput(context, other_constants[0], other_inst);
      uint32_t merged_id = PerformOperation(const_mgr, inst->opcode(),
                                            const_input1, const_input2);
      if (merged_id == 0) return false;

      inst->SetInOperands(
          {{SPV_OPERAND_TYPE_ID, {non_const_input->result_id()}},
           {SPV_OPERAND_TYPE_ID, {merged_id}}});
      return true;
    }
    return false;
  };
}







FoldingRule MergeAddSubArithmetic() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(inst->opcode() == spv::Op::OpFAdd ||
           inst->opcode() == spv::Op::OpIAdd);
    const analysis::Type* type =
        context->get_type_mgr()->GetType(inst->type_id());

    if (type->IsCooperativeMatrix()) {
      return false;
    }

    analysis::ConstantManager* const_mgr = context->get_constant_mgr();
    bool uses_float = HasFloatingPoint(type);
    if (uses_float && !inst->IsFloatingPointFoldingAllowed()) return false;

    uint32_t width = ElementWidth(type);
    if (width != 32 && width != 64) return false;

    const analysis::Constant* const_input1 = ConstInput(constants);
    if (!const_input1) return false;
    Instruction* other_inst = NonConstInput(context, constants[0], inst);
    if (uses_float && !other_inst->IsFloatingPointFoldingAllowed())
      return false;

    if (other_inst->opcode() == spv::Op::OpFSub ||
        other_inst->opcode() == spv::Op::OpISub) {
      std::vector<const analysis::Constant*> other_constants =
          const_mgr->GetOperandConstants(other_inst);
      const analysis::Constant* const_input2 = ConstInput(other_constants);
      if (!const_input2) return false;

      bool first_is_variable = other_constants[0] == nullptr;
      spv::Op op = inst->opcode();
      uint32_t op1 = 0;
      uint32_t op2 = 0;
      if (first_is_variable) {

        op1 = other_inst->GetSingleWordInOperand(0u);
        op2 = PerformOperation(const_mgr, other_inst->opcode(), const_input1,
                               const_input2);
      } else {

        op1 = PerformOperation(const_mgr, inst->opcode(), const_input1,
                               const_input2);
        op2 = other_inst->GetSingleWordInOperand(1u);
        op = other_inst->opcode();
      }
      if (op1 == 0 || op2 == 0) return false;

      inst->SetOpcode(op);
      inst->SetInOperands(
          {{SPV_OPERAND_TYPE_ID, {op1}}, {SPV_OPERAND_TYPE_ID, {op2}}});
      return true;
    }
    return false;
  };
}







FoldingRule MergeSubAddArithmetic() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(inst->opcode() == spv::Op::OpFSub ||
           inst->opcode() == spv::Op::OpISub);
    const analysis::Type* type =
        context->get_type_mgr()->GetType(inst->type_id());

    if (type->IsCooperativeMatrix()) {
      return false;
    }

    analysis::ConstantManager* const_mgr = context->get_constant_mgr();
    bool uses_float = HasFloatingPoint(type);
    if (uses_float && !inst->IsFloatingPointFoldingAllowed()) return false;

    uint32_t width = ElementWidth(type);
    if (width != 32 && width != 64) return false;

    const analysis::Constant* const_input1 = ConstInput(constants);
    if (!const_input1) return false;
    Instruction* other_inst = NonConstInput(context, constants[0], inst);
    if (uses_float && !other_inst->IsFloatingPointFoldingAllowed())
      return false;

    if (other_inst->opcode() == spv::Op::OpFAdd ||
        other_inst->opcode() == spv::Op::OpIAdd) {
      std::vector<const analysis::Constant*> other_constants =
          const_mgr->GetOperandConstants(other_inst);
      const analysis::Constant* const_input2 = ConstInput(other_constants);
      if (!const_input2) return false;

      Instruction* non_const_input =
          NonConstInput(context, other_constants[0], other_inst);



      if (constants[0] == nullptr) std::swap(const_input1, const_input2);

      uint32_t merged_id = PerformOperation(const_mgr, inst->opcode(),
                                            const_input1, const_input2);
      spv::Op op = inst->opcode();
      uint32_t op1 = 0;
      uint32_t op2 = 0;
      if (constants[0] == nullptr) {

        op1 = non_const_input->result_id();
        op2 = merged_id;
        op = other_inst->opcode();
      } else {

        op1 = merged_id;
        op2 = non_const_input->result_id();
      }
      if (op1 == 0 || op2 == 0) return false;

      inst->SetOpcode(op);
      inst->SetInOperands(
          {{SPV_OPERAND_TYPE_ID, {op1}}, {SPV_OPERAND_TYPE_ID, {op2}}});
      return true;
    }
    return false;
  };
}







FoldingRule MergeSubSubArithmetic() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(inst->opcode() == spv::Op::OpFSub ||
           inst->opcode() == spv::Op::OpISub);
    const analysis::Type* type =
        context->get_type_mgr()->GetType(inst->type_id());

    if (type->IsCooperativeMatrix()) {
      return false;
    }

    analysis::ConstantManager* const_mgr = context->get_constant_mgr();
    bool uses_float = HasFloatingPoint(type);
    if (uses_float && !inst->IsFloatingPointFoldingAllowed()) return false;

    uint32_t width = ElementWidth(type);
    if (width != 32 && width != 64) return false;

    const analysis::Constant* const_input1 = ConstInput(constants);
    if (!const_input1) return false;
    Instruction* other_inst = NonConstInput(context, constants[0], inst);
    if (uses_float && !other_inst->IsFloatingPointFoldingAllowed())
      return false;

    if (other_inst->opcode() == spv::Op::OpFSub ||
        other_inst->opcode() == spv::Op::OpISub) {
      std::vector<const analysis::Constant*> other_constants =
          const_mgr->GetOperandConstants(other_inst);
      const analysis::Constant* const_input2 = ConstInput(other_constants);
      if (!const_input2) return false;

      Instruction* non_const_input =
          NonConstInput(context, other_constants[0], other_inst);


      uint32_t merged_id = 0;
      spv::Op merge_op = inst->opcode();
      if (other_constants[0] == nullptr) {
        merge_op = uses_float ? spv::Op::OpFAdd : spv::Op::OpIAdd;
      } else if (constants[0] == nullptr) {
        std::swap(const_input1, const_input2);
      }
      merged_id =
          PerformOperation(const_mgr, merge_op, const_input1, const_input2);
      if (merged_id == 0) return false;

      spv::Op op = inst->opcode();
      if (constants[0] != nullptr && other_constants[0] != nullptr) {

        op = uses_float ? spv::Op::OpFAdd : spv::Op::OpIAdd;
      }

      uint32_t op1 = 0;
      uint32_t op2 = 0;
      if ((constants[0] == nullptr) ^ (other_constants[0] == nullptr)) {
        op1 = merged_id;
        op2 = non_const_input->result_id();
      } else {
        op1 = non_const_input->result_id();
        op2 = merged_id;
      }

      inst->SetOpcode(op);
      inst->SetInOperands(
          {{SPV_OPERAND_TYPE_ID, {op1}}, {SPV_OPERAND_TYPE_ID, {op2}}});
      return true;
    }
    return false;
  };
}



bool MergeGenericAddendSub(uint32_t addend, uint32_t sub, Instruction* inst) {
  IRContext* context = inst->context();
  analysis::DefUseManager* def_use_mgr = context->get_def_use_mgr();
  Instruction* sub_inst = def_use_mgr->GetDef(sub);
  if (sub_inst->opcode() != spv::Op::OpFSub &&
      sub_inst->opcode() != spv::Op::OpISub)
    return false;
  if (sub_inst->opcode() == spv::Op::OpFSub &&
      !sub_inst->IsFloatingPointFoldingAllowed())
    return false;
  if (addend != sub_inst->GetSingleWordInOperand(1)) return false;
  inst->SetOpcode(spv::Op::OpCopyObject);
  inst->SetInOperands(
      {{SPV_OPERAND_TYPE_ID, {sub_inst->GetSingleWordInOperand(0)}}});
  context->UpdateDefUse(inst);
  return true;
}







FoldingRule MergeGenericAddSubArithmetic() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>&) {
    assert(inst->opcode() == spv::Op::OpFAdd ||
           inst->opcode() == spv::Op::OpIAdd);
    const analysis::Type* type =
        context->get_type_mgr()->GetType(inst->type_id());

    if (type->IsCooperativeMatrix()) {
      return false;
    }

    bool uses_float = HasFloatingPoint(type);
    if (uses_float && !inst->IsFloatingPointFoldingAllowed()) return false;

    uint32_t width = ElementWidth(type);
    if (width != 32 && width != 64) return false;

    uint32_t add_op0 = inst->GetSingleWordInOperand(0);
    uint32_t add_op1 = inst->GetSingleWordInOperand(1);
    if (MergeGenericAddendSub(add_op0, add_op1, inst)) return true;
    return MergeGenericAddendSub(add_op1, add_op0, inst);
  };
}





bool FactorAddSubMulsOpnds(uint32_t factor0_0, uint32_t factor0_1,
                           uint32_t factor1_0, uint32_t factor1_1,
                           Instruction* inst) {
  IRContext* context = inst->context();
  if (factor0_0 != factor1_0) return false;
  InstructionBuilder ir_builder(
      context, inst,
      IRContext::kAnalysisDefUse | IRContext::kAnalysisInstrToBlockMapping);
  Instruction* new_add_inst = ir_builder.AddBinaryOp(
      inst->type_id(), inst->opcode(), factor0_1, factor1_1);
  if (!new_add_inst) {
    return false;
  }

  bool is_float =
      inst->opcode() == spv::Op::OpFAdd || inst->opcode() == spv::Op::OpFSub;
  inst->SetOpcode(is_float ? spv::Op::OpFMul : spv::Op::OpIMul);
  inst->SetInOperands({{SPV_OPERAND_TYPE_ID, {factor0_0}},
                       {SPV_OPERAND_TYPE_ID, {new_add_inst->result_id()}}});
  context->UpdateDefUse(inst);
  return true;
}





FoldingRule FactorAddSubMuls() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>&) {
    assert(inst->opcode() == spv::Op::OpFAdd ||
           inst->opcode() == spv::Op::OpFSub ||
           inst->opcode() == spv::Op::OpIAdd ||
           inst->opcode() == spv::Op::OpISub);
    const analysis::Type* type =
        context->get_type_mgr()->GetType(inst->type_id());
    bool uses_float = HasFloatingPoint(type);
    if (uses_float && !inst->IsFloatingPointFoldingAllowed()) return false;

    analysis::DefUseManager* def_use_mgr = context->get_def_use_mgr();
    uint32_t add_op0 = inst->GetSingleWordInOperand(0);
    Instruction* add_op0_inst = def_use_mgr->GetDef(add_op0);
    if (add_op0_inst->opcode() != spv::Op::OpFMul &&
        add_op0_inst->opcode() != spv::Op::OpIMul)
      return false;
    uint32_t add_op1 = inst->GetSingleWordInOperand(1);
    Instruction* add_op1_inst = def_use_mgr->GetDef(add_op1);
    if (add_op1_inst->opcode() != spv::Op::OpFMul &&
        add_op1_inst->opcode() != spv::Op::OpIMul)
      return false;



    if (def_use_mgr->NumUses(add_op0_inst) > 1) return false;
    if (def_use_mgr->NumUses(add_op1_inst) > 1) return false;

    if (add_op0_inst->opcode() == spv::Op::OpFMul &&
        (!add_op0_inst->IsFloatingPointFoldingAllowed() ||
         !add_op1_inst->IsFloatingPointFoldingAllowed()))
      return false;

    for (int i = 0; i < 2; i++) {
      for (int j = 0; j < 2; j++) {

        if (FactorAddSubMulsOpnds(add_op0_inst->GetSingleWordInOperand(i),
                                  add_op0_inst->GetSingleWordInOperand(1 - i),
                                  add_op1_inst->GetSingleWordInOperand(j),
                                  add_op1_inst->GetSingleWordInOperand(1 - j),
                                  inst))
          return true;
      }
    }
    return false;
  };
}








static const constexpr spv::Op ReassociateNestedGenericIntOps[] = {
    spv::Op::OpIMul, spv::Op::OpBitwiseOr, spv::Op::OpBitwiseXor,
    spv::Op::OpBitwiseAnd};

FoldingRule ReassociateNestedGenericInt(spv::Op opcode) {
  assert(std::find(std::begin(ReassociateNestedGenericIntOps),
                   std::end(ReassociateNestedGenericIntOps),
                   opcode) != std::end(ReassociateNestedGenericIntOps) &&
         "Wrong opcode.");

  return [opcode](IRContext* context, Instruction* inst,
                  const std::vector<const analysis::Constant*>& constants) {

    if (constants[0] || constants[1]) {
      return false;
    }

    if (inst->opcode() != opcode) {
      return false;
    }

    const analysis::Type* type =
        context->get_type_mgr()->GetType(inst->type_id());

    if (type->IsCooperativeMatrix()) {
      return false;
    }

    uint32_t width = ElementWidth(type);
    if (width != 32 && width != 64) return false;

    analysis::DefUseManager* def_use_mgr = context->get_def_use_mgr();
    Instruction* lhs = def_use_mgr->GetDef(inst->GetSingleWordInOperand(0));
    Instruction* rhs = def_use_mgr->GetDef(inst->GetSingleWordInOperand(1));

    if (lhs->opcode() != opcode || rhs->opcode() != opcode) {
      return false;
    }

    analysis::ConstantManager* const_mgr = context->get_constant_mgr();
    std::vector<const analysis::Constant*> lhs_constants =
        const_mgr->GetOperandConstants(lhs);
    const analysis::Constant* lhs_const = ConstInput(lhs_constants);
    if (!lhs_const) {
      return false;
    }

    std::vector<const analysis::Constant*> rhs_constants =
        const_mgr->GetOperandConstants(rhs);
    const analysis::Constant* rhs_const = ConstInput(rhs_constants);
    if (!rhs_const) {
      return false;
    }

    uint32_t merged_constant =
        PerformOperation(const_mgr, opcode, lhs_const, rhs_const);
    if (!merged_constant) {
      return false;
    }

    InstructionBuilder ir_builder(
        context, inst,
        IRContext::kAnalysisDefUse | IRContext::kAnalysisInstrToBlockMapping);

    Instruction* new_rhs = ir_builder.AddBinaryOp(
        inst->type_id(), opcode,
        NonConstInput(context, lhs_constants[0], lhs)->result_id(),
        NonConstInput(context, rhs_constants[0], rhs)->result_id());

    if (!new_rhs) {
      return false;
    }

    inst->SetInOperands({{SPV_OPERAND_TYPE_ID, {merged_constant}},
                         {SPV_OPERAND_TYPE_ID, {new_rhs->result_id()}}});
    return true;
  };
}







FoldingRule ReassociateNestedMulDivFloat() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(inst->opcode() == spv::Op::OpFMul ||
           inst->opcode() == spv::Op::OpFDiv);


    if (constants[0] || constants[1]) {
      return false;
    }

    const analysis::Type* type =
        context->get_type_mgr()->GetType(inst->type_id());

    if (type->IsCooperativeMatrix()) {
      return false;
    }

    uint32_t width = ElementWidth(type);
    if (width != 32 && width != 64) return false;

    if (!inst->IsFloatingPointFoldingAllowed()) return false;

    analysis::DefUseManager* def_use_mgr = context->get_def_use_mgr();
    Instruction* lhs = def_use_mgr->GetDef(inst->GetSingleWordInOperand(0));
    Instruction* rhs = def_use_mgr->GetDef(inst->GetSingleWordInOperand(1));

    bool lhs_is_mul = lhs->opcode() == spv::Op::OpFMul;
    bool lhs_is_div = lhs->opcode() == spv::Op::OpFDiv;
    bool rhs_is_mul = rhs->opcode() == spv::Op::OpFMul;
    bool rhs_is_div = rhs->opcode() == spv::Op::OpFDiv;
    if (!(lhs_is_mul || lhs_is_div) || !(rhs_is_mul || rhs_is_div)) {
      return false;
    }

    if (!lhs->IsFloatingPointFoldingAllowed() ||
        !rhs->IsFloatingPointFoldingAllowed()) {
      return false;
    }

    analysis::ConstantManager* const_mgr = context->get_constant_mgr();
    std::vector<const analysis::Constant*> lhs_constants =
        const_mgr->GetOperandConstants(lhs);
    if (!lhs_constants[0] && !lhs_constants[1]) {
      return false;
    }

    std::vector<const analysis::Constant*> rhs_constants =
        const_mgr->GetOperandConstants(rhs);
    if (!rhs_constants[0] && !rhs_constants[1]) {
      return false;
    }

    const analysis::Constant* lhs_const =
        lhs_constants[0] ? lhs_constants[0] : lhs_constants[1];
    const analysis::Constant* rhs_const =
        rhs_constants[0] ? rhs_constants[0] : rhs_constants[1];
    if (!lhs_const || !rhs_const) return false;

    bool const_lhs_rcp = lhs_constants[0] ? false : lhs_is_div;
    bool const_rhs_rcp = rhs_constants[0] ? false : rhs_is_div;

    uint32_t non_const_lhs = lhs_constants[0] ? lhs->GetSingleWordInOperand(1)
                                              : lhs->GetSingleWordInOperand(0);
    bool non_const_lhs_rcp = lhs_constants[0] ? lhs_is_div : false;

    uint32_t non_const_rhs = rhs_constants[0] ? rhs->GetSingleWordInOperand(1)
                                              : rhs->GetSingleWordInOperand(0);
    bool non_const_rhs_rcp = rhs_constants[0] ? rhs_is_div : false;


    if (inst->opcode() == spv::Op::OpFDiv) {
      const_rhs_rcp = !const_rhs_rcp;
      non_const_rhs_rcp = !non_const_rhs_rcp;
    }

    if (const_lhs_rcp) {
      lhs_const =
          const_mgr->FindDeclaredConstant(Reciprocal(const_mgr, lhs_const));
      if (!lhs_const) {
        return false;
      }
    }
    if (const_rhs_rcp) {
      rhs_const =
          const_mgr->FindDeclaredConstant(Reciprocal(const_mgr, rhs_const));
      if (!rhs_const) {
        return false;
      }
    }

    uint32_t merged_constant =
        PerformOperation(const_mgr, spv::Op::OpFMul, lhs_const, rhs_const);

    if (!merged_constant) {
      return false;
    }

    spv::Op op = spv::Op::OpNop;
    Instruction* new_rhs = nullptr;

    InstructionBuilder ir_builder(
        context, inst,
        IRContext::kAnalysisDefUse | IRContext::kAnalysisInstrToBlockMapping);


    if (!non_const_lhs_rcp && !non_const_rhs_rcp) {
      new_rhs = ir_builder.AddBinaryOp(inst->type_id(), spv::Op::OpFMul,
                                       non_const_lhs, non_const_rhs);
      op = spv::Op::OpFMul;
    }

    else if (non_const_lhs_rcp && !non_const_rhs_rcp) {
      new_rhs = ir_builder.AddBinaryOp(inst->type_id(), spv::Op::OpFDiv,
                                       non_const_rhs, non_const_lhs);
      op = spv::Op::OpFMul;
    }

    else if (!non_const_lhs_rcp && non_const_rhs_rcp) {
      new_rhs = ir_builder.AddBinaryOp(inst->type_id(), spv::Op::OpFDiv,
                                       non_const_lhs, non_const_rhs);
      op = spv::Op::OpFMul;
    }

    else {
      new_rhs = ir_builder.AddBinaryOp(inst->type_id(), spv::Op::OpFMul,
                                       non_const_lhs, non_const_rhs);
      op = spv::Op::OpFDiv;
    }

    if (!new_rhs) {
      return false;
    }

    inst->SetOpcode(op);
    inst->SetInOperands({{SPV_OPERAND_TYPE_ID, {merged_constant}},
                         {SPV_OPERAND_TYPE_ID, {new_rhs->result_id()}}});
    return true;
  };
}







FoldingRule ReassociateNestedAddSub() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(inst->opcode() == spv::Op::OpFAdd ||
           inst->opcode() == spv::Op::OpIAdd ||
           inst->opcode() == spv::Op::OpFSub ||
           inst->opcode() == spv::Op::OpISub);


    if (constants[0] || constants[1]) {
      return false;
    }

    const analysis::Type* type =
        context->get_type_mgr()->GetType(inst->type_id());

    if (type->IsCooperativeMatrix()) {
      return false;
    }

    uint32_t width = ElementWidth(type);
    if (width != 32 && width != 64) return false;

    bool uses_float = HasFloatingPoint(type);
    if (uses_float && !inst->IsFloatingPointFoldingAllowed()) return false;

    analysis::DefUseManager* def_use_mgr = context->get_def_use_mgr();
    Instruction* lhs = def_use_mgr->GetDef(inst->GetSingleWordInOperand(0));
    Instruction* rhs = def_use_mgr->GetDef(inst->GetSingleWordInOperand(1));

    spv::Op add_op = uses_float ? spv::Op::OpFAdd : spv::Op::OpIAdd;
    spv::Op sub_op = uses_float ? spv::Op::OpFSub : spv::Op::OpISub;

    bool lhs_is_add = lhs->opcode() == add_op;
    bool lhs_is_sub = lhs->opcode() == sub_op;
    bool rhs_is_add = rhs->opcode() == add_op;
    bool rhs_is_sub = rhs->opcode() == sub_op;
    if (!(lhs_is_add || lhs_is_sub) || !(rhs_is_add || rhs_is_sub)) {
      return false;
    }

    if (uses_float && (!lhs->IsFloatingPointFoldingAllowed() ||
                       !rhs->IsFloatingPointFoldingAllowed())) {
      return false;
    }

    analysis::ConstantManager* const_mgr = context->get_constant_mgr();
    std::vector<const analysis::Constant*> lhs_constants =
        const_mgr->GetOperandConstants(lhs);
    if (!lhs_constants[0] && !lhs_constants[1]) {
      return false;
    }

    std::vector<const analysis::Constant*> rhs_constants =
        const_mgr->GetOperandConstants(rhs);
    if (!rhs_constants[0] && !rhs_constants[1]) {
      return false;
    }

    const analysis::Constant* lhs_const =
        lhs_constants[0] ? lhs_constants[0] : lhs_constants[1];
    const analysis::Constant* rhs_const =
        rhs_constants[0] ? rhs_constants[0] : rhs_constants[1];
    if (!lhs_const || !rhs_const) return false;

    bool const_lhs_neg = lhs_constants[0] ? false : lhs_is_sub;
    bool const_rhs_neg = rhs_constants[0] ? false : rhs_is_sub;

    uint32_t non_const_lhs = lhs_constants[0] ? lhs->GetSingleWordInOperand(1)
                                              : lhs->GetSingleWordInOperand(0);
    bool non_const_lhs_neg = lhs_constants[0] ? lhs_is_sub : false;

    uint32_t non_const_rhs = rhs_constants[0] ? rhs->GetSingleWordInOperand(1)
                                              : rhs->GetSingleWordInOperand(0);
    bool non_const_rhs_neg = rhs_constants[0] ? rhs_is_sub : false;


    if (inst->opcode() == spv::Op::OpFSub ||
        inst->opcode() == spv::Op::OpISub) {
      const_rhs_neg = !const_rhs_neg;
      non_const_rhs_neg = !non_const_rhs_neg;
    }

    if (const_lhs_neg) {
      lhs_const =
          const_mgr->FindDeclaredConstant(NegateConstant(const_mgr, lhs_const));
      if (!lhs_const) {
        return false;
      }
    }
    if (const_rhs_neg) {
      rhs_const =
          const_mgr->FindDeclaredConstant(NegateConstant(const_mgr, rhs_const));
      if (!rhs_const) {
        return false;
      }
    }

    uint32_t merged_constant =
        PerformOperation(const_mgr, add_op, lhs_const, rhs_const);

    if (!merged_constant) {
      return false;
    }

    spv::Op op = spv::Op::OpNop;
    Instruction* new_rhs = nullptr;

    InstructionBuilder ir_builder(
        context, inst,
        IRContext::kAnalysisDefUse | IRContext::kAnalysisInstrToBlockMapping);


    if (!non_const_lhs_neg && !non_const_rhs_neg) {
      new_rhs = ir_builder.AddBinaryOp(inst->type_id(), add_op, non_const_lhs,
                                       non_const_rhs);
      op = add_op;
    }

    else if (non_const_lhs_neg && !non_const_rhs_neg) {
      new_rhs = ir_builder.AddBinaryOp(inst->type_id(), sub_op, non_const_rhs,
                                       non_const_lhs);
      op = add_op;
    }

    else if (!non_const_lhs_neg && non_const_rhs_neg) {
      new_rhs = ir_builder.AddBinaryOp(inst->type_id(), sub_op, non_const_lhs,
                                       non_const_rhs);
      op = add_op;
    }

    else {
      new_rhs = ir_builder.AddBinaryOp(inst->type_id(), add_op, non_const_lhs,
                                       non_const_rhs);
      op = sub_op;
    }

    if (!new_rhs) {
      return false;
    }

    inst->SetOpcode(op);
    inst->SetInOperands({{SPV_OPERAND_TYPE_ID, {merged_constant}},
                         {SPV_OPERAND_TYPE_ID, {new_rhs->result_id()}}});
    return true;
  };
}

FoldingRule IntMultipleBy1() {
  return [](IRContext*, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(inst->opcode() == spv::Op::OpIMul &&
           "Wrong opcode.  Should be OpIMul.");
    for (uint32_t i = 0; i < 2; i++) {
      if (constants[i] == nullptr) {
        continue;
      }
      const analysis::IntConstant* int_constant = constants[i]->AsIntConstant();
      if (int_constant) {
        uint32_t width = ElementWidth(int_constant->type());
        if (width != 32 && width != 64) return false;
        bool is_one = (width == 32) ? int_constant->GetU32BitValue() == 1u
                                    : int_constant->GetU64BitValue() == 1ull;
        if (is_one) {
          inst->SetOpcode(spv::Op::OpCopyObject);
          inst->SetInOperands(
              {{SPV_OPERAND_TYPE_ID, {inst->GetSingleWordInOperand(1 - i)}}});
          return true;
        }
      }
    }
    return false;
  };
}




uint32_t GetNumOfElementsContributedByOperand(IRContext* context,
                                              const Instruction* inst,
                                              uint32_t index) {
  assert(inst->opcode() == spv::Op::OpCompositeConstruct);
  analysis::DefUseManager* def_use_mgr = context->get_def_use_mgr();
  analysis::TypeManager* type_mgr = context->get_type_mgr();

  analysis::Vector* result_type =
      type_mgr->GetType(inst->type_id())->AsVector();
  if (result_type == nullptr) {


    return 1;
  }





  uint32_t id = inst->GetSingleWordInOperand(index);
  Instruction* def = def_use_mgr->GetDef(id);
  analysis::Vector* type = type_mgr->GetType(def->type_id())->AsVector();
  if (type == nullptr) {
    return 1;
  }
  return type->element_count();
}





std::vector<Operand> GetExtractOperandsForElementOfCompositeConstruct(
    IRContext* context, const Instruction* inst, uint32_t result_index) {
  assert(inst->opcode() == spv::Op::OpCompositeConstruct);
  analysis::DefUseManager* def_use_mgr = context->get_def_use_mgr();
  analysis::TypeManager* type_mgr = context->get_type_mgr();

  analysis::Type* result_type = type_mgr->GetType(inst->type_id());
  if (result_type->AsVector() == nullptr) {
    if (result_index < inst->NumInOperands()) {
      uint32_t id = inst->GetSingleWordInOperand(result_index);
      return {Operand(SPV_OPERAND_TYPE_ID, {id})};
    }
    return {};
  }


  uint32_t total_element_count = 0;
  for (uint32_t idx = 0; idx < inst->NumInOperands(); ++idx) {
    uint32_t element_count =
        GetNumOfElementsContributedByOperand(context, inst, idx);
    total_element_count += element_count;
    if (result_index < total_element_count) {
      std::vector<Operand> operands;
      uint32_t id = inst->GetSingleWordInOperand(idx);
      Instruction* operand_def = def_use_mgr->GetDef(id);
      analysis::Type* operand_type = type_mgr->GetType(operand_def->type_id());

      operands.push_back({SPV_OPERAND_TYPE_ID, {id}});
      if (operand_type->AsVector()) {
        uint32_t start_index_of_id = total_element_count - element_count;
        uint32_t index_into_id = result_index - start_index_of_id;
        operands.push_back({SPV_OPERAND_TYPE_LITERAL_INTEGER, {index_into_id}});
      }
      return operands;
    }
  }
  return {};
}

bool CompositeConstructFeedingExtract(
    IRContext* context, Instruction* inst,
    const std::vector<const analysis::Constant*>&) {


  assert(inst->opcode() == spv::Op::OpCompositeExtract &&
         "Wrong opcode.  Should be OpCompositeExtract.");
  analysis::DefUseManager* def_use_mgr = context->get_def_use_mgr();


  if (inst->NumInOperands() <= 1) {
    return false;
  }

  uint32_t cid = inst->GetSingleWordInOperand(kExtractCompositeIdInIdx);
  Instruction* cinst = def_use_mgr->GetDef(cid);

  if (cinst->opcode() != spv::Op::OpCompositeConstruct) {
    return false;
  }

  uint32_t index_into_result = inst->GetSingleWordInOperand(1);
  std::vector<Operand> operands =
      GetExtractOperandsForElementOfCompositeConstruct(context, cinst,
                                                       index_into_result);

  if (operands.empty()) {
    return false;
  }


  for (uint32_t i = 2; i < inst->NumInOperands(); ++i) {
    operands.push_back(
        {SPV_OPERAND_TYPE_LITERAL_INTEGER, {inst->GetSingleWordInOperand(i)}});
  }

  if (operands.size() == 1) {


    inst->SetOpcode(spv::Op::OpCopyObject);
  }

  inst->SetInOperands(std::move(operands));
  return true;
}




uint32_t GetElementType(uint32_t type_id, Instruction::iterator start,
                        Instruction::iterator end,
                        const analysis::DefUseManager* def_use_manager) {
  for (auto index : make_range(std::move(start), std::move(end))) {
    const Instruction* type_inst = def_use_manager->GetDef(type_id);
    assert(index.type == SPV_OPERAND_TYPE_LITERAL_INTEGER &&
           index.words.size() == 1);
    switch (type_inst->opcode()) {
      case spv::Op::OpTypeArray:
      case spv::Op::OpTypeMatrix:
      case spv::Op::OpTypeVector:
      case spv::Op::OpTypeVectorIdEXT:
        type_id = type_inst->GetSingleWordInOperand(0);
        break;
      case spv::Op::OpTypeStruct:
        type_id = type_inst->GetSingleWordInOperand(index.words[0]);
        break;
      default:
        return 0;
    }
  }
  return type_id;
}



bool CopyLogicalFeedingExtract(IRContext* context, Instruction* inst,
                               const std::vector<const analysis::Constant*>&) {
  assert(inst->opcode() == spv::Op::OpCompositeExtract &&
         "Wrong opcode.  Should be OpCompositeExtract.");

  analysis::DefUseManager* def_use_mgr = context->get_def_use_mgr();
  uint32_t cid = inst->GetSingleWordInOperand(kExtractCompositeIdInIdx);
  Instruction* cinst = def_use_mgr->GetDef(cid);

  if (cinst->opcode() != spv::Op::OpCopyLogical) {
    return false;
  }

  uint32_t original_composite_id = cinst->GetSingleWordInOperand(0);
  Instruction* original_composite_inst =
      def_use_mgr->GetDef(original_composite_id);

  std::vector<uint32_t> indices;
  for (uint32_t i = 1; i < inst->NumInOperands(); ++i) {
    indices.push_back(inst->GetSingleWordInOperand(i));
  }

  uint32_t original_element_type_id =
      GetElementType(original_composite_inst->type_id(), inst->begin() + 3,
                     inst->end(), def_use_mgr);
  assert(original_element_type_id != 0 &&
         "Could not find the element type.  Invalid SPIR-V.");

  InstructionBuilder ir_builder(
      context, inst,
      IRContext::kAnalysisDefUse | IRContext::kAnalysisInstrToBlockMapping);

  Instruction* new_extract = ir_builder.AddCompositeExtract(
      original_element_type_id, original_composite_id, indices);

  if (original_element_type_id == inst->type_id())
    inst->SetOpcode(spv::Op::OpCopyObject);
  else
    inst->SetOpcode(spv::Op::OpCopyLogical);
  inst->SetInOperands({{SPV_OPERAND_TYPE_ID, {new_extract->result_id()}}});
  return true;
}



bool LoadFeedingExtract(IRContext* context, Instruction* inst,
                        const std::vector<const analysis::Constant*>&) {
  assert(inst->opcode() == spv::Op::OpCompositeExtract &&
         "Wrong opcode.  Should be OpCompositeExtract.");

  analysis::DefUseManager* def_use_mgr = context->get_def_use_mgr();
  uint32_t cid = inst->GetSingleWordInOperand(kExtractCompositeIdInIdx);
  Instruction* cinst = def_use_mgr->GetDef(cid);

  if (cinst->opcode() != spv::Op::OpLoad) {
    return false;
  }

  Instruction* composite_type_inst = def_use_mgr->GetDef(cinst->type_id());
  if (composite_type_inst->opcode() != spv::Op::OpTypeStruct &&
      composite_type_inst->opcode() != spv::Op::OpTypeArray) {
    return false;
  }


  if (cinst->NumInOperands() > 1) {
    uint32_t memory_access_mask = cinst->GetSingleWordInOperand(1);
    if (memory_access_mask & uint32_t(spv::MemoryAccessMask::Volatile)) {
      return false;
    }
  }

  uint32_t ptr_id = cinst->GetSingleWordInOperand(0);
  Instruction* ptr_inst = def_use_mgr->GetDef(ptr_id);
  Instruction* ptr_type_inst = def_use_mgr->GetDef(ptr_inst->type_id());
  assert(ptr_type_inst->opcode() == spv::Op::OpTypePointer);
  spv::StorageClass storage_class =
      static_cast<spv::StorageClass>(ptr_type_inst->GetSingleWordInOperand(0));




  if (storage_class == spv::StorageClass::Function ||
      storage_class == spv::StorageClass::Private) {
    return false;
  }

  analysis::ConstantManager* const_mgr = context->get_constant_mgr();
  analysis::TypeManager* type_mgr = context->get_type_mgr();
  std::vector<uint32_t> index_ids;
  for (uint32_t i = 1; i < inst->NumInOperands(); ++i) {
    uint32_t index = inst->GetSingleWordInOperand(i);
    const analysis::Constant* index_const =
        const_mgr->GetConstant(type_mgr->GetUIntType(), {index});
    index_ids.push_back(
        const_mgr->GetDefiningInstruction(index_const)->result_id());
  }

  InstructionBuilder ir_builder(
      context, cinst,
      IRContext::kAnalysisDefUse | IRContext::kAnalysisInstrToBlockMapping);

  uint32_t element_ptr_type_id =
      type_mgr->FindPointerToType(inst->type_id(), storage_class);
  if (element_ptr_type_id == 0) {
    return false;
  }

  Instruction* access_chain =
      ir_builder.AddAccessChain(element_ptr_type_id, ptr_id, index_ids);
  std::vector<Operand> load_operands;
  load_operands.push_back({SPV_OPERAND_TYPE_ID, {access_chain->result_id()}});

  if (cinst->NumInOperands() > 1) {
    uint32_t memory_access_mask = cinst->GetSingleWordInOperand(1);
    load_operands.push_back(
        {SPV_OPERAND_TYPE_MEMORY_ACCESS, {memory_access_mask}});

    uint32_t current_operand_index = 2;
    if (memory_access_mask & uint32_t(spv::MemoryAccessMask::Aligned)) {
      uint32_t original_alignment =
          cinst->GetSingleWordInOperand(current_operand_index);

      std::vector<uint32_t> extract_indices;
      for (uint32_t i = 1; i < inst->NumInOperands(); ++i) {
        extract_indices.push_back(inst->GetSingleWordInOperand(i));
      }

      std::optional<uint32_t> offset =
          type_mgr->GetType(cinst->type_id())->GetByteOffset(extract_indices);
      if (!offset) {
        return false;
      }

      uint32_t new_alignment = original_alignment;
      if (*offset != 0) {
        uint32_t offset_alignment = *offset & ~(*offset - 1);
        new_alignment = std::min(original_alignment, offset_alignment);
      }

      load_operands.push_back(
          {SPV_OPERAND_TYPE_TYPED_LITERAL_NUMBER, {new_alignment}});
      current_operand_index++;
    }


    for (; current_operand_index < cinst->NumInOperands();
         ++current_operand_index) {
      load_operands.push_back(cinst->GetInOperand(current_operand_index));
    }
  }

  uint32_t load_result_id = context->TakeNextId();
  if (load_result_id == 0) return false;

  std::unique_ptr<Instruction> new_load_inst(
      new Instruction(context, spv::Op::OpLoad, inst->type_id(), load_result_id,
                      load_operands));
  Instruction* new_load = ir_builder.AddInstruction(std::move(new_load_inst));

  inst->SetOpcode(spv::Op::OpCopyObject);
  inst->SetInOperands({{SPV_OPERAND_TYPE_ID, {new_load->result_id()}}});

  return true;
}





bool HaveSameIndexesExceptForLast(Instruction* inst_1, Instruction* inst_2) {
  assert(inst_1->opcode() == inst_2->opcode() &&
         "Expecting the opcodes to be the same.");
  assert((inst_1->opcode() == spv::Op::OpCompositeInsert ||
          inst_1->opcode() == spv::Op::OpCompositeExtract) &&
         "Instructions must be OpCompositeInsert or OpCompositeExtract.");

  if (inst_1->NumInOperands() != inst_2->NumInOperands()) {
    return false;
  }

  uint32_t first_index_position =
      (inst_1->opcode() == spv::Op::OpCompositeInsert ? 2 : 1);
  for (uint32_t i = first_index_position; i < inst_1->NumInOperands() - 1;
       i++) {
    if (inst_1->GetSingleWordInOperand(i) !=
        inst_2->GetSingleWordInOperand(i)) {
      return false;
    }
  }
  return true;
}






bool CompositeExtractFeedingConstruct(
    IRContext* context, Instruction* inst,
    const std::vector<const analysis::Constant*>&) {
  assert(inst->opcode() == spv::Op::OpCompositeConstruct &&
         "Wrong opcode.  Should be OpCompositeConstruct.");
  analysis::DefUseManager* def_use_mgr = context->get_def_use_mgr();
  uint32_t original_id = 0;

  if (inst->NumInOperands() == 0) {

    return false;
  }





  Instruction* first_element_inst = nullptr;
  for (uint32_t i = 0; i < inst->NumInOperands(); ++i) {
    const uint32_t element_id = inst->GetSingleWordInOperand(i);
    Instruction* element_inst = def_use_mgr->GetDef(element_id);
    if (first_element_inst == nullptr) {
      first_element_inst = element_inst;
    }

    if (element_inst->opcode() != spv::Op::OpCompositeExtract) {
      return false;
    }

    if (!HaveSameIndexesExceptForLast(element_inst, first_element_inst)) {
      return false;
    }

    if (element_inst->GetSingleWordInOperand(element_inst->NumInOperands() -
                                             1) != i) {
      return false;
    }

    if (i == 0) {
      original_id =
          element_inst->GetSingleWordInOperand(kExtractCompositeIdInIdx);
    } else if (original_id !=
               element_inst->GetSingleWordInOperand(kExtractCompositeIdInIdx)) {
      return false;
    }
  }
  assert(first_element_inst != nullptr);



  Instruction* original_inst = def_use_mgr->GetDef(original_id);
  uint32_t original_type_id =
      GetElementType(original_inst->type_id(), first_element_inst->begin() + 3,
                     first_element_inst->end() - 1, def_use_mgr);

  if (inst->type_id() != original_type_id) {
    return false;
  }

  if (first_element_inst->NumInOperands() == 2) {

    inst->SetOpcode(spv::Op::OpCopyObject);
    inst->SetInOperands({{SPV_OPERAND_TYPE_ID, {original_id}}});
    return true;
  }



  inst->SetOpcode(spv::Op::OpCompositeExtract);
  inst->SetInOperands(std::vector<Operand>(first_element_inst->begin() + 2,
                                           first_element_inst->end() - 1));
  return true;
}

FoldingRule InsertFeedingExtract() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>&) {
    assert(inst->opcode() == spv::Op::OpCompositeExtract &&
           "Wrong opcode.  Should be OpCompositeExtract.");
    analysis::DefUseManager* def_use_mgr = context->get_def_use_mgr();
    uint32_t cid = inst->GetSingleWordInOperand(kExtractCompositeIdInIdx);
    Instruction* cinst = def_use_mgr->GetDef(cid);

    if (cinst->opcode() != spv::Op::OpCompositeInsert) {
      return false;
    }



    uint32_t i;
    for (i = 1; i < inst->NumInOperands(); ++i) {
      if (i + 1 >= cinst->NumInOperands()) {
        break;
      }

      if (inst->GetSingleWordInOperand(i) !=
          cinst->GetSingleWordInOperand(i + 1)) {
        break;
      }
    }


    if (i == inst->NumInOperands() && i + 1 == cinst->NumInOperands()) {
      inst->SetOpcode(spv::Op::OpCopyObject);
      inst->SetInOperands(
          {{SPV_OPERAND_TYPE_ID,
            {cinst->GetSingleWordInOperand(kInsertObjectIdInIdx)}}});
      return true;
    }



    if (i == inst->NumInOperands()) {
      return false;
    }



    if (i + 1 == cinst->NumInOperands()) {
      std::vector<Operand> operands;
      operands.push_back(
          {SPV_OPERAND_TYPE_ID,
           {cinst->GetSingleWordInOperand(kInsertObjectIdInIdx)}});
      for (; i < inst->NumInOperands(); ++i) {
        operands.push_back({SPV_OPERAND_TYPE_LITERAL_INTEGER,
                            {inst->GetSingleWordInOperand(i)}});
      }
      inst->SetInOperands(std::move(operands));
      return true;
    }



    std::vector<Operand> operands;
    operands.push_back(
        {SPV_OPERAND_TYPE_ID,
         {cinst->GetSingleWordInOperand(kInsertCompositeIdInIdx)}});
    for (i = 1; i < inst->NumInOperands(); ++i) {
      operands.push_back({SPV_OPERAND_TYPE_LITERAL_INTEGER,
                          {inst->GetSingleWordInOperand(i)}});
    }
    inst->SetInOperands(std::move(operands));
    return true;
  };
}




FoldingRule VectorShuffleFeedingExtract() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>&) {
    assert(inst->opcode() == spv::Op::OpCompositeExtract &&
           "Wrong opcode.  Should be OpCompositeExtract.");
    analysis::DefUseManager* def_use_mgr = context->get_def_use_mgr();
    analysis::TypeManager* type_mgr = context->get_type_mgr();
    uint32_t cid = inst->GetSingleWordInOperand(kExtractCompositeIdInIdx);
    Instruction* cinst = def_use_mgr->GetDef(cid);

    if (cinst->opcode() != spv::Op::OpVectorShuffle) {
      return false;
    }


    Instruction* first_input =
        def_use_mgr->GetDef(cinst->GetSingleWordInOperand(0));
    analysis::Type* first_input_type =
        type_mgr->GetType(first_input->type_id());
    assert(first_input_type->AsVector() &&
           "Input to vector shuffle should be vectors.");
    uint32_t first_input_size = first_input_type->AsVector()->element_count();



    uint32_t new_index =
        cinst->GetSingleWordInOperand(2 + inst->GetSingleWordInOperand(1));


    const uint32_t undef_literal_value = 0xffffffff;
    if (new_index == undef_literal_value) {
      inst->SetOpcode(spv::Op::OpUndef);
      inst->SetInOperands({});
      return true;
    }



    uint32_t new_vector = 0;
    if (new_index < first_input_size) {
      new_vector = cinst->GetSingleWordInOperand(0);
    } else {
      new_vector = cinst->GetSingleWordInOperand(1);
      new_index -= first_input_size;
    }


    inst->SetInOperand(kExtractCompositeIdInIdx, {new_vector});
    inst->SetInOperand(1, {new_index});
    return true;
  };
}




FoldingRule FMixFeedingExtract() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>&) {
    assert(inst->opcode() == spv::Op::OpCompositeExtract &&
           "Wrong opcode.  Should be OpCompositeExtract.");
    analysis::DefUseManager* def_use_mgr = context->get_def_use_mgr();
    analysis::ConstantManager* const_mgr = context->get_constant_mgr();

    uint32_t composite_id =
        inst->GetSingleWordInOperand(kExtractCompositeIdInIdx);
    Instruction* composite_inst = def_use_mgr->GetDef(composite_id);

    if (composite_inst->opcode() != spv::Op::OpExtInst) {
      return false;
    }

    uint32_t inst_set_id =
        context->get_feature_mgr()->GetExtInstImportId_GLSLstd450();

    if (composite_inst->GetSingleWordInOperand(kExtInstSetIdInIdx) !=
            inst_set_id ||
        composite_inst->GetSingleWordInOperand(kExtInstInstructionInIdx) !=
            GLSLstd450FMix) {
      return false;
    }


    uint32_t a_id = composite_inst->GetSingleWordInOperand(kFMixAIdInIdx);
    std::unique_ptr<Instruction> a(inst->Clone(context));
    a->SetInOperand(kExtractCompositeIdInIdx, {a_id});
    context->get_instruction_folder().FoldInstruction(a.get());

    if (a->opcode() != spv::Op::OpCopyObject) {
      return false;
    }

    const analysis::Constant* a_const =
        const_mgr->FindDeclaredConstant(a->GetSingleWordInOperand(0));

    if (!a_const) {
      return false;
    }

    bool use_x = false;

    assert(a_const->type()->AsFloat());

    const analysis::Type* type =
        context->get_type_mgr()->GetType(inst->type_id());
    uint32_t width = ElementWidth(type);
    if (width != 32 && width != 64) {

      return false;
    }

    double element_value = a_const->GetValueAsDouble();
    if (element_value == 0.0) {
      use_x = true;
    } else if (element_value == 1.0) {
      use_x = false;
    } else {
      return false;
    }


    uint32_t new_vector = 0;
    if (use_x) {
      new_vector = composite_inst->GetSingleWordInOperand(kFMixXIdInIdx);
    } else {
      new_vector = composite_inst->GetSingleWordInOperand(kFMixYIdInIdx);
    }


    inst->SetInOperand(kExtractCompositeIdInIdx, {new_vector});
    return true;
  };
}




uint32_t GetNumberOfElements(const analysis::Type* type) {
  if (auto* vector_type = type->AsVector()) {
    return vector_type->element_count();
  }
  if (auto* matrix_type = type->AsMatrix()) {
    return matrix_type->element_count();
  }
  if (auto* struct_type = type->AsStruct()) {
    return static_cast<uint32_t>(struct_type->element_types().size());
  }
  if (auto* array_type = type->AsArray()) {
    if (array_type->length_info().words[0] ==
            analysis::Array::LengthInfo::kConstant &&
        array_type->length_info().words.size() == 2) {
      return array_type->length_info().words[1];
    }
    return UINT32_MAX;
  }
  return 0;
}





std::map<uint32_t, uint32_t> GetInsertedValues(Instruction* inst) {
  analysis::DefUseManager* def_use_mgr = inst->context()->get_def_use_mgr();
  std::map<uint32_t, uint32_t> values_inserted;
  Instruction* current_inst = inst;
  while (current_inst->opcode() == spv::Op::OpCompositeInsert) {
    if (current_inst->NumInOperands() > inst->NumInOperands()) {





      uint32_t partially_inserted_element_index =
          current_inst->GetSingleWordInOperand(inst->NumInOperands() - 1);
      if (values_inserted.count(partially_inserted_element_index) == 0)
        return {};
    }
    if (HaveSameIndexesExceptForLast(inst, current_inst)) {
      values_inserted.insert(
          {current_inst->GetSingleWordInOperand(current_inst->NumInOperands() -
                                                1),
           current_inst->GetSingleWordInOperand(kInsertObjectIdInIdx)});
    }
    current_inst = def_use_mgr->GetDef(
        current_inst->GetSingleWordInOperand(kInsertCompositeIdInIdx));
  }
  return values_inserted;
}



bool DoInsertedValuesCoverEntireObject(
    const analysis::Type* type, std::map<uint32_t, uint32_t>& values_inserted) {
  uint32_t container_size = GetNumberOfElements(type);
  if (container_size != values_inserted.size()) {
    return false;
  }

  if (values_inserted.rbegin()->first >= container_size) {
    return false;
  }
  return true;
}




uint32_t GetContainerTypeId(Instruction* inst) {
  assert(inst->opcode() == spv::Op::OpCompositeInsert);
  analysis::DefUseManager* def_use_manager = inst->context()->get_def_use_mgr();
  uint32_t container_type_id = GetElementType(
      inst->type_id(), inst->begin() + 4, inst->end() - 1, def_use_manager);
  return container_type_id;
}





Instruction* BuildCompositeConstruct(
    uint32_t type_id, const std::map<uint32_t, uint32_t>& values_inserted,
    Instruction* insert_before) {
  InstructionBuilder ir_builder(
      insert_before->context(), insert_before,
      IRContext::kAnalysisDefUse | IRContext::kAnalysisInstrToBlockMapping);

  std::vector<uint32_t> ids_in_order;
  for (auto it : values_inserted) {
    ids_in_order.push_back(it.second);
  }
  Instruction* construct =
      ir_builder.AddCompositeConstruct(type_id, ids_in_order);
  return construct;
}





void InsertConstructedObject(Instruction* inst, const Instruction* construct) {
  if (inst->NumInOperands() == 3) {
    inst->SetOpcode(spv::Op::OpCopyObject);
    inst->SetInOperands({{SPV_OPERAND_TYPE_ID, {construct->result_id()}}});
  } else {
    inst->SetInOperand(kInsertObjectIdInIdx, {construct->result_id()});
    inst->RemoveOperand(inst->NumOperands() - 1);
  }
}



bool CompositeInsertToCompositeConstruct(
    IRContext* context, Instruction* inst,
    const std::vector<const analysis::Constant*>&) {
  assert(inst->opcode() == spv::Op::OpCompositeInsert &&
         "Wrong opcode.  Should be OpCompositeInsert.");
  if (inst->NumInOperands() < 3) return false;

  std::map<uint32_t, uint32_t> values_inserted = GetInsertedValues(inst);
  uint32_t container_type_id = GetContainerTypeId(inst);
  if (container_type_id == 0) {
    return false;
  }

  analysis::TypeManager* type_mgr = context->get_type_mgr();
  const analysis::Type* container_type = type_mgr->GetType(container_type_id);
  assert(container_type && "GetContainerTypeId returned a bad id.");
  if (!DoInsertedValuesCoverEntireObject(container_type, values_inserted)) {
    return false;
  }

  Instruction* construct =
      BuildCompositeConstruct(container_type_id, values_inserted, inst);
  InsertConstructedObject(inst, construct);
  return true;
}

FoldingRule RedundantPhi() {


  return [](IRContext*, Instruction* inst,
            const std::vector<const analysis::Constant*>&) {
    assert(inst->opcode() == spv::Op::OpPhi &&
           "Wrong opcode.  Should be OpPhi.");

    uint32_t incoming_value = 0;

    for (uint32_t i = 0; i < inst->NumInOperands(); i += 2) {
      uint32_t op_id = inst->GetSingleWordInOperand(i);
      if (op_id == inst->result_id()) {
        continue;
      }

      if (incoming_value == 0) {
        incoming_value = op_id;
      } else if (op_id != incoming_value) {

        return false;
      }
    }

    if (incoming_value == 0) {

      return false;
    }


    inst->SetOpcode(spv::Op::OpCopyObject);
    inst->SetInOperands({{SPV_OPERAND_TYPE_ID, {incoming_value}}});
    return true;
  };
}

FoldingRule BitCastScalarOrVector() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(inst->opcode() == spv::Op::OpBitcast && constants.size() == 1);
    if (constants[0] == nullptr) return false;

    const analysis::Type* type =
        context->get_type_mgr()->GetType(inst->type_id());
    if (HasFloatingPoint(type) && !inst->IsFloatingPointFoldingAllowed())
      return false;

    analysis::ConstantManager* const_mgr = context->get_constant_mgr();
    std::vector<uint32_t> words =
        GetWordsFromNumericScalarOrVectorConstant(const_mgr, constants[0]);
    if (words.size() == 0) return false;

    const analysis::Constant* bitcasted_constant =
        ConvertWordsToNumericScalarOrVectorConstant(const_mgr, words, type);
    if (!bitcasted_constant) return false;

    auto new_feeder_id =
        const_mgr->GetDefiningInstruction(bitcasted_constant, inst->type_id())
            ->result_id();
    inst->SetOpcode(spv::Op::OpCopyObject);
    inst->SetInOperands({{SPV_OPERAND_TYPE_ID, {new_feeder_id}}});
    return true;
  };
}





FoldingRule RedundantBitcast() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>&) {
    assert(inst->opcode() == spv::Op::OpBitcast);

    analysis::DefUseManager* def_mgr = context->get_def_use_mgr();
    Instruction* child = def_mgr->GetDef(inst->GetSingleWordInOperand(0));

    if (inst->type_id() == child->type_id()) {
      inst->SetOpcode(spv::Op::OpCopyObject);
      inst->SetInOperands({{SPV_OPERAND_TYPE_ID, {child->result_id()}}});
      return true;
    }

    if (child->opcode() != spv::Op::OpBitcast) {
      return false;
    }

    if (def_mgr->GetDef(child->GetSingleWordInOperand(0))->type_id() ==
        inst->type_id()) {
      inst->SetOpcode(spv::Op::OpCopyObject);
    }
    inst->SetInOperands(
        {{SPV_OPERAND_TYPE_ID, {child->GetSingleWordInOperand(0)}}});

    return true;
  };
}

FoldingRule BitReverseScalarOrVector() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(inst->opcode() == spv::Op::OpBitReverse && constants.size() == 1);
    if (constants[0] == nullptr) return false;

    const analysis::Type* type =
        context->get_type_mgr()->GetType(inst->type_id());
    assert(!HasFloatingPoint(type) &&
           "BitReverse cannot be applied to floating point types.");
    assert((type->AsInteger() || type->AsVector()) &&
           "BitReverse can only be applied to integer scalars or vectors.");
    assert((ElementWidth(type) == 32) &&
           "BitReverse can only be applied to integer types of width 32");

    analysis::ConstantManager* const_mgr = context->get_constant_mgr();
    std::vector<uint32_t> words =
        GetWordsFromNumericScalarOrVectorConstant(const_mgr, constants[0]);
    if (words.size() == 0) return false;

    for (uint32_t& word : words) {

      word = ((word & 0x55555555) << 1) | ((word >> 1) & 0x55555555);
      word = ((word & 0x33333333) << 2) | ((word >> 2) & 0x33333333);
      word = ((word & 0x0F0F0F0F) << 4) | ((word >> 4) & 0x0F0F0F0F);
      word = ((word & 0x00FF00FF) << 8) | ((word >> 8) & 0x00FF00FF);
      word = (word << 16) | (word >> 16);
    }

    const analysis::Constant* bitreversed_constant =
        ConvertWordsToNumericScalarOrVectorConstant(const_mgr, words, type);
    if (!bitreversed_constant) return false;

    auto new_feeder_id =
        const_mgr->GetDefiningInstruction(bitreversed_constant, inst->type_id())
            ->result_id();
    inst->SetOpcode(spv::Op::OpCopyObject);
    inst->SetInOperands({{SPV_OPERAND_TYPE_ID, {new_feeder_id}}});
    return true;
  };
}

FoldingRule RedundantSelect() {


  return [](IRContext*, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(inst->opcode() == spv::Op::OpSelect &&
           "Wrong opcode.  Should be OpSelect.");
    assert(inst->NumInOperands() == 3);
    assert(constants.size() == 3);

    uint32_t true_id = inst->GetSingleWordInOperand(1);
    uint32_t false_id = inst->GetSingleWordInOperand(2);

    if (true_id == false_id) {

      inst->SetOpcode(spv::Op::OpCopyObject);
      inst->SetInOperands({{SPV_OPERAND_TYPE_ID, {true_id}}});
      return true;
    } else if (constants[0]) {
      const analysis::Type* type = constants[0]->type();
      if (type->AsBool()) {

        inst->SetOpcode(spv::Op::OpCopyObject);
        if (constants[0]->AsNullConstant() ||
            !constants[0]->AsBoolConstant()->value()) {
          inst->SetInOperands({{SPV_OPERAND_TYPE_ID, {false_id}}});
        } else {
          inst->SetInOperands({{SPV_OPERAND_TYPE_ID, {true_id}}});
        }
        return true;
      } else {
        assert(type->AsVector());
        if (constants[0]->AsNullConstant()) {

          inst->SetOpcode(spv::Op::OpCopyObject);
          inst->SetInOperands({{SPV_OPERAND_TYPE_ID, {false_id}}});
          return true;
        } else {

          std::vector<Operand> ops;
          ops.push_back({SPV_OPERAND_TYPE_ID, {true_id}});
          ops.push_back({SPV_OPERAND_TYPE_ID, {false_id}});
          const analysis::VectorConstant* vector_const =
              constants[0]->AsVectorConstant();
          uint32_t size =
              static_cast<uint32_t>(vector_const->GetComponents().size());
          for (uint32_t i = 0; i != size; ++i) {
            const analysis::Constant* component =
                vector_const->GetComponents()[i];
            if (component->AsNullConstant() ||
                !component->AsBoolConstant()->value()) {


              ops.push_back({SPV_OPERAND_TYPE_LITERAL_INTEGER, {i + size}});
            } else {


              ops.push_back({SPV_OPERAND_TYPE_LITERAL_INTEGER, {i}});
            }
          }

          inst->SetOpcode(spv::Op::OpVectorShuffle);
          inst->SetInOperands(std::move(ops));
          return true;
        }
      }
    }

    return false;
  };
}

std::optional<bool> GetBoolConstantKind(const analysis::Constant* c) {
  if (!c) {
    return {};
  }
  if (auto composite = c->AsCompositeConstant()) {
    auto& components = composite->GetComponents();
    if (components.empty()) {
      return {};
    }
    auto first = GetBoolConstantKind(components[0]);
    if (!first) {
      return {};
    }
    if (std::all_of(std::begin(components) + 1, std::end(components),
                    [first](const analysis::Constant* c2) {
                      return GetBoolConstantKind(c2) == first;
                    })) {
      return first;
    }
    return {};
  } else if (c->AsNullConstant()) {
    return false;
  } else if (c->AsBoolConstant()) {
    return c->AsBoolConstant()->value();
  }
  return {};
}




FoldingRule FoldConstantBooleanSelect() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(inst->opcode() == spv::Op::OpSelect);
    assert(inst->NumInOperands() == 3);
    assert(constants.size() == 3);

    if (!constants[1] || !constants[2]) {
      return false;
    }

    analysis::DefUseManager* def_mgr = context->get_def_use_mgr();
    if (inst->type_id() !=
        def_mgr->GetDef(inst->GetSingleWordInOperand(0))->type_id()) {
      return false;
    }

    std::optional<bool> uniform_true = GetBoolConstantKind(constants[1]);
    std::optional<bool> uniform_false = GetBoolConstantKind(constants[2]);

    if (!uniform_true || !uniform_false) {
      return false;
    }

    if (uniform_true.value() && !uniform_false.value()) {
      inst->SetOpcode(spv::Op::OpCopyObject);
      inst->SetInOperands(
          {{SPV_OPERAND_TYPE_ID, {inst->GetSingleWordInOperand(0)}}});
      return true;
    } else if (!uniform_true.value() && uniform_false.value()) {
      inst->SetOpcode(spv::Op::OpLogicalNot);
      inst->SetInOperands(
          {{SPV_OPERAND_TYPE_ID, {inst->GetSingleWordInOperand(0)}}});
      return true;
    }
    return false;
  };
}




FoldingRule RedundantLogicalAnd() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(inst->opcode() == spv::Op::OpLogicalAnd);

    if (GetBoolConstantKind(ConstInput(constants)) ==
        std::optional<bool>(true)) {
      Instruction* other_inst = NonConstInput(context, constants[0], inst);
      inst->SetOpcode(spv::Op::OpCopyObject);
      inst->SetInOperands({{SPV_OPERAND_TYPE_ID, {other_inst->result_id()}}});
      return true;
    }
    return false;
  };
}




FoldingRule RedundantLogicalOr() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(inst->opcode() == spv::Op::OpLogicalOr);

    if (GetBoolConstantKind(ConstInput(constants)) ==
        std::optional<bool>(false)) {
      Instruction* other_inst = NonConstInput(context, constants[0], inst);
      inst->SetOpcode(spv::Op::OpCopyObject);
      inst->SetInOperands({{SPV_OPERAND_TYPE_ID, {other_inst->result_id()}}});
      return true;
    }
    return false;
  };
}



FoldingRule RedundantLogicalNot() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>&) {
    assert(inst->opcode() == spv::Op::OpLogicalNot);
    Instruction* child =
        context->get_def_use_mgr()->GetDef(inst->GetSingleWordInOperand(0));
    if (child->opcode() == spv::Op::OpLogicalNot) {
      inst->SetOpcode(spv::Op::OpCopyObject);
      inst->SetInOperands(
          {{SPV_OPERAND_TYPE_ID, {child->GetSingleWordInOperand(0)}}});
      return true;
    }
    return false;
  };
}



















static const constexpr spv::Op MergeBinaryOpSelectOps[] = {
    spv::Op::OpLogicalEqual,
    spv::Op::OpLogicalNotEqual,
    spv::Op::OpLogicalAnd,
    spv::Op::OpLogicalOr,
    spv::Op::OpIEqual,
    spv::Op::OpINotEqual,
    spv::Op::OpUGreaterThan,
    spv::Op::OpSGreaterThan,
    spv::Op::OpUGreaterThanEqual,
    spv::Op::OpSGreaterThanEqual,
    spv::Op::OpULessThan,
    spv::Op::OpSLessThan,
    spv::Op::OpULessThanEqual,
    spv::Op::OpSLessThanEqual,
    spv::Op::OpFOrdEqual,
    spv::Op::OpFUnordEqual,
    spv::Op::OpFOrdNotEqual,
    spv::Op::OpFUnordNotEqual,
    spv::Op::OpFOrdLessThan,
    spv::Op::OpFUnordLessThan,
    spv::Op::OpFOrdGreaterThan,
    spv::Op::OpFUnordGreaterThan,
    spv::Op::OpFOrdLessThanEqual,
    spv::Op::OpFUnordLessThanEqual,
    spv::Op::OpFOrdGreaterThanEqual,
    spv::Op::OpFUnordGreaterThanEqual,
    spv::Op::OpIAdd,
    spv::Op::OpFAdd,
    spv::Op::OpISub,
    spv::Op::OpFSub,
    spv::Op::OpIMul,
    spv::Op::OpFMul,
    spv::Op::OpUDiv,
    spv::Op::OpSDiv,
    spv::Op::OpFDiv,
    spv::Op::OpVectorTimesScalar,
    spv::Op::OpShiftRightLogical,
    spv::Op::OpShiftRightArithmetic,
    spv::Op::OpShiftLeftLogical,
    spv::Op::OpBitwiseXor,
    spv::Op::OpBitwiseOr,
    spv::Op::OpBitwiseAnd};

FoldingRule MergeBinaryOpSelect(spv::Op opcode) {
  assert(std::find(std::begin(MergeBinaryOpSelectOps),
                   std::end(MergeBinaryOpSelectOps),
                   opcode) != std::end(MergeBinaryOpSelectOps) &&
         "Wrong opcode.");

  return [opcode](IRContext* context, Instruction* inst,
                  const std::vector<const analysis::Constant*>& constants) {
    const analysis::Constant* const_input = ConstInput(constants);
    if (!const_input) {
      return false;
    }
    Instruction* non_const = NonConstInput(context, constants[0], inst);
    if (non_const->opcode() != spv::Op::OpSelect) {
      return false;
    }
    std::vector<const analysis::Constant*> select_constants =
        context->get_constant_mgr()->GetOperandConstants(non_const);
    if (!select_constants[1] || !select_constants[2]) {
      return false;
    }

    InstructionBuilder ir_builder(
        context, inst,
        IRContext::kAnalysisDefUse | IRContext::kAnalysisInstrToBlockMapping);

    Instruction *lhs, *rhs;
    if (constants[0]) {
      lhs = ir_builder.AddBinaryOp(inst->type_id(), opcode,
                                   inst->GetSingleWordInOperand(0),
                                   non_const->GetSingleWordInOperand(1));
      rhs = ir_builder.AddBinaryOp(inst->type_id(), opcode,
                                   inst->GetSingleWordInOperand(0),
                                   non_const->GetSingleWordInOperand(2));
    } else {
      lhs = ir_builder.AddBinaryOp(inst->type_id(), opcode,
                                   non_const->GetSingleWordInOperand(1),
                                   inst->GetSingleWordInOperand(1));
      rhs = ir_builder.AddBinaryOp(inst->type_id(), opcode,
                                   non_const->GetSingleWordInOperand(2),
                                   inst->GetSingleWordInOperand(1));
    }

    if (!lhs || !rhs) {
      return false;
    }

    if (context->get_instruction_folder().FoldInstruction(lhs)) {
      context->AnalyzeDefUse(lhs);
      while (lhs->opcode() == spv::Op::OpCopyObject) {
        lhs =
            context->get_def_use_mgr()->GetDef(lhs->GetSingleWordInOperand(0));
      }
    }
    if (context->get_instruction_folder().FoldInstruction(rhs)) {
      context->AnalyzeDefUse(rhs);
      while (rhs->opcode() == spv::Op::OpCopyObject) {
        rhs =
            context->get_def_use_mgr()->GetDef(rhs->GetSingleWordInOperand(0));
      }
    }
    inst->SetOpcode(spv::Op::OpSelect);
    inst->SetInOperands(
        {{SPV_OPERAND_TYPE_ID, {non_const->GetSingleWordInOperand(0)}},
         {SPV_OPERAND_TYPE_ID, {lhs->result_id()}},
         {SPV_OPERAND_TYPE_ID, {rhs->result_id()}}});
    return true;
  };
}










FoldingRule FoldLogicalNotComparison() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>&) {
    assert(inst->opcode() == spv::Op::OpLogicalNot);
    analysis::DefUseManager* def_mgr = context->get_def_use_mgr();
    Instruction* child =
        context->get_def_use_mgr()->GetDef(inst->GetSingleWordInOperand(0));

    if (def_mgr->NumUses(child) > 1) {
      return false;
    }

    spv::Op new_opcode = spv::Op::OpNop;
    switch (child->opcode()) {

      case spv::Op::OpIEqual:
        new_opcode = spv::Op::OpINotEqual;
        break;
      case spv::Op::OpINotEqual:
        new_opcode = spv::Op::OpIEqual;
        break;
      case spv::Op::OpFOrdEqual:
        new_opcode = spv::Op::OpFUnordNotEqual;
        break;
      case spv::Op::OpFOrdNotEqual:
        new_opcode = spv::Op::OpFUnordEqual;
        break;
      case spv::Op::OpFUnordEqual:
        new_opcode = spv::Op::OpFOrdNotEqual;
        break;
      case spv::Op::OpFUnordNotEqual:
        new_opcode = spv::Op::OpFOrdEqual;
        break;
      case spv::Op::OpLogicalEqual:
        new_opcode = spv::Op::OpLogicalNotEqual;
        break;
      case spv::Op::OpLogicalNotEqual:
        new_opcode = spv::Op::OpLogicalEqual;
        break;


      case spv::Op::OpUGreaterThan:
        new_opcode = spv::Op::OpULessThanEqual;
        break;
      case spv::Op::OpULessThanEqual:
        new_opcode = spv::Op::OpUGreaterThan;
        break;
      case spv::Op::OpSGreaterThan:
        new_opcode = spv::Op::OpSLessThanEqual;
        break;
      case spv::Op::OpSLessThanEqual:
        new_opcode = spv::Op::OpSGreaterThan;
        break;
      case spv::Op::OpFOrdGreaterThan:
        new_opcode = spv::Op::OpFUnordLessThanEqual;
        break;
      case spv::Op::OpFOrdLessThanEqual:
        new_opcode = spv::Op::OpFUnordGreaterThan;
        break;
      case spv::Op::OpFUnordGreaterThan:
        new_opcode = spv::Op::OpFOrdLessThanEqual;
        break;
      case spv::Op::OpFUnordLessThanEqual:
        new_opcode = spv::Op::OpFOrdGreaterThan;
        break;


      case spv::Op::OpULessThan:
        new_opcode = spv::Op::OpUGreaterThanEqual;
        break;
      case spv::Op::OpUGreaterThanEqual:
        new_opcode = spv::Op::OpULessThan;
        break;
      case spv::Op::OpSLessThan:
        new_opcode = spv::Op::OpSGreaterThanEqual;
        break;
      case spv::Op::OpSGreaterThanEqual:
        new_opcode = spv::Op::OpSLessThan;
        break;
      case spv::Op::OpFOrdLessThan:
        new_opcode = spv::Op::OpFUnordGreaterThanEqual;
        break;
      case spv::Op::OpFOrdGreaterThanEqual:
        new_opcode = spv::Op::OpFUnordLessThan;
        break;
      case spv::Op::OpFUnordLessThan:
        new_opcode = spv::Op::OpFOrdGreaterThanEqual;
        break;
      case spv::Op::OpFUnordGreaterThanEqual:
        new_opcode = spv::Op::OpFOrdLessThan;
        break;

      default:
        break;
    }

    if (new_opcode == spv::Op::OpNop) {
      return false;
    }

    inst->SetOpcode(new_opcode);
    inst->SetInOperands(
        {{SPV_OPERAND_TYPE_ID, {child->GetSingleWordInOperand(0)}},
         {SPV_OPERAND_TYPE_ID, {child->GetSingleWordInOperand(1)}}});

    return true;
  };
}





FoldingRule RedundantLogicalEqual() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(inst->opcode() == spv::Op::OpLogicalEqual ||
           inst->opcode() == spv::Op::OpLogicalNotEqual);

    const analysis::Constant* const_input = ConstInput(constants);
    if (!const_input) {
      return false;
    }

    analysis::DefUseManager* def_mgr = context->get_def_use_mgr();
    if (inst->type_id() !=
        def_mgr->GetDef(inst->GetSingleWordInOperand(0))->type_id()) {
      return false;
    }

    std::optional<bool> uniform_const = GetBoolConstantKind(const_input);
    if (!uniform_const) {
      return false;
    }

    bool direct_copy = inst->opcode() == spv::Op::OpLogicalEqual
                           ? uniform_const.value()
                           : !uniform_const.value();

    inst->SetOpcode(direct_copy ? spv::Op::OpCopyObject
                                : spv::Op::OpLogicalNot);
    inst->SetInOperands(
        {{SPV_OPERAND_TYPE_ID,
          {NonConstInput(context, constants[0], inst)->result_id()}}});
    return true;
  };
}

enum class FloatConstantKind { Unknown, Zero, One };

FloatConstantKind getFloatConstantKind(const analysis::Constant* constant) {
  if (constant == nullptr) {
    return FloatConstantKind::Unknown;
  }

  assert(HasFloatingPoint(constant->type()) && "Unexpected constant type");

  if (constant->AsNullConstant()) {
    return FloatConstantKind::Zero;
  } else if (const analysis::VectorConstant* vc =
                 constant->AsVectorConstant()) {
    const std::vector<const analysis::Constant*>& components =
        vc->GetComponents();
    assert(!components.empty());

    FloatConstantKind kind = getFloatConstantKind(components[0]);

    for (size_t i = 1; i < components.size(); ++i) {
      if (getFloatConstantKind(components[i]) != kind) {
        return FloatConstantKind::Unknown;
      }
    }

    return kind;
  } else if (const analysis::FloatConstant* fc = constant->AsFloatConstant()) {
    if (fc->IsZero()) return FloatConstantKind::Zero;

    uint32_t width = fc->type()->AsFloat()->width();
    if (width != 32 && width != 64) return FloatConstantKind::Unknown;

    double value = (width == 64) ? fc->GetDoubleValue() : fc->GetFloatValue();

    if (value == 0.0) {
      return FloatConstantKind::Zero;
    } else if (value == 1.0) {
      return FloatConstantKind::One;
    } else {
      return FloatConstantKind::Unknown;
    }
  } else {
    return FloatConstantKind::Unknown;
  }
}

FoldingRule RedundantFAdd() {
  return [](IRContext*, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(inst->opcode() == spv::Op::OpFAdd &&
           "Wrong opcode.  Should be OpFAdd.");
    assert(constants.size() == 2);

    if (!inst->IsFloatingPointFoldingAllowed()) {
      return false;
    }

    FloatConstantKind kind0 = getFloatConstantKind(constants[0]);
    FloatConstantKind kind1 = getFloatConstantKind(constants[1]);

    if (kind0 == FloatConstantKind::Zero || kind1 == FloatConstantKind::Zero) {
      inst->SetOpcode(spv::Op::OpCopyObject);
      inst->SetInOperands({{SPV_OPERAND_TYPE_ID,
                            {inst->GetSingleWordInOperand(
                                kind0 == FloatConstantKind::Zero ? 1 : 0)}}});
      return true;
    }

    return false;
  };
}

FoldingRule RedundantFSub() {
  return [](IRContext*, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(inst->opcode() == spv::Op::OpFSub &&
           "Wrong opcode.  Should be OpFSub.");
    assert(constants.size() == 2);

    if (!inst->IsFloatingPointFoldingAllowed()) {
      return false;
    }

    FloatConstantKind kind0 = getFloatConstantKind(constants[0]);
    FloatConstantKind kind1 = getFloatConstantKind(constants[1]);

    if (kind0 == FloatConstantKind::Zero) {
      inst->SetOpcode(spv::Op::OpFNegate);
      inst->SetInOperands(
          {{SPV_OPERAND_TYPE_ID, {inst->GetSingleWordInOperand(1)}}});
      return true;
    }

    if (kind1 == FloatConstantKind::Zero) {
      inst->SetOpcode(spv::Op::OpCopyObject);
      inst->SetInOperands(
          {{SPV_OPERAND_TYPE_ID, {inst->GetSingleWordInOperand(0)}}});
      return true;
    }

    return false;
  };
}

FoldingRule RedundantFMul() {
  return [](IRContext*, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(inst->opcode() == spv::Op::OpFMul &&
           "Wrong opcode.  Should be OpFMul.");
    assert(constants.size() == 2);

    if (!inst->IsFloatingPointFoldingAllowed()) {
      return false;
    }

    FloatConstantKind kind0 = getFloatConstantKind(constants[0]);
    FloatConstantKind kind1 = getFloatConstantKind(constants[1]);

    if (kind0 == FloatConstantKind::Zero || kind1 == FloatConstantKind::Zero) {
      inst->SetOpcode(spv::Op::OpCopyObject);
      inst->SetInOperands({{SPV_OPERAND_TYPE_ID,
                            {inst->GetSingleWordInOperand(
                                kind0 == FloatConstantKind::Zero ? 0 : 1)}}});
      return true;
    }

    if (kind0 == FloatConstantKind::One || kind1 == FloatConstantKind::One) {
      inst->SetOpcode(spv::Op::OpCopyObject);
      inst->SetInOperands({{SPV_OPERAND_TYPE_ID,
                            {inst->GetSingleWordInOperand(
                                kind0 == FloatConstantKind::One ? 1 : 0)}}});
      return true;
    }

    return false;
  };
}

FoldingRule RedundantFDiv() {
  return [](IRContext*, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(inst->opcode() == spv::Op::OpFDiv &&
           "Wrong opcode.  Should be OpFDiv.");
    assert(constants.size() == 2);

    if (!inst->IsFloatingPointFoldingAllowed()) {
      return false;
    }

    FloatConstantKind kind0 = getFloatConstantKind(constants[0]);
    FloatConstantKind kind1 = getFloatConstantKind(constants[1]);

    if (kind0 == FloatConstantKind::Zero || kind1 == FloatConstantKind::One) {
      inst->SetOpcode(spv::Op::OpCopyObject);
      inst->SetInOperands(
          {{SPV_OPERAND_TYPE_ID, {inst->GetSingleWordInOperand(0)}}});
      return true;
    }

    return false;
  };
}

FoldingRule RedundantFMod() {
  return [](IRContext*, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(inst->opcode() == spv::Op::OpFMod &&
           "Wrong opcode.  Should be OpFMod.");
    assert(constants.size() == 2);

    if (!inst->IsFloatingPointFoldingAllowed()) {
      return false;
    }

    FloatConstantKind kind0 = getFloatConstantKind(constants[0]);

    if (kind0 == FloatConstantKind::Zero) {
      inst->SetOpcode(spv::Op::OpCopyObject);
      inst->SetInOperands(
          {{SPV_OPERAND_TYPE_ID, {inst->GetSingleWordInOperand(0)}}});
      return true;
    }

    return false;
  };
}

FoldingRule RedundantFMix() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(inst->opcode() == spv::Op::OpExtInst &&
           "Wrong opcode.  Should be OpExtInst.");

    if (!inst->IsFloatingPointFoldingAllowed()) {
      return false;
    }

    uint32_t instSetId =
        context->get_feature_mgr()->GetExtInstImportId_GLSLstd450();

    if (inst->GetSingleWordInOperand(kExtInstSetIdInIdx) == instSetId &&
        inst->GetSingleWordInOperand(kExtInstInstructionInIdx) ==
            GLSLstd450FMix) {
      assert(constants.size() == 5);

      FloatConstantKind kind4 = getFloatConstantKind(constants[4]);

      if (kind4 == FloatConstantKind::Zero || kind4 == FloatConstantKind::One) {
        inst->SetOpcode(spv::Op::OpCopyObject);
        inst->SetInOperands(
            {{SPV_OPERAND_TYPE_ID,
              {inst->GetSingleWordInOperand(kind4 == FloatConstantKind::Zero
                                                ? kFMixXIdInIdx
                                                : kFMixYIdInIdx)}}});
        return true;
      }
    }

    return false;
  };
}



FoldingRule RedundantBinaryOpWithZeroOperand(uint32_t arg, uint32_t foldToArg) {
  return [arg, foldToArg](
             IRContext* context, Instruction* inst,
             const std::vector<const analysis::Constant*>& constants) {
    assert(constants.size() == 2);

    if (constants[arg] && constants[arg]->IsZero()) {
      auto operand = inst->GetSingleWordInOperand(foldToArg);
      auto operand_type = constants[arg]->type();

      const analysis::Type* inst_type =
          context->get_type_mgr()->GetType(inst->type_id());
      if (inst_type->IsSame(operand_type)) {
        inst->SetOpcode(spv::Op::OpCopyObject);
      } else {
        inst->SetOpcode(spv::Op::OpBitcast);
      }
      inst->SetInOperands({{SPV_OPERAND_TYPE_ID, {operand}}});
      return true;
    }
    return false;
  };
}



static const constexpr spv::Op RedundantBinaryRhs0Ops[] = {
    spv::Op::OpBitwiseOr,
    spv::Op::OpBitwiseXor,
    spv::Op::OpShiftRightLogical,
    spv::Op::OpShiftRightArithmetic,
    spv::Op::OpShiftLeftLogical,
    spv::Op::OpIAdd,
    spv::Op::OpISub};
FoldingRule RedundantBinaryRhs0(spv::Op op) {
  assert(std::find(std::begin(RedundantBinaryRhs0Ops),
                   std::end(RedundantBinaryRhs0Ops),
                   op) != std::end(RedundantBinaryRhs0Ops) &&
         "Wrong opcode.");
  (void)op;
  return RedundantBinaryOpWithZeroOperand(1, 0);
}



static const constexpr spv::Op RedundantBinaryLhs0Ops[] = {
    spv::Op::OpBitwiseOr, spv::Op::OpBitwiseXor, spv::Op::OpIAdd};
FoldingRule RedundantBinaryLhs0(spv::Op op) {
  assert(std::find(std::begin(RedundantBinaryLhs0Ops),
                   std::end(RedundantBinaryLhs0Ops),
                   op) != std::end(RedundantBinaryLhs0Ops) &&
         "Wrong opcode.");
  (void)op;
  return RedundantBinaryOpWithZeroOperand(0, 1);
}



static const constexpr spv::Op RedundantBinaryLhs0To0Ops[] = {
    spv::Op::OpShiftRightLogical,
    spv::Op::OpShiftRightArithmetic,
    spv::Op::OpShiftLeftLogical,
    spv::Op::OpSDiv,
    spv::Op::OpUDiv,
    spv::Op::OpSMod,
    spv::Op::OpUMod};
FoldingRule RedundantBinaryLhs0To0(spv::Op op) {
  assert(std::find(std::begin(RedundantBinaryLhs0To0Ops),
                   std::end(RedundantBinaryLhs0To0Ops),
                   op) != std::end(RedundantBinaryLhs0To0Ops) &&
         "Wrong opcode.");
  (void)op;
  return RedundantBinaryOpWithZeroOperand(0, 0);
}

FoldingRule ReassociateCommutiveOp() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    const analysis::Type* type =
        context->get_type_mgr()->GetType(inst->type_id());
    uint32_t width = ElementWidth(type);
    if (width != 32) return false;

    analysis::ConstantManager* const_mgr = context->get_constant_mgr();
    const analysis::Constant* const_input1 = ConstInput(constants);
    if (!const_input1) return false;
    Instruction* other_inst = NonConstInput(context, constants[0], inst);

    if (other_inst->opcode() == inst->opcode()) {
      std::vector<const analysis::Constant*> other_constants =
          const_mgr->GetOperandConstants(other_inst);
      const analysis::Constant* const_input2 = ConstInput(other_constants);
      if (!const_input2) return false;

      Instruction* non_const_input =
          NonConstInput(context, other_constants[0], other_inst);
      uint32_t merged_id = PerformOperation(const_mgr, inst->opcode(),
                                            const_input1, const_input2);

      if (merged_id == 0) return false;
      inst->SetInOperands(
          {{SPV_OPERAND_TYPE_ID, {non_const_input->result_id()}},
           {SPV_OPERAND_TYPE_ID, {merged_id}}});
      return true;
    }

    return false;
  };
}





static const constexpr spv::Op ReassociateCommutiveBitwiseOps[] = {
    spv::Op::OpBitwiseOr, spv::Op::OpBitwiseXor, spv::Op::OpBitwiseAnd};
FoldingRule ReassociateCommutiveBitwise(spv::Op op) {
  assert(std::find(std::begin(ReassociateCommutiveBitwiseOps),
                   std::end(ReassociateCommutiveBitwiseOps),
                   op) != std::end(ReassociateCommutiveBitwiseOps) &&
         "Wrong opcode.");
  (void)op;
  return ReassociateCommutiveOp();
}


bool IsAllInt1(const analysis::Constant* c) {
  if (auto composite = c->AsCompositeConstant()) {
    auto& components = composite->GetComponents();
    return std::all_of(std::begin(components), std::end(components), IsAllInt1);
  } else if (c->AsIntConstant()) {
    return c->GetSignExtendedValue() == 1;
  }

  return false;
}


FoldingRule RedundantSUDiv() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(constants.size() == 2);
    assert((inst->opcode() == spv::Op::OpUDiv ||
            inst->opcode() == spv::Op::OpSDiv) &&
           "Wrong opcode.");

    if (constants[1] && IsAllInt1(constants[1])) {
      auto operand = inst->GetSingleWordInOperand(0);
      auto operand_type = constants[1]->type();

      const analysis::Type* inst_type =
          context->get_type_mgr()->GetType(inst->type_id());
      if (inst_type->IsSame(operand_type)) {
        inst->SetOpcode(spv::Op::OpCopyObject);
      } else {
        inst->SetOpcode(spv::Op::OpBitcast);
      }
      inst->SetInOperands({{SPV_OPERAND_TYPE_ID, {operand}}});
      return true;
    }
    return false;
  };
}


FoldingRule RedundantSUMod() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(constants.size() == 2);
    assert((inst->opcode() == spv::Op::OpUMod ||
            inst->opcode() == spv::Op::OpSMod) &&
           "Wrong opcode.");

    if (constants[1] && IsAllInt1(constants[1])) {
      auto type = context->get_type_mgr()->GetType(inst->type_id());
      auto zero_id = context->get_constant_mgr()->GetNullConstId(type);

      inst->SetOpcode(spv::Op::OpCopyObject);
      inst->SetInOperands({{SPV_OPERAND_TYPE_ID, {zero_id}}});
      return true;
    }
    return false;
  };
}




template <typename Callback>
void ForEachIntegerConstantPair(analysis::ConstantManager* const_mgr,
                                const analysis::Constant* input1,
                                const analysis::Constant* input2,
                                Callback&& callback) {
  assert(input1 && input2);

  auto Dispatch = [&callback](const analysis::Constant* lhs,
                              const analysis::Constant* rhs) {
    assert(lhs->type()->AsInteger());
    const analysis::Integer* type = lhs->type()->AsInteger();
    uint32_t width = type->AsInteger()->width();
    assert(width == 32 || width == 64);
    if (width == 32) {
      callback(lhs->GetU32(), rhs->GetU32());
    } else {
      callback(lhs->GetU64(), rhs->GetU64());
    }
  };

  const analysis::Type* type = input1->type();
  if (const analysis::Vector* vector_type = type->AsVector()) {
    const analysis::Type* ele_type = vector_type->element_type();
    assert(ele_type->AsInteger());
    for (uint32_t i = 0; i != vector_type->element_count(); ++i) {
      const analysis::Constant* input1_comp = nullptr;
      if (const analysis::VectorConstant* input1_vector =
              input1->AsVectorConstant()) {
        input1_comp = input1_vector->GetComponents()[i];
      } else {
        assert(input1->AsNullConstant());
        input1_comp = const_mgr->GetConstant(ele_type, {});
      }

      const analysis::Constant* input2_comp = nullptr;
      if (const analysis::VectorConstant* input2_vector =
              input2->AsVectorConstant()) {
        input2_comp = input2_vector->GetComponents()[i];
      } else {
        assert(input2->AsNullConstant());
        input2_comp = const_mgr->GetConstant(ele_type, {});
      }

      assert(ele_type->AsInteger());
      Dispatch(input1_comp, input2_comp);
    }

  } else {
    assert(type->AsInteger());
    Dispatch(input1, input2);
  }
}






FoldingRule RedundantAndOrXor() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(inst->opcode() == spv::Op::OpBitwiseAnd && "Wrong opcode.");
    const analysis::Type* type =
        context->get_type_mgr()->GetType(inst->type_id());
    uint32_t width = ElementWidth(type);
    if ((width != 32) && (width != 64)) return false;

    analysis::ConstantManager* const_mgr = context->get_constant_mgr();
    const analysis::Constant* const_input1 = ConstInput(constants);
    if (!const_input1) return false;
    Instruction* other_inst = NonConstInput(context, constants[0], inst);

    if (other_inst->opcode() == spv::Op::OpBitwiseOr ||
        other_inst->opcode() == spv::Op::OpBitwiseXor) {
      std::vector<const analysis::Constant*> other_constants =
          const_mgr->GetOperandConstants(other_inst);
      const analysis::Constant* const_input2 = ConstInput(other_constants);
      if (!const_input2) return false;

      bool can_convert_to_const = other_inst->opcode() == spv::Op::OpBitwiseOr;
      bool can_remove_inner = true;

      ForEachIntegerConstantPair(
          const_mgr, const_input1, const_input2,
          [&can_remove_inner, &can_convert_to_const](auto lhs, auto rhs) {

            can_convert_to_const = can_convert_to_const && ((lhs & rhs) == lhs);

            can_remove_inner = can_remove_inner && ((lhs & rhs) == 0);
          });

      if (can_convert_to_const) {
        Instruction* const_inst =
            const_mgr->GetDefiningInstruction(const_input1);
        inst->SetOpcode(spv::Op::OpCopyObject);
        inst->SetInOperands({{SPV_OPERAND_TYPE_ID, {const_inst->result_id()}}});
        return true;
      } else if (can_remove_inner) {
        Instruction* non_const_input =
            NonConstInput(context, other_constants[0], other_inst);
        Instruction* const_inst =
            const_mgr->GetDefiningInstruction(const_input1);
        inst->SetInOperands(
            {{SPV_OPERAND_TYPE_ID, {non_const_input->result_id()}},
             {SPV_OPERAND_TYPE_ID, {const_inst->result_id()}}});
        return true;
      }
    }
    return false;
  };
}





FoldingRule RedundantAndAddSub() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(inst->opcode() == spv::Op::OpBitwiseAnd && "Wrong opcode.");
    const analysis::Type* type =
        context->get_type_mgr()->GetType(inst->type_id());
    uint32_t width = ElementWidth(type);
    if ((width != 32) && (width != 64)) return false;

    analysis::ConstantManager* const_mgr = context->get_constant_mgr();
    const analysis::Constant* const_input1 = ConstInput(constants);
    if (!const_input1) return false;
    Instruction* other_inst = NonConstInput(context, constants[0], inst);

    if (other_inst->opcode() != spv::Op::OpIAdd &&
        other_inst->opcode() != spv::Op::OpISub) {
      return false;
    }
    std::vector<const analysis::Constant*> other_constants =
        const_mgr->GetOperandConstants(other_inst);
    const analysis::Constant* const_input2 = ConstInput(other_constants);
    if (!const_input2) return false;


    if ((other_inst->opcode() == spv::Op::OpISub) && other_constants[0]) {
      return false;
    }

    bool can_remove_inner = true;
    ForEachIntegerConstantPair(const_mgr, const_input1, const_input2,
                               [&can_remove_inner](auto and_op, auto add_op) {
                                 if (can_remove_inner) {


                                   can_remove_inner =
                                       utils::LSB(add_op) > and_op;
                                 }
                               });

    if (can_remove_inner) {
      Instruction* non_const_input =
          NonConstInput(context, other_constants[0], other_inst);
      Instruction* const_inst = const_mgr->GetDefiningInstruction(const_input1);
      inst->SetInOperands(
          {{SPV_OPERAND_TYPE_ID, {non_const_input->result_id()}},
           {SPV_OPERAND_TYPE_ID, {const_inst->result_id()}}});
      return true;
    }
    return false;
  };
}





FoldingRule RedundantAndShift() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(inst->opcode() == spv::Op::OpBitwiseAnd && "Wrong opcode.");
    const analysis::Type* type =
        context->get_type_mgr()->GetType(inst->type_id());
    uint32_t width = ElementWidth(type);
    if (width != 8 && width != 16 && width != 32 && width != 64) return false;
    const uint64_t width_mask =
        (width == 64) ? ~0ull : ((1ull << width) - 1ull);

    analysis::ConstantManager* const_mgr = context->get_constant_mgr();
    const analysis::Constant* const_input1 = ConstInput(constants);
    if (!const_input1) return false;
    Instruction* other_inst = NonConstInput(context, constants[0], inst);

    spv::Op other_op = other_inst->opcode();
    if (other_op != spv::Op::OpShiftLeftLogical &&
        other_op != spv::Op::OpShiftRightLogical) {
      return false;
    }

    std::vector<const analysis::Constant*> other_constants =
        const_mgr->GetOperandConstants(other_inst);


    if (other_constants[0]) return false;
    const analysis::Constant* const_input2 = other_constants[1];
    if (!const_input2) return false;

    auto get_value_u64 =
        [](const analysis::Constant* c) -> std::optional<uint64_t> {
      if (!c) return std::nullopt;
      const analysis::Integer* int_t = c->type()->AsInteger();
      if (!int_t) return std::nullopt;
      return c->GetZeroExtendedValue();
    };

    auto can_fold_component =
        [&](const analysis::Constant* mask_const,
            const analysis::Constant* shift_const) -> std::optional<bool> {
      auto lhs = get_value_u64(mask_const);
      auto rhs = get_value_u64(shift_const);
      if (!lhs || !rhs) return std::nullopt;
      if (*rhs >= width) return false;
      uint64_t lhs_masked = *lhs & width_mask;
      if (other_op == spv::Op::OpShiftRightLogical) {
        return ((lhs_masked << *rhs) & width_mask) == 0;
      }
      return ((lhs_masked >> *rhs) & width_mask) == 0;
    };

    if (const analysis::Vector* mask_vec = type->AsVector()) {
      const analysis::Vector* shift_vec = const_input2->type()->AsVector();
      if (!shift_vec ||
          shift_vec->element_count() != mask_vec->element_count()) {
        return false;
      }
      const auto mask_components = const_input1->GetVectorComponents(const_mgr);
      const auto shift_components =
          const_input2->GetVectorComponents(const_mgr);
      for (uint32_t i = 0; i != mask_vec->element_count(); ++i) {
        auto result =
            can_fold_component(mask_components[i], shift_components[i]);
        if (!result || !*result) return false;
      }
    } else {
      if (const_input2->type()->AsVector()) return false;
      auto result = can_fold_component(const_input1, const_input2);
      if (!result || !*result) return false;
    }

    auto zero_id = context->get_constant_mgr()->GetNullConstId(type);
    inst->SetOpcode(spv::Op::OpCopyObject);
    inst->SetInOperands({{SPV_OPERAND_TYPE_ID, {zero_id}}});
    return true;
  };
}



FoldingRule DotProductDoingExtract() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    assert(inst->opcode() == spv::Op::OpDot &&
           "Wrong opcode.  Should be OpDot.");

    analysis::ConstantManager* const_mgr = context->get_constant_mgr();

    if (!inst->IsFloatingPointFoldingAllowed()) {
      return false;
    }

    for (int i = 0; i < 2; ++i) {
      if (!constants[i]) {
        continue;
      }

      const analysis::Vector* vector_type = constants[i]->type()->AsVector();
      assert(vector_type && "Inputs to OpDot must be vectors.");
      const analysis::Float* element_type =
          vector_type->element_type()->AsFloat();
      assert(element_type && "Inputs to OpDot must be vectors of floats.");
      uint32_t element_width = element_type->width();
      if (element_width != 32 && element_width != 64) {
        return false;
      }

      std::vector<const analysis::Constant*> components;
      components = constants[i]->GetVectorComponents(const_mgr);

      constexpr uint32_t kNotFound = std::numeric_limits<uint32_t>::max();

      uint32_t component_with_one = kNotFound;
      bool all_others_zero = true;
      for (uint32_t j = 0; j < components.size(); ++j) {
        const analysis::Constant* element = components[j];
        double value =
            (element_width == 32 ? element->GetFloat() : element->GetDouble());
        if (value == 0.0) {
          continue;
        } else if (value == 1.0) {
          if (component_with_one == kNotFound) {
            component_with_one = j;
          } else {
            component_with_one = kNotFound;
            break;
          }
        } else {
          all_others_zero = false;
          break;
        }
      }

      if (!all_others_zero || component_with_one == kNotFound) {
        continue;
      }

      std::vector<Operand> operands;
      operands.push_back(
          {SPV_OPERAND_TYPE_ID, {inst->GetSingleWordInOperand(1u - i)}});
      operands.push_back(
          {SPV_OPERAND_TYPE_LITERAL_INTEGER, {component_with_one}});

      inst->SetOpcode(spv::Op::OpCompositeExtract);
      inst->SetInOperands(std::move(operands));
      return true;
    }
    return false;
  };
}





FoldingRule StoringUndef() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>&) {
    assert(inst->opcode() == spv::Op::OpStore &&
           "Wrong opcode.  Should be OpStore.");

    analysis::DefUseManager* def_use_mgr = context->get_def_use_mgr();


    if (inst->NumInOperands() == 3) {
      if (inst->GetSingleWordInOperand(2) &
          uint32_t(spv::MemoryAccessMask::Volatile)) {
        return false;
      }
    }

    uint32_t object_id = inst->GetSingleWordInOperand(kStoreObjectInIdx);
    Instruction* object_inst = def_use_mgr->GetDef(object_id);
    if (object_inst->opcode() == spv::Op::OpUndef) {
      inst->ToNop();
      return true;
    }
    return false;
  };
}

FoldingRule VectorShuffleFeedingShuffle() {
  return [](IRContext* context, Instruction* inst,
            const std::vector<const analysis::Constant*>&) {
    assert(inst->opcode() == spv::Op::OpVectorShuffle &&
           "Wrong opcode.  Should be OpVectorShuffle.");

    analysis::DefUseManager* def_use_mgr = context->get_def_use_mgr();
    analysis::TypeManager* type_mgr = context->get_type_mgr();

    Instruction* feeding_shuffle_inst =
        def_use_mgr->GetDef(inst->GetSingleWordInOperand(0));
    analysis::Vector* op0_type =
        type_mgr->GetType(feeding_shuffle_inst->type_id())->AsVector();
    uint32_t op0_length = op0_type->element_count();

    bool feeder_is_op0 = true;
    if (feeding_shuffle_inst->opcode() != spv::Op::OpVectorShuffle) {
      feeding_shuffle_inst =
          def_use_mgr->GetDef(inst->GetSingleWordInOperand(1));
      feeder_is_op0 = false;
    }

    if (feeding_shuffle_inst->opcode() != spv::Op::OpVectorShuffle) {
      return false;
    }

    Instruction* feeder2 =
        def_use_mgr->GetDef(feeding_shuffle_inst->GetSingleWordInOperand(0));
    analysis::Vector* feeder_op0_type =
        type_mgr->GetType(feeder2->type_id())->AsVector();
    uint32_t feeder_op0_length = feeder_op0_type->element_count();

    uint32_t new_feeder_id = 0;
    std::vector<Operand> new_operands;
    new_operands.resize(
        2, {SPV_OPERAND_TYPE_ID, {0}});
    const uint32_t undef_literal = 0xffffffff;
    for (uint32_t op = 2; op < inst->NumInOperands(); ++op) {
      uint32_t component_index = inst->GetSingleWordInOperand(op);


      if (component_index != undef_literal &&
          feeder_is_op0 == (component_index < op0_length)) {





        if (component_index >= op0_length) {
          component_index -= op0_length;
        }
        component_index =
            feeding_shuffle_inst->GetSingleWordInOperand(component_index + 2);



        if (component_index < feeder_op0_length) {
          if (new_feeder_id == 0) {


            new_feeder_id = feeding_shuffle_inst->GetSingleWordInOperand(0);
          } else if (new_feeder_id !=
                     feeding_shuffle_inst->GetSingleWordInOperand(0)) {


            return false;
          }
        } else if (component_index != undef_literal) {
          if (new_feeder_id == 0) {


            new_feeder_id = feeding_shuffle_inst->GetSingleWordInOperand(1);
          } else if (new_feeder_id !=
                     feeding_shuffle_inst->GetSingleWordInOperand(1)) {


            return false;
          }
          component_index -= feeder_op0_length;
        }

        if (!feeder_is_op0 && component_index != undef_literal) {
          component_index += op0_length;
        }
      }
      new_operands.push_back(
          {SPV_OPERAND_TYPE_LITERAL_INTEGER, {component_index}});
    }

    if (new_feeder_id == 0) {
      analysis::ConstantManager* const_mgr = context->get_constant_mgr();
      const analysis::Type* type =
          type_mgr->GetType(feeding_shuffle_inst->type_id());
      const analysis::Constant* null_const = const_mgr->GetConstant(type, {});
      new_feeder_id =
          const_mgr->GetDefiningInstruction(null_const, 0)->result_id();
    }

    if (feeder_is_op0) {


      Instruction* new_feeder_inst = def_use_mgr->GetDef(new_feeder_id);
      analysis::Type* new_feeder_type =
          type_mgr->GetType(new_feeder_inst->type_id());
      uint32_t new_op0_size = new_feeder_type->AsVector()->element_count();
      int32_t adjustment = op0_length - new_op0_size;

      if (adjustment != 0) {
        for (uint32_t i = 2; i < new_operands.size(); i++) {
          uint32_t operand = inst->GetSingleWordInOperand(i);
          if (operand >= op0_length && operand != undef_literal) {
            new_operands[i].words[0] -= adjustment;
          }
        }
      }

      new_operands[0].words[0] = new_feeder_id;
      new_operands[1] = inst->GetInOperand(1);
    } else {
      new_operands[1].words[0] = new_feeder_id;
      new_operands[0] = inst->GetInOperand(0);
    }

    inst->SetInOperands(std::move(new_operands));
    return true;
  };
}



FoldingRule RemoveRedundantOperands() {
  return [](IRContext*, Instruction* inst,
            const std::vector<const analysis::Constant*>&) {
    assert(inst->opcode() == spv::Op::OpEntryPoint &&
           "Wrong opcode.  Should be OpEntryPoint.");
    bool has_redundant_operand = false;
    std::unordered_set<uint32_t> seen_operands;
    std::vector<Operand> new_operands;

    new_operands.emplace_back(inst->GetOperand(0));
    new_operands.emplace_back(inst->GetOperand(1));
    new_operands.emplace_back(inst->GetOperand(2));
    for (uint32_t i = 3; i < inst->NumOperands(); ++i) {
      if (seen_operands.insert(inst->GetSingleWordOperand(i)).second) {
        new_operands.emplace_back(inst->GetOperand(i));
      } else {
        has_redundant_operand = true;
      }
    }

    if (!has_redundant_operand) {
      return false;
    }

    inst->SetInOperands(std::move(new_operands));
    return true;
  };
}



FoldingRule UpdateImageOperands() {
  return [](IRContext*, Instruction* inst,
            const std::vector<const analysis::Constant*>& constants) {
    const auto opcode = inst->opcode();
    (void)opcode;
    assert((opcode == spv::Op::OpImageSampleImplicitLod ||
            opcode == spv::Op::OpImageSampleExplicitLod ||
            opcode == spv::Op::OpImageSampleDrefImplicitLod ||
            opcode == spv::Op::OpImageSampleDrefExplicitLod ||
            opcode == spv::Op::OpImageSampleProjImplicitLod ||
            opcode == spv::Op::OpImageSampleProjExplicitLod ||
            opcode == spv::Op::OpImageSampleProjDrefImplicitLod ||
            opcode == spv::Op::OpImageSampleProjDrefExplicitLod ||
            opcode == spv::Op::OpImageFetch ||
            opcode == spv::Op::OpImageGather ||
            opcode == spv::Op::OpImageDrefGather ||
            opcode == spv::Op::OpImageRead || opcode == spv::Op::OpImageWrite ||
            opcode == spv::Op::OpImageSparseSampleImplicitLod ||
            opcode == spv::Op::OpImageSparseSampleExplicitLod ||
            opcode == spv::Op::OpImageSparseSampleDrefImplicitLod ||
            opcode == spv::Op::OpImageSparseSampleDrefExplicitLod ||
            opcode == spv::Op::OpImageSparseSampleProjImplicitLod ||
            opcode == spv::Op::OpImageSparseSampleProjExplicitLod ||
            opcode == spv::Op::OpImageSparseSampleProjDrefImplicitLod ||
            opcode == spv::Op::OpImageSparseSampleProjDrefExplicitLod ||
            opcode == spv::Op::OpImageSparseFetch ||
            opcode == spv::Op::OpImageSparseGather ||
            opcode == spv::Op::OpImageSparseDrefGather ||
            opcode == spv::Op::OpImageSparseRead) &&
           "Wrong opcode.  Should be an image instruction.");

    int32_t operand_index = ImageOperandsMaskInOperandIndex(inst);
    if (operand_index >= 0) {
      auto image_operands = inst->GetSingleWordInOperand(operand_index);
      if (image_operands & uint32_t(spv::ImageOperandsMask::Offset)) {
        uint32_t offset_operand_index = operand_index + 1;
        if (image_operands & uint32_t(spv::ImageOperandsMask::Bias))
          offset_operand_index++;
        if (image_operands & uint32_t(spv::ImageOperandsMask::Lod))
          offset_operand_index++;
        if (image_operands & uint32_t(spv::ImageOperandsMask::Grad))
          offset_operand_index += 2;
        assert(((image_operands &
                 uint32_t(spv::ImageOperandsMask::ConstOffset)) == 0) &&
               "Offset and ConstOffset may not be used together");
        if (offset_operand_index < inst->NumOperands()) {
          if (constants[offset_operand_index]) {
            if (constants[offset_operand_index]->IsZero()) {
              inst->RemoveInOperand(offset_operand_index);
            } else {
              image_operands = image_operands |
                               uint32_t(spv::ImageOperandsMask::ConstOffset);
            }
            image_operands =
                image_operands & ~uint32_t(spv::ImageOperandsMask::Offset);
            inst->SetInOperand(operand_index, {image_operands});
            return true;
          }
        }
      }
    }

    return false;
  };
}

}

void FoldingRules::AddFoldingRules() {




  for (auto op : RedundantBinaryRhs0Ops)
    rules_[op].push_back(RedundantBinaryRhs0(op));
  for (auto op : RedundantBinaryLhs0Ops)
    rules_[op].push_back(RedundantBinaryLhs0(op));
  for (auto op : RedundantBinaryLhs0To0Ops)
    rules_[op].push_back(RedundantBinaryLhs0To0(op));
  for (auto op : ReassociateCommutiveBitwiseOps)
    rules_[op].push_back(ReassociateCommutiveBitwise(op));
  for (auto op : ReassociateNestedGenericIntOps)
    rules_[op].push_back(ReassociateNestedGenericInt(op));
  for (auto op : MergeBinaryOpSelectOps)
    rules_[op].push_back(MergeBinaryOpSelect(op));
  rules_[spv::Op::OpSDiv].push_back(RedundantSUDiv());
  rules_[spv::Op::OpUDiv].push_back(RedundantSUDiv());
  rules_[spv::Op::OpSMod].push_back(RedundantSUMod());
  rules_[spv::Op::OpUMod].push_back(RedundantSUMod());

  rules_[spv::Op::OpBitcast].push_back(BitCastScalarOrVector());
  rules_[spv::Op::OpBitcast].push_back(RedundantBitcast());

  rules_[spv::Op::OpBitReverse].push_back(BitReverseScalarOrVector());

  rules_[spv::Op::OpCompositeConstruct].push_back(
      CompositeExtractFeedingConstruct);

  rules_[spv::Op::OpCompositeExtract].push_back(InsertFeedingExtract());
  rules_[spv::Op::OpCompositeExtract].push_back(
      CompositeConstructFeedingExtract);
  rules_[spv::Op::OpCompositeExtract].push_back(VectorShuffleFeedingExtract());
  rules_[spv::Op::OpCompositeExtract].push_back(FMixFeedingExtract());
  rules_[spv::Op::OpCompositeExtract].push_back(CopyLogicalFeedingExtract);
  rules_[spv::Op::OpCompositeExtract].push_back(LoadFeedingExtract);

  rules_[spv::Op::OpCompositeInsert].push_back(
      CompositeInsertToCompositeConstruct);

  rules_[spv::Op::OpDot].push_back(DotProductDoingExtract());

  rules_[spv::Op::OpEntryPoint].push_back(RemoveRedundantOperands());

  rules_[spv::Op::OpFAdd].push_back(RedundantFAdd());
  rules_[spv::Op::OpFAdd].push_back(MergeAddNegateArithmetic());
  rules_[spv::Op::OpFAdd].push_back(MergeAddAddArithmetic());
  rules_[spv::Op::OpFAdd].push_back(MergeAddSubArithmetic());
  rules_[spv::Op::OpFAdd].push_back(MergeGenericAddSubArithmetic());
  rules_[spv::Op::OpFAdd].push_back(ReassociateNestedAddSub());
  rules_[spv::Op::OpFAdd].push_back(FactorAddSubMuls());

  rules_[spv::Op::OpFDiv].push_back(RedundantFDiv());
  rules_[spv::Op::OpFDiv].push_back(ReciprocalFDiv());
  rules_[spv::Op::OpFDiv].push_back(MergeDivDivArithmetic());
  rules_[spv::Op::OpFDiv].push_back(MergeDivMulArithmetic());
  rules_[spv::Op::OpFDiv].push_back(MergeDivNegateArithmetic());
  rules_[spv::Op::OpFDiv].push_back(MergeDivMulDoubleNegative());
  rules_[spv::Op::OpFDiv].push_back(ReassociateNestedMulDivFloat());

  rules_[spv::Op::OpFMod].push_back(RedundantFMod());

  rules_[spv::Op::OpFMul].push_back(RedundantFMul());
  rules_[spv::Op::OpFMul].push_back(MergeMulMulArithmetic());
  rules_[spv::Op::OpFMul].push_back(MergeMulDivArithmetic());
  rules_[spv::Op::OpFMul].push_back(MergeMulNegateArithmetic());
  rules_[spv::Op::OpFMul].push_back(MergeDivMulDoubleNegative());
  rules_[spv::Op::OpFMul].push_back(ReassociateNestedMulDivFloat());

  rules_[spv::Op::OpVectorTimesScalar].push_back(MergeDivMulDoubleNegative());

  rules_[spv::Op::OpFNegate].push_back(MergeNegateArithmetic());
  rules_[spv::Op::OpFNegate].push_back(MergeNegateAddSubArithmetic());
  rules_[spv::Op::OpFNegate].push_back(MergeNegateMulDivArithmetic());

  rules_[spv::Op::OpFSub].push_back(RedundantFSub());
  rules_[spv::Op::OpFSub].push_back(MergeSubNegateArithmetic());
  rules_[spv::Op::OpFSub].push_back(MergeSubAddArithmetic());
  rules_[spv::Op::OpFSub].push_back(MergeSubSubArithmetic());
  rules_[spv::Op::OpFSub].push_back(ReassociateNestedAddSub());
  rules_[spv::Op::OpFSub].push_back(FactorAddSubMuls());

  rules_[spv::Op::OpIAdd].push_back(MergeAddNegateArithmetic());
  rules_[spv::Op::OpIAdd].push_back(MergeAddAddArithmetic());
  rules_[spv::Op::OpIAdd].push_back(MergeAddSubArithmetic());
  rules_[spv::Op::OpIAdd].push_back(MergeGenericAddSubArithmetic());
  rules_[spv::Op::OpIAdd].push_back(ReassociateNestedAddSub());
  rules_[spv::Op::OpIAdd].push_back(FactorAddSubMuls());

  rules_[spv::Op::OpSDiv].push_back(MergeDivMulDoubleNegative());

  rules_[spv::Op::OpIMul].push_back(IntMultipleBy1());
  rules_[spv::Op::OpIMul].push_back(MergeMulMulArithmetic());
  rules_[spv::Op::OpIMul].push_back(MergeMulNegateArithmetic());
  rules_[spv::Op::OpIMul].push_back(MergeDivMulDoubleNegative());

  rules_[spv::Op::OpISub].push_back(MergeSubNegateArithmetic());
  rules_[spv::Op::OpISub].push_back(MergeSubAddArithmetic());
  rules_[spv::Op::OpISub].push_back(MergeSubSubArithmetic());
  rules_[spv::Op::OpISub].push_back(ReassociateNestedAddSub());
  rules_[spv::Op::OpISub].push_back(FactorAddSubMuls());

  rules_[spv::Op::OpBitwiseAnd].push_back(RedundantAndOrXor());
  rules_[spv::Op::OpBitwiseAnd].push_back(RedundantAndAddSub());
  rules_[spv::Op::OpBitwiseAnd].push_back(RedundantAndShift());

  rules_[spv::Op::OpPhi].push_back(RedundantPhi());

  rules_[spv::Op::OpSNegate].push_back(MergeNegateArithmetic());
  rules_[spv::Op::OpSNegate].push_back(MergeNegateMulDivArithmetic());
  rules_[spv::Op::OpSNegate].push_back(MergeNegateAddSubArithmetic());

  rules_[spv::Op::OpSelect].push_back(RedundantSelect());
  rules_[spv::Op::OpSelect].push_back(FoldConstantBooleanSelect());

  rules_[spv::Op::OpLogicalAnd].push_back(RedundantLogicalAnd());

  rules_[spv::Op::OpLogicalOr].push_back(RedundantLogicalOr());

  rules_[spv::Op::OpLogicalNot].push_back(RedundantLogicalNot());
  rules_[spv::Op::OpLogicalNot].push_back(FoldLogicalNotComparison());

  rules_[spv::Op::OpLogicalEqual].push_back(RedundantLogicalEqual());
  rules_[spv::Op::OpLogicalNotEqual].push_back(RedundantLogicalEqual());

  rules_[spv::Op::OpStore].push_back(StoringUndef());

  rules_[spv::Op::OpVectorShuffle].push_back(VectorShuffleFeedingShuffle());

  rules_[spv::Op::OpImageSampleImplicitLod].push_back(UpdateImageOperands());
  rules_[spv::Op::OpImageSampleExplicitLod].push_back(UpdateImageOperands());
  rules_[spv::Op::OpImageSampleDrefImplicitLod].push_back(
      UpdateImageOperands());
  rules_[spv::Op::OpImageSampleDrefExplicitLod].push_back(
      UpdateImageOperands());
  rules_[spv::Op::OpImageSampleProjImplicitLod].push_back(
      UpdateImageOperands());
  rules_[spv::Op::OpImageSampleProjExplicitLod].push_back(
      UpdateImageOperands());
  rules_[spv::Op::OpImageSampleProjDrefImplicitLod].push_back(
      UpdateImageOperands());
  rules_[spv::Op::OpImageSampleProjDrefExplicitLod].push_back(
      UpdateImageOperands());
  rules_[spv::Op::OpImageFetch].push_back(UpdateImageOperands());
  rules_[spv::Op::OpImageGather].push_back(UpdateImageOperands());
  rules_[spv::Op::OpImageDrefGather].push_back(UpdateImageOperands());
  rules_[spv::Op::OpImageRead].push_back(UpdateImageOperands());
  rules_[spv::Op::OpImageWrite].push_back(UpdateImageOperands());
  rules_[spv::Op::OpImageSparseSampleImplicitLod].push_back(
      UpdateImageOperands());
  rules_[spv::Op::OpImageSparseSampleExplicitLod].push_back(
      UpdateImageOperands());
  rules_[spv::Op::OpImageSparseSampleDrefImplicitLod].push_back(
      UpdateImageOperands());
  rules_[spv::Op::OpImageSparseSampleDrefExplicitLod].push_back(
      UpdateImageOperands());
  rules_[spv::Op::OpImageSparseSampleProjImplicitLod].push_back(
      UpdateImageOperands());
  rules_[spv::Op::OpImageSparseSampleProjExplicitLod].push_back(
      UpdateImageOperands());
  rules_[spv::Op::OpImageSparseSampleProjDrefImplicitLod].push_back(
      UpdateImageOperands());
  rules_[spv::Op::OpImageSparseSampleProjDrefExplicitLod].push_back(
      UpdateImageOperands());
  rules_[spv::Op::OpImageSparseFetch].push_back(UpdateImageOperands());
  rules_[spv::Op::OpImageSparseGather].push_back(UpdateImageOperands());
  rules_[spv::Op::OpImageSparseDrefGather].push_back(UpdateImageOperands());
  rules_[spv::Op::OpImageSparseRead].push_back(UpdateImageOperands());

  FeatureManager* feature_manager = context_->get_feature_mgr();

  uint32_t ext_inst_glslstd450_id =
      feature_manager->GetExtInstImportId_GLSLstd450();
  if (ext_inst_glslstd450_id != 0) {
    ext_rules_[{ext_inst_glslstd450_id, GLSLstd450FMix}].push_back(
        RedundantFMix());
  }
}
}
}
