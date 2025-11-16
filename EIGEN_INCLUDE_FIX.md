# Eigenインクルードエラーの原因と修正

## 以前のエラーの原因

### 考えられる原因

1. **インクルードパスの設定の問題**
   - 以前は `find_package(Eigen3)` が失敗した場合の処理が適切でなかった可能性
   - または、インクルードパスが正しく設定されていなかった

2. **インクルード方法の違い**
   - コードでは `#include "Eigen/Dense"` とクォート（`"`）を使用
   - システムのライブラリの場合、通常は `#include <Eigen/Dense>` とアングルブラケット（`<>`）を使用
   - しかし、インクルードパスが正しく設定されていれば、どちらでも動作する

3. **システムのEigen3とローカルのEigen3の混在**
   - システムにEigen3がインストールされている場合と、ローカルのEigen3を使う場合で、パスの設定が異なる
   - 以前は、この違いが適切に処理されていなかった可能性

## 現在の修正内容

### CMakeLists.txtの設定

```cmake
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
```

### コードでのインクルード

```cpp
#include "Eigen/Dense"
```

### 動作の仕組み

1. **システムのEigen3を使う場合**:
   - `EIGEN3_INCLUDE_DIR` は通常 `/usr/include/eigen3` になる
   - インクルードパスに `/usr/include/eigen3` が追加される
   - `#include "Eigen/Dense"` は `/usr/include/eigen3/Eigen/Dense` を探す
   - これは正しく動作する

2. **ローカルのEigen3を使う場合**:
   - インクルードパスに `include/eigen-3.4.0` が追加される
   - `#include "Eigen/Dense"` は `include/eigen-3.4.0/Eigen/Dense` を探す
   - これも正しく動作する

## 以前の問題点

### 考えられる問題

1. **`find_package(Eigen3)` が失敗した場合の処理**
   - 以前は、システムのEigen3が見つからない場合の処理が適切でなかった可能性
   - 現在は、`QUIET` オプションを使用して、エラーを出さずにフォールバック処理を行っている

2. **インクルードパスの設定タイミング**
   - 以前は、インクルードパスの設定が適切なタイミングで行われていなかった可能性
   - 現在は、`target_include_directories` を使用して、各ターゲットに対して明示的にインクルードパスを設定している

3. **パスの表記**
   - Windows環境とLinux環境でパスの表記が異なる可能性
   - 現在は、`${CMAKE_SOURCE_DIR}` を使用して、プラットフォームに依存しないパス指定を行っている

## 確認方法

### ビルド時のメッセージを確認

```bash
cmake ..
```

以下のようなメッセージが表示されます：

- システムのEigen3を使う場合（**正常な動作**）:
  ```
  -- Found Eigen3: /usr/include/eigen3
  ```
  → このメッセージが表示されれば、システムのEigen3が正しく見つかっています。

- ローカルのEigen3を使う場合:
  ```
  -- Eigen3 not found in system, using local: /path/to/project/include/eigen-3.4.0
  ```

### システムのEigen3のパス構造

システムのEigen3が `/usr/include/eigen3` にある場合：
- インクルードパス: `/usr/include/eigen3`
- コードでの記述: `#include "Eigen/Dense"`
- 実際に読み込まれるファイル: `/usr/include/eigen3/Eigen/Dense`

これは正しく動作します。CMakeが自動的にインクルードパスを設定しているため、コードでは `#include "Eigen/Dense"` と書くだけで、正しいパスから読み込まれます。

### コンパイル時のインクルードパスを確認

```bash
make VERBOSE=1
```

または

```bash
cmake --build . --verbose
```

コンパイルコマンドに `-I/path/to/eigen3` または `-I/path/to/include/eigen-3.4.0` が含まれていることを確認できます。

## まとめ

現在の設定では、以下のように動作します：

1. システムのEigen3を優先的に探す
2. 見つからない場合は、ローカルのEigen3を使用
3. インクルードパスは、各ターゲットに対して明示的に設定される
4. プラットフォームに依存しないパス指定を使用

これにより、システムにEigen3がインストールされているかどうかに関わらず、正しくコンパイルできるようになっています。

## 現在の状態（`-- Found Eigen3: /usr/include/eigen3` が表示された場合）

このメッセージが表示されれば、**正常に動作しています**。

- ✅ システムのEigen3が正しく見つかっている
- ✅ インクルードパスが正しく設定されている
- ✅ コンパイル時に `/usr/include/eigen3` がインクルードパスに追加される
- ✅ コード内の `#include "Eigen/Dense"` が正しく解決される

### 以前のエラーが起きていた原因（推測）

1. **インクルードパスの設定が不適切だった**
   - `target_include_directories` が正しく設定されていなかった
   - または、設定するタイミングが間違っていた

2. **Eigen3の検出が失敗していた**
   - `find_package(Eigen3)` が失敗し、フォールバック処理も適切でなかった
   - または、`QUIET` オプションがなく、エラーが表示されていた

3. **パスの表記の問題**
   - Windows環境とLinux環境でのパス表記の違い
   - 相対パスと絶対パスの混在

現在の設定では、これらの問題が解決されているため、正常に動作しています。

