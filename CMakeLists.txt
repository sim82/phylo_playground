cmake_minimum_required(VERSION 2.8)

project(phylo_playground )
set( CMAKE_NO_VERBOSE yes )
add_subdirectory(ivy_mike)

include_directories( ${include_directories} ivy_mike/src . )

# handle the stupid autolinking shit of visualc++
IF(WIN32)
include_directories( ${include_directories} C:\\src\\boost_1_46_1)
LINK_DIRECTORIES( ${LINK_DIRECTORIES} C:\\src\\boost_1_46_1\\stage\\lib)
set( BOOST_LIBS )
set( SYSDEP_LIBS )
ELSE()
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Winvalid-pch -Wall -Wwrite-strings -march=native")# -std=c++0x")
set( BOOST_LIBS boost_thread boost_program_options)
set(SYSDEP_LIBS pthread)
#LINK_DIRECTORIES( ${LINK_DIRECTORIES} /usr/lib64/atlas-sse2 )
ENDIF()


add_executable( spr_vis_test spr_vis_test.cpp )
target_link_libraries( spr_vis_test ${SYSDEP_LIBS} ivymike )

# add_executable( fixed_decimal fixed_decimal.cpp )
# target_link_libraries( fixed_decimal ${SYSDEP_LIBS} ivymike )