cmake_minimum_required(VERSION 3.16)
project(HotelBookingManagement LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/data")

find_package(Qt6 REQUIRED COMPONENTS Core Sql Widgets Charts)

set(SOURCE_FILES
    src/main.cpp
    src/controllers/HotelManager.cpp
    src/database/DataManager.cpp
    src/models/Booking.cpp
    src/models/Customer.cpp
    src/models/DeluxeRoom.cpp
    src/models/Invoice.cpp
    src/models/Room.cpp
    src/models/RoomFactory.cpp
    src/models/StandardRoom.cpp
    src/models/SuiteRoom.cpp
    src/views/CustomerDialog.cpp
    src/views/CustomerPageWidget.cpp
    src/views/DashboardWidget.cpp
    src/views/dashboardwidgets.cpp
    src/views/mainwindow.cpp
    src/views/ReservationDialog.cpp
    src/views/ReservationsPageWidget.cpp
    src/views/RoomDialog.cpp
    src/views/RoomPageWidget.cpp
    src/views/RoomStatusPageWidget.cpp
)

file(GLOB CONFIGURE_DEPENDS VIEW_HEADERS
    "src/views/*.h"
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