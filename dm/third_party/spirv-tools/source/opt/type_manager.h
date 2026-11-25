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

#ifndef SOURCE_OPT_TYPE_MANAGER_H_
#define SOURCE_OPT_TYPE_MANAGER_H_

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "source/opt/module.h"
#include "source/opt/types.h"
#include "spirv-tools/libspirv.hpp"

namespace spvtools {
namespace opt {

class IRContext;

namespace analysis {




struct HashTypePointer {
  size_t operator()(const Type* type) const {
    assert(type);
    return type->HashValue();
  }
};
struct HashTypeUniquePointer {
  size_t operator()(const std::unique_ptr<Type>& type) const {
    assert(type);
    return type->HashValue();
  }
};






struct CompareTypePointers {
  bool operator()(const Type* lhs, const Type* rhs) const {
    assert(lhs && rhs);
    return lhs->IsSame(rhs);
  }
};
struct CompareTypeUniquePointers {
  bool operator()(const std::unique_ptr<Type>& lhs,
                  const std::unique_ptr<Type>& rhs) const {
    assert(lhs && rhs);
    return lhs->IsSame(rhs.get());
  }
};


class TypeManager {
 public:
  using IdToTypeMap = std::unordered_map<uint32_t, Type*>;





  TypeManager(const MessageConsumer& consumer, IRContext* c);

  TypeManager(const TypeManager&) = delete;
  TypeManager(TypeManager&&) = delete;
  TypeManager& operator=(const TypeManager&) = delete;
  TypeManager& operator=(TypeManager&&) = delete;



  Type* GetType(uint32_t id) const;


  uint32_t GetId(const Type* type) const;

  size_t NumTypes() const { return id_to_type_.size(); }

  IdToTypeMap::const_iterator begin() const { return id_to_type_.cbegin(); }
  IdToTypeMap::const_iterator end() const { return id_to_type_.cend(); }




  std::pair<Type*, std::unique_ptr<Pointer>> GetTypeAndPointerType(
      uint32_t id, spv::StorageClass sc) const;







  uint32_t GetTypeInstruction(const Type* type);




  uint32_t FindPointerToType(uint32_t type_id, spv::StorageClass storage_class);





  void RegisterType(uint32_t id, const Type& type);


  Type* GetRegisteredType(const Type* type);







  void RemoveId(uint32_t id);






  const Type* GetMemberType(const Type* parent_type,
                            const std::vector<uint32_t>& access_chain);




  void AttachDecoration(const Instruction& inst, Type* type);

  Type* GetUIntType() { return GetIntType(32, false); }

  uint32_t GetUIntTypeId() { return GetTypeInstruction(GetUIntType()); }

  Type* GetIntType(int32_t bitWidth, bool isSigned) {
    Integer int_type(bitWidth, isSigned);
    return GetRegisteredType(&int_type);
  }

  Type* GetSIntType() { return GetIntType(32, true); }

  uint32_t GetSIntTypeId() { return GetTypeInstruction(GetSIntType()); }

  Type* GetFloatType() {
    Float float_type(32);
    return GetRegisteredType(&float_type);
  }

  uint32_t GetFloatTypeId() { return GetTypeInstruction(GetFloatType()); }

  Type* GetDoubleType() {
    Float float_type(64);
    return GetRegisteredType(&float_type);
  }

  uint32_t GetDoubleTypeId() { return GetTypeInstruction(GetDoubleType()); }

  Type* GetUIntVectorType(uint32_t size) {
    Vector vec_type(GetUIntType(), size);
    return GetRegisteredType(&vec_type);
  }

  uint32_t GetUIntVectorTypeId(uint32_t size) {
    return GetTypeInstruction(GetUIntVectorType(size));
  }

  Type* GetSIntVectorType(uint32_t size) {
    Vector vec_type(GetSIntType(), size);
    return GetRegisteredType(&vec_type);
  }

  uint32_t GetSIntVectorTypeId(uint32_t size) {
    return GetTypeInstruction(GetSIntVectorType(size));
  }

  Type* GetFloatVectorType(uint32_t size) {
    Vector vec_type(GetFloatType(), size);
    return GetRegisteredType(&vec_type);
  }

  uint32_t GetFloatVectorTypeId(uint32_t size) {
    return GetTypeInstruction(GetFloatVectorType(size));
  }

  Type* GetBoolType() {
    Bool bool_type;
    return GetRegisteredType(&bool_type);
  }

  uint32_t GetBoolTypeId() {
    Type* bool_type = GetBoolType();
    if (bool_type == nullptr) return 0;
    return GetTypeInstruction(bool_type);
  }

  Type* GetVoidType() {
    Void void_type;
    return GetRegisteredType(&void_type);
  }

  uint32_t GetVoidTypeId() { return GetTypeInstruction(GetVoidType()); }

 private:
  using TypeToIdMap = std::unordered_map<const Type*, uint32_t, HashTypePointer,
                                         CompareTypePointers>;
  using TypePool =
      std::unordered_set<std::unique_ptr<Type>, HashTypeUniquePointer,
                         CompareTypeUniquePointers>;

  class UnresolvedType {
   public:
    UnresolvedType(uint32_t i, Type* t) : id_(i), type_(t) {}
    UnresolvedType(const UnresolvedType&) = delete;
    UnresolvedType(UnresolvedType&& that)
        : id_(that.id_), type_(std::move(that.type_)) {}

    uint32_t id() { return id_; }
    Type* type() { return type_.get(); }
    std::unique_ptr<Type>&& ReleaseType() { return std::move(type_); }
    void ResetType(Type* t) { type_.reset(t); }

   private:
    uint32_t id_;
    std::unique_ptr<Type> type_;
  };
  using IdToUnresolvedType = std::vector<UnresolvedType>;


  void AnalyzeTypes(const Module& module);

  IRContext* context() { return context_; }


  void AttachDecorations(uint32_t id, const Type* type);






  void CreateDecoration(uint32_t id, const std::vector<uint32_t>& decoration,
                        bool is_member = false, uint32_t element = 0);



  Type* RecordIfTypeDefinition(const Instruction& inst);






  Type* RebuildType(uint32_t type_id, const Type& type);



  void ReplaceForwardPointers(Type* type);



  void ReplaceType(Type* new_type, Type* original_type);

  const MessageConsumer& consumer_;
  IRContext* context_;
  IdToTypeMap id_to_type_;
  TypeToIdMap type_to_id_;
  TypePool type_pool_;
  IdToUnresolvedType incomplete_types_;



  IdToTypeMap id_to_incomplete_type_;


  std::unordered_map<uint32_t, const Instruction*> id_to_constant_inst_;
};

}
}
}

#endif
