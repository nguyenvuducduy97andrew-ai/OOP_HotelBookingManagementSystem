cmake_minimum_required(VERSION 3.16)

project(HotelBookingManagement LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

add_executable(HotelBookingManagement
    src/main.cpp
    src/controllers/HotelManager.cpp
    src/controllers/HotelManager.h
    src/database/DataManager.cpp
    src/database/DataManager.h
    src/views/MainWindow.cpp
    src/views/MainWindow.h
    src/views/MainWindow.ui
    src/models/Room.h
    src/models/StandardRoom.h
    src/models/DeluxeRoom.h
    src/models/SuiteRoom.h
    src/models/Customer.h
    src/models/Booking.h
    src/models/Invoice.h
)

target_include_directories(HotelBookingManagement PRIVATE
    src/models
    src/controllers
    src/database
    src/views
)

target_link_libraries(HotelBookingManagement PRIVATE Qt6::Widgets)
