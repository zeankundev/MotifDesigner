cmake_minimum_required(VERSION 3.20) # Lowered slightly for better compatibility, change back to 4.0 if required
project(MotifDesigner VERSION 0.5 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Setup Motif/X11 Paths
set(MOTIFDESIGNER_GENERAL_PATH "/opt/MaXX" CACHE PATH "Path to MaXX/Motif installation")
list(APPEND CMAKE_PREFIX_PATH "${MOTIFDESIGNER_GENERAL_PATH}")

find_package(X11 REQUIRED COMPONENTS Xt Xmu)
find_path(MOTIF_INCLUDE_DIR NAMES Xm/Xm.h HINTS "${MOTIFDESIGNER_GENERAL_PATH}/include")
find_library(MOTIF_LIBRARY NAMES Xm HINTS "${MOTIFDESIGNER_GENERAL_PATH}/lib64")

# Sources and Headers
set(SOURCES
  src/main.cpp
  src/tests/IMCPasser.cpp
  src/external/AboutWindow.cpp
  src/utils/XPMPixmapManager.cpp
)

# --- INSTALL RULES (Required for AppImage) ---
add_executable(MotifDesigner ${SOURCES})

# Modern target-based inclusion and linking
target_include_directories(MotifDesigner PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${MOTIF_INCLUDE_DIR}
    ${X11_INCLUDE_DIR}
)

target_link_libraries(MotifDesigner PRIVATE
    ${MOTIF_LIBRARY}
    ${X11_Xt_LIB}
    ${X11_X11_LIB}
    Xpm # if find_package didn't grab it, explicit works if it's in the linker path
)

install(TARGETS MotifDesigner DESTINATION bin)
install(FILES appimage/motifdesigner.png DESTINATION share/icons/hicolor/256x256/apps)
install(FILES appimage/motifdesigner.desktop DESTINATION share/applications)

set(CPACK_GENERATOR "AppImage")
set(CPACK_APPIMAGE_APPLICATION_NAME "MotifDesigner")
set(CPACK_APPIMAGE_ICON "${CMAKE_SOURCE_DIR}/appimage/motifdesigner.png")
set(CPACK_APPIMAGE_DESKTOP_FILE "${CMAKE_SOURCE_DIR}/appimage/motifdesigner.desktop")

include(CPack)

# Optional: Install an icon and .desktop file if you have them
# install(FILES assets/motifdesigner.desktop DESTINATION share/applications)
# install(FILES assets/motifdesigner.png DESTINATION share/icons/hicolor/256x256/apps)
