# THとIFのパラメータ比較ツール

このツールは、閾値調整方式のステガノグラフィにおいて、TH（閾値）とIF（Insert Factor）の組み合わせを変えて実行し、結果を比較・可視化するためのものです。

## ファイル構成

- `src/compare_th_if.cpp`: THとIFを変えて実行し、結果をCSVに出力するC++プログラム
- `plot_comparison.py`: CSVファイルからデータを読み込み、プロットを生成するPythonスクリプト

## 使い方

### 1. ビルド

```bash
cd build
cmake ..
make
```

これで `compare_th_if` という実行ファイルが生成されます。

### 2. 実行（データ収集）

```bash
./compare_th_if
```

または、buildディレクトリから実行する場合:

```bash
./compare_th_if
```

このプログラムは以下の処理を行います：

1. TH値: 10, 15, 20, 25, 30, 35, 40, 45, 50
2. IF値: 0.05, 0.10, 0.15, 0.20, 0.25, 0.30
3. 行列タイプ: Sylvester_Hadamard, Walsh_Hadamard, Debruijn

の全組み合わせ（合計 9 × 6 × 3 = 162パターン）について埋め込み・抽出を実行し、
結果を `../output/th_if_comparison/comparison_results.csv` に保存します。

### 3. プロット生成

```bash
python plot_comparison.py
```

このスクリプトは以下のプロットを生成します：

1. **ヒートマップ**: THとIFの組み合わせによるPSNRの2次元ヒートマップ
2. **3Dサーフェスプロット**: THとIFに対するPSNRの3次元プロット
3. **線グラフ**: IF固定時のTH vs PSNR、TH固定時のIF vs PSNR
4. **全体比較**: 各行列タイプの比較散布図

すべてのプロットは `../output/th_if_comparison/plots/` に保存されます。

## パラメータのカスタマイズ

### TH値とIF値の範囲を変更する場合

`src/compare_th_if.cpp` の以下の部分を編集してください：

```cpp
// THとIFの範囲を定義
vector<int> TH_values = {10, 15, 20, 25, 30, 35, 40, 45, 50};
vector<double> IF_values = {0.05, 0.10, 0.15, 0.20, 0.25, 0.30};
```

### 使用する画像を変更する場合

`src/compare_th_if.cpp` の以下の部分を編集してください：

```cpp
// 読み込むカバー画像のパス
std::string coverImageFilePath = "../input/img/cover_image/Lenna_grayscale_256.bmp";

// 読み込む秘密画像のパス
std::string secretImageFilePath = "../input/img/secret_image/secret_grayscale_64x64.bmp";
```

## 出力ファイル

### CSVファイル

`output/th_if_comparison/comparison_results.csv`

形式：
```
Matrix Type,TH,IF,Stego PSNR,Extracted PSNR
Sylvester_Hadamard,10,0.050,45.23,38.56
Walsh_Hadamard,10,0.050,45.18,38.51
...
```

### プロット画像

`output/th_if_comparison/plots/`

- `{MatrixType}_heatmap.png`: ヒートマップ
- `{MatrixType}_3d.png`: 3Dサーフェスプロット
- `{MatrixType}_line.png`: 線グラフ
- `overall_comparison.png`: 全体比較

## 必要なライブラリ

### C++プログラム
- OpenCV
- Eigen3

### Pythonスクリプト
```bash
pip install pandas matplotlib numpy
```

## 注意事項

- 実行には時間がかかる場合があります（162パターン × 埋め込み・抽出処理）
- 十分なディスク容量があることを確認してください（生成される画像ファイルが多いため）
