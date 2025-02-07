/*
 * Copyright 2018- The Pixie Authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <arrow/array.h>
#include <arrow/buffer.h>
#include <arrow/builder.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "src/shared/types/arrow_adapter.h"
#include "src/shared/types/type_utils.h"
#include "src/shared/types/types.h"

namespace px {
namespace types {

class ColumnWrapper;
using SharedColumnWrapper = std::shared_ptr<ColumnWrapper>;
using ColumnWrapperRecordBatch = std::vector<types::SharedColumnWrapper>;

// TODO(oazizi): Find a better place for this.
using TabletID = std::string;
using TabletIDView = std::string_view;

/**
 * Column wrapper stores underlying data so that it can be retrieved in a type erased way
 * to allow column chucks to be transparently passed.
 */
class ColumnWrapper {
 public:
  ColumnWrapper() = default;
  virtual ~ColumnWrapper() = default;

  static SharedColumnWrapper Make(DataType data_type, size_t size);
  static SharedColumnWrapper FromArrow(const std::shared_ptr<arrow::Array>& arr);

  virtual BaseValueType* UnsafeRawData() = 0;
  virtual const BaseValueType* UnsafeRawData() const = 0;
  virtual DataType data_type() const = 0;
  virtual size_t Size() const = 0;
  virtual bool Empty() const = 0;
  virtual int64_t Bytes() const = 0;

  virtual void Reserve(size_t size) = 0;
  virtual void Clear() = 0;
  virtual void ShrinkToFit() = 0;
  virtual std::shared_ptr<arrow::Array> ConvertToArrow(arrow::MemoryPool* mem_pool) = 0;

  template <class TValueType>
  void Append(TValueType val);

  template <class TValueType>
  TValueType& Get(size_t idx);

  template <class TValueType>
  TValueType Get(size_t idx) const;

  template <class TValueType>
  void AppendNoTypeCheck(TValueType val);

  template <class TValueType>
  TValueType& GetNoTypeCheck(size_t idx);

  template <class TValueType>
  TValueType GetNoTypeCheck(size_t idx) const;

  template <class TValueType>
  void AppendFromVector(const std::vector<TValueType>& value_vector);

  // Return a new SharedColumnWrapper with values according to the spec:
  //    { data[idx[0]], data[idx[1]], data[idx[2]], ... }
  // CopyIndexes leaves the original untouched, while MoveIndexes destroys the moved indexes.
  virtual SharedColumnWrapper CopyIndexes(const std::vector<size_t>& indexes) const = 0;
  virtual SharedColumnWrapper MoveIndexes(const std::vector<size_t>& indexes) = 0;
};

/**
 * Implementation of type erased vectors for a specific data type.
 * @tparam T The UDFValueType.
 */
template <typename T>
class ColumnWrapperTmpl : public ColumnWrapper {
 public:
  explicit ColumnWrapperTmpl(size_t size) : data_(size) {}
  explicit ColumnWrapperTmpl(size_t size, const T& val) : data_(size, val) {}
  explicit ColumnWrapperTmpl(const std::vector<T>& vals) : data_(vals) {}

  ~ColumnWrapperTmpl() override = default;

  T* UnsafeRawData() override { return data_.data(); }
  const T* UnsafeRawData() const override { return data_.data(); }
  DataType data_type() const override { return ValueTypeTraits<T>::data_type; }

  size_t Size() const override { return data_.size(); }
  bool Empty() const override { return data_.empty(); }

  std::shared_ptr<arrow::Array> ConvertToArrow(arrow::MemoryPool* mem_pool) override {
    return ToArrow(data_, mem_pool);
  }

  T operator[](size_t idx) const { return data_[idx]; }

  T& operator[](size_t idx) { return data_[idx]; }

  void Append(T val) { data_.push_back(val); }

  void Reserve(size_t size) override { data_.reserve(size); }

  void ShrinkToFit() override { data_.shrink_to_fit(); }

  void Resize(size_t size) { data_.resize(size); }

  void Clear() override { data_.clear(); }

  int64_t Bytes() const override;

  void AppendFromVector(const std::vector<T>& value_vector) {
    for (const auto& value : value_vector) {
      Append(value);
    }
  }

  // Return a new SharedColumnWrapper with values according to the spec:
  //    { data[idx[0]], data[idx[1]], data[idx[2]], ... }
  SharedColumnWrapper CopyIndexes(const std::vector<size_t>& indexes) const override {
    DCHECK_LE(indexes.size(), data_.size());
    auto copy = std::make_shared<ColumnWrapperTmpl<T>>(indexes.size());
    for (size_t i = 0; i < indexes.size(); ++i) {
      copy->data_[i] = data_[indexes[i]];
    }
    return copy;
  }

  // Return a new SharedColumnWrapper with values according to the spec:
  //    { data[idx[0]], data[idx[1]], data[idx[2]], ... }
  // Warning: Indexes in "this" ColumnWrapper have their contents moved,
  // so "this" should be discarded.
  SharedColumnWrapper MoveIndexes(const std::vector<size_t>& indexes) override {
    DCHECK_LE(indexes.size(), data_.size());
    auto col = std::make_shared<ColumnWrapperTmpl<T>>(indexes.size());
    for (size_t i = 0; i < indexes.size(); ++i) {
      col->data_[i] = std::move(data_[indexes[i]]);
    }
    return col;
  }

 private:
  std::vector<T> data_;
};

template <typename T>
int64_t ColumnWrapperTmpl<T>::Bytes() const {
  return Size() * sizeof(T);
}

template <>
inline int64_t ColumnWrapperTmpl<StringValue>::Bytes() const {
  int64_t bytes = 0;
  for (const auto& data : data_) {
    bytes += data.bytes();
  }
  return bytes;
}

// PL_CARNOT_UPDATE_FOR_NEW_TYPES.
using BoolValueColumnWrapper = ColumnWrapperTmpl<BoolValue>;
using Int64ValueColumnWrapper = ColumnWrapperTmpl<Int64Value>;
using UInt128ValueColumnWrapper = ColumnWrapperTmpl<UInt128Value>;
using Float64ValueColumnWrapper = ColumnWrapperTmpl<Float64Value>;
using StringValueColumnWrapper = ColumnWrapperTmpl<StringValue>;
using Time64NSValueColumnWrapper = ColumnWrapperTmpl<Time64NSValue>;

template <typename TColumnWrapper, types::DataType DType>
inline SharedColumnWrapper FromArrowImpl(const std::shared_ptr<arrow::Array>& arr) {
  CHECK_EQ(arr->type_id(), DataTypeTraits<DType>::arrow_type_id);
  size_t size = arr->length();
  auto wrapper = TColumnWrapper::Make(DType, size);
  auto arr_casted = static_cast<typename DataTypeTraits<DType>::arrow_array_type*>(arr.get());
  typename DataTypeTraits<DType>::value_type* out_data =
      static_cast<TColumnWrapper*>(wrapper.get())->UnsafeRawData();
  for (size_t i = 0; i < size; ++i) {
    out_data[i] = arr_casted->Value(i);
    // <Time64NSValue> = <int64_t>
  }
  return wrapper;
}

template <>
inline SharedColumnWrapper FromArrowImpl<Time64NSValueColumnWrapper, DataType::TIME64NS>(
    const std::shared_ptr<arrow::Array>& arr) {
  CHECK_EQ(arr->type_id(), DataTypeTraits<types::TIME64NS>::arrow_type_id);
  size_t size = arr->length();
  auto wrapper = StringValueColumnWrapper::Make(types::TIME64NS, size);
  auto arr_casted = static_cast<arrow::Int64Array*>(arr.get());
  Time64NSValue* out_data =
      static_cast<Time64NSValueColumnWrapper*>(wrapper.get())->UnsafeRawData();
  for (size_t i = 0; i < size; ++i) {
    out_data[i] = Time64NSValue(arr_casted->Value(i));
  }
  return wrapper;
}

template <>
inline SharedColumnWrapper FromArrowImpl<StringValueColumnWrapper, DataType::STRING>(
    const std::shared_ptr<arrow::Array>& arr) {
  CHECK_EQ(arr->type_id(), DataTypeTraits<types::STRING>::arrow_type_id);
  size_t size = arr->length();
  auto wrapper = StringValueColumnWrapper::Make(types::STRING, size);
  auto arr_casted = static_cast<arrow::StringArray*>(arr.get());
  StringValue* out_data = static_cast<StringValueColumnWrapper*>(wrapper.get())->UnsafeRawData();
  for (size_t i = 0; i < size; ++i) {
    out_data[i] = arr_casted->GetString(i);
  }
  return wrapper;
}

/**
 * Create a type erased ColumnWrapper from an ArrowArray.
 * @param arr the arrow array.
 * @return A shared_ptr to the ColumnWrapper.
 * PL_CARNOT_UPDATE_FOR_NEW_TYPES.
 */
inline SharedColumnWrapper ColumnWrapper::FromArrow(const std::shared_ptr<arrow::Array>& arr) {
  auto type_id = arr->type_id();
  switch (type_id) {
    case arrow::Type::BOOL:
      return FromArrowImpl<BoolValueColumnWrapper, DataType::BOOLEAN>(arr);
    case arrow::Type::INT64:
      return FromArrowImpl<Int64ValueColumnWrapper, DataType::INT64>(arr);
    case arrow::Type::UINT128:
      return FromArrowImpl<UInt128ValueColumnWrapper, DataType::UINT128>(arr);
    case arrow::Type::DOUBLE:
      return FromArrowImpl<Float64ValueColumnWrapper, DataType::FLOAT64>(arr);
    case arrow::Type::STRING:
      return FromArrowImpl<StringValueColumnWrapper, DataType::STRING>(arr);
    case arrow::Type::TIME64:
      return FromArrowImpl<Time64NSValueColumnWrapper, DataType::TIME64NS>(arr);
    case arrow::Type::DURATION:
      return FromArrowImpl<Time64NSValueColumnWrapper, DataType::TIME64NS>(arr);
    default:
      CHECK(0) << "Unknown arrow type: " << type_id;
  }
}

/**
 * Create a column wrapper.
 * @param data_type The UDFDataType
 * @param size The length of the column.
 * @return A shared_ptr to the ColumnWrapper.
 * PL_CARNOT_UPDATE_FOR_NEW_TYPES.
 */
inline SharedColumnWrapper ColumnWrapper::Make(DataType data_type, size_t size) {
  switch (data_type) {
    case DataType::BOOLEAN:
      return std::make_shared<BoolValueColumnWrapper>(size);
    case DataType::INT64:
      return std::make_shared<Int64ValueColumnWrapper>(size);
    case DataType::UINT128:
      return std::make_shared<UInt128ValueColumnWrapper>(size);
    case DataType::FLOAT64:
      return std::make_shared<Float64ValueColumnWrapper>(size);
    case DataType::STRING:
      return std::make_shared<StringValueColumnWrapper>(size);
    case DataType::TIME64NS:
      return std::make_shared<Time64NSValueColumnWrapper>(size);
    default:
      CHECK(0) << "Unknown data type";
  }
}

template <class TValueType>
inline void ColumnWrapper::Append(TValueType val) {
  CHECK_EQ(data_type(), ValueTypeTraits<TValueType>::data_type)
      << "Expect " << ToString(data_type()) << " got "
      << ToString(ValueTypeTraits<TValueType>::data_type);
  static_cast<ColumnWrapperTmpl<TValueType>*>(this)->Append(val);
}

template <class TValueType>
inline TValueType& ColumnWrapper::Get(size_t idx) {
  CHECK_EQ(data_type(), ValueTypeTraits<TValueType>::data_type);
  return static_cast<ColumnWrapperTmpl<TValueType>*>(this)->operator[](idx);
}

template <class TValueType>
inline TValueType ColumnWrapper::Get(size_t idx) const {
  CHECK_EQ(data_type(), ValueTypeTraits<TValueType>::data_type);
  return static_cast<const ColumnWrapperTmpl<TValueType>*>(this)->operator[](idx);
}

template <class TValueType>
inline void ColumnWrapper::AppendNoTypeCheck(TValueType val) {
  DCHECK_EQ(data_type(), ValueTypeTraits<TValueType>::data_type);
  static_cast<ColumnWrapperTmpl<TValueType>*>(this)->Append(val);
}

template <class TValueType>
inline TValueType& ColumnWrapper::GetNoTypeCheck(size_t idx) {
  DCHECK_EQ(data_type(), ValueTypeTraits<TValueType>::data_type);
  return static_cast<ColumnWrapperTmpl<TValueType>*>(this)->operator[](idx);
}

template <class TValueType>
inline TValueType ColumnWrapper::GetNoTypeCheck(size_t idx) const {
  DCHECK_EQ(data_type(), ValueTypeTraits<TValueType>::data_type);
  return static_cast<ColumnWrapperTmpl<TValueType>*>(this)->operator[](idx);
}

template <class TValueType>
inline void ColumnWrapper::AppendFromVector(const std::vector<TValueType>& val) {
  CHECK_EQ(data_type(), ValueTypeTraits<TValueType>::data_type)
      << "Expect " << ToString(data_type()) << " got "
      << ToString(ValueTypeTraits<TValueType>::data_type);
  static_cast<ColumnWrapperTmpl<TValueType>*>(this)->AppendFromVector(val);
}

template <DataType DT>
struct ColumnWrapperType {};

template <>
struct ColumnWrapperType<DataType::BOOLEAN> {
  using type = BoolValueColumnWrapper;
};

template <>
struct ColumnWrapperType<DataType::INT64> {
  using type = Int64ValueColumnWrapper;
};

template <>
struct ColumnWrapperType<DataType::UINT128> {
  using type = UInt128ValueColumnWrapper;
};

template <>
struct ColumnWrapperType<DataType::FLOAT64> {
  using type = Float64ValueColumnWrapper;
};

template <>
struct ColumnWrapperType<DataType::TIME64NS> {
  using type = Time64NSValueColumnWrapper;
};

template <>
struct ColumnWrapperType<DataType::STRING> {
  using type = StringValueColumnWrapper;
};

template <types::DataType DT>
void ExtractValueToColumnWrapper(ColumnWrapper* wrapper, arrow::Array* arr, int64_t row_idx) {
  static_cast<typename ColumnWrapperType<DT>::type*>(wrapper)->Append(
      types::GetValueFromArrowArray<DT>(arr, row_idx));
}

}  // namespace types
}  // namespace px
