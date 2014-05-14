cmake_minimum_required(VERSION 2.8.12.2)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
project(SoloMiner)

include_directories(../boost_1_55_0 contrib/epee/include ../wxWidgets-3.0.0/include .)

if(MSVC)
  add_definitions(/D_CRT_SECURE_NO_WARNINGS /D_WIN32_WINNT=0x0600 /bigobj)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:10485760")
  string(REPLACE "/MDd" "/MTd" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
  string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
  string(REPLACE "/MDd" "/MTd" CMAKE_C_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
  string(REPLACE "/MD" "/MT" CMAKE_C_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
  if(CMAKE_CL_64)
    include_directories(platform/msc ../wxWidgets-3.0.0/lib/vc_x64_lib/mswud)
    link_directories(../boost_1_55_0/stage64/lib ../wxWidgets-3.0.0/lib/vc_x64_lib)
  else()
    include_directories(platform/msc ../wxWidgets-3.0.0/lib/vc_lib/mswud)
    link_directories(../boost_1_55_0/stage32/lib ../wxWidgets-3.0.0/lib/vc_lib)
  endif()
else()
  add_definitions(-D__WXGTK__)
  include_directories(../wxWidgets-3.0.0/lib/wx/include/gtk2-unicode-static-3.0)
  link_directories(../boost_1_55_0/stage/lib ../wxWidgets-3.0.0/lib)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE -std=c11")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GNU_SOURCE -std=c++11")
endif()

add_executable(SoloMiner WIN32
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

if(MSVC)
  target_link_libraries(SoloMiner comctl32 rpcrt4 debug wxbase30ud optimized wxbase30u debug wxmsw30ud_core optimized wxmsw30u_core debug wxpngd optimized wxpng debug wxzlibd optimized wxzlib)
else()
  target_link_libraries(SoloMiner boost_date_time boost_filesystem boost_program_options boost_regex boost_system boost_thread wx_gtk2u_core-3.0 wx_baseu-3.0 SM X11 cairo dl gdk_pixbuf-2.0 gdk-x11-2.0 glib-2.0 gtk-x11-2.0 gobject-2.0 pango-1.0 pangocairo-1.0 pthread)
endif()
