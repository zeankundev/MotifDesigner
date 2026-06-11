cmake_minimum_required(VERSION 3.20)
project(MotifDesigner VERSION 0.7 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- BUILD TOGGLE ---
option(USE_MAXX_DESKTOP "Build specifically for MaXX Desktop environments" OFF)

find_package(X11 REQUIRED COMPONENTS Xt Xmu)

if(USE_MAXX_DESKTOP)
  message(STATUS "----> Configuring for MaXX Desktop (IRIX SGI Mode)")
  set(MOTIFDESIGNER_GENERAL_PATH "/opt/MaXX" CACHE PATH "Path to MaXX/Motif installation")
  list(APPEND CMAKE_PREFIX_PATH "${MOTIFDESIGNER_GENERAL_PATH}")

  find_path(MOTIF_INCLUDE_DIR NAMES Xm/Xm.h HINTS "${MOTIFDESIGNER_GENERAL_PATH}/include")
  find_library(MOTIF_LIBRARY NAMES Xm HINTS "${MOTIFDESIGNER_GENERAL_PATH}/lib64")
else()
  message(STATUS "----> Configuring for Generic Linux Desktop (Standard Motif)")
  # Uses standard system libraries installed via apt/dnf
  find_path(MOTIF_INCLUDE_DIR NAMES Xm/Xm.h)
  find_library(MOTIF_LIBRARY NAMES Xm)
endif()

# Sources
set(SOURCES
# Soon I guess
  src/main.cpp
  src/tests/IMCPasser.cpp
  src/external/AboutWindow.cpp
  src/utils/XPMPixmapManager.cpp
  # components
  src/components/Toolbar.cpp
  src/components/Menubar.cpp
  src/components/PropertiesPanel.cpp
  src/components/Canvas.cpp
  src/components/StatusBar.cpp
)

add_executable(MotifDesigner ${SOURCES})

# Target Directories & Libraries
target_include_directories(MotifDesigner PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${MOTIF_INCLUDE_DIR}
    ${X11_INCLUDE_DIR}
)

target_link_libraries(MotifDesigner PRIVATE
    ${MOTIF_LIBRARY}
    ${X11_Xt_LIB}
    ${X11_X11_LIB}
    Xpm
)

target_compile_definitions(MotifDesigner PRIVATE
    PROJECT_VERSION_STR="${PROJECT_VERSION}"
    PROJECT_NAME_STR="${PROJECT_NAME}"
)

# Install rules
install(TARGETS MotifDesigner DESTINATION bin)
