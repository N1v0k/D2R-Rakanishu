
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

#ifndef SOURCE_OPT_INSTRUCTION_LIST_H_
#define SOURCE_OPT_INSTRUCTION_LIST_H_

#include <cassert>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include "source/latest_version_spirv_header.h"
#include "source/operand.h"
#include "source/opt/instruction.h"
#include "source/util/ilist.h"
#include "spirv-tools/libspirv.h"

namespace spvtools {
namespace opt {











class InstructionList : public utils::IntrusiveList<Instruction> {
 public:
  InstructionList() = default;
  InstructionList(InstructionList&& that)
      : utils::IntrusiveList<Instruction>(std::move(that)) {}
  InstructionList& operator=(InstructionList&& that) {
    auto p = static_cast<utils::IntrusiveList<Instruction>*>(this);
    *p = std::move(that);
    return *this;
  }


  inline ~InstructionList() override;

  class iterator : public utils::IntrusiveList<Instruction>::iterator {
   public:
    iterator(const utils::IntrusiveList<Instruction>::iterator& i)
        : utils::IntrusiveList<Instruction>::iterator(i) {}
    iterator(Instruction* i) : utils::IntrusiveList<Instruction>::iterator(i) {}

    iterator& operator++() {
      utils::IntrusiveList<Instruction>::iterator::operator++();
      return *this;
    }

    iterator& operator--() {
      utils::IntrusiveList<Instruction>::iterator::operator--();
      return *this;
    }








    iterator InsertBefore(std::vector<std::unique_ptr<Instruction>>&& list);




    iterator InsertBefore(std::unique_ptr<Instruction>&& i);



    iterator Erase() {
      iterator_template next_node = *this;
      ++next_node;
      node_->RemoveFromList();
      delete node_;
      return next_node;
    }
  };

  iterator begin() { return utils::IntrusiveList<Instruction>::begin(); }
  iterator end() { return utils::IntrusiveList<Instruction>::end(); }
  const_iterator begin() const {
    return utils::IntrusiveList<Instruction>::begin();
  }
  const_iterator end() const {
    return utils::IntrusiveList<Instruction>::end();
  }

  void push_back(std::unique_ptr<Instruction>&& inst) {
    utils::IntrusiveList<Instruction>::push_back(inst.release());
  }


  inline void clear();



  inline void ForEachInst(const std::function<void(Instruction*)>& f,
                          bool run_on_debug_line_insts) {
    auto next = begin();
    for (auto i = next; i != end(); i = next) {
      ++next;
      i->ForEachInst(f, run_on_debug_line_insts);
    }
  }
};

InstructionList::~InstructionList() { clear(); }

void InstructionList::clear() {
  while (!empty()) {
    Instruction* inst = &front();
    inst->RemoveFromList();
    delete inst;
  }
}

}
}

#endif
