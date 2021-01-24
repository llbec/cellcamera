project(cellcamera)

if(!WIN32)
return()
endif()

include_directories(SYSTEM 
	"${CMAKE_SOURCE_DIR}/libobs"
	"${CMAKE_SOURCE_DIR}/libobs/util"
)
#include_directories("${CMAKE_CURRENT_SOURCE_DIR}/libs")

if(MSVC)
	set(CELLCAMERA_PLATFORM_DEPS
		w32-pthreads)
endif()

set(AIRPLAY_LIBS
	"${CMAKE_CURRENT_SOURCE_DIR}/libs/avcodec.lib"
	"${CMAKE_CURRENT_SOURCE_DIR}/libs/avformat.lib"
	"${CMAKE_CURRENT_SOURCE_DIR}/libs/avutil.lib"
	"${CMAKE_CURRENT_SOURCE_DIR}/libs/avdevice.lib"
	"${CMAKE_CURRENT_SOURCE_DIR}/libs/avfilter.lib"
	"${CMAKE_CURRENT_SOURCE_DIR}/libs/postproc.lib"
	"${CMAKE_CURRENT_SOURCE_DIR}/libs/swresample.lib"
	"${CMAKE_CURRENT_SOURCE_DIR}/libs/swscale.lib"
	"${CMAKE_CURRENT_SOURCE_DIR}/libs/fdk-aac.lib"
	"${CMAKE_CURRENT_SOURCE_DIR}/libs/mycrypt.lib"
	"${CMAKE_CURRENT_SOURCE_DIR}/libs/AirPlay.lib"
	#"ws2_32.lib"
	#"${CMAKE_CURRENT_SOURCE_DIR}/libs/libplist.dll"
)
#link_directories("libs")

FILE(GLOB CELLCAMERA_SOURCES 
	*.c
	airrcv/*.c
	${CELLCAMERA_PLATFORM_SOURCES}
)

add_library(cellcamera MODULE
	${CELLCAMERA_SOURCES})

target_link_libraries(cellcamera
	${CELLCAMERA_PLATFORM_DEPS}
	libobs
	${AIRPLAY_LIBS}
	ws2_32)

set_target_properties(cellcamera PROPERTIES FOLDER "plugins")

install_obs_plugin_with_data(cellcamera data)
