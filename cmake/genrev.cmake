# Copyright (C) 2011 SingularityCore <http://www.singularitycore.org/>
# Copyright (C) 2008-2010 Trinity <http://www.trinitycore.org/>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

include(${CMAKE_SOURCE_DIR}/cmake/macros/EnsureVersion.cmake)

set(_REQUIRED_GIT_VERSION "1.7")

find_program(_GIT_EXEC
  NAMES
    git git.cmd
  HINTS
    ENV PATH
  DOC "git installation path"
)

if(_GIT_EXEC)
  execute_process(
    COMMAND "${_GIT_EXEC}" --version
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE _GIT_VERSION
    ERROR_QUIET
  )

  # make sure we're using minimum the required version of git, so the "dirty-testing" will work properly
  ensure_version( "${_REQUIRED_GIT_VERSION}" "${_GIT_VERSION}" _GIT_VERSION_OK)
endif()

if(_GIT_VERSION_OK)
  execute_process(
    COMMAND "${_GIT_EXEC}" describe --match init --dirty=+ --abbrev=12
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE rev_info
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
else()
  message("")
  message(STATUS "WARNING - Missing or outdated git - did you forget to install a recent version?")
  message(STATUS "WARNING - Observe that for revision ID/hash to work you need at least version ${_REQUIRED_GIT_VERSION}")
  message(STATUS "WARNING - Continuing anyway, but setting the revision-ID and hash to Rev:0 Hash: Archive")
  message("")
endif()

# Last minute check - ensure that we have a proper revision
# If everything above fails (means the user has erased the git revision control directory or removed the origin/HEAD tag)
if(NOT rev_info)
  # No valid ways available to find/set the revision/hash, so let's force some defaults
  set(rev_hash_str "Archive")
  set(rev_hash "0")
else()
  # Extract revision and hash from git
  string(REGEX REPLACE init-|[0-9]+-g "" rev_hash_str ${rev_info})
  string(REGEX REPLACE [+]+ "" rev_hash ${rev_hash_str})
endif()

# Its not set during initial run
if(NOT BUILDDIR)
  set(BUILDDIR ${CMAKE_BINARY_DIR})
endif()

# Create the actual revision.h file from the above params
if(NOT "${rev_hash_cached}" MATCHES "${rev_hash_str}")
  configure_file(
    "${CMAKE_SOURCE_DIR}/revision.h.in.cmake"
    "${BUILDDIR}/revision.h"
    @ONLY
  )
  set(rev_hash_cached "${rev_hash_str}" CACHE INTERNAL "Cached commit-hash")
endif()
