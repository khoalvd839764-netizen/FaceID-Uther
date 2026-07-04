cmake_minimum_required(VERSION 3.19)
project(FacieID_UtherDashbbord LANGUAGES CXX)

# Require C++17 or later (needed for OpenCV and modern procedural features)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# TÌM CÁC THƯ VIỆN CẦN THIẾT
# ============================================================================

# Qt6: Core (nền tảng), Widgets (giao diện), Sql (SQLite)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Sql)

# OpenCV: Xử lý ảnh + Camera + DNN (chạy model AI)
find_package(OpenCV REQUIRED COMPONENTS core imgproc videoio objdetect dnn)

qt_standard_project_setup()

# ============================================================================
# KHAI BÁO CÁC FILE NGUỒN (Phong cách lập trình thủ tục, không OOP)
# ============================================================================
qt_add_executable(FacieID_UtherDashbbord
    WIN32 MACOSX_BUNDLE

    # --- File chạy chính và giao diện ---
    main.cpp

    # --- Backend thủ tục (dùng namespace & free functions) ---
    backend/Database.h
    backend/Database.cpp
    backend/Camera.h
    backend/Camera.cpp
    backend/Face.h
    backend/Face.cpp
)

# ============================================================================
# LIÊN KẾT THƯ VIỆN
# ============================================================================
target_link_libraries(FacieID_UtherDashbbord
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Sql
        ${OpenCV_LIBS}   # Tự động link tất cả module OpenCV đã tìm thấy
)

# Cho phép #include "backend/xxx.h" hoạt động từ main.cpp
target_include_directories(FacieID_UtherDashbbord PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
