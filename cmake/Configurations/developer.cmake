set(SCORE_PCH True)
set(SCORE_STATIC_PLUGINS True)
set(CMAKE_BUILD_TYPE Debug)
set(SCORE_AUDIO_PLUGINS True CACHE INTERNAL "")
if(NOT DEFINED DEPLOYMENT_BUILD)
  set(DEPLOYMENT_BUILD False)
endif()

include(all-plugins)