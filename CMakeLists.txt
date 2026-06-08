cmake_minimum_required(VERSION 4.0)
project(MotifDesigner VERSION 0.5 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Sources here, yay
set(SOURCES
# Soon I guess
)

set(HEADERS
# Yeah again, but this is just a bootstrap ;)
)

target_include_directories(MotifDesigner PRIVATE ${PROJECT_SOURCE_DIR}/include)
add_executable(MotifDesigner ${SOURCES} ${HEADERS})
