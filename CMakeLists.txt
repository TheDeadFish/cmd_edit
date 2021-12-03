project(peFile)
cmake_minimum_required(VERSION 3.16)

set( EXECUTABLE_OUTPUT_PATH ../bin)
add_link_options(-mwindows)
link_libraries(-liberty)

add_executable(cmdEdit
	src/resource.rc
	src/main.c
	src/clipboard.c
)
