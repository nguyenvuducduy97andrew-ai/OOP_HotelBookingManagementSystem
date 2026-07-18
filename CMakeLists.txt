cmake_minimum_required(VERSION 3.19)
project(hotel_booking LANGUAGES CXX)
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Charts Sql)
qt_standard_project_setup()

set(BACKEND_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

file(GLOB BACKEND_SOURCES
    ${BACKEND_DIR}/controllers/*.cpp
    ${BACKEND_DIR}/controllers/*.h
    ${BACKEND_DIR}/database/*.cpp
    ${BACKEND_DIR}/database/*.h
    ${BACKEND_DIR}/models/*.cpp
    ${BACKEND_DIR}/models/*.h
)
# không lấy src/main.cpp và src/views/ vì bạn tự làm UI

qt_add_executable(hotel_booking
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    DashboardWidget.ui
    DashboardWidget.h
    DashboardWidget.cpp
    dashboardwidgets.cpp
    dashboardwidgets.h
    RoomPageWidget.h
    RoomPageWidget.cpp
    ReservationsPageWidget.h
    ReservationsPageWidget.cpp
    RoomStatusPageWidget.h
    RoomStatusPageWidget.cpp
    RoomDialog.h
    RoomDialog.cpp
    ReservationDialog.h
    ReservationDialog.cpp
    CustomSuccessDialog.h
    CustomConfirmDialog.h
    InvoiceDialog.h
    ${BACKEND_SOURCES}
)

target_include_directories(hotel_booking PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${BACKEND_DIR}
    ${BACKEND_DIR}/controllers
    ${BACKEND_DIR}/database
    ${BACKEND_DIR}/models
)

target_link_libraries(hotel_booking
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Charts
        Qt::Sql
)

install(TARGETS hotel_booking
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET hotel_booking
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})