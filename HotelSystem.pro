#-------------------------------------------------
# Project mới được thiết lập tự động phân chia thư mục
#-------------------------------------------------

QT       += core gui

# Yêu cầu phiên bản C++ tối thiểu là C++17 (để dùng các tính năng hiện đại)
CONFIG += c++17

# Tên file thực thi (.exe) sau khi build xong
TARGET = HotelBookingManagement
TEMPLATE = app

# Định nghĩa các macro để báo lỗi nếu dùng các hàm cũ bị khai tử
DEFINES += QT_DEPRECATED_WARNINGS

#-------------------------------------------------
# Đường dẫn tìm kiếm các file Header (.h)
# Giúp các thành viên khi #include không cần viết dài dòng
#-------------------------------------------------
INCLUDEPATH += src/models src/controllers src/database src/views

#-------------------------------------------------
# Danh sách các file Source Code (.cpp) của dự án
#-------------------------------------------------
SOURCES += \
    src/main.cpp \
    src/controllers/HotelManager.cpp \
    src/database/DatabaseManager.cpp \
    src/views/MainWindow.cpp

#-------------------------------------------------
# Danh sách các file Header (.h) của dự án
#-------------------------------------------------
HEADERS += \
    src/models/Room.h \
    src/models/StandardRoom.h \
    src/models/DeluxeRoom.h \
    src/models/SuiteRoom.h \
    src/models/Customer.h \
    src/models/Booking.h \
    src/models/Invoice.h \
    src/controllers/HotelManager.h \
    src/database/DatabaseManager.h \
    src/views/MainWindow.h

#-------------------------------------------------
# Danh sách các file thiết kế Giao diện (.ui)
#-------------------------------------------------
FORMS += \
    src/views/MainWindow.ui

# Mặc định cấu hình thư mục build riêng để tránh rác (Shadow Build)
# Đã được chặn tự động bởi file .gitignore của nhóm