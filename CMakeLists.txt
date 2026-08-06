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

# Modified and optimized performance: reconfigure automatically when a new controller or view source file is added.
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS

    "src/models/*.cpp" "src/models/*.h"
    "src/controllers/hotel/*.cpp" "src/controllers/hotel/*.h"
    "src/controllers/booking/*.cpp" "src/controllers/booking/*.h"
    "src/controllers/customer/*.cpp" "src/controllers/customer/*.h"
    "src/controllers/room/*.cpp" "src/controllers/room/*.h"
    "src/controllers/report/*.cpp" "src/controllers/report/*.h"
    "src/database/*.cpp" "src/database/*.h"
    "src/views/*.cpp" "src/views/*.h" "src/views/*.ui"
    "src/resources/*.qrc"
    "src/main.cpp"
)

# Modified: Remove the superseded legacy dialog before the target source list is consumed by CMake.
list(FILTER SOURCE_FILES EXCLUDE REGEX ".*bookingdialog\\.(cpp|h|ui)$")

add_executable(HotelBookingManagement ${SOURCE_FILES})

target_include_directories(HotelBookingManagement PRIVATE
    src/models
    src/controllers
    src/controllers/hotel
    src/controllers/booking
    src/controllers/customer
    src/controllers/room
    src/controllers/report
    src/database
    src/views
)

target_link_libraries(HotelBookingManagement PRIVATE
    Qt6::Core
    Qt6::Sql
    Qt6::Widgets
    Qt6::Charts
    Qt6::Svg
)

if(BUILD_TESTING)
    # Modified: Keep automated business tests independent from QWidget code so lifecycle, availability,
    # billing, and SQLite migration rules can run headlessly through CTest.
    file(GLOB_RECURSE BUSINESS_TEST_SOURCES CONFIGURE_DEPENDS
        "src/models/*.cpp"
        "src/controllers/hotel/*.cpp"
        "src/controllers/booking/*.cpp"
        "src/controllers/customer/*.cpp"
        "src/controllers/room/*.cpp"
        "src/database/*.cpp"
    )

    add_executable(HotelBookingManagementBusinessTests
        tests/BusinessWorkflowTests.cpp
        ${BUSINESS_TEST_SOURCES}
    )

    target_include_directories(HotelBookingManagementBusinessTests PRIVATE
        src/models
        src/controllers
        src/controllers/hotel
        src/controllers/booking
        src/controllers/customer
        src/controllers/room
        src/database
        src/views
    )

    target_link_libraries(HotelBookingManagementBusinessTests PRIVATE
        Qt6::Core
        Qt6::Sql
    )

    # Modified: CTest does not inherit Qt Creator's runtime DLL path on Windows. Resolve the Qt and
    # compiler runtime directories from the configured toolchain so `ctest` works from a terminal too.
    get_target_property(QT_CORE_RUNTIME Qt6::Core IMPORTED_LOCATION_DEBUG)
    if(NOT QT_CORE_RUNTIME)
        get_target_property(QT_CORE_RUNTIME Qt6::Core IMPORTED_LOCATION_RELEASE)
    endif()
    get_filename_component(QT_RUNTIME_DIRECTORY "${QT_CORE_RUNTIME}" DIRECTORY)
    get_filename_component(MINGW_RUNTIME_DIRECTORY "${CMAKE_CXX_COMPILER}" DIRECTORY)
    string(REPLACE ";" "\\;" EXISTING_TEST_PATH "$ENV{PATH}")

    add_test(NAME HotelBookingManagement.BusinessWorkflow
             COMMAND "${CMAKE_COMMAND}" -E env
                     "PATH=${QT_RUNTIME_DIRECTORY};${MINGW_RUNTIME_DIRECTORY};${EXISTING_TEST_PATH}"
                     "$<TARGET_FILE:HotelBookingManagementBusinessTests>")
endif()
