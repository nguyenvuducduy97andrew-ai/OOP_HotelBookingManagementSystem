
#Mỗi khi add một class mới (ví dụ: tạo thêm VipCustomer.h, VipCustomer.cpp), 
#người đó phải mở file CMakeLists.txt ra 
#và điền thêm dòng src/models/VipCustomer.h và scr/models/VipCustomer.cpp
#vào đúng vị trí  thì mới compile được
# Yêu cầu phiên bản CMake tối thiểu để chạy dự án này
cmake_minimum_required(VERSION 3.16)

# Định nghĩa tên dự án và chỉ định ngôn ngữ lập trình sử dụng là C++ (CXX)
project(HotelBookingManagement LANGUAGES CXX)

# Thiết lập tiêu chuẩn mã nguồn C++ tối thiểu là C++17 (để sử dụng các tính năng OOP hiện đại)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Bật tính năng tự động xử lý các thành phần đặc trưng của Qt:
# AUTOMOC: Tự động biên dịch cơ chế Signal/Slot (Meta-Object Compiler)
# AUTOUIC: Tự động chuyển đổi các file thiết kế kéo thả giao diện .ui thành code C++
# AUTORCC: Tự động quản lý các file tài nguyên hệ thống như hình ảnh, icon (.qrc)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Tìm và nạp module đồ họa giao diện Qt6 Widgets (bắt buộc phải có)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
find_package(Qt6 REQUIRED COMPONENTS Widgets Sql)

# Thiết lập các cấu hình mặc định chuẩn của Qt cho dự án (giúp tối ưu hóa đường dẫn build)
qt_standard_project_setup()

#-------------------------------------------------------------------
# ĐỊNH NGHĨA FILE THỰC THI & QUẢN LÝ DANH SÁCH FILE MÃ NGUỒN
# Khi thành viên thêm file mới, hãy điền chính xác đường dẫn vào nhóm tương ứng dưới đây.
#-------------------------------------------------------------------
add_executable(HotelBookingManagement
    # File chạy chính của chương trình
    src/main.cpp

    # Phân hệ Điều phối (Controllers / Lớp xử lý logic)
    src/controllers/HotelManager.cpp
    src/controllers/HotelManager.h

    # Phân hệ Cơ sở dữ liệu (Database / Lưu trữ tệp)
    src/database/DataManager.cpp
    src/database/DataManager.h

    # Phân hệ Giao diện người dùng (Views / Giao diện kéo thả)
    src/views/MainWindow.cpp
    src/views/MainWindow.h
    src/views/MainWindow.ui
    # src/views/RoomsView.cpp
    # src/views/RoomsView.h
    # src/views/CustomersView.cpp
    # src/views/CustomersView.h
    # src/views/BookingsView.cpp
    # src/views/BookingsView.h
    # src/views/InvoicesView.cpp
    # src/views/InvoicesView.h

    # Phân hệ Thực thể OOP (Models / Các lớp đối tượng quản lý)
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

#-------------------------------------------------------------------
# CẤU HÌNH THƯ MỤC TÌM KIẾM HEADER (INCLUDE PATH)
# Giúp các thành viên khi gọi chéo lớp chỉ cần #include "Room.h" 
# thay vì phải gõ đường dẫn dài dòng như #include "../models/Room.h"
# => Đồng nghĩa phải đặt tên file khác nhau cho toàn bộ 
#-------------------------------------------------------------------
target_include_directories(HotelBookingManagement PRIVATE
    src/models
    src/controllers
    src/database
    src/views
)

#-------------------------------------------------------------------
# LIÊN KẾT THƯ VIỆN GIAO DIỆN
# Kéo các thư viện Core và GUI tích hợp của Qt6 vào file thực thi cuối cùng
#-------------------------------------------------------------------
target_link_libraries(HotelBookingManagement PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Sql
)