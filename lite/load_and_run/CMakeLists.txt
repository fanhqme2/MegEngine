# BUILD the load and run for lite
include_directories(PUBLIC
                    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lite/load_and_run/src>)
file(GLOB_RECURSE SOURCES ./*.cpp ${PROJECT_SOURCE_DIR}/lite/src/pack_model/*.cpp)

add_executable(load_and_run ${SOURCES})
target_link_libraries(load_and_run lite_static)
target_link_libraries(load_and_run megbrain)
if(APPLE)
  target_link_libraries(load_and_run gflags)
else()
  target_link_libraries(load_and_run gflags -Wl,--version-script=${MGE_VERSION_SCRIPT})
endif()

if(LITE_BUILD_WITH_RKNPU)
  # rknn sdk1.0.0 depend on libc++_shared, use gold to remove NEEDED so symbol check
  target_link_options(load_and_run PRIVATE "-fuse-ld=gold")
endif()

if(MGE_WITH_ROCM)
  # FIXME: hip obj can not find cpp obj only through lite_static
  target_link_libraries(load_and_run megdnn)
endif()

if(UNIX)
  if(APPLE OR ANDROID)
    target_link_libraries(load_and_run dl)
  else()
    target_link_libraries(load_and_run dl rt)
  endif()
endif()

if(LITE_BUILD_WITH_MGE
   AND LITE_WITH_CUDA
   AND NOT WIN32)
  # FXIME third_party cpp redis do not support build with clang-cl
  target_include_directories(load_and_run PRIVATE ${CPP_REDIS_INCLUDES})
endif()

install(
  TARGETS load_and_run
  EXPORT ${LITE_EXPORT_TARGETS}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
if(BUILD_SHARED_LIBS)
  if(LITE_BUILD_WITH_MGE
     AND LITE_WITH_CUDA
     AND NOT WIN32)
    # FXIME third_party cpp redis do not support build with clang-cl
    list(APPEND SOURCES ${CPP_REDIS_SRCS})
  endif()
  add_executable(load_and_run_depends_shared ${SOURCES})
  target_link_libraries(load_and_run_depends_shared lite_shared)
  target_link_libraries(load_and_run_depends_shared gflags)
  target_link_libraries(load_and_run_depends_shared megengine)

  if(LITE_BUILD_WITH_RKNPU)
    # rknn sdk1.0.0 depend on libc++_shared, use gold to remove NEEDED so symbol check
    target_link_options(load_and_run_depends_shared PRIVATE "-fuse-ld=gold")
  endif()

  if(MGE_WITH_ROCM)
    # FIXME: hip obj can not find cpp obj only through lite_static
    target_link_libraries(load_and_run_depends_shared megdnn)
  endif()

  if(UNIX)
    if(APPLE OR ANDROID)
      target_link_libraries(load_and_run_depends_shared dl)
    else()
      target_link_libraries(load_and_run_depends_shared dl rt)
    endif()
  endif()

  if(LITE_BUILD_WITH_MGE
     AND LITE_WITH_CUDA
     AND NOT WIN32)
    # FXIME third_party cpp redis do not support build with clang-cl
    target_include_directories(load_and_run_depends_shared
                               PRIVATE ${CPP_REDIS_INCLUDES})
  endif()

  install(
    TARGETS load_and_run_depends_shared
    EXPORT ${MGE_EXPORT_TARGETS}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
