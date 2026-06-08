cmake_minimum_required(VERSION 4.0)
project(MotifDesigner VERSION 0.5 LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# We'll compile with -lXm, -lXt and -lX11 later on, these are required.
find_package(X11 REQUIRED COMPONENTS Xt Xmu)

# Change this if you have another path to compile ;)
set(MOTIFDESIGNER_GENERAL_PATH "/opt/MaXX")

# Sometimes best compiled with /opt/MaXX includes and lib64, so it's better to use them (-I/opt/MaXX/include and -L/opt/MaXX/lib64).
list(APPEND CMAKE_PREFIX_PATH "${MOTIFDESIGNER_GENERAL_PATH}")

find_path(MOTIF_INCLUDE_DIR NAMES Xm/Xm.h HINTS "${MOTIFDESIGNER_GENERAL_PATH}/include")
find_library(MOTIF_LIBRARY NAMES Xm HINTS "${MOTIFDESIGNER_GENERAL_PATH}/lib64")

# Sources here, yay
set(SOURCES
# Soon I guess
  src/main.cpp
  src/tests/IMCPasser.cpp
  src/external/AboutWindow.cpp
)

set(HEADERS
  include/IMCPasser.h
  include/Misc.h
# Yeah again, but this is just a bootstrap ;)
)

add_executable(MotifDesigner ${SOURCES} ${HEADERS})

target_include_directories(MotifDesigner PRIVATE "${MOTIFDESIGNER_GENERAL_PATH}/include")
target_link_directories(MotifDesigner PRIVATE "${MOTIFDESIGNER_GENERAL_PATH}/lib64")
target_link_libraries(MotifDesigner PRIVATE Xm Xt X11 Xpm)
target_include_directories(MotifDesigner PRIVATE ${PROJECT_SOURCE_DIR}/include)
