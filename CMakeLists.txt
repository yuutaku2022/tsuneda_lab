# CMakeのバージョンを設定
cmake_minimum_required(VERSION 3.10)

# C++のバージョンをC++17に指定
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 文字エンコーディングをUTF-8に指定


# プロジェクト名を設定
project(steganography_project)

# 実行ファイルを作成
add_executable(${PROJECT_NAME} src/main.cpp src/matrix_transform_utils.cpp src/embed_by_bit_inversion.cpp src/embed_by_threshold_adjust.cpp)

# OpenCVライブラリを探す
find_package(OpenCV REQUIRED)

# OpenCVライブラリをリンク
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})

# Eigen3のincludeパスを指定
target_include_directories(${PROJECT_NAME} PUBLIC "./include/eigen-3.4.0")

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)