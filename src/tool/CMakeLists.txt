# Define the tools interface library, all tools will inherit properties

add_library(tools INTERFACE)
target_include_directories(tools INTERFACE 
	${YAML_INCLUDE_DIRS}
	${ZLIB_INCLUDE_DIRS}
	${RA_INCLUDE_DIRS}
)

target_sources(tools INTERFACE
	"${COMMON_SOURCE_DIR}/utils.cpp"
	"${COMMON_SOURCE_DIR}/des.cpp"
	"${COMMON_SOURCE_DIR}/grfio.cpp"
	"${COMMON_SOURCE_DIR}/nullpo.cpp"
	"${COMMON_SOURCE_DIR}/database.cpp"
)

target_compile_definitions(tools INTERFACE
	"MINICORE"
)

target_link_libraries(tools INTERFACE
	${GLOBAL_LIBRARIES}
	${ZLIB_LIBRARIES}
	yaml-cpp
	ryml
	minicore
)

# mapcache
message( STATUS "Creating target mapcache" )
add_executable(mapcache)
target_link_libraries(mapcache PRIVATE tools)
target_sources(mapcache PRIVATE "mapcache.cpp")

set( TARGET_LIST ${TARGET_LIST} mapcache CACHE INTERNAL "" )

if( INSTALL_COMPONENT_RUNTIME )
	cpack_add_component( Runtime_mapcache DESCRIPTION "mapcache generator" DISPLAY_NAME "mapcache" GROUP Runtime )
	install( TARGETS mapcache
		DESTINATION "."
		COMPONENT Runtime_mapcache
	)
	install (TARGETS )
endif( INSTALL_COMPONENT_RUNTIME )
