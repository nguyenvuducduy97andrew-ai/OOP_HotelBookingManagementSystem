cmake_minimum_required(VERSION 3.16)
project(HotelBookingManagement LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/data")

find_package(Qt6 REQUIRED COMPONENTS Core Sql Widgets Charts Svg)
include(CTest)

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
list(FILTER VIEW_SOURCES EXCLUDE REGEX ".*/views/legacy/.*")

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
target_include_directories(HotelBookingManagement PRIVATE ${VIEW_INCLUDE_DIRS})
target_precompile_headers(HotelBookingManagement PRIVATE src/pch/widgets_pch.h)

target_link_libraries(HotelBookingManagement PRIVATE
    models_lib
    controllers_lib
    database_lib
    Qt6::Widgets
    Qt6::Charts
    Qt6::Svg
)

