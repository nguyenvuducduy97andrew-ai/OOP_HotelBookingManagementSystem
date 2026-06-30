cmake_minimum_required(VERSION 3.16)

project(HotelBookingManagement LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ================================================================
# QT CONFIG
# ================================================================
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Hiện tại project đang demo bằng console terminal, giữ Core và Sql.
# Khi nào bật GUI Widgets, CMake sẽ tự động xử lý nhờ các lệnh AUTO bên trên.
find_package(Qt6 REQUIRED COMPONENTS Core Sql Widgets)

# ================================================================
# AUTOMATIC SOURCE DISCOVERY (Tự động quét file)
# ================================================================

# Lệnh này tự động tìm TẤT CẢ các file .cpp, .h, .ui trong thư mục src
file(GLOB SOURCE_FILES
    "src/*.cpp" "src/*.h"
    "src/models/*.cpp" "src/models/*.h"
    "src/controllers/*.cpp" "src/controllers/*.h"
    "src/database/*.cpp" "src/database/*.h"
    "src/views/*.cpp" "src/views/*.h" "src/views/*.ui"
)

# Tạo gói thực thi từ danh sách file đã tự động quét được ở trên
add_executable(HotelBookingManagement ${SOURCE_FILES})

# ================================================================
# INCLUDE PATHS
# ================================================================

target_include_directories(HotelBookingManagement PRIVATE
    src/models
    src/controllers
    src/database
    src/views
)

# ================================================================
# LINK LIBRARIES
# ================================================================

target_link_libraries(HotelBookingManagement PRIVATE
    Qt6::Core
    Qt6::Sql
    Qt6::Widgets # Đã mở sẵn, dùng được cho cả Console lẫn GUI sau này
)