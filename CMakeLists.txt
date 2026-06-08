cmake_minimum_required(VERSION 4.0)
project(MotifDesigner VERSION 0.5 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# We'll compile with -lXm, -lXt and -lX11 later on, these are required.
find_package(X11 REQUIRED)
find_package(Xt REQUIRED)
find_package(Xmu REQUIRED)

# Change this if you have another path to compile ;)
set(MOTIFDESIGNER_GENERAL_PATH "/opt/MaXX")

# Sometimes best compiled with /opt/MaXX includes and lib64, so it's better to use them (-I/opt/MaXX/include and -L/opt/MaXX/lib64).
set(CMAKE_PREFIX_PATH ${MOTIFDESIGNER_GENERAL_PATH})

target_link_libraries(MotifDesigner PRIVATE ${X11_LIBRARIES} ${Xt_LIBRARIES} ${Xmu_LIBRARIES})

# Sources here, yay
set(SOURCES
# Soon I guess
)

set(HEADERS
# Yeah again, but this is just a bootstrap ;)
)

target_include_directories(MotifDesigner PRIVATE ${PROJECT_SOURCE_DIR}/include)
add_executable(MotifDesigner ${SOURCES} ${HEADERS})
