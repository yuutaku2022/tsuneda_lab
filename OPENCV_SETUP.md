# OpenCVパス設定ガイド（Windows環境）

## 方法1: CMake実行時にOpenCV_DIRを指定する（推奨）

CMakeを実行する際に、OpenCVのパスを直接指定します。

```powershell
cd build
cmake -DOpenCV_DIR=C:\opencv\build -G "Visual Studio 17 2022" ..
cmake --build . --target compare_th_if --config Release
```

Visual Studio 2022の場合は、適切なコンパイラバージョンのパスを指定する必要がある場合もあります：

```powershell
cmake -DOpenCV_DIR=C:\opencv\build\x64\vc17 -G "Visual Studio 17 2022" ..
```

## 方法2: 環境変数を設定する

PowerShellで一時的に設定（現在のセッションのみ）：

```powershell
$env:OpenCV_DIR = "C:\opencv\build"
cd build
cmake -G "Visual Studio 17 2022" ..
```

永続的に設定する場合（システム環境変数）：

1. Windowsの設定を開く
2. 「システム」→「詳細情報」→「システムの詳細設定」
3. 「環境変数」をクリック
4. 「システム環境変数」で「新規」をクリック
5. 変数名: `OpenCV_DIR`
6. 変数値: `C:\opencv\build`
7. OKをクリック

または、PowerShell（管理者権限）で：

```powershell
[System.Environment]::SetEnvironmentVariable("OpenCV_DIR", "C:\opencv\build", "Machine")
```

## 方法3: CMakeLists.txtを修正する

CMakeLists.txtに直接パスを指定することもできます：

```cmake
# OpenCVのパスを直接指定（find_packageの前）
set(OpenCV_DIR "C:/opencv/build" CACHE PATH "OpenCV directory")

# OpenCVライブラリを探す
find_package(OpenCV REQUIRED)
```

## 方法4: CMAKE_PREFIX_PATHを使用する

```powershell
cmake -DCMAKE_PREFIX_PATH=C:\opencv\build -G "Visual Studio 17 2022" ..
```

## 確認方法

CMakeがOpenCVを見つけたか確認：

```powershell
cd build
cmake -DOpenCV_DIR=C:\opencv\build -G "Visual Studio 17 2022" .. 2>&1 | Select-String -Pattern "OpenCV"
```

正常に見つかると以下のようなメッセージが表示されます：
```
-- Found OpenCV: C:/opencv/build (found version "4.x.x")
```

## トラブルシューティング

### OpenCVが見つからない場合

1. **パスの確認**:
   ```powershell
   Test-Path "C:\opencv\build\OpenCVConfig.cmake"
   ```
   これが`True`を返すことを確認してください。

2. **複数のOpenCVバージョンがある場合**:
   ビルドに使用するVisual Studioのバージョンに対応するOpenCVのパスを指定してください。
   - Visual Studio 2015: `C:\opencv\build\x64\vc14`
   - Visual Studio 2017: `C:\opencv\build\x64\vc15`
   - Visual Studio 2019/2022: `C:\opencv\build\x64\vc16` または `C:\opencv\build`

3. **CMakeCache.txtを削除して再試行**:
   ```powershell
   cd build
   Remove-Item CMakeCache.txt -ErrorAction SilentlyContinue
   cmake -DOpenCV_DIR=C:\opencv\build -G "Visual Studio 17 2022" ..
   ```

## 実行例

```powershell
# 1. buildディレクトリに移動
cd build

# 2. CMakeCache.txtを削除（最初の設定時）
Remove-Item CMakeCache.txt -ErrorAction SilentlyContinue

# 3. OpenCV_DIRを指定してCMakeを実行
cmake -DOpenCV_DIR=C:\opencv\build -G "Visual Studio 17 2022" ..

# 4. ビルド
cmake --build . --target compare_th_if --config Release

# 5. 実行
.\Release\compare_th_if.exe
```
