# ステガノグラフィ研究プロジェクト

このリポジトリは、画像の周波数領域に秘密画像を埋め込むステガノグラフィの実験コードです。
主に、Hadamard変換とDe Bruijn系列を使って、埋め込み手法ごとの差分や変換行列の違いが画質や抽出精度にどう影響するかを比較するための研究用プロジェクトです。

このリポジトリの中心的な研究テーマは、次の3つです。

- どの変換行列を使うと、埋め込み後の画質が良くなるか
- どの領域に、どのビット位置で埋め込みすると安定するか
- 閾値調整手法とビット反転手法の比較

---

## 1. このプロジェクトで何をしているか

このプロジェクトでは、カバー画像を変換し、係数行列の一部を利用して秘密画像を埋め込みます。
その後、埋め込んだステゴ画像から秘密画像を抽出し、PSNRなどの値で評価します。

主な流れは次の通りです。

1. カバー画像を読み込む
2. 画像をHadamard変換などの変換行列で係数化する
3. 係数行列の一部を利用して秘密データを埋め込む
4. 逆変換してステゴ画像を生成する
5. その画像から秘密画像を抽出する
6. 元画像との比較から画質評価を行う

実験では、以下のような比較をしています。

- Sylvester Hadamard行列
- Walsh Hadamard行列
- De Bruijn系列で並べ替えた行列
- ビット反転埋め込み
- 閾値調整埋め込み
- 1bit / 2bit 埋め込み
- さまざまな秘密画像サイズや画像種類

実際のコードでは、埋め込み領域やビット位置を変えながら複数の条件を自動で試し、結果を出力ディレクトリに保存しています。

---

## 2. ディレクトリ構成

```text
tsuneda_lab/
├── CMakeLists.txt                  # CMakeのビルド設定
├── README.md                       # この説明書
├── README_TH_IF_COMPARISON.md      # TH/IF比較用の補足資料
├── IMAGE_SIZES.md                  # 画像サイズや埋め込み範囲の補足資料
├── EIGEN_INCLUDE_FIX.md            # Eigenのインクルード設定メモ
├── OPENCV_SETUP.md                 # OpenCVセットアップメモ
├── build/                          # CMakeで生成されたビルド成果物
│   ├── cmake_install.cmake
│   ├── CMakeCache.txt
│   ├── steganography_project
│   ├── compare_th_if
│   └── CMakeFiles/
├── include/
│   └── eigen-3.4.0/                # Eigen3のローカルコピー
├── input/
│   ├── dat/                        # 追加データ
│   ├── debruijn/                   # De Bruijn系列の.datファイル
│   └── img/
│       ├── cover_image/            # カバー画像
│       └── secret_image/           # 秘密画像
├── output/
│   ├── cover_image_comparison/     # 比較出力
│   ├── cover_image_comparison1/
│   ├── cover_image_comparison2/
│   ├── cover_image_comparison3/
│   ├── cover_image_comparison4/
│   ├── cover_image_comparison5/
│   ├── cover_image_comparison_python/
│   ├── 係数をビット反転して埋め込み/
│   ├── 閾値に基づいて係数行列に埋め込み/
│   └── ...
├── output1/
│   ├── 係数をビット反転して埋め込み/
│   └── 閾値に基づいて係数行列に埋め込み/
├── python/
│   ├── compare_th_if.py            # Python版の比較実験
│   ├── embed_by_threshold_adjust.py
│   ├── heatmap_coefficient_matrix.py
│   ├── matrix_transform_utils.py
│   ├── README.md
│   └── requirements.txt
├── src/
│   ├── main.cpp                    # 実験のメイン実行
│   ├── matrix_transform_utils.hpp/.cpp
│   ├── embed_by_bit_inversion.hpp/.cpp
│   ├── embed_by_threshold_adjust.hpp/.cpp
│   ├── compare_th_if.cpp           # TH/IF比較用実行
│   ├── compare_th_if2.cpp
│   ├── compare_debruijn.cpp
│   ├── lenna_hadamard_debruijn_heatmap.cpp
│   └── ...
├── plot_by_image.py
├── plot_by_targetline.py
├── plot_comparison.py
├── plot_TH.py
└── .gitignore
```

主な用途別に言うと、次のような役割です。

- src/: C++実装
- python/: Python版の比較・可視化に関するスクリプト
- input/: 画像やDe Bruijn系列の入力データ
- output/: 実験結果や画像出力の保存先
- include/: ローカルのEigenライブラリ
- build/: ビルド用ディレクトリ

---

## 3. 実装されている手法

### 3.1 ビット反転埋め込み
係数行列の特定ビットを反転して、秘密情報を埋め込む手法です。

- 1bitまたは2bitずつ埋め込む
- 埋め込み領域を変えて実験する
- 係数行列の左上、右上、左下、右下などの領域を比較
- 全体の係数行列や四隅にも対応

### 3.2 閾値調整埋め込み
論文で扱われた閾値に基づく埋め込み手法です。TH（threshold）とIF（insert factor）を調整しながら比較します。

- THとIFを変えながら品質と容量のバランスを検討
- さまざまな秘密画像サイズに対応
- 変換行列ごとの比較が可能

### 3.3 変換行列の比較
以下の変換行列を使って比較しています。

- Sylvester型 Hadamard行列
- Walsh型 Hadamard行列
- De Bruijn系列に基づいて並べ替えた行列

これにより、同じ画像でも行列の選び方で画質や埋め込みの安定性が変わるかを検証できます。

---

## 4. 使用しているライブラリ

### C++側

- C++17
- CMake
- OpenCV
- Eigen 3.4

### Python側

- NumPy
- pandas
- matplotlib
- OpenCV（必要に応じて）

実際には、C++で主な処理が実装されていて、Pythonでは比較や可視化を補助する用途が中心です。

特にこのプロジェクトでは、

- OpenCV: 画像読み込み、保存、変換
- Eigen: 係数行列や線形代数計算
- CMake: ビルド管理

の3つが中心です。

---

## 5. 実行される内容のイメージ

このプロジェクトの主な出力は次のようなものです。

- ステゴ画像
- 抽出した秘密画像
- PSNR値
- 実験結果CSV
- ヒートマップや比較グラフ

例えば、入力として以下を使います。

- カバー画像: 256×256のグレースケール画像
- 秘密画像: 32×32, 64×64, 128×128, 256×256の画像
- 実験画像: Lenna, Barbara, Airplane, Boat, Cameraman, Mandrill など

結果は output/ 以下や build/ で確認できます。

---

## 6. ビルド手順

### 6.1 必要なもの

- Git
- CMake
- C++17対応コンパイラ
- OpenCV
- Python 3.x（Pythonスクリプトを使う場合）

Windows環境では、Visual Studio Build Tools または MSYS2 / MinGW などのC++開発環境が必要になることがあります。

### 6.2 ビルド例

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

または、WindowsのPowerShellでは次のように実行します。

```powershell
New-Item -ItemType Directory -Force -Path build
Set-Location build
cmake ..
cmake --build .
```

生成される実行ファイルは主に次の2つです。

- steganography_project
- compare_th_if

---

## 7. 実行例

### 7.1 C++版の実行

```bash
./steganography_project
```

または Windows では:

```powershell
.
\build\steganography_project.exe
```

### 7.2 TH/IF比較の実行

```bash
./compare_th_if
```

または Windows では:

```powershell
.
\build\compare_th_if.exe
```

出力結果は output/ 以下に保存されます。

---

## 8. Python版の実行

Pythonの実行例は python/README.md にまとめられています。

```powershell
python .\python\compare_th_if.py --quick
```

または、依存関係が入っている環境で:

```powershell
python -m pip install -r .\python\requirements.txt
python .\python\compare_th_if.py
```

---

## 9. Git初めての人向け：クローン手順

ここでは、Gitを初めて使う人向け

### 9.1 Gitをインストールする

まず Git をインストールします。

- Windows: https://git-scm.com/download/win
- Mac: Homebrew なら `brew install git`
- Linux: `sudo apt install git`

インストール後、ターミナルで次を実行して確認します。

```bash
git --version
```

表示されればOKです。

---

### 9.2 GitHub でリポジトリのURLを確認する

このプロジェクトがGitHub上にある場合、GitHubのページにある「Code」ボタンを押し、HTTPSのURLをコピーします。

例:

```text
https://github.com/yuutaku2022/tsuneda_lab.git
```

もし GitHub で公開されていないローカルのリポジトリを使う場合は、別途そのURLを使ってクローンします。

> URLは人によって違うため、実際のURLは自分のリポジトリのページを確認してください。

---

### 9.3 どこにクローンするか決める

任意の作業フォルダを作成して、その中へ入れます。

例:

```bash
mkdir my_projects
cd my_projects
```

または、デスクトップやDocumentsの中で実行しても構いません。

---

### 9.4 クローンする

ターミナルで次を実行します。

```bash
git clone <リポジトリURL>
```

例:

```bash
git clone https://github.com/yuutaku2022/tsuneda_lab.git
```

これで、リポジトリ名のフォルダが作られて、その中にコードが入ります。

次にそのフォルダに入ります。

```bash
cd tsuneda_lab
```

---

### 9.5 フォルダの中身を確認する

```bash
ls
```

または Windows PowerShell では:

```powershell
Get-ChildItem
```

READMEや src/、input/ が見えればクローン成功です。

---

### 9.6 ブランチ確認

初期状態では通常 main または master ブランチの状態です。

```bash
git branch
```

もしブランチ名がmasterになっていて、mainにしたい場合は次のようにできます。

```bash
git branch -M main
```

---

### 9.7 変更を確認する

Gitでファイルがどう変わったかを確認したいときは:

```bash
git status
```

これで、変更中のファイルを確認できます。

---

### 9.8 変更を追加して保存する

ファイルを編集したあと、変更をGitに記録する流れは次の通りです。

```bash
git add .
git commit -m "Update README"
```

ここで、Gitの初期設定で名前とメールアドレスが未設定だと失敗することがあります。

```bash
git config --global user.name "あなたの名前"
git config --global user.email "あなたのメールアドレス"
```

そのあとでコミットを実行します。

---

### 9.9 GitHub に反映する

新しいリポジトリにpushしたい場合:

```bash
git remote -v
git push origin main
```

もし remote が未設定なら次のように追加します。

```bash
git remote add origin <リポジトリURL>
git push -u origin main
```

---

### 9.10 VS Code を使う場合の手順

Git初心者には、VS Code の画面から操作する方法もわかりやすいです。

1. VS Code を開く
2. 左下の緑色のソース管理アイコンを押す
3. 「Clone Repository」を選ぶ
4. GitHubのURLを入力する
5. 保存先を選ぶ
6. フォルダを開く

その後、ファイルの編集やGitの状態確認は VS Code のソース管理画面で行えます。

---

## 10. よくあるトラブル

### OpenCV が見つからない

CMakeLists.txt の中で OpenCV のパスを探します。Windows環境では以下のように確認します。

- C:\opencv\build
- C:\opencv\build\x64\vc15

もしOpenCVが別の場所にある場合は、CMakeLists.txt の設定を修正してください。

### Eigen が見つからない

ローカルに include/eigen-3.4.0 を入れているので、通常はそのまま動作します。
ただし、システムにEigenがインストールされている場合は、CMakeLists.txt の判定でそちらが優先されることがあります。

### ビルドに失敗する

以下を確認してください。

- CMake がインストールされているか
- C++17対応のコンパイラがあるか
- OpenCVが正しくインストールされているか
- パスに問題がないか

---

## 11. まとめ

このプロジェクトは、画像の周波数領域で秘密情報を埋め込むステガノグラフィの実験コードです。

- Hadamard変換を使う
- De Bruijn系列も比較する
- 異なる変換行列や埋め込み方式を比較する
- PSNRなどで画質や抽出精度を評価する
- C++で実験をし、Pythonで可視化や比較を補助する

Git初心者の方は、まず最初にこのREADMEを見て、クローン→ビルド→実行の順に進めてください。

必要なら次に、

- 実験の流れを図で整理した説明
- src/ の関数ごとの役割説明
- どのファイルを編集すれば実験条件を変更できるか

まで続けて書けます。


