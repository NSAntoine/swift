//===--- ImageInspection.h - Image inspection routines ----------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
///
/// \file
///
/// This file includes routines that extract metadata from executable and
/// dynamic library image files generated by the Swift compiler. The concrete
/// implementations vary greatly by platform.
///
//===----------------------------------------------------------------------===//

#ifndef SWIFT_RUNTIME_IMAGEINSPECTION_H
#define SWIFT_RUNTIME_IMAGEINSPECTION_H

#include "swift/Runtime/Config.h"

#include <cstdint>
#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

namespace swift {

/// This is a platform independent version of Dl_info from dlfcn.h
#if defined(__cplusplus)

template <typename T>
struct null_deleter {
  void operator()(T *) const {}
  void operator()(typename std::remove_cv<T>::type *value) const {}
};

template <typename T>
struct free_deleter {
  void operator()(T *value) const {
    free(const_cast<typename std::remove_cv<T>::type *>(value));
  }
  void operator()(typename std::remove_cv<T>::type *value) const {
    free(value);
  }
};

struct SymbolInfo {
  const char *fileName;
  void *baseAddress;
#if defined(_WIN32)
  std::unique_ptr<const char, free_deleter<const char>> symbolName;
#else
  std::unique_ptr<const char, null_deleter<const char>> symbolName;
#endif
  void *symbolAddress;
};
#endif

/// Load the metadata from the image necessary to find protocols by name.
void initializeProtocolLookup();

/// Load the metadata from the image necessary to find a type's
/// protocol conformance.
void initializeProtocolConformanceLookup();

/// Load the metadata from the image necessary to find a type by name.
void initializeTypeMetadataRecordLookup();

/// Load the metadata from the image necessary to perform dynamic replacements.
void initializeDynamicReplacementLookup();

/// Load the metadata from the image necessary to find functions by name.
void initializeAccessibleFunctionsLookup();

// Callbacks to register metadata from an image to the runtime.
void addImageProtocolsBlockCallback(const void *baseAddress,
                                    const void *start, uintptr_t size);
void addImageProtocolsBlockCallbackUnsafe(const void *baseAddress,
                                          const void *start, uintptr_t size);
void addImageProtocolConformanceBlockCallback(const void *baseAddress,
                                              const void *start,
                                              uintptr_t size);
void addImageProtocolConformanceBlockCallbackUnsafe(const void *baseAddress,
                                                    const void *start,
                                                    uintptr_t size);
void addImageTypeMetadataRecordBlockCallback(const void *baseAddress,
                                             const void *start,
                                             uintptr_t size);
void addImageTypeMetadataRecordBlockCallbackUnsafe(const void *baseAddress,
                                                   const void *start,
                                                   uintptr_t size);
void addImageDynamicReplacementBlockCallback(const void *baseAddress,
                                             const void *start, uintptr_t size,
                                             const void *start2,
                                             uintptr_t size2);
void addImageAccessibleFunctionsBlockCallback(const void *baseAddress,
                                              const void *start,
                                              uintptr_t size);
void addImageAccessibleFunctionsBlockCallbackUnsafe(const void *baseAddress,
                                                    const void *start,
                                                    uintptr_t size);

int lookupSymbol(const void *address, SymbolInfo *info);

#if defined(_WIN32)
/// Configure the environment to allow calling into the Debug Help library.
///
/// \param body A function to invoke. This function attempts to first initialize
///   the Debug Help library. If it did so successfully, the handle used during
///   initialization is passed to this function and should be used with
///   subsequent calls to the Debug Help library. Do not close this handle.
/// \param context A caller-supplied value to pass to \a body.
///
/// On Windows, the Debug Help library (DbgHelp.lib) is not thread-safe. All
/// calls into it from the Swift runtime and stdlib should route through this
/// function.
///
/// This function sets the Debug Help library's options by calling
/// \c SymSetOptions() before \a body is invoked, and then resets them back to
/// their old value before returning. \a body can also call \c SymSetOptions()
/// if needed.
SWIFT_RUNTIME_STDLIB_SPI
void _swift_withWin32DbgHelpLibrary(
  void (* body)(HANDLE hProcess, void *context), void *context);

/// Configure the environment to allow calling into the Debug Help library.
///
/// \param body A function to invoke. This function attempts to first initialize
///   the Debug Help library. If it did so successfully, the handle used during
///   initialization is passed to this function and should be used with
///   subsequent calls to the Debug Help library. Do not close this handle.
///
/// On Windows, the Debug Help library (DbgHelp.lib) is not thread-safe. All
/// calls into it from the Swift runtime and stdlib should route through this
/// function.
///
/// This function sets the Debug Help library's options by calling
/// \c SymSetOptions() before \a body is invoked, and then resets them back to
/// their old value before returning. \a body can also call \c SymSetOptions()
/// if needed.
static inline void _swift_withWin32DbgHelpLibrary(
  const std::function<void(HANDLE /*hProcess*/)> &body) {
  _swift_withWin32DbgHelpLibrary([](HANDLE hProcess, void *context) {
    auto bodyp = reinterpret_cast<std::function<void(bool)> *>(context);
    (* bodyp)(hProcess);
  }, const_cast<void *>(reinterpret_cast<const void *>(&body)));
}

/// Configure the environment to allow calling into the Debug Help library.
///
/// \param body A function to invoke. This function attempts to first initialize
///   the Debug Help library. If it did so successfully, the handle used during
///   initialization is passed to this function and should be used with
///   subsequent calls to the Debug Help library. Do not close this handle.
///
/// \returns Whatever is returned from \a body.
///
/// On Windows, the Debug Help library (DbgHelp.lib) is not thread-safe. All
/// calls into it from the Swift runtime and stdlib should route through this
/// function.
///
/// This function sets the Debug Help library's options by calling
/// \c SymSetOptions() before \a body is invoked, and then resets them back to
/// their old value before returning. \a body can also call \c SymSetOptions()
/// if needed.
template <
  typename F,
  typename R = typename std::result_of_t<F&(HANDLE /*hProcess*/)>,
  typename = typename std::enable_if_t<!std::is_same<void, R>::value>
>
static inline R _swift_withWin32DbgHelpLibrary(const F& body) {
  R result;

  _swift_withWin32DbgHelpLibrary([&body, &result] (HANDLE hProcess) {
    result = body(hProcess);
  });

  return result;
}
#endif

} // end namespace swift

#endif
