#
# setup
#
set( MAP_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}  CACHE INTERNAL "" )


#
# map server
#
if( BUILD_SERVERS )
message( STATUS "Creating target map-server" )
file(GLOB MAP_HEADERS ${MAP_SOURCE_DIR}/*.hpp)
file(GLOB MAP_SOURCES ${MAP_SOURCE_DIR}/*.ixx ${MAP_SOURCE_DIR}/*.cpp)
file(GLOB MAP_MODULES ${MAP_SOURCE_DIR}/*.ixx)
set( DEPENDENCIES common ryml)
set( LIBRARIES ${GLOBAL_LIBRARIES})
set( INCLUDE_DIRS ${GLOBAL_INCLUDE_DIRS} ${COMMON_BASE_INCLUDE_DIRS} ${RA_INCLUDE_DIRS} )
set( DEFINITIONS "${GLOBAL_DEFINITIONS} ${COMMON_BASE_DEFINITIONS}" )
if( WITH_PCRE )
	message( STATUS "Enabled PCRE code" )
	set( LIBRARIES ${LIBRARIES} ${PCRE_LIBRARIES} )
	set( INCLUDE_DIRS ${INCLUDE_DIRS} ${PCRE_INCLUDE_DIRS} )
	set( DEFINITIONS "${DEFINITIONS} -DPCRE_SUPPORT" )
else()
	message( STATUS "Disabled PCRE code" )
endif()
set( SOURCE_FILES ${COMMON_BASE_HEADERS} ${COMMON_HEADERS} ${MAP_HEADERS} ${MAP_SOURCES} )
source_group( common FILES ${COMMON_BASE_HEADERS} ${COMMON_HEADERS} )
source_group( map FILES ${MAP_HEADERS} ${MAP_SOURCES} )
include_directories( ${INCLUDE_DIRS} )


#add_compile_options(-fmodules)
#add_compile_options(-stdlib=libc++)
#add_compile_options(-fbuiltin-module-map)
#add_compile_options(-fimplicit-module-maps)
#add_compile_options(-fprebuilt-module-path=${MAP_SOURCE_DIR})

function(add_module name)
    file(MAKE_DIRECTORY ${MAP_SOURCE_DIR})
    add_custom_target(${name}.pcm
            COMMAND
                ${CMAKE_CXX_COMPILER}
                -std=c++20
                -stdlib=libc++
                -fmodules
                -c
                ${CMAKE_CURRENT_SOURCE_DIR}/${ARGN}
                -Xclang -emit-module-interface
                -o ${MAP_SOURCE_DIR}/${name}.pcm

            )
endfunction()

foreach(MODULE_FILE ${MAP_MODULES})
	# Extract the module name without the extension
	get_filename_component(MODULE_NAME ${MODULE_FILE} NAME_WE)
	add_module(${MODULE_NAME} ${MODULE_FILE})
	# Add a dependency for each module on the map-server target
	set( DEPENDENCIES ${DEPENDENCIES} ${MODULE_NAME} )
endforeach()

#message( STATUS "map-server SOURCE_FILES=${SOURCE_FILES}")
add_executable( map-server ${SOURCE_FILES} )
#message( STATUS "map-server LIBRARIES=${LIBRARIES}, DEPENDENCIES=${DEPENDENCIES} DEFINITIONS=${DEFINITIONS}")
add_dependencies( map-server ${DEPENDENCIES} )
target_link_libraries( map-server ${LIBRARIES} ${DEPENDENCIES} )
set_target_properties( map-server PROPERTIES COMPILE_FLAGS "${DEFINITIONS}" )
if( INSTALL_COMPONENT_RUNTIME )
	cpack_add_component( Runtime_mapserver DESCRIPTION "map-server" DISPLAY_NAME "map-server" GROUP Runtime )
	install( TARGETS map-server
		DESTINATION "."
		COMPONENT Runtime_mapserver )
endif( INSTALL_COMPONENT_RUNTIME )
set( TARGET_LIST ${TARGET_LIST} map-server  CACHE INTERNAL "" )
message( STATUS "Creating target map-server - done" )
endif( BUILD_SERVERS )
