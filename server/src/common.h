// Copyright 2022, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#pragma once

#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <vector>
#include "triton/core/tritonserver.h"
#ifdef TRITON_ENABLE_GPU
#include <cuda_runtime_api.h>
#endif  // TRITON_ENABLE_GPU

namespace triton { namespace server { namespace wrapper {

#define FAIL_IF_TRITON_ERR(X, MSG)                                \
  do {                                                            \
    TRITONSERVER_Error* err__ = (X);                              \
    if (err__ != nullptr) {                                       \
      std::cerr << "error: " << (MSG) << ": "                     \
                << TRITONSERVER_ErrorCodeString(err__) << " - "   \
                << TRITONSERVER_ErrorMessage(err__) << std::endl; \
      TRITONSERVER_ErrorDelete(err__);                            \
      exit(1);                                                    \
    }                                                             \
  } while (false)
#define FAIL_IF_ERR(X, MSG)                                        \
  {                                                                \
    Error err = (X);                                               \
    if (!err.IsOk()) {                                             \
      std::cerr << "error: " << (MSG) << ": " << err << std::endl; \
      exit(1);                                                     \
    }                                                              \
  }
#define RETURN_ERR_IF_TRITON_ERR(X)                                \
  do {                                                             \
    TRITONSERVER_Error* err__ = (X);                               \
    if (err__ != nullptr) {                                        \
      Error err = Error(                                           \
          TRITONSERVER_ErrorCodeString(err__) + std::string("-") + \
          TRITONSERVER_ErrorMessage(err__) + "\n");                \
      TRITONSERVER_ErrorDelete(err__);                             \
      return err;                                                  \
    }                                                              \
  } while (false)
#define IGNORE_ERROR(X)                   \
  do {                                    \
    TRITONSERVER_Error* ie_err__ = (X);   \
    if (ie_err__ != nullptr) {            \
      TRITONSERVER_ErrorDelete(ie_err__); \
    }                                     \
  } while (false)
#define RETURN_IF_ERR(X)           \
  {                                \
    Error err = (X);               \
    if (!err.IsOk()) {             \
      return Error(err.Message()); \
    }                              \
  }
#define LOG_IF_ERROR(X, MSG)                                                   \
  do {                                                                         \
    TRITONSERVER_Error* lie_err__ = (X);                                       \
    if (lie_err__ != nullptr) {                                                \
      IGNORE_ERROR(TRITONSERVER_LogMessage(                                    \
          TRITONSERVER_LOG_ERROR, __FILE__, __LINE__,                          \
          (std::string(MSG) + ": " + TRITONSERVER_ErrorCodeString(lie_err__) + \
           " - " + TRITONSERVER_ErrorMessage(lie_err__))                       \
              .c_str()));                                                      \
      TRITONSERVER_ErrorDelete(lie_err__);                                     \
    }                                                                          \
  } while (false)
#define LOG_MESSAGE(LEVEL, MSG)                                  \
  do {                                                           \
    LOG_IF_ERROR(                                                \
        TRITONSERVER_LogMessage(LEVEL, __FILE__, __LINE__, MSG), \
        ("failed to log message: "));                            \
  } while (false)
#define THROW_IF_ERR(X)                            \
  do {                                             \
    Error err = (X);                               \
    if (!err.IsOk()) {                             \
      throw ServerWrapperException(err.Message()); \
    }                                              \
  } while (false)
#define THROW_IF_TRITON_ERR(X)                                     \
  do {                                                             \
    TRITONSERVER_Error* err__ = (X);                               \
    if (err__ != nullptr) {                                        \
      ServerWrapperException ex(                                   \
          TRITONSERVER_ErrorCodeString(err__) + std::string("-") + \
          TRITONSERVER_ErrorMessage(err__) + "\n");                \
      TRITONSERVER_ErrorDelete(err__);                             \
      throw ex;                                                    \
    }                                                              \
  } while (false)
#define THROW_ERR_IF_TRITON_ERR(X)                                 \
  do {                                                             \
    TRITONSERVER_Error* err__ = (X);                               \
    if (err__ != nullptr) {                                        \
      Error err = Error(                                           \
          TRITONSERVER_ErrorCodeString(err__) + std::string("-") + \
          TRITONSERVER_ErrorMessage(err__) + "\n");                \
      TRITONSERVER_ErrorDelete(err__);                             \
      throw ServerWrapperException(err.Message());                 \
    }                                                              \
  } while (false)
#define RETURN_TRITON_ERR_IF_ERR(X)                              \
  do {                                                           \
    Error err = (X);                                             \
    if (!err.IsOk()) {                                           \
      return TRITONSERVER_ErrorNew(                              \
          TRITONSERVER_ERROR_INTERNAL, (err.Message()).c_str()); \
    }                                                            \
  } while (false)

//==============================================================================
enum ModelControlMode {
  MODEL_CONTROL_NONE,
  MODEL_CONTROL_POLL,
  MODEL_CONTROL_EXPLICIT
};
enum MemoryType { CPU, CPU_PINNED, GPU };
enum LogFormat { LOG_DEFAULT, LOG_ISO8601 };
enum DataType {
  INVALID,
  BOOL,
  UINT8,
  UINT16,
  UINT32,
  UINT64,
  INT8,
  INT16,
  INT32,
  INT64,
  FP16,
  FP32,
  FP64,
  BYTES,
  BF16
};
enum ModelReadyState { UNKNOWN, READY, UNAVAILABLE, LOADING, UNLOADING };

//==============================================================================
// ServerWrapperException
//
struct ServerWrapperException : std::exception {
  ServerWrapperException(const std::string& message) : message_(message) {}

  const char* what() const throw() { return message_.c_str(); }

  std::string message_;
};

//==============================================================================
/// Structure to hold response parameters for InfeResult object. The kinds
/// of parameters in a response can be created by the backend side using
/// 'TRITONBACKEND_ResponseSet*Parameter' APIs.
/// See here for more information:
/// https://github.com/triton-inference-server/backend/tree/main/examples#add-key-value-parameters-to-a-response
struct ResponseParameters {
  explicit ResponseParameters(
      const char* name, TRITONSERVER_ParameterType type, const void* vvalue)
      : name_(name), type_(type), vvalue_(vvalue)
  {
  }

  // The name of the parameter.
  const char* name_;
  // The type of the parameter. Valid types are TRITONSERVER_PARAMETER_STRING,
  // TRITONSERVER_PARAMETER_INT, TRITONSERVER_PARAMETER_BOOL, and
  // TRITONSERVER_PARAMETER_BYTES.
  TRITONSERVER_ParameterType type_;
  // The pointer to the parameter value.
  const void* vvalue_;
};

//==============================================================================
/// Error status reported by server C++ API.
///
class Error {
 public:
  /// Create an error with the specified message.
  /// \param msg The message for the error
  explicit Error(const std::string& msg = "");

  /// Accessor for the message of this error.
  /// \return The messsage for the error. Empty if no error.
  const std::string& Message() const { return msg_; }

  /// Does this error indicate OK status?
  /// \return True if this error indicates "ok"/"success", false if
  /// error indicates a failure.
  bool IsOk() const { return msg_.empty(); }

  /// Convenience "success" value. Can be used as Error::Success to
  /// indicate no error.
  static const Error Success;

 private:
  friend std::ostream& operator<<(std::ostream&, const Error&);
  std::string msg_;
};

//==============================================================================
/// Custom Response Allocator Callback function signatures.
///
using ResponseAllocatorAllocFn_t = Error (*)(
    const char* tensor_name, size_t byte_size, MemoryType preferred_memory_type,
    int64_t preferred_memory_type_id, void** buffer,
    MemoryType* actual_memory_type, int64_t* actual_memory_type_id);
using OutputBufferReleaseFn_t = Error (*)(
    void* buffer, size_t byte_size, MemoryType memory_type,
    int64_t memory_type_id);
using ResponseAllocatorStartFn_t = Error (*)(void* userp);

//==============================================================================
/// Helper functions.
///
Error WrapperToTritonModelControlMode(
    TRITONSERVER_ModelControlMode* model_control_mode,
    const ModelControlMode& mode);
Error WrapperToTritonLogFormat(
    TRITONSERVER_LogFormat* log_format, const LogFormat& format);
TRITONSERVER_DataType WrapperToTritonDataType(const DataType& dtype);
DataType TritonToWrapperDataType(const TRITONSERVER_DataType& dtype);
Error WrapperToTritonMemoryType(
    TRITONSERVER_MemoryType* memory_type, const MemoryType& mem_type);
Error TritonToWrapperMemoryType(
    MemoryType* memory_type, const TRITONSERVER_MemoryType& mem_type);
ModelReadyState StringToWrapperModelReadyState(const std::string& state);

//==============================================================================
/// An InferRequestedOutput object is used to describe the requested model
/// output for inference.
///
class InferRequestedOutput {
 public:
  /// Create an InferRequestedOutput instance that describes a model output
  /// being requested.
  /// \param name The name of output being requested.
  /// \return Returns a new InferRequestedOutput object.
  static std::unique_ptr<InferRequestedOutput> Create(const std::string& name)
  {
    return std::unique_ptr<InferRequestedOutput>(
        new InferRequestedOutput(name));
  }

  /// Create a InferRequestedOutput instance that describes a model output being
  /// requested with pre-allocated output buffer.
  /// \param[out] infer_output Returns a new InferRequestedOutput object.
  /// \param name The name of output being requested.
  /// \param buffer The pointer to the start of the pre-allocated buffer.
  /// \param byte_size The size of buffer in bytes.
  /// \param memory_type The memory type of the output.
  /// \param memory_type_id The memory type id of the output.
  /// \return Error object indicating success or failure.
  static Error Create(
      std::unique_ptr<InferRequestedOutput>& infer_output,
      const std::string& name, const char* buffer, size_t byte_size,
      MemoryType memory_type, int64_t memory_type_id)
  {
    TRITONSERVER_MemoryType output_memory_type;
    RETURN_IF_ERR(WrapperToTritonMemoryType(&output_memory_type, memory_type));
    infer_output.reset(new InferRequestedOutput(
        name, buffer, byte_size, output_memory_type, memory_type_id));
    return Error::Success;
  }

  /// Gets name of the associated output tensor.
  /// \return The name of the tensor.
  const std::string& Name() const { return name_; }

  /// Gets buffer of the associated output tensor.
  /// \return The name of the tensor.
  const char* Buffer() { return buffer_; }

  /// Gets byte size of the associated output tensor.
  /// \return The name of the tensor.
  size_t ByteSize() { return byte_size_; }

  /// Gets the memory type of the output tensor.
  /// \return The memory type of the tensor.
  const TRITONSERVER_MemoryType& MemoryType() const { return memory_type_; }

  /// Gets the memory type id of the output tensor.
  /// \return The memory type id of the tensor.
  const int64_t& MemoryTypeId() const { return memory_type_id_; }

  InferRequestedOutput(const std::string& name)
      : name_(name), buffer_(nullptr), byte_size_(0),
        memory_type_(TRITONSERVER_MEMORY_CPU), memory_type_id_(0)
  {
  }

  InferRequestedOutput(
      const std::string& name, const char* buffer, size_t byte_size,
      TRITONSERVER_MemoryType memory_type, int64_t memory_type_id)
      : name_(name), buffer_(buffer), byte_size_(byte_size),
        memory_type_(memory_type), memory_type_id_(memory_type_id)
  {
  }

 private:
  std::string name_;
  const char* buffer_;
  size_t byte_size_;
  TRITONSERVER_MemoryType memory_type_;
  int64_t memory_type_id_;
};

}}}  // namespace triton::server::wrapper
