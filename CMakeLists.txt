CMAKE_MINIMUM_REQUIRED (VERSION 3.1)
project (open-plc-utils)

if(CMAKE_COMPILER_IS_GNUCXX)
    add_compile_options (-std=c++17 -fpermissive -lpthread)
endif()

if (MSVC_VERSION GREATER_EQUAL "1900")
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("/std:c++latest" _cpp_latest_flag_supported)
    if (_cpp_latest_flag_supported)
        add_compile_options("/std:c++latest")
    endif()
endif()

add_compile_definitions(WIN32)
add_compile_definitions(NDEBUG)
add_compile_definitions(_CONSOLE)
add_compile_definitions(MAKEFILE)
add_compile_definitions(WINPCAP)

file(GLOB_RECURSE PLC_SOURCES CONFIGURE_DEPENDS
  ./open-plc-utils/*.*
  ./pib/pibpeek1.c
  ./pib/pibpeek2.c
  ./pib/pibfile.c
  ./pib/piblock.c
  ./pib/pibscalers.c
  ./pib/pibfile1.c
  ./pib/pibfile2.c
  ./key/SHA256Write.c
  ./key/SHA256Reset.c
  ./key/SHA256Fetch.c
  ./key/SHA256Block.c
  ./key/HPAVKeyNID.c
  ./key/keys.c
  ./tools/*.*
)

include_directories(./tools)
include_directories(./open-plc-utils/include)
# link_directories("C:\\Program Files\\PostgreSQL\\15\\lib")

add_executable(open-plc-utils ${PLC_SOURCES})
# target_link_libraries(postgre_sql libpq)