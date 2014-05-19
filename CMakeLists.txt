cmake_minimum_required(VERSION 2.8.12.2)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
project(SoloMiner)
include_directories(. contrib/epee/include)
set(SOURCES
  common/base58.cpp
  crypto/blake256.c
  crypto/crypto-ops-data.c
  crypto/crypto-ops.c
  crypto/crypto.cpp
  crypto/groestl.c
  crypto/hash-extra-blake.c
  crypto/hash-extra-groestl.c
  crypto/hash-extra-jh.c
  crypto/hash-extra-skein.c
  crypto/hash.c
  crypto/jh.c
  crypto/keccak.c
  crypto/oaes_lib.c
  crypto/random.c
  crypto/skein.c
  crypto/slow-hash.c
  crypto/tree-hash.c
  cryptonote_core/cryptonote_basic_impl.cpp
  cryptonote_core/cryptonote_format_utils.cpp
  cryptonote_core/difficulty.cpp
  cryptonote_core/miner.cpp
  main.cpp
  MergedMiner.cpp
)

if(WIN32)
  if (NOT MSVC)
    message(FATAL_ERROR "Only MSVC is supported on this platform")
  endif()

  add_definitions(/D_CRT_SECURE_NO_WARNINGS /D_WIN32_WINNT=0x0600 /bigobj)
  include_directories(platform/msc)
  string(REPLACE "/MDd" "/MTd" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
  string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
  string(REPLACE "/MDd" "/MTd" CMAKE_C_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
  string(REPLACE "/MD" "/MT" CMAKE_C_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})

  set(Boost_USE_STATIC_LIBS ON)
  set(Boost_USE_STATIC_RUNTIME ON)
  find_package(Boost 1.55 REQUIRED COMPONENTS atomic chrono date_time filesystem program_options regex system thread)
  include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

  if (NOT EXISTS $ENV{wxWidgets_ROOT_DIR})
    message(FATAL_ERROR "wxWidgets_ROOT_DIR should be set")
  endif()

  include_directories($ENV{wxWidgets_ROOT_DIR}/include)

  if(CMAKE_CL_64)
    include_directories($ENV{wxWidgets_ROOT_DIR}/lib/vc_x64_lib/mswud)
    link_directories($ENV{wxWidgets_ROOT_DIR}/lib/vc_x64_lib)
  else()
    include_directories($ENV{wxWidgets_ROOT_DIR}/lib/vc_lib/mswud)
    link_directories($ENV{wxWidgets_ROOT_DIR}/lib/vc_lib)
  endif()

  add_executable(SoloMiner WIN32 ${SOURCES})
  target_link_libraries(SoloMiner comctl32 rpcrt4 ${Boost_LIBRARIES} debug wxbase30ud optimized wxbase30u debug wxmsw30ud_core optimized wxmsw30u_core debug wxpngd optimized wxpng debug wxzlibd optimized wxzlib)
elseif(UNIX)
  if(APPLE)
    #on Mac OS X we have to link statically with boost
    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_STATIC_RUNTIME ON)
  endif()

  find_package(Boost 1.53 REQUIRED COMPONENTS date_time filesystem program_options regex system thread)
  if((${Boost_MAJOR_VERSION} EQUAL 1) AND (${Boost_MINOR_VERSION} EQUAL 54))
    message(SEND_ERROR "Boost version 1.54 is unsupported, more details are available here http://goo.gl/RrCFmA")
  endif()

  if(APPLE)
    set(MACOSX_BUNDLE_INFO_STRING "SoloMiner")
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "0.1.0")
    set(MACOSX_BUNDLE_BUNDLE_NAME "SoloMiner")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "0.1")

    include_directories(/usr/include/malloc)
    include_directories(../wxWidgets-3.0.0/lib/wx/include/osx_cocoa-unicode-static-3.0)

    link_directories(/usr/local/lib/ ../wxWidgets-3.0.0/lib)
    add_definitions(-D_FILE_OFFSET_BITS=64 -D__WXMAC__ -D__WXOSX__ -D__WXOSX_COCOA__)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")

    set(MACOSX_WXWIDGETS_DEFS "-I/usr/local/lib/wx/include/osx_cocoa-unicode-static-3.0 -I/usr/local/include/wx-3.0 -D_FILE_OFFSET_BITS=64 -D__WXMAC__ -D__WXOSX__ -D__WXOSX_COCOA__")
    set(MACOSX_WXWIDGETS_STATIC_LINK_FLAGS "-L/usr/local/lib -framework IOKit -framework Carbon -framework Cocoa -framework AudioToolbox -framework System -framework OpenGL /usr/local/lib/libwx_osx_cocoau_xrc-3.0.a /usr/local/lib/libwx_osx_cocoau_webview-3.0.a /usr/local/lib/libwx_osx_cocoau_qa-3.0.a /usr/local/lib/libwx_baseu_net-3.0.a /usr/local/lib/libwx_osx_cocoau_html-3.0.a /usr/local/lib/libwx_osx_cocoau_adv-3.0.a /usr/local/lib/libwx_osx_cocoau_core-3.0.a /usr/local/lib/libwx_baseu_xml-3.0.a /usr/local/lib/libwx_baseu-3.0.a /usr/local/lib/libwxpng-3.0.a -framework WebKit -lexpat -lwxregexu-3.0 -lwxtiff-3.0 -lwxjpeg-3.0 -lz -liconv")

    add_executable(SoloMiner MACOSX_BUNDLE ${SOURCES})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MACOSX_WXWIDGETS_DEFS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${MACOSX_WXWIDGETS_STATIC_LINK_FLAGS}")
    target_link_libraries(SoloMiner pthread ${Boost_LIBRARIES})

  else()
    find_package(wxWidgets REQUIRED)

    foreach(def ${wxWidgets_DEFINITIONS})
      add_definitions(-D${def})
    endforeach()

    include_directories(${wxWidgets_INCLUDE_DIRS})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${wxWidgets_CXX_FLAGS} -std=c++11 -D_GNU_SOURCE")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -D_GNU_SOURCE")

    add_executable(SoloMiner ${SOURCES})
    target_link_libraries(SoloMiner ${wxWidgets_LIBRARIES} ${Boost_LIBRARIES})
endif(APPLE)
endif()
