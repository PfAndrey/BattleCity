cmake_minimum_required(VERSION 3.5)
project(BattleCity VERSION 0.99 LANGUAGES C CXX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

if(NOT CMAKE_BUILD_TYPE)
 set(CMAKE_BUILD_TYPE "Release")
endif()

# BUILD EXTERNAL LIBRARIES
include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)
ExternalProject_Add(SFML 
		SOURCE_DIR  ${CMAKE_SOURCE_DIR}/ext/SFML 
    		CMAKE_ARGS 
			-DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
			-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})
ExternalProject_Get_Property(SFML INSTALL_DIR)
set(SFML_LIB ${INSTALL_DIR})

# ADD SOURCE FILES AND HEADERS OF EXECUTABLE
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include ${SFML_LIB}/include ${TINY_XML_LIB}/include)

set(SOURCE_ENGINE
	${CMAKE_SOURCE_DIR}/source/GameEngine/GameEngine.h
	${CMAKE_SOURCE_DIR}/source/GameEngine/GameEngine.cpp
	${CMAKE_SOURCE_DIR}/source/GameEngine/Geometry.h
	${CMAKE_SOURCE_DIR}/source/GameEngine/Geometry.cpp
	${CMAKE_SOURCE_DIR}/source/GameEngine/Graphs.h
	${CMAKE_SOURCE_DIR}/source/GameEngine/Graphs.cpp
	${CMAKE_SOURCE_DIR}/source/GameEngine/TileMap.h
	${CMAKE_SOURCE_DIR}/source/GameEngine/HierarchicalPathFinder.h
)

set(SOURCE_GAME
	${CMAKE_SOURCE_DIR}/source/BattleCityGame.h
	${CMAKE_SOURCE_DIR}/source/BattleCityGame.cpp
	${CMAKE_SOURCE_DIR}/source/Pickups.h
	${CMAKE_SOURCE_DIR}/source/Pickups.cpp
	${CMAKE_SOURCE_DIR}/source/Main.cpp
)
 
source_group("GameEngine"	FILES ${SOURCE_ENGINE})
source_group("BattleCity"	FILES ${SOURCE_GAME})


# LINK EXTERNAL LIBRARIES TO EXECUTABLE
LINK_DIRECTORIES(${SFML_LIB}/lib)

# ADD EXECUTABLE

add_executable(BattleCity ${SOURCE_ENGINE} ${SOURCE_GAME})

if (MSVC)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT BattleCity)
	set_target_properties( BattleCity PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/Build")
	set_target_properties( BattleCity PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/Build")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
endif(MSVC)

# ADD DEPENDISIES FOR EXECUTABLE
add_dependencies(BattleCity SFML)

TARGET_LINK_LIBRARIES(BattleCity 
					optimized sfml-system		debug sfml-system-d 
					optimized sfml-window		debug sfml-window-d 
					optimized sfml-graphics		debug sfml-graphics-d 
					optimized sfml-audio		debug sfml-audio-d)                

# POST BUILD SCRIPTS
set(POST_LIB_DIR "lib")
if (WIN32)
	set(POST_LIB_DIR "bin")
endif()

ADD_CUSTOM_COMMAND(TARGET BattleCity POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${SFML_LIB}/${POST_LIB_DIR}" "${CMAKE_CURRENT_BINARY_DIR}"
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/res" "${CMAKE_CURRENT_BINARY_DIR}/res")