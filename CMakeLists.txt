cmake_minimum_required(VERSION 3.15)
project(PROYECTO)

set(CMAKE_CXX_STANDARD 20)

set(SFML_DIR "/usr/local/opt/sfml@2/lib/cmake/SFML")

add_executable(PROYECTO
    main.cpp
    formula.cpp
    spreadsheet_ui.cpp
)

find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)
target_link_libraries(PROYECTO PRIVATE sfml-graphics sfml-window sfml-system)
