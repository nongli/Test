# Copyright (c) 2012 Cloudera, Inc. All rights reserved.

set(CMAKE_BUILD_TYPE "Release")
include_directories(${CMAKE_SOURCE_DIR})
add_executable(atof-benchmark atof-benchmark.cc benchmark.cc cpu-info.cc)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.2")

SET(A $ENV{'blahblah'})
MESSAGE("BEFORE ${A}")

if (NOT A)
  SET(A 'HERE')
  SET(B 'INSIDE')
  MESSAGE("HERE")
ENDIF (NOT A)

MESSAGE("AFTER ${A}")
MESSAGE("AFTER ${B}")
