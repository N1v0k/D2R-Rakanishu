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

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "source/opt/scalar_analysis.h"










































namespace spvtools {
namespace opt {










class SENodeSimplifyImpl {
 public:
  SENodeSimplifyImpl(ScalarEvolutionAnalysis* analysis,
                     SENode* node_to_simplify)
      : analysis_(*analysis),
        node_(node_to_simplify),
        constant_accumulator_(0) {}


  SENode* Simplify();

 private:





  void GatherAccumulatorsFromChildNodes(SENode* new_node, SENode* child,
                                        bool negation);






  bool AccumulatorsFromMultiply(SENode* multiply, bool negation);

  SERecurrentNode* UpdateCoefficient(SERecurrentNode* recurrent,
                                     int64_t coefficient_update) const;




  SENode* SimplifyRecurrentAddExpression(SERecurrentNode* node);




  SENode* SimplifyPolynomial();












  SENode* FoldRecurrentAddExpressions(SENode*);





  SENode* EliminateZeroCoefficientRecurrents(SENode* node);


  ScalarEvolutionAnalysis& analysis_;


  SENode* node_;



  int64_t constant_accumulator_;


  std::map<SENode*, int64_t> accumulators_;
};


bool SENodeSimplifyImpl::AccumulatorsFromMultiply(SENode* multiply,
                                                  bool negation) {
  if (multiply->GetChildren().size() != 2 ||
      multiply->GetType() != SENode::Multiply)
    return false;

  SENode* operand_1 = multiply->GetChild(0);
  SENode* operand_2 = multiply->GetChild(1);

  SENode* value_unknown = nullptr;
  SENode* constant = nullptr;


  if (operand_1->GetType() == SENode::ValueUnknown ||
      operand_1->GetType() == SENode::RecurrentAddExpr)
    value_unknown = operand_1;
  else if (operand_2->GetType() == SENode::ValueUnknown ||
           operand_2->GetType() == SENode::RecurrentAddExpr)
    value_unknown = operand_2;


  if (operand_1->GetType() == SENode::Constant)
    constant = operand_1;
  else if (operand_2->GetType() == SENode::Constant)
    constant = operand_2;



  if (!(value_unknown && constant)) {
    return false;
  }

  int64_t sign = negation ? -1 : 1;

  auto iterator = accumulators_.find(value_unknown);
  int64_t new_value = constant->AsSEConstantNode()->FoldToSingleValue() * sign;

  if (iterator != accumulators_.end()) {
    (*iterator).second += new_value;
  } else {
    accumulators_.insert({value_unknown, new_value});
  }

  return true;
}

SENode* SENodeSimplifyImpl::Simplify() {


  if (node_->GetType() != SENode::Add && node_->GetType() != SENode::Multiply &&
      node_->GetType() != SENode::Negative)
    return node_;

  SENode* simplified_polynomial = SimplifyPolynomial();

  SERecurrentNode* recurrent_expr = nullptr;
  node_ = simplified_polynomial;



  simplified_polynomial = FoldRecurrentAddExpressions(simplified_polynomial);

  simplified_polynomial =
      EliminateZeroCoefficientRecurrents(simplified_polynomial);



  for (SENode* child : simplified_polynomial->GetChildren()) {
    if (child->GetType() == SENode::RecurrentAddExpr) {
      recurrent_expr = child->AsSERecurrentNode();
    }
  }



  for (auto child_iterator = simplified_polynomial->graph_begin();
       child_iterator != simplified_polynomial->graph_end(); ++child_iterator) {
    if (child_iterator->GetType() == SENode::RecurrentAddExpr &&
        recurrent_expr != child_iterator->AsSERecurrentNode()) {
      return simplified_polynomial;
    }
  }

  if (recurrent_expr) {
    return SimplifyRecurrentAddExpression(recurrent_expr);
  }

  return simplified_polynomial;
}


void SENodeSimplifyImpl::GatherAccumulatorsFromChildNodes(SENode* new_node,
                                                          SENode* child,
                                                          bool negation) {
  int32_t sign = negation ? -1 : 1;

  if (child->GetType() == SENode::Constant) {

    constant_accumulator_ +=
        child->AsSEConstantNode()->FoldToSingleValue() * sign;

  } else if (child->GetType() == SENode::ValueUnknown ||
             child->GetType() == SENode::RecurrentAddExpr) {





    auto iterator = accumulators_.find(child);

    if (iterator == accumulators_.end())
      accumulators_.insert({child, sign});
    else
      iterator->second += sign;

  } else if (child->GetType() == SENode::Multiply) {
    if (!AccumulatorsFromMultiply(child, negation)) {
      new_node->AddChild(child);
    }

  } else if (child->GetType() == SENode::Add) {
    for (SENode* next_child : *child) {
      GatherAccumulatorsFromChildNodes(new_node, next_child, negation);
    }

  } else if (child->GetType() == SENode::Negative) {
    SENode* negated_node = child->GetChild(0);
    GatherAccumulatorsFromChildNodes(new_node, negated_node, !negation);
  } else {


    new_node->AddChild(child);
  }
}

SERecurrentNode* SENodeSimplifyImpl::UpdateCoefficient(
    SERecurrentNode* recurrent, int64_t coefficient_update) const {
  std::unique_ptr<SERecurrentNode> new_recurrent_node{new SERecurrentNode(
      recurrent->GetParentAnalysis(), recurrent->GetLoop())};

  SENode* new_coefficient = analysis_.CreateMultiplyNode(
      recurrent->GetCoefficient(),
      analysis_.CreateConstant(coefficient_update));


  SENode* simplified = analysis_.SimplifyExpression(new_coefficient);
  if (simplified->GetType() != SENode::CanNotCompute)
    new_coefficient = simplified;

  if (coefficient_update < 0) {
    new_recurrent_node->AddOffset(
        analysis_.CreateNegation(recurrent->GetOffset()));
  } else {
    new_recurrent_node->AddOffset(recurrent->GetOffset());
  }

  new_recurrent_node->AddCoefficient(new_coefficient);

  return analysis_.GetCachedOrAdd(std::move(new_recurrent_node))
      ->AsSERecurrentNode();
}


SENode* SENodeSimplifyImpl::SimplifyPolynomial() {
  std::unique_ptr<SENode> new_add{new SEAddNode(node_->GetParentAnalysis())};


  GatherAccumulatorsFromChildNodes(new_add.get(), node_, false);


  if (constant_accumulator_ != 0) {
    new_add->AddChild(analysis_.CreateConstant(constant_accumulator_));
  }

  for (auto& pair : accumulators_) {
    SENode* term = pair.first;
    int64_t count = pair.second;


    if (count == 0) continue;

    if (count == 1) {
      new_add->AddChild(term);
    } else if (count == -1 && term->GetType() != SENode::RecurrentAddExpr) {




      new_add->AddChild(analysis_.CreateNegation(term));
    } else {



      if (term->GetType() == SENode::ValueUnknown) {
        SENode* count_as_constant = analysis_.CreateConstant(count);
        new_add->AddChild(
            analysis_.CreateMultiplyNode(count_as_constant, term));
      } else {
        assert(term->GetType() == SENode::RecurrentAddExpr &&
               "We only handle value unknowns or recurrent expressions");



        new_add->AddChild(UpdateCoefficient(term->AsSERecurrentNode(), count));
      }
    }
  }


  if (new_add->GetChildren().size() == 1) {
    return new_add->GetChild(0);
  }


  if (new_add->GetChildren().size() == 0) {
    return analysis_.CreateConstant(0);
  }

  return analysis_.GetCachedOrAdd(std::move(new_add));
}

SENode* SENodeSimplifyImpl::FoldRecurrentAddExpressions(SENode* root) {
  std::unique_ptr<SEAddNode> new_node{new SEAddNode(&analysis_)};



  std::map<const Loop*, std::vector<std::pair<SERecurrentNode*, bool>>>
      loops_to_recurrent{};

  bool has_multiple_same_loop_recurrent_terms = false;

  for (SENode* child : *root) {
    bool negation = false;

    if (child->GetType() == SENode::Negative) {
      child = child->GetChild(0);
      negation = true;
    }

    if (child->GetType() == SENode::RecurrentAddExpr) {
      const Loop* loop = child->AsSERecurrentNode()->GetLoop();

      SERecurrentNode* rec = child->AsSERecurrentNode();
      if (loops_to_recurrent.find(loop) == loops_to_recurrent.end()) {
        loops_to_recurrent[loop] = {std::make_pair(rec, negation)};
      } else {
        loops_to_recurrent[loop].push_back(std::make_pair(rec, negation));
        has_multiple_same_loop_recurrent_terms = true;
      }
    } else {
      new_node->AddChild(child);
    }
  }

  if (!has_multiple_same_loop_recurrent_terms) return root;

  for (auto pair : loops_to_recurrent) {
    std::vector<std::pair<SERecurrentNode*, bool>>& recurrent_expressions =
        pair.second;
    const Loop* loop = pair.first;

    std::unique_ptr<SENode> new_coefficient{new SEAddNode(&analysis_)};
    std::unique_ptr<SENode> new_offset{new SEAddNode(&analysis_)};

    for (auto node_pair : recurrent_expressions) {
      SERecurrentNode* node = node_pair.first;
      bool negative = node_pair.second;

      if (!negative) {
        new_coefficient->AddChild(node->GetCoefficient());
        new_offset->AddChild(node->GetOffset());
      } else {
        new_coefficient->AddChild(
            analysis_.CreateNegation(node->GetCoefficient()));
        new_offset->AddChild(analysis_.CreateNegation(node->GetOffset()));
      }
    }

    std::unique_ptr<SERecurrentNode> new_recurrent{
        new SERecurrentNode(&analysis_, loop)};

    SENode* new_coefficient_simplified =
        analysis_.SimplifyExpression(new_coefficient.get());

    SENode* new_offset_simplified =
        analysis_.SimplifyExpression(new_offset.get());

    if (new_coefficient_simplified->GetType() == SENode::Constant &&
        new_coefficient_simplified->AsSEConstantNode()->FoldToSingleValue() ==
            0) {
      return new_offset_simplified;
    }

    new_recurrent->AddCoefficient(new_coefficient_simplified);
    new_recurrent->AddOffset(new_offset_simplified);

    new_node->AddChild(analysis_.GetCachedOrAdd(std::move(new_recurrent)));
  }


  if (new_node->GetChildren().size() == 1) {
    return new_node->GetChild(0);
  }

  return analysis_.GetCachedOrAdd(std::move(new_node));
}

SENode* SENodeSimplifyImpl::EliminateZeroCoefficientRecurrents(SENode* node) {
  if (node->GetType() != SENode::Add) return node;

  bool has_change = false;

  std::vector<SENode*> new_children{};
  for (SENode* child : *node) {
    if (child->GetType() == SENode::RecurrentAddExpr) {
      SENode* coefficient = child->AsSERecurrentNode()->GetCoefficient();



      if (coefficient->GetType() == SENode::Constant &&
          coefficient->AsSEConstantNode()->FoldToSingleValue() == 0) {
        new_children.push_back(child->AsSERecurrentNode()->GetOffset());
        has_change = true;
      } else {
        new_children.push_back(child);
      }
    } else {
      new_children.push_back(child);
    }
  }

  if (!has_change) return node;

  std::unique_ptr<SENode> new_add{new SEAddNode(node_->GetParentAnalysis())};

  for (SENode* child : new_children) {
    new_add->AddChild(child);
  }

  return analysis_.GetCachedOrAdd(std::move(new_add));
}

SENode* SENodeSimplifyImpl::SimplifyRecurrentAddExpression(
    SERecurrentNode* recurrent_expr) {
  const std::vector<SENode*>& children = node_->GetChildren();

  std::unique_ptr<SERecurrentNode> recurrent_node{new SERecurrentNode(
      recurrent_expr->GetParentAnalysis(), recurrent_expr->GetLoop())};


  std::unique_ptr<SENode> new_offset{
      new SEAddNode(recurrent_expr->GetParentAnalysis())};
  new_offset->AddChild(recurrent_expr->GetOffset());

  for (SENode* child : children) {
    if (child->GetType() != SENode::RecurrentAddExpr) {
      new_offset->AddChild(child);
    }
  }


  SENode* simplified_child = analysis_.SimplifyExpression(new_offset.get());



  if (simplified_child->GetType() != SENode::CanNotCompute) {
    recurrent_node->AddOffset(simplified_child);
  } else {
    recurrent_expr->AddOffset(analysis_.GetCachedOrAdd(std::move(new_offset)));
  }

  recurrent_node->AddCoefficient(recurrent_expr->GetCoefficient());

  return analysis_.GetCachedOrAdd(std::move(recurrent_node));
}





SENode* ScalarEvolutionAnalysis::SimplifyExpression(SENode* node) {
  SENodeSimplifyImpl impl{this, node};

  return impl.Simplify();
}

}
}
