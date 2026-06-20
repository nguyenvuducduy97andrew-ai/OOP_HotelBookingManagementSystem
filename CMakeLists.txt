# Mỗi khi add một class mới (ví dụ: tạo thêm VipCustomer.h, VipCustomer.cpp),
# người đó phải mở file CMakeLists.txt ra
# và điền thêm dòng src/models/VipCustomer.h và src/models/VipCustomer.cpp
# vào đúng vị trí thì mới compile được.

cmake_minimum_required(VERSION 3.16)

project(HotelBookingManagement LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ================================================================
# QT CONFIG
# ================================================================

# Hiện tại project đang demo bằng console terminal.
# Vì đang dùng QDate nên chỉ cần Qt Core.
# Nếu DataManager có dùng QSqlDatabase / QSqlQuery thì giữ thêm Sql.
find_package(Qt6 REQUIRED COMPONENTS Core Sql)

# Nếu sau này làm GUI bằng Qt Widgets thì mở lại các dòng dưới:
# set(CMAKE_AUTOMOC ON)
# set(CMAKE_AUTOUIC ON)
# set(CMAKE_AUTORCC ON)
# find_package(Qt6 REQUIRED COMPONENTS Core Widgets Sql)
# qt_standard_project_setup()

# ================================================================
# SOURCE FILES
# ================================================================

add_executable(HotelBookingManagement
    # File chạy chính của chương trình
    src/main.cpp

    # Controllers
    src/controllers/HotelManager.cpp
    src/controllers/HotelManager.h

    # Database
    src/database/DataManager.cpp
    src/database/DataManager.h

    # Views - tạm comment vì hiện tại demo console, chưa dùng GUI
    # src/views/MainWindow.cpp
    # src/views/MainWindow.h
    # src/views/MainWindow.ui
    # src/views/RoomsView.cpp
    # src/views/RoomsView.h
    # src/views/CustomersView.cpp
    # src/views/CustomersView.h
    # src/views/BookingsView.cpp
    # src/views/BookingsView.h
    # src/views/InvoicesView.cpp
    # src/views/InvoicesView.h

    # Models
    src/models/Room.cpp
    src/models/Room.h
    src/models/StandardRoom.cpp
    src/models/StandardRoom.h
    src/models/DeluxeRoom.cpp
    src/models/DeluxeRoom.h
    src/models/SuiteRoom.cpp
    src/models/SuiteRoom.h
    src/models/Customer.cpp
    src/models/Customer.h
    src/models/Booking.cpp
    src/models/Booking.h
    src/models/Invoice.cpp
    src/models/Invoice.h
    src/models/RoomFactory.cpp
    src/models/RoomFactory.h
)

# ================================================================
# INCLUDE PATHS
# ================================================================

target_include_directories(HotelBookingManagement PRIVATE
    src/models
    src/controllers
    src/database

    # Nếu sau này dùng GUI thì mở lại dòng dưới
    # src/views
)

# ================================================================
# LINK LIBRARIES
# ================================================================

target_link_libraries(HotelBookingManagement PRIVATE
    Qt6::Core
    Qt6::Sql

    # Nếu sau này dùng GUI thì mở lại dòng dưới
    # Qt6::Widgets
)