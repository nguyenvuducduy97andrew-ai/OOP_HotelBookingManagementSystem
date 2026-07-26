cmake_minimum_required(VERSION 3.16)
project(HotelBookingManagement LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/data")

find_package(Qt6 REQUIRED COMPONENTS Core Sql Widgets Charts)

file(GLOB SOURCE_FILES
    "src/*.cpp" "src/*.h"
    "src/models/*.cpp" "src/models/*.h"
    "src/controllers/*.cpp" "src/controllers/*.h"
    "src/database/*.cpp" "src/database/*.h"
    "src/views/*.cpp" "src/views/*.h" "src/views/*.ui"
    "src/resources/*.qrc"
)

add_executable(HotelBookingManagement ${SOURCE_FILES} ${VIEW_HEADERS})

target_include_directories(HotelBookingManagement PRIVATE
    src/models
    src/controllers
    src/database
    src/views
)

target_link_libraries(HotelBookingManagement PRIVATE
    Qt6::Core
    Qt6::Sql
    Qt6::Widgets
    Qt6::Charts
)