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

#include "source/opt/scalar_analysis.h"

#include <functional>
#include <string>
#include <utility>

#include "source/opt/ir_context.h"























namespace spvtools {
namespace opt {

uint32_t SENode::NumberOfNodes = 0;

ScalarEvolutionAnalysis::ScalarEvolutionAnalysis(IRContext* context)
    : context_(context), pretend_equal_{} {

  cached_cant_compute_ =
      GetCachedOrAdd(std::unique_ptr<SECantCompute>(new SECantCompute(this)));
}

SENode* ScalarEvolutionAnalysis::CreateNegation(SENode* operand) {

  if (operand->IsCantCompute()) return CreateCantComputeNode();

  if (operand->GetType() == SENode::Constant) {
    return CreateConstant(-operand->AsSEConstantNode()->FoldToSingleValue());
  }
  std::unique_ptr<SENode> negation_node{new SENegative(this)};
  negation_node->AddChild(operand);
  return GetCachedOrAdd(std::move(negation_node));
}

SENode* ScalarEvolutionAnalysis::CreateConstant(int64_t integer) {
  return GetCachedOrAdd(
      std::unique_ptr<SENode>(new SEConstantNode(this, integer)));
}

SENode* ScalarEvolutionAnalysis::CreateRecurrentExpression(
    const Loop* loop, SENode* offset, SENode* coefficient) {
  assert(loop && "Recurrent add expressions must have a valid loop.");


  if (offset->IsCantCompute() || coefficient->IsCantCompute())
    return CreateCantComputeNode();

  const Loop* loop_to_use = nullptr;
  if (pretend_equal_[loop]) {
    loop_to_use = pretend_equal_[loop];
  } else {
    loop_to_use = loop;
  }

  std::unique_ptr<SERecurrentNode> phi_node{
      new SERecurrentNode(this, loop_to_use)};
  phi_node->AddOffset(offset);
  phi_node->AddCoefficient(coefficient);

  return GetCachedOrAdd(std::move(phi_node));
}

SENode* ScalarEvolutionAnalysis::AnalyzeMultiplyOp(
    const Instruction* multiply) {
  assert(multiply->opcode() == spv::Op::OpIMul &&
         "Multiply node did not come from a multiply instruction");
  analysis::DefUseManager* def_use = context_->get_def_use_mgr();

  SENode* op1 =
      AnalyzeInstruction(def_use->GetDef(multiply->GetSingleWordInOperand(0)));
  SENode* op2 =
      AnalyzeInstruction(def_use->GetDef(multiply->GetSingleWordInOperand(1)));

  return CreateMultiplyNode(op1, op2);
}

SENode* ScalarEvolutionAnalysis::CreateMultiplyNode(SENode* operand_1,
                                                    SENode* operand_2) {

  if (operand_1->IsCantCompute() || operand_2->IsCantCompute())
    return CreateCantComputeNode();

  if (operand_1->GetType() == SENode::Constant &&
      operand_2->GetType() == SENode::Constant) {
    return CreateConstant(operand_1->AsSEConstantNode()->FoldToSingleValue() *
                          operand_2->AsSEConstantNode()->FoldToSingleValue());
  }

  std::unique_ptr<SENode> multiply_node{new SEMultiplyNode(this)};

  multiply_node->AddChild(operand_1);
  multiply_node->AddChild(operand_2);

  return GetCachedOrAdd(std::move(multiply_node));
}

SENode* ScalarEvolutionAnalysis::CreateSubtraction(SENode* operand_1,
                                                   SENode* operand_2) {

  if (operand_1->GetType() == SENode::Constant &&
      operand_2->GetType() == SENode::Constant) {
    return CreateConstant(operand_1->AsSEConstantNode()->FoldToSingleValue() -
                          operand_2->AsSEConstantNode()->FoldToSingleValue());
  }

  return CreateAddNode(operand_1, CreateNegation(operand_2));
}

SENode* ScalarEvolutionAnalysis::CreateAddNode(SENode* operand_1,
                                               SENode* operand_2) {

  if (operand_1->GetType() == SENode::Constant &&
      operand_2->GetType() == SENode::Constant) {
    return CreateConstant(operand_1->AsSEConstantNode()->FoldToSingleValue() +
                          operand_2->AsSEConstantNode()->FoldToSingleValue());
  }


  if (operand_1->IsCantCompute() || operand_2->IsCantCompute())
    return CreateCantComputeNode();

  std::unique_ptr<SENode> add_node{new SEAddNode(this)};

  add_node->AddChild(operand_1);
  add_node->AddChild(operand_2);

  return GetCachedOrAdd(std::move(add_node));
}

SENode* ScalarEvolutionAnalysis::AnalyzeInstruction(const Instruction* inst) {
  auto itr = recurrent_node_map_.find(inst);
  if (itr != recurrent_node_map_.end()) return itr->second;

  SENode* output = nullptr;
  switch (inst->opcode()) {
    case spv::Op::OpPhi: {
      output = AnalyzePhiInstruction(inst);
      break;
    }
    case spv::Op::OpConstant:
    case spv::Op::OpConstantNull: {
      output = AnalyzeConstant(inst);
      break;
    }
    case spv::Op::OpISub:
    case spv::Op::OpIAdd: {
      output = AnalyzeAddOp(inst);
      break;
    }
    case spv::Op::OpIMul: {
      output = AnalyzeMultiplyOp(inst);
      break;
    }
    default: {
      output = CreateValueUnknownNode(inst);
      break;
    }
  }

  return output;
}

SENode* ScalarEvolutionAnalysis::AnalyzeConstant(const Instruction* inst) {
  if (inst->opcode() == spv::Op::OpConstantNull) return CreateConstant(0);

  assert(inst->opcode() == spv::Op::OpConstant);
  assert(inst->NumInOperands() == 1);
  int64_t value = 0;


  const analysis::Constant* constant =
      context_->get_constant_mgr()->FindDeclaredConstant(inst->result_id());

  if (!constant) return CreateCantComputeNode();

  const analysis::IntConstant* int_constant = constant->AsIntConstant();


  if (!int_constant || int_constant->words().size() != 1)
    return CreateCantComputeNode();

  if (int_constant->type()->AsInteger()->IsSigned()) {
    value = int_constant->GetS32BitValue();
  } else {
    value = int_constant->GetU32BitValue();
  }

  return CreateConstant(value);
}



SENode* ScalarEvolutionAnalysis::AnalyzeAddOp(const Instruction* inst) {
  assert((inst->opcode() == spv::Op::OpIAdd ||
          inst->opcode() == spv::Op::OpISub) &&
         "Add node must be created from a OpIAdd or OpISub instruction");

  analysis::DefUseManager* def_use = context_->get_def_use_mgr();

  SENode* op1 =
      AnalyzeInstruction(def_use->GetDef(inst->GetSingleWordInOperand(0)));

  SENode* op2 =
      AnalyzeInstruction(def_use->GetDef(inst->GetSingleWordInOperand(1)));


  if (inst->opcode() == spv::Op::OpISub) {
    op2 = CreateNegation(op2);
  }

  return CreateAddNode(op1, op2);
}

SENode* ScalarEvolutionAnalysis::AnalyzePhiInstruction(const Instruction* phi) {

  if (phi->NumInOperands() != 4) {
    return CreateCantComputeNode();
  }

  analysis::DefUseManager* def_use = context_->get_def_use_mgr();


  BasicBlock* basic_block =
      context_->get_instr_block(const_cast<Instruction*>(phi));


  Function* function = basic_block->GetParent();


  LoopDescriptor* loop_descriptor = context_->GetLoopDescriptor(function);


  if (!loop_descriptor) return CreateCantComputeNode();


  Loop* loop = (*loop_descriptor)[basic_block->id()];



  if (!loop || !loop->GetLatchBlock() || !loop->GetPreHeaderBlock() ||
      loop->GetHeaderBlock() != basic_block)
    return recurrent_node_map_[phi] = CreateCantComputeNode();

  const Loop* loop_to_use = nullptr;
  if (pretend_equal_[loop]) {
    loop_to_use = pretend_equal_[loop];
  } else {
    loop_to_use = loop;
  }
  std::unique_ptr<SERecurrentNode> phi_node{
      new SERecurrentNode(this, loop_to_use)};





  recurrent_node_map_[phi] = phi_node.get();


  for (uint32_t i = 0; i < phi->NumInOperands(); i += 2) {
    uint32_t value_id = phi->GetSingleWordInOperand(i);
    uint32_t incoming_label_id = phi->GetSingleWordInOperand(i + 1);

    Instruction* value_inst = def_use->GetDef(value_id);
    SENode* value_node = AnalyzeInstruction(value_inst);


    if (value_node->IsCantCompute())
      return recurrent_node_map_[phi] = CreateCantComputeNode();



    if (incoming_label_id == loop->GetPreHeaderBlock()->id()) {
      phi_node->AddOffset(value_node);
    } else if (incoming_label_id == loop->GetLatchBlock()->id()) {

      if (value_node->GetType() != SENode::Add)
        return recurrent_node_map_[phi] = CreateCantComputeNode();

      SENode* step_node = nullptr;
      SENode* phi_operand = nullptr;
      SENode* operand_1 = value_node->GetChild(0);
      SENode* operand_2 = value_node->GetChild(1);


      if (!operand_1->AsSERecurrentNode())
        step_node = operand_1;
      else if (!operand_2->AsSERecurrentNode())
        step_node = operand_2;


      if (operand_1->AsSERecurrentNode())
        phi_operand = operand_1;
      else if (operand_2->AsSERecurrentNode())
        phi_operand = operand_2;


      if (!(step_node && phi_operand))
        return recurrent_node_map_[phi] = CreateCantComputeNode();


      if (phi_operand != phi_node.get())
        return recurrent_node_map_[phi] = CreateCantComputeNode();

      if (!IsLoopInvariant(loop, step_node))
        return recurrent_node_map_[phi] = CreateCantComputeNode();

      phi_node->AddCoefficient(step_node);
    }
  }



  return recurrent_node_map_[phi] = GetCachedOrAdd(std::move(phi_node));
}

SENode* ScalarEvolutionAnalysis::CreateValueUnknownNode(
    const Instruction* inst) {
  std::unique_ptr<SEValueUnknown> load_node{
      new SEValueUnknown(this, inst->result_id())};
  return GetCachedOrAdd(std::move(load_node));
}

SENode* ScalarEvolutionAnalysis::CreateCantComputeNode() {
  return cached_cant_compute_;
}


SENode* ScalarEvolutionAnalysis::GetCachedOrAdd(
    std::unique_ptr<SENode> prospective_node) {
  auto itr = node_cache_.find(prospective_node);
  if (itr != node_cache_.end()) {
    return (*itr).get();
  }

  SENode* raw_ptr_to_node = prospective_node.get();
  node_cache_.insert(std::move(prospective_node));
  return raw_ptr_to_node;
}

bool ScalarEvolutionAnalysis::IsLoopInvariant(const Loop* loop,
                                              const SENode* node) const {
  for (auto itr = node->graph_cbegin(); itr != node->graph_cend(); ++itr) {
    if (const SERecurrentNode* rec = itr->AsSERecurrentNode()) {
      const BasicBlock* header = rec->GetLoop()->GetHeaderBlock();



      if (loop->IsInsideLoop(header)) {
        return false;
      }
    } else if (const SEValueUnknown* unknown = itr->AsSEValueUnknown()) {


      if (loop->IsInsideLoop(unknown->ResultId())) return false;
    }
  }

  return true;
}

SENode* ScalarEvolutionAnalysis::GetCoefficientFromRecurrentTerm(
    SENode* node, const Loop* loop) {

  for (auto itr = node->graph_begin(); itr != node->graph_end(); ++itr) {
    SERecurrentNode* rec = itr->AsSERecurrentNode();
    if (rec && rec->GetLoop() == loop) {
      return rec->GetCoefficient();
    }
  }
  return CreateConstant(0);
}

SENode* ScalarEvolutionAnalysis::UpdateChildNode(SENode* parent,
                                                 SENode* old_child,
                                                 SENode* new_child) {

  if (parent->GetType() != SENode::Add) return parent;

  std::vector<SENode*> new_children;
  for (SENode* child : *parent) {
    if (child == old_child) {
      new_children.push_back(new_child);
    } else {
      new_children.push_back(child);
    }
  }

  std::unique_ptr<SENode> add_node{new SEAddNode(this)};
  for (SENode* child : new_children) {
    add_node->AddChild(child);
  }

  return SimplifyExpression(GetCachedOrAdd(std::move(add_node)));
}



SENode* ScalarEvolutionAnalysis::BuildGraphWithoutRecurrentTerm(
    SENode* node, const Loop* loop) {


  SERecurrentNode* recurrent = node->AsSERecurrentNode();
  if (recurrent) {
    if (recurrent->GetLoop() == loop) {
      return recurrent->GetOffset();
    } else {
      return node;
    }
  }

  std::vector<SENode*> new_children;

  for (auto itr : *node) {
    recurrent = itr->AsSERecurrentNode();
    if (recurrent && recurrent->GetLoop() == loop) {
      new_children.push_back(recurrent->GetOffset());
    } else {
      new_children.push_back(itr);
    }
  }

  std::unique_ptr<SENode> add_node{new SEAddNode(this)};
  for (SENode* child : new_children) {
    add_node->AddChild(child);
  }

  return SimplifyExpression(GetCachedOrAdd(std::move(add_node)));
}



SERecurrentNode* ScalarEvolutionAnalysis::GetRecurrentTerm(SENode* node,
                                                           const Loop* loop) {
  for (auto itr = node->graph_begin(); itr != node->graph_end(); ++itr) {
    SERecurrentNode* rec = itr->AsSERecurrentNode();
    if (rec && rec->GetLoop() == loop) {
      return rec;
    }
  }
  return nullptr;
}
std::string SENode::AsString() const {
  switch (GetType()) {
    case Constant:
      return "Constant";
    case RecurrentAddExpr:
      return "RecurrentAddExpr";
    case Add:
      return "Add";
    case Negative:
      return "Negative";
    case Multiply:
      return "Multiply";
    case ValueUnknown:
      return "Value Unknown";
    case CanNotCompute:
      return "Can not compute";
  }
  return "NULL";
}

bool SENode::operator==(const SENode& other) const {
  if (GetType() != other.GetType()) return false;

  if (other.GetChildren().size() != children_.size()) return false;

  const SERecurrentNode* this_as_recurrent = AsSERecurrentNode();




  if (!this_as_recurrent) {
    for (size_t index = 0; index < children_.size(); ++index) {
      if (other.GetChildren()[index] != children_[index]) return false;
    }
  } else {
    const SERecurrentNode* other_as_recurrent = other.AsSERecurrentNode();



    assert(other_as_recurrent);

    if (this_as_recurrent->GetCoefficient() !=
        other_as_recurrent->GetCoefficient())
      return false;

    if (this_as_recurrent->GetOffset() != other_as_recurrent->GetOffset())
      return false;

    if (this_as_recurrent->GetLoop() != other_as_recurrent->GetLoop())
      return false;
  }



  if (GetType() == SENode::ValueUnknown) {
    if (AsSEValueUnknown()->ResultId() !=
        other.AsSEValueUnknown()->ResultId()) {
      return false;
    }
  }

  if (AsSEConstantNode()) {
    if (AsSEConstantNode()->FoldToSingleValue() !=
        other.AsSEConstantNode()->FoldToSingleValue())
      return false;
  }

  return true;
}

bool SENode::operator!=(const SENode& other) const { return !(*this == other); }

namespace {






template <typename T, size_t size_of_t>
struct PushToStringImpl;

template <typename T>
struct PushToStringImpl<T, 8> {
  void operator()(T id, std::u32string* str) {
    str->push_back(static_cast<uint32_t>(id >> 32));
    str->push_back(static_cast<uint32_t>(id));
  }
};

template <typename T>
struct PushToStringImpl<T, 4> {
  void operator()(T id, std::u32string* str) {
    str->push_back(static_cast<uint32_t>(id));
  }
};

template <typename T>
void PushToString(T id, std::u32string* str) {
  PushToStringImpl<T, sizeof(T)>{}(id, str);
}

}


size_t SENodeHash::operator()(const SENode* node) const {

  std::u32string hash_string{};



  for (char ch : node->AsString()) {
    hash_string.push_back(static_cast<char32_t>(ch));
  }


  if (node->GetType() == SENode::Constant)
    PushToString(node->AsSEConstantNode()->FoldToSingleValue(), &hash_string);

  const SERecurrentNode* recurrent = node->AsSERecurrentNode();



  if (recurrent) {
    PushToString(reinterpret_cast<uintptr_t>(recurrent->GetLoop()),
                 &hash_string);



    PushToString(reinterpret_cast<uintptr_t>(recurrent->GetCoefficient()),
                 &hash_string);
    PushToString(reinterpret_cast<uintptr_t>(recurrent->GetOffset()),
                 &hash_string);

    return std::hash<std::u32string>{}(hash_string);
  }



  if (node->GetType() == SENode::ValueUnknown) {
    PushToString(node->AsSEValueUnknown()->ResultId(), &hash_string);
  }



  const std::vector<SENode*>& children = node->GetChildren();
  for (const SENode* child : children) {
    PushToString(reinterpret_cast<uintptr_t>(child), &hash_string);
  }

  return std::hash<std::u32string>{}(hash_string);
}


size_t SENodeHash::operator()(const std::unique_ptr<SENode>& node) const {
  return this->operator()(node.get());
}

void SENode::DumpDot(std::ostream& out, bool recurse) const {
  size_t unique_id = std::hash<const SENode*>{}(this);
  out << unique_id << " [label=\"" << AsString() << " ";
  if (GetType() == SENode::Constant) {
    out << "\nwith value: " << this->AsSEConstantNode()->FoldToSingleValue();
  }
  out << "\"]\n";
  for (const SENode* child : children_) {
    size_t child_unique_id = std::hash<const SENode*>{}(child);
    out << unique_id << " -> " << child_unique_id << " \n";
    if (recurse) child->DumpDot(out, true);
  }
}

namespace {
class IsGreaterThanZero {
 public:
  explicit IsGreaterThanZero(IRContext* context) : context_(context) {}







  bool Eval(const SENode* node, bool or_equal_zero, bool* result) {
    *result = false;
    switch (Visit(node)) {
      case Signedness::kPositiveOrNegative: {
        return false;
      }
      case Signedness::kStrictlyNegative: {
        *result = false;
        break;
      }
      case Signedness::kNegative: {
        if (!or_equal_zero) {
          return false;
        }
        *result = false;
        break;
      }
      case Signedness::kStrictlyPositive: {
        *result = true;
        break;
      }
      case Signedness::kPositive: {
        if (!or_equal_zero) {
          return false;
        }
        *result = true;
        break;
      }
    }
    return true;
  }

 private:
  enum class Signedness {
    kPositiveOrNegative,
    kStrictlyNegative,
    kNegative,
    kStrictlyPositive,
    kPositive
  };


  using Combiner = std::function<Signedness(Signedness, Signedness)>;



  Combiner GetAddCombiner() const {
    return [](Signedness lhs, Signedness rhs) {
      switch (lhs) {
        case Signedness::kPositiveOrNegative:
          break;
        case Signedness::kStrictlyNegative:
          if (rhs == Signedness::kStrictlyNegative ||
              rhs == Signedness::kNegative)
            return lhs;
          break;
        case Signedness::kNegative: {
          if (rhs == Signedness::kStrictlyNegative)
            return Signedness::kStrictlyNegative;
          if (rhs == Signedness::kNegative) return Signedness::kNegative;
          break;
        }
        case Signedness::kStrictlyPositive: {
          if (rhs == Signedness::kStrictlyPositive ||
              rhs == Signedness::kPositive) {
            return Signedness::kStrictlyPositive;
          }
          break;
        }
        case Signedness::kPositive: {
          if (rhs == Signedness::kStrictlyPositive)
            return Signedness::kStrictlyPositive;
          if (rhs == Signedness::kPositive) return Signedness::kPositive;
          break;
        }
      }
      return Signedness::kPositiveOrNegative;
    };
  }



  Combiner GetMulCombiner() const {
    return [](Signedness lhs, Signedness rhs) {
      switch (lhs) {
        case Signedness::kPositiveOrNegative:
          break;
        case Signedness::kStrictlyNegative: {
          switch (rhs) {
            case Signedness::kPositiveOrNegative: {
              break;
            }
            case Signedness::kStrictlyNegative: {
              return Signedness::kStrictlyPositive;
            }
            case Signedness::kNegative: {
              return Signedness::kPositive;
            }
            case Signedness::kStrictlyPositive: {
              return Signedness::kStrictlyNegative;
            }
            case Signedness::kPositive: {
              return Signedness::kNegative;
            }
          }
          break;
        }
        case Signedness::kNegative: {
          switch (rhs) {
            case Signedness::kPositiveOrNegative: {
              break;
            }
            case Signedness::kStrictlyNegative:
            case Signedness::kNegative: {
              return Signedness::kPositive;
            }
            case Signedness::kStrictlyPositive:
            case Signedness::kPositive: {
              return Signedness::kNegative;
            }
          }
          break;
        }
        case Signedness::kStrictlyPositive: {
          return rhs;
        }
        case Signedness::kPositive: {
          switch (rhs) {
            case Signedness::kPositiveOrNegative: {
              break;
            }
            case Signedness::kStrictlyNegative:
            case Signedness::kNegative: {
              return Signedness::kNegative;
            }
            case Signedness::kStrictlyPositive:
            case Signedness::kPositive: {
              return Signedness::kPositive;
            }
          }
          break;
        }
      }
      return Signedness::kPositiveOrNegative;
    };
  }

  Signedness Visit(const SENode* node) {
    switch (node->GetType()) {
      case SENode::Constant:
        return Visit(node->AsSEConstantNode());
        break;
      case SENode::RecurrentAddExpr:
        return Visit(node->AsSERecurrentNode());
        break;
      case SENode::Negative:
        return Visit(node->AsSENegative());
        break;
      case SENode::CanNotCompute:
        return Visit(node->AsSECantCompute());
        break;
      case SENode::ValueUnknown:
        return Visit(node->AsSEValueUnknown());
        break;
      case SENode::Add:
        return VisitExpr(node, GetAddCombiner());
        break;
      case SENode::Multiply:
        return VisitExpr(node, GetMulCombiner());
        break;
    }
    return Signedness::kPositiveOrNegative;
  }


  Signedness Visit(const SEConstantNode* node) {
    if (0 == node->FoldToSingleValue()) return Signedness::kPositive;
    if (0 < node->FoldToSingleValue()) return Signedness::kStrictlyPositive;
    if (0 > node->FoldToSingleValue()) return Signedness::kStrictlyNegative;
    return Signedness::kPositiveOrNegative;
  }


  Signedness Visit(const SEValueUnknown* node) {
    Instruction* insn = context_->get_def_use_mgr()->GetDef(node->ResultId());
    analysis::Type* type = context_->get_type_mgr()->GetType(insn->type_id());
    assert(type && "Can't retrieve a type for the instruction");
    analysis::Integer* int_type = type->AsInteger();
    assert(type && "Can't retrieve an integer type for the instruction");
    return int_type->IsSigned() ? Signedness::kPositiveOrNegative
                                : Signedness::kPositive;
  }


  Signedness Visit(const SERecurrentNode* node) {
    Signedness coeff_sign = Visit(node->GetCoefficient());


    switch (coeff_sign) {
      default:
        break;
      case Signedness::kStrictlyNegative:
        coeff_sign = Signedness::kNegative;
        break;
      case Signedness::kStrictlyPositive:
        coeff_sign = Signedness::kPositive;
        break;
    }
    return GetAddCombiner()(coeff_sign, Visit(node->GetOffset()));
  }


  Signedness Visit(const SENegative* node) {
    switch (Visit(*node->begin())) {
      case Signedness::kPositiveOrNegative: {
        return Signedness::kPositiveOrNegative;
      }
      case Signedness::kStrictlyNegative: {
        return Signedness::kStrictlyPositive;
      }
      case Signedness::kNegative: {
        return Signedness::kPositive;
      }
      case Signedness::kStrictlyPositive: {
        return Signedness::kStrictlyNegative;
      }
      case Signedness::kPositive: {
        return Signedness::kNegative;
      }
    }
    return Signedness::kPositiveOrNegative;
  }

  Signedness Visit(const SECantCompute*) {
    return Signedness::kPositiveOrNegative;
  }



  Signedness VisitExpr(
      const SENode* node,
      std::function<Signedness(Signedness, Signedness)> reduce) {
    Signedness result = Visit(*node->begin());
    for (const SENode* operand : make_range(++node->begin(), node->end())) {
      if (result == Signedness::kPositiveOrNegative) {
        return Signedness::kPositiveOrNegative;
      }
      result = reduce(result, Visit(operand));
    }
    return result;
  }

  IRContext* context_;
};
}

bool ScalarEvolutionAnalysis::IsAlwaysGreaterThanZero(SENode* node,
                                                      bool* is_gt_zero) const {
  return IsGreaterThanZero(context_).Eval(node, false, is_gt_zero);
}

bool ScalarEvolutionAnalysis::IsAlwaysGreaterOrEqualToZero(
    SENode* node, bool* is_ge_zero) const {
  return IsGreaterThanZero(context_).Eval(node, true, is_ge_zero);
}

namespace {



SENode* RemoveOneNodeFromMultiplyChain(SEMultiplyNode* mul,
                                       const SENode* node) {
  SENode* lhs = mul->GetChildren()[0];
  SENode* rhs = mul->GetChildren()[1];
  if (lhs == node) {
    return rhs;
  }
  if (rhs == node) {
    return lhs;
  }
  if (lhs->AsSEMultiplyNode()) {
    SENode* res = RemoveOneNodeFromMultiplyChain(lhs->AsSEMultiplyNode(), node);
    if (res != lhs)
      return mul->GetParentAnalysis()->CreateMultiplyNode(res, rhs);
  }
  if (rhs->AsSEMultiplyNode()) {
    SENode* res = RemoveOneNodeFromMultiplyChain(rhs->AsSEMultiplyNode(), node);
    if (res != rhs)
      return mul->GetParentAnalysis()->CreateMultiplyNode(res, rhs);
  }

  return mul;
}
}

std::pair<SExpression, int64_t> SExpression::operator/(
    SExpression rhs_wrapper) const {
  SENode* lhs = node_;
  SENode* rhs = rhs_wrapper.node_;

  if (rhs->AsSEConstantNode() &&
      !rhs->AsSEConstantNode()->FoldToSingleValue()) {
    return {scev_->CreateCantComputeNode(), 0};
  }


  if (lhs->AsSEConstantNode() && rhs->AsSEConstantNode()) {
    int64_t lhs_value = lhs->AsSEConstantNode()->FoldToSingleValue();
    int64_t rhs_value = rhs->AsSEConstantNode()->FoldToSingleValue();
    return {scev_->CreateConstant(lhs_value / rhs_value),
            lhs_value % rhs_value};
  }


  if (lhs->AsSEMultiplyNode()) {
    assert(lhs->GetChildren().size() == 2 &&
           "More than 2 operand for a multiply node.");
    SENode* res = RemoveOneNodeFromMultiplyChain(lhs->AsSEMultiplyNode(), rhs);
    if (res != lhs) {
      return {res, 0};
    }
  }

  return {scev_->CreateCantComputeNode(), 0};
}

}
}
