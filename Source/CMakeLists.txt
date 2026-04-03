add_library(Osiris SHARED dllmain.cpp)

set_target_properties(Osiris PROPERTIES
  CXX_VISIBILITY_PRESET hidden
  C_VISIBILITY_PRESET hidden
  POSITION_INDEPENDENT_CODE ON)
if(WIN32)
  set_target_properties(Osiris PROPERTIES PREFIX "")
endif()

target_include_directories(Osiris PRIVATE "${CMAKE_SOURCE_DIR}/Source")
target_link_libraries(Osiris PRIVATE ${CMAKE_DL_LIBS})
if(WIN32 AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
  target_link_libraries(Osiris PRIVATE ntdll)
endif()

if(WIN32 AND (MSVC OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
  target_sources(Osiris PRIVATE Platform/Windows/Syscalls/WindowsSyscall.asm)
endif()

if(MSVC)
  set_target_properties(Osiris PROPERTIES MSVC_RUNTIME_LIBRARY "$<$<CONFIG:Debug>:MultiThreadedDebugDLL>")
  string(REGEX REPLACE "/EH[a-z]+" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
  target_sources(Osiris PRIVATE $<$<CONFIG:Release>:Platform/Windows/CRTWindows.cpp>)
  target_compile_options(Osiris PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/W4 $<$<CONFIG:Release>:/sdl- /GS->>)
  target_compile_definitions(Osiris PRIVATE $<$<CONFIG:Release>:_USE_STD_VECTOR_ALGORITHMS=0>)
  target_link_options(Osiris PRIVATE $<$<CONFIG:Release>:/nodefaultlib /ENTRY:"DllMain">)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  if(NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    target_compile_options(Osiris PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wall>)
    target_link_options(Osiris PRIVATE LINKER:--no-undefined -z noexecstack)
  endif()

  target_compile_options(Osiris PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-missing-braces>)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 14.0)
  target_compile_options(Osiris PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-dangling-reference>) # false positive
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  if(UNIX)
    target_compile_options(Osiris PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-stack-protector -fno-exceptions -fno-asynchronous-unwind-tables -fno-unwind-tables>)
    target_link_options(Osiris PRIVATE -nostdlib)
    target_link_libraries(Osiris PRIVATE c m)

    if(CMAKE_STRIP)
      add_custom_command(TARGET Osiris POST_BUILD COMMAND ${CMAKE_STRIP} "$<TARGET_FILE:Osiris>")
    endif()

    # Create inject.sh script
    add_custom_command(TARGET Osiris POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/inject.sh.in" "$<TARGET_FILE_DIR:Osiris>/inject.sh"
      COMMAND chmod +x "$<TARGET_FILE_DIR:Osiris>/inject.sh"
      COMMENT "Creating inject.sh script"
    )
  endif()
endif()
