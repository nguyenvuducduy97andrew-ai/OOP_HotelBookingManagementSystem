# Modified: CMake 3.17 is the minimum reliable version for Qt BIG_RESOURCES, which avoids compiling large room photos as generated C++ arrays.
cmake_minimum_required(VERSION 3.17)
project(HotelBookingManagement LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/data")

find_package(Qt6 REQUIRED COMPONENTS Core Sql Widgets Charts Svg)
include(CTest)

# Modified: Discover room-photo assets in a repository-relative, deterministic list so every developer and evaluator receives the same optimized resource build.
file(GLOB_RECURSE ROOM_PHOTO_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/resources/room_images/*.jpg"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/resources/room_images/*.jpeg"
)
list(SORT ROOM_PHOTO_FILES)

# Keep each architectural layer independently compilable and reusable by the application and tests.
file(GLOB_RECURSE MODEL_SOURCES CONFIGURE_DEPENDS
    "src/models/*.cpp" "src/models/*.h"
)
file(GLOB_RECURSE CONTROLLER_SOURCES CONFIGURE_DEPENDS
    "src/controllers/*.cpp" "src/controllers/*.h"
)
file(GLOB_RECURSE DATABASE_SOURCES CONFIGURE_DEPENDS
    "src/database/*.cpp" "src/database/*.h"
)
file(GLOB_RECURSE VIEW_SOURCES CONFIGURE_DEPENDS
    "src/views/*.cpp" "src/views/*.h" "src/views/*.ui"
)

set(MODEL_INCLUDE_DIRS
    src/models
    src/models/booking
    src/models/customer
    src/models/room
    src/models/session
)
set(CONTROLLER_INCLUDE_DIRS
    src/controllers
    src/controllers/hotel
    src/controllers/booking
    src/controllers/customer
    src/controllers/room
    src/controllers/report
)
set(VIEW_INCLUDE_DIRS
    src/views
    src/views/application
    src/views/authentication
    src/views/dashboard
    src/views/bookings
    src/views/customers
    src/views/rooms
    src/views/reports
    src/views/common
)

add_library(models_lib STATIC ${MODEL_SOURCES})
target_include_directories(models_lib PUBLIC ${MODEL_INCLUDE_DIRS})
target_link_libraries(models_lib PUBLIC Qt6::Core)
target_precompile_headers(models_lib PRIVATE src/pch/core_pch.h)

add_library(controllers_lib STATIC ${CONTROLLER_SOURCES})
target_include_directories(controllers_lib PUBLIC ${CONTROLLER_INCLUDE_DIRS})
target_link_libraries(controllers_lib PUBLIC models_lib Qt6::Core)
target_precompile_headers(controllers_lib PRIVATE src/pch/core_pch.h)

add_library(database_lib STATIC ${DATABASE_SOURCES})
target_include_directories(database_lib PUBLIC
    src/database
)
target_link_libraries(database_lib PUBLIC controllers_lib Qt6::Core Qt6::Sql)
target_precompile_headers(database_lib PRIVATE src/pch/core_pch.h)

add_executable(HotelBookingManagement
    src/main.cpp
    src/resources/resources.qrc
    ${VIEW_SOURCES}
)

# Modified: Embed large, already-compressed room photos directly as object data instead of generating and compiling a very large qrc_resources.cpp file.
qt_add_resources(HotelBookingManagement "room_photos"
    PREFIX "/"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/src/resources"
    BIG_RESOURCES
    FILES ${ROOM_PHOTO_FILES}
    OPTIONS -no-compress
)
target_include_directories(HotelBookingManagement PRIVATE ${VIEW_INCLUDE_DIRS})
target_precompile_headers(HotelBookingManagement PRIVATE src/pch/widgets_pch.h)

target_link_libraries(HotelBookingManagement PRIVATE
    database_lib
    Qt6::Widgets
    Qt6::Charts
    Qt6::Svg
)

