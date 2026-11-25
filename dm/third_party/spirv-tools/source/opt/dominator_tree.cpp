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

#include <iostream>
#include <memory>
#include <set>

#include "source/cfa.h"
#include "source/opt/dominator_tree.h"
#include "source/opt/ir_context.h"


















namespace spvtools {
namespace opt {
namespace {













template <typename BBType, typename SuccessorLambda, typename PreLambda,
          typename PostLambda>
void DepthFirstSearch(const BBType* bb, SuccessorLambda successors,
                      PreLambda pre, PostLambda post) {
  auto no_terminal_blocks = [](const BBType*) { return false; };
  CFA<BBType>::DepthFirstTraversal(bb, successors, pre, post,
                                   no_terminal_blocks);
}











template <typename BBType, typename SuccessorLambda, typename PostLambda>
void DepthFirstSearchPostOrder(const BBType* bb, SuccessorLambda successors,
                               PostLambda post) {

  auto nop_preorder = [](const BBType*) {};
  DepthFirstSearch(bb, successors, nop_preorder, post);
}


template <typename BBType>
struct GetFunctionClass {
  using FunctionType = Function;
};










template <typename BBType>
class BasicBlockSuccessorHelper {

  using BasicBlock = BBType;
  using Function = typename GetFunctionClass<BBType>::FunctionType;

  using BasicBlockListTy = std::vector<BasicBlock*>;
  using BasicBlockMapTy =
      std::unordered_map<const BasicBlock*, BasicBlockListTy>;

 public:


  BasicBlockSuccessorHelper(Function& func,
                            const BasicBlock* placeholder_start_node,
                            bool post);


  using GetBlocksFunction =
      std::function<const std::vector<BasicBlock*>*(const BasicBlock*)>;


  GetBlocksFunction GetPredFunctor() {
    return [this](const BasicBlock* bb) {
      BasicBlockListTy* v = &this->predecessors_[bb];
      return v;
    };
  }


  GetBlocksFunction GetSuccessorFunctor() {
    return [this](const BasicBlock* bb) {
      BasicBlockListTy* v = &this->successors_[bb];
      return v;
    };
  }

 private:
  bool invert_graph_;
  BasicBlockMapTy successors_;
  BasicBlockMapTy predecessors_;








  void CreateSuccessorMap(Function& f,
                          const BasicBlock* placeholder_start_node);
};

template <typename BBType>
BasicBlockSuccessorHelper<BBType>::BasicBlockSuccessorHelper(
    Function& func, const BasicBlock* placeholder_start_node, bool invert)
    : invert_graph_(invert) {
  CreateSuccessorMap(func, placeholder_start_node);
}

template <typename BBType>
void BasicBlockSuccessorHelper<BBType>::CreateSuccessorMap(
    Function& f, const BasicBlock* placeholder_start_node) {
  IRContext* context = f.DefInst().context();

  if (invert_graph_) {






    for (BasicBlock& bb : f) {
      if (bb.hasSuccessor()) {
        BasicBlockListTy& pred_list = predecessors_[&bb];
        const auto& const_bb = bb;
        const_bb.ForEachSuccessorLabel(
            [this, &pred_list, &bb, context](const uint32_t successor_id) {
              BasicBlock* succ = context->get_instr_block(successor_id);


              this->successors_[succ].push_back(&bb);
              pred_list.push_back(succ);
            });
      } else {
        successors_[placeholder_start_node].push_back(&bb);
        predecessors_[&bb].push_back(
            const_cast<BasicBlock*>(placeholder_start_node));
      }
    }
  } else {
    successors_[placeholder_start_node].push_back(f.entry().get());
    predecessors_[f.entry().get()].push_back(
        const_cast<BasicBlock*>(placeholder_start_node));
    for (BasicBlock& bb : f) {
      BasicBlockListTy& succ_list = successors_[&bb];

      const auto& const_bb = bb;
      const_bb.ForEachSuccessorLabel([&](const uint32_t successor_id) {
        BasicBlock* succ = context->get_instr_block(successor_id);
        succ_list.push_back(succ);
        predecessors_[succ].push_back(&bb);
      });
    }
  }
}

}

bool DominatorTree::StrictlyDominates(uint32_t a, uint32_t b) const {
  if (a == b) return false;
  return Dominates(a, b);
}

bool DominatorTree::StrictlyDominates(const BasicBlock* a,
                                      const BasicBlock* b) const {
  return DominatorTree::StrictlyDominates(a->id(), b->id());
}

bool DominatorTree::StrictlyDominates(const DominatorTreeNode* a,
                                      const DominatorTreeNode* b) const {
  if (a == b) return false;
  return Dominates(a, b);
}

bool DominatorTree::Dominates(uint32_t a, uint32_t b) const {

  const DominatorTreeNode* a_node = GetTreeNode(a);
  const DominatorTreeNode* b_node = GetTreeNode(b);
  if (!a_node || !b_node) return false;

  return Dominates(a_node, b_node);
}

bool DominatorTree::Dominates(const DominatorTreeNode* a,
                              const DominatorTreeNode* b) const {
  if (!a || !b) return false;

  if (a == b) return true;

  return a->dfs_num_pre_ < b->dfs_num_pre_ &&
         a->dfs_num_post_ > b->dfs_num_post_;
}

bool DominatorTree::Dominates(const BasicBlock* A, const BasicBlock* B) const {
  return Dominates(A->id(), B->id());
}

BasicBlock* DominatorTree::ImmediateDominator(const BasicBlock* A) const {
  return ImmediateDominator(A->id());
}

BasicBlock* DominatorTree::ImmediateDominator(uint32_t a) const {

  auto a_itr = nodes_.find(a);
  if (a_itr == nodes_.end()) return nullptr;

  const DominatorTreeNode* node = &a_itr->second;

  if (node->parent_ == nullptr) {
    return nullptr;
  }

  return node->parent_->bb_;
}

DominatorTreeNode* DominatorTree::GetOrInsertNode(BasicBlock* bb) {
  DominatorTreeNode* dtn = nullptr;

  std::map<uint32_t, DominatorTreeNode>::iterator node_iter =
      nodes_.find(bb->id());
  if (node_iter == nodes_.end()) {
    dtn = &nodes_.emplace(std::make_pair(bb->id(), DominatorTreeNode{bb}))
               .first->second;
  } else {
    dtn = &node_iter->second;
  }

  return dtn;
}

void DominatorTree::GetDominatorEdges(
    const Function* f, const BasicBlock* placeholder_start_node,
    std::vector<std::pair<BasicBlock*, BasicBlock*>>* edges) {



  std::vector<const BasicBlock*> postorder;
  auto postorder_function = [&](const BasicBlock* b) {
    postorder.push_back(b);
  };




  BasicBlockSuccessorHelper<BasicBlock> helper{
      *const_cast<Function*>(f), placeholder_start_node, postdominator_};




  auto successor_functor = helper.GetSuccessorFunctor();



  auto predecessor_functor = helper.GetPredFunctor();




  DepthFirstSearchPostOrder(placeholder_start_node, successor_functor,
                            postorder_function);
  *edges = CFA<BasicBlock>::CalculateDominators(postorder, predecessor_functor);
}

void DominatorTree::InitializeTree(const CFG& cfg, const Function* f) {
  ClearTree();


  if (f->cbegin() == f->cend()) {
    return;
  }

  const BasicBlock* placeholder_start_node =
      postdominator_ ? cfg.pseudo_exit_block() : cfg.pseudo_entry_block();


  std::vector<std::pair<BasicBlock*, BasicBlock*>> edges;
  GetDominatorEdges(f, placeholder_start_node, &edges);



  for (auto edge : edges) {
    DominatorTreeNode* first = GetOrInsertNode(edge.first);

    if (edge.first == edge.second) {
      if (std::find(roots_.begin(), roots_.end(), first) == roots_.end())
        roots_.push_back(first);
      continue;
    }

    DominatorTreeNode* second = GetOrInsertNode(edge.second);

    first->parent_ = second;
    second->children_.push_back(first);
  }
  ResetDFNumbering();
}

void DominatorTree::ResetDFNumbering() {
  int index = 0;
  auto preFunc = [&index](const DominatorTreeNode* node) {
    const_cast<DominatorTreeNode*>(node)->dfs_num_pre_ = ++index;
  };

  auto postFunc = [&index](const DominatorTreeNode* node) {
    const_cast<DominatorTreeNode*>(node)->dfs_num_post_ = ++index;
  };

  auto getSucc = [](const DominatorTreeNode* node) { return &node->children_; };

  for (auto root : roots_) DepthFirstSearch(root, getSucc, preFunc, postFunc);
}

void DominatorTree::DumpTreeAsDot(std::ostream& out_stream) const {
  out_stream << "digraph {\n";
  Visit([&out_stream](const DominatorTreeNode* node) {

    if (node->bb_) {
      out_stream << node->bb_->id() << "[label=\"" << node->bb_->id()
                 << "\"];\n";
    }



    if (node->parent_) {
      out_stream << node->parent_->bb_->id() << " -> " << node->bb_->id()
                 << ";\n";
    }


    return true;
  });
  out_stream << "}\n";
}

}
}
