include(${CMAKE_SOURCE_DIR}/cmake/Utils.cmake)

set(HEADER_FILES ApplicationState.h Scene.h)

set(SOURCE_FILES main.cpp ApplicationState.cpp Scene.cpp)

set(SHADER_SOURCE_FILES Shaders/mesh.vert Shaders/shadow.vert Shaders/shadow.geom Shaders/phong.frag Shaders/empty.frag Shaders/ambient.frag)

create_directory_link(${CMAKE_CURRENT_SOURCE_DIR}/Shaders ${CMAKE_CURRENT_BINARY_DIR}/Shaders)
create_directory_link(${CMAKE_SOURCE_DIR}/assets ${CMAKE_CURRENT_BINARY_DIR}/assets)

add_executable(robot ${HEADER_FILES} ${SOURCE_FILES} ${SHADER_SOURCE_FILES})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Header Files" FILES ${HEADER_FILES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source Files" FILES ${SOURCE_FILES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Shaders PREFIX "Shaders" FILES ${SHADER_SOURCE_FILES})

target_include_directories(robot PRIVATE ${CMAKE_SOURCE_DIR}/robot)

target_compile_options(robot PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /MP>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)

target_link_libraries(robot PRIVATE ref glm)
