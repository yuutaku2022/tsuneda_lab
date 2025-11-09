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

# THとIFの比較用実行ファイルを作成
add_executable(compare_th_if src/compare_th_if.cpp src/matrix_transform_utils.cpp src/embed_by_threshold_adjust.cpp)

# OpenCVライブラリを探す
# OpenCVのパスを指定（Windows環境の場合）
if(WIN32 AND NOT OpenCV_DIR)
    # 一般的なOpenCVのインストールパスを確認
    if(EXISTS "C:/opencv/build")
        set(OpenCV_DIR "C:/opencv/build" CACHE PATH "OpenCV directory")
    elseif(EXISTS "C:/opencv/build/x64/vc15")
        set(OpenCV_DIR "C:/opencv/build/x64/vc15" CACHE PATH "OpenCV directory")
    endif()
endif()
find_package(OpenCV REQUIRED)

# OpenCVライブラリをリンク
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})
target_link_libraries(compare_th_if ${OpenCV_LIBS})

# OpenCVのincludeディレクトリを追加
target_include_directories(compare_th_if PUBLIC ${OpenCV_INCLUDE_DIRS})

# Eigen3ライブラリを探す（システムにインストールされているものを優先）
find_package(Eigen3 3.4 QUIET)
if(Eigen3_FOUND)
    # システムにインストールされているEigen3を使用
    message(STATUS "Found Eigen3: ${EIGEN3_INCLUDE_DIR}")
    target_include_directories(${PROJECT_NAME} PUBLIC ${EIGEN3_INCLUDE_DIR})
    target_include_directories(compare_th_if PUBLIC ${EIGEN3_INCLUDE_DIR})
else()
    # システムにEigen3が見つからない場合は、ローカルのEigen3を使用
    message(STATUS "Eigen3 not found in system, using local: ${CMAKE_SOURCE_DIR}/include/eigen-3.4.0")
    target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/include/eigen-3.4.0")
    target_include_directories(compare_th_if PUBLIC "${CMAKE_SOURCE_DIR}/include/eigen-3.4.0")
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)