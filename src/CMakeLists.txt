# force define a SHARED target for whl, caused by when build for APPLE we will force set
# BUILD_SHARED_LIBS=OFF for xcode needed
set(MGE_SHARED_LIB megengine_shared)
set(MGE_SHARED_LIB
    ${MGE_SHARED_LIB}
    PARENT_SCOPE)

if(MGE_WITH_JIT_MLIR)
  add_subdirectory(jit/include/megbrain/jit/mlir/ir)
endif()

file(
  GLOB_RECURSE
  SOURCES
  core/impl/*.cpp
  gopt/impl/*.cpp
  opr/impl/*.cpp
  opr/impl/nvof/*.cpp
  plugin/impl/*.cpp
  serialization/impl/*.cpp
  rdnn/impl/*.cpp
  core/impl/*.inl
  gopt/impl/*.inl
  opr/impl/*.inl
  plugin/impl/*.inl
  serialization/impl/*.inl)

if(MGE_WITH_JIT)
  file(GLOB_RECURSE SOURCES_ jit/impl/*.cpp jit/impl/*.inl)
  if(MGE_WITH_JIT_MLIR)
    file(GLOB_RECURSE MLIR_SOURCES_ jit/impl/mlir/ir/*.cpp jit/impl/mlir/*.cpp)
    list(APPEND SOURCES_ ${MLIR_SOURCES_})
  endif()
  list(APPEND SOURCES ${SOURCES_})
endif()

if(MGE_WITH_DISTRIBUTED)
  file(GLOB_RECURSE SOURCES_ opr-mm/impl/*.cpp opr-mm/impl/*.inl)
  list(APPEND SOURCES ${SOURCES_})
  file(
    GLOB_RECURSE PROTO_FILES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "../src/opr-mm/proto/*.proto")
  protobuf_generate_cpp_with_root(GRPC_SRCS GRPC_HDRS ${CMAKE_CURRENT_SOURCE_DIR}
                                  ${PROTO_FILES})
  add_custom_target(mgb_proto_target DEPENDS ${GRPC_SRCS} ${GRPC_HDRS}
                                             ${PROTOBUF_PROTOC_EXECUTABLE})
  list(APPEND SOURCES ${GRPC_SRCS})
endif()

set(MGB_INC
    ${PROJECT_BINARY_DIR}/genfiles
    ${CMAKE_CURRENT_LIST_DIR}/core/include
    ${CMAKE_CURRENT_LIST_DIR}/gopt/include
    ${CMAKE_CURRENT_LIST_DIR}/opr/include
    ${CMAKE_CURRENT_LIST_DIR}/plugin/include
    ${CMAKE_CURRENT_LIST_DIR}/serialization/include
    ${CMAKE_CURRENT_LIST_DIR}/rdnn/include)

if(MGE_WITH_JIT)
  list(APPEND MGB_INC ${CMAKE_CURRENT_LIST_DIR}/jit/include)
  if(MGE_WITH_CUDA)
    list(APPEND MGB_INC ${CMAKE_CURRENT_LIST_DIR}/jit/impl/cuda)
  endif()
endif()

if(MGE_WITH_DISTRIBUTED)
  list(APPEND MGB_INC ${CMAKE_CURRENT_LIST_DIR}/opr-mm/include)
endif()

if(MGE_WITH_CUDA AND MGE_WITH_TRT)
  list(APPEND MGB_INC ${CMAKE_CURRENT_LIST_DIR}/tensorrt/include)
  file(GLOB_RECURSE SOURCES_ tensorrt/impl/*.cpp tensorrt/impl/*.inl)
  list(APPEND SOURCES ${SOURCES_})
endif()
if(MGE_WITH_CAMBRICON)
  list(APPEND MGB_INC ${CMAKE_CURRENT_LIST_DIR}/cambricon/include)
  file(GLOB_RECURSE SOURCES_ cambricon/impl/*.cpp cambricon/impl/*.inl)
  list(APPEND SOURCES ${SOURCES_})
endif()
set(MGB_CAMBRICON ${MGE_WITH_CAMBRICON})

set(MGB_ATLAS ${MGE_WITH_ATLAS})

if(MGE_WITH_CUDA)
  file(GLOB_RECURSE SOURCES_ opr/impl/standalone/*.cu)
  list(APPEND SOURCES ${SOURCES_})
endif()

if(MGE_WITH_CUSTOM_OP)
  list(APPEND MGB_INC ${CMAKE_CURRENT_LIST_DIR}/custom/include)
  file(GLOB_RECURSE SOURCES_ custom/impl/*.cpp)
  list(APPEND SOURCES ${SOURCES_})
endif()

add_library(megbrain OBJECT ${SOURCES})
target_link_libraries(megbrain PUBLIC mgb_opr_param_defs)
if(MGE_WITH_CUDA)
  target_include_directories(megbrain PUBLIC ${TRT_INCLUDE_DIR})
  target_include_directories(megbrain PRIVATE ${CUDNN_INCLUDE_DIR})
  find_path(
    NVTX3_INCLUDE
    NAMES nvToolsExtCudaRt.h
    HINTS $ENV{CUDA_ROOT_DIR} $ENV{CUDA_PATH} $ENV{CUDA_BIN_PATH}
    PATH_SUFFIXES include/nvtx3
    DOC "NVTX3_INCLUDE")
  if(NVTX3_INCLUDE STREQUAL "NVTX3_INCLUDE-NOTFOUND")
    message(
      FATAL_ERROR
        "Can not find NVTX3 INCLUDE, please export cuda sdk path to CUDA_ROOT_DIR or CUDA_PATH or CUDA_BIN_PATH"
    )
  endif()
  target_include_directories(megbrain PRIVATE ${NVTX3_INCLUDE})
endif()
target_include_directories(
  megbrain
  PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE ${PROJECT_SOURCE_DIR}/third_party/midout/src)

foreach(INCPATH IN LISTS MGB_INC)
  target_include_directories(megbrain PUBLIC $<BUILD_INTERFACE:${INCPATH}>)
endforeach()

if(MGE_WITH_CUDA)
  if(NOT WIN32 AND NOT MSVC)
    target_compile_options(
      megbrain PRIVATE "$<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=-Wno-unused-parameter>"
                       "$<$<NOT:$<COMPILE_LANGUAGE:CUDA>>:-Wno-unused-parameter>")
  endif()
else()
  target_compile_options(megbrain PRIVATE "-Wno-unused-parameter")
endif()
if(CXX_SUPPORT_WCLASS_MEMACCESS)
  if(MGE_WITH_CUDA)
    target_compile_options(
      megbrain PRIVATE "$<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=-Wno-class-memaccess>"
                       "$<$<NOT:$<COMPILE_LANGUAGE:CUDA>>:-Wno-class-memaccess>")
  else()
    target_compile_options(megbrain PRIVATE "-Wno-class-memaccess")
  endif()
endif()
target_link_libraries(megbrain PUBLIC megdnn)
if(MGE_WITH_DISTRIBUTED)
  add_dependencies(megbrain mgb_proto_target)
  target_link_libraries(megbrain PRIVATE libprotobuf libzmq)
  set(CPPZMQ_INC ${PROJECT_SOURCE_DIR}/third_party/cppzmq)
  # FIXME: add CMAKE_CURRENT_BINARY_DIR for including mm_handler.pb.h
  target_include_directories(megbrain PRIVATE ${CPPZMQ_INC} ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(megbrain PRIVATE megray)
endif()
target_link_libraries(megbrain PUBLIC ${MGE_CAMBRICON_LIBS})
target_link_libraries(megbrain PUBLIC ${MGE_ATLAS_LIBS})
if(MGE_WITH_JIT AND MGE_WITH_HALIDE)
  target_link_libraries(megbrain PRIVATE libhalide)
  target_link_libraries(megbrain PRIVATE ${HALIDE_LLVM_LIBS})
endif()
if(MGE_WITH_JIT_MLIR)
  target_include_directories(megbrain PRIVATE ${MLIR_LLVM_INCLUDE_DIR})
  target_link_libraries(megbrain PRIVATE ${MLIR_LLVM_LIBS})
  add_dependencies(megbrain mgb_dialect)
  target_include_directories(megbrain PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/jit/include)
endif()
if(MGB_WITH_FLATBUFFERS)
  set(GEN_FLATBUFFERS_SCHEMA_PY
      ${PROJECT_SOURCE_DIR}/dnn/scripts/gen_flatbuffers_schema.py)
  set(OPR_PARAM_DEFS_PY ${PROJECT_SOURCE_DIR}/dnn/scripts/opr_param_defs.py)
  set(MGB_PARAM_DEFS_PY ${PROJECT_SOURCE_DIR}/tools/param_defs/mgb_opr_param_defs.py)
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/serialization/impl)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/serialization/impl/opr_param_defs.fbs
    COMMAND ${PYTHON_EXECUTABLE} ${GEN_FLATBUFFERS_SCHEMA_PY} ${OPR_PARAM_DEFS_PY}
            ${CMAKE_CURRENT_BINARY_DIR}/serialization/impl/opr_param_defs.fbs
    DEPENDS ${GEN_FLATBUFFERS_SCHEMA_PY} ${OPR_PARAM_DEFS_PY}
    VERBATIM)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/serialization/impl/mgb_opr_param_defs.fbs
    COMMAND ${PYTHON_EXECUTABLE} ${GEN_FLATBUFFERS_SCHEMA_PY} ${MGB_PARAM_DEFS_PY}
            ${CMAKE_CURRENT_BINARY_DIR}/serialization/impl/mgb_opr_param_defs.fbs
    DEPENDS ${GEN_FLATBUFFERS_SCHEMA_PY} ${MGB_PARAM_DEFS_PY}
    VERBATIM)
  list(
    APPEND
    FLATBUFFERS_SCHEMA_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/serialization/impl/dtype.fbs
    ${CMAKE_CURRENT_BINARY_DIR}/serialization/impl/opr_param_defs.fbs
    ${CMAKE_CURRENT_BINARY_DIR}/serialization/impl/mgb_opr_param_defs.fbs
    ${CMAKE_CURRENT_SOURCE_DIR}/opr/impl/mgb_cpp_opr.fbs
    ${CMAKE_CURRENT_SOURCE_DIR}/serialization/impl/schema.fbs)
  list(
    APPEND FLATBUFFERS_SCHEMA_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/serialization/impl
    ${CMAKE_CURRENT_BINARY_DIR}/serialization/impl ${CMAKE_CURRENT_SOURCE_DIR}/opr/impl)
  build_flatbuffers(
    "${FLATBUFFERS_SCHEMA_FILES}"
    "${FLATBUFFERS_SCHEMA_INCLUDE_DIR}"
    mgb_serialization_schema_fbs
    "${FLATBUFFERS_SCHEMA_FILES}"
    "${CMAKE_CURRENT_BINARY_DIR}/serialization/include/megbrain/serialization/internal"
    ""
    "")
  add_dependencies(megbrain mgb_serialization_schema_fbs)
  target_include_directories(megbrain
                             PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/serialization/include)
  target_compile_definitions(megbrain PUBLIC MGB_ENABLE_FBS_SERIALIZATION=1)
  target_link_libraries(megbrain PUBLIC flatbuffers)
  set(GENERATED_FLATBUFFERS_CONVERTER_PATH ${CMAKE_CURRENT_BINARY_DIR}/genfiles)
  set(GEN_FLATBUFFERS_CONVERTER_PY
      ${PROJECT_SOURCE_DIR}/dnn/scripts/gen_flatbuffers_converter.py)
  file(MAKE_DIRECTORY ${GENERATED_FLATBUFFERS_CONVERTER_PATH})
  add_custom_command(
    OUTPUT ${GENERATED_FLATBUFFERS_CONVERTER_PATH}/opr_param_defs_converter.inl
    COMMAND ${PYTHON_EXECUTABLE} ${GEN_FLATBUFFERS_CONVERTER_PY} ${OPR_PARAM_DEFS_PY}
            ${GENERATED_FLATBUFFERS_CONVERTER_PATH}/opr_param_defs_converter.inl
    DEPENDS ${GEN_FLATBUFFERS_CONVERTER_PY} ${OPR_PARAM_DEFS_PY}
    VERBATIM)
  add_custom_command(
    OUTPUT ${GENERATED_FLATBUFFERS_CONVERTER_PATH}/mgb_opr_param_defs_converter.inl
    COMMAND ${PYTHON_EXECUTABLE} ${GEN_FLATBUFFERS_CONVERTER_PY} ${MGB_PARAM_DEFS_PY}
            ${GENERATED_FLATBUFFERS_CONVERTER_PATH}/mgb_opr_param_defs_converter.inl
    DEPENDS ${GEN_FLATBUFFERS_CONVERTER_PY} ${MGB_PARAM_DEFS_PY}
    VERBATIM)
  target_sources(
    megbrain
    PRIVATE ${GENERATED_FLATBUFFERS_CONVERTER_PATH}/opr_param_defs_converter.inl)
  target_sources(
    megbrain
    PRIVATE ${GENERATED_FLATBUFFERS_CONVERTER_PATH}/mgb_opr_param_defs_converter.inl)
  target_include_directories(megbrain PRIVATE ${GENERATED_FLATBUFFERS_CONVERTER_PATH})
endif()
if(UNIX
   AND NOT ANDROID
   AND NOT APPLE)
  target_link_libraries(megbrain PUBLIC dl rt atomic)
endif()

if(ANDROID)
  target_link_libraries(megbrain PUBLIC log)
endif()

set(_VER_FILE ${PROJECT_SOURCE_DIR}/src/version.ld)

# Build as SHARED or STATIC depending on BUILD_SHARED_LIBS=ON/OFF
add_library(megengine)
add_library(${MGE_SHARED_LIB} SHARED)
target_link_libraries(megengine PRIVATE ${MGE_CUDA_LIBS})
target_link_libraries(megengine PUBLIC megbrain megdnn)
target_link_libraries(${MGE_SHARED_LIB} PUBLIC megbrain megdnn)
target_link_libraries(${MGE_SHARED_LIB} PRIVATE ${MGE_CUDA_LIBS})
if(UNIX AND NOT APPLE)
  target_link_options(megengine PRIVATE -Wl,--no-undefined
                      -Wl,--version-script=${_VER_FILE})
  set_target_properties(megengine PROPERTIES LINK_DEPENDS ${_VER_FILE})
  target_link_options(${MGE_SHARED_LIB} PRIVATE -Wl,--no-undefined
                      -Wl,--version-script=${_VER_FILE})
  set_target_properties(${MGE_SHARED_LIB} PROPERTIES LINK_DEPENDS ${_VER_FILE})
endif()
if(WIN32 OR MSVC)
  target_compile_definitions(megbrain PRIVATE MGE_DLL_EXPORT)
  target_compile_definitions(megdnn PRIVATE MGE_DLL_EXPORT)
  target_compile_definitions(megengine PRIVATE MGE_DLL_EXPORT)
  target_compile_definitions(${MGE_SHARED_LIB} PRIVATE MGE_DLL_EXPORT)
  # please do not use WINDOWS_EXPORT_ALL_SYMBOLS, as symbols max than 65535 when build
  # with CUDA set_target_properties(megengine PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS
  # TRUE) set_target_properties(${MGE_SHARED_LIB} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS
  # TRUE)
endif()
if(MGE_WITH_DISTRIBUTED)
  message(VERBOSE "megengine configured to link megray")
  target_link_libraries(megengine PUBLIC megray)
  target_link_libraries(${MGE_SHARED_LIB} PUBLIC megray)
endif()
# Do not export targets if MGE_WITH_DISTRIBUTED is on. MegRay is not ready for this.
install(
  TARGETS megengine
  EXPORT ${MGE_EXPORT_TARGETS}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(NOT MGE_WITH_DISTRIBUTED)
  install(TARGETS megbrain EXPORT ${MGE_EXPORT_TARGETS})
endif()

foreach(_PATH ${MGB_INC})
  install(
    DIRECTORY ${_PATH}/megbrain
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
    PATTERN "*.h")
endforeach()
