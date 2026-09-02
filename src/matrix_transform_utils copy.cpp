// 最終のもの
#include "matrix_transform_utils.hpp"
#include <algorithm>
#include <bitset>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <tuple>
#include <unordered_map>
#include <random>

#include "Eigen/Dense"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/opencv.hpp"

using std::cout;
using std::endl;
using std::vector;

/**
 * @fn
 * 交番数順"ではない"アダマール行列を生成する(シルベスターの生成法によるアダマール行列)
 * @param N 生成するアダマール行列の次元（2のべき乗である必要がある）
 * @return アダマール行列
 */
vector<vector<int>> generateSylvesterHadamardMatrix(int N) {
  vector<vector<int>> SylvesterHadamardMatrix(N, vector<int>(N, 0));  // N x N の行列を作成し、0で初期化
  SylvesterHadamardMatrix[0][0] = 1;   // 左上の要素を1に設定

  // シルベスターの生成法によるアダマール行列を生成
  for (int n = 2; n <= N; n *= 2) {
    for (int i = 0; i < n / 2; i++) {
      for (int j = 0; j < n / 2; j++) {
        SylvesterHadamardMatrix[i + (n / 2)][j] = SylvesterHadamardMatrix[i][j];
        SylvesterHadamardMatrix[i][j + (n / 2)] = SylvesterHadamardMatrix[i][j];
        SylvesterHadamardMatrix[i + (n / 2)][j + (n / 2)] = -SylvesterHadamardMatrix[i][j];
      }
    }
  }

  return SylvesterHadamardMatrix;
}

/**
 * @fn
 * 交番数順アダマール行列を生成する(ウォルシュ型のアダマール行列)
 * @param N 生成するアダマール行列の次元（2のべき乗である必要がある）
 * @return 交番数順アダマール行列
 */
vector<vector<int>> generateWalshHadamardMatrix(int N) {
  // シルベスターの生成法によるアダマール行列を生成
	vector<vector<int>> SylvesterHadamardMatrix = generateSylvesterHadamardMatrix(N);

  // アダマール行列をウォルシュ順（交番数順）に並び替え
  vector<vector<int>> WalshHadamardMatrix(N, vector<int>(N, 0));
  for (int i = 0; i < N; i++) {
    int signSwapCount = 0;
    for (int j = 0; j < N - 1; j++) {
      if (SylvesterHadamardMatrix[i][j] != SylvesterHadamardMatrix[i][j + 1]) {
        signSwapCount++;
      }
    }
    for (int j = 0; j < N; j++) {
      WalshHadamardMatrix[signSwapCount][j] = SylvesterHadamardMatrix[i][j];
    }
  }

  return WalshHadamardMatrix;
}

/**
 * @fn
 * ドブルイン行列を生成する（ドブルイン系列に基づいて交番数順アダマール行列を並び替えたもの）
 * @param N 行列の行サイズ（あるいは列サイズ）
 * @param debruijnSeqFilePath ドブルイン系列のdatファイルのパス
 * @param targetLine 指定する行
 * @param lineStartIndex 行内の開始位置
 * @return ドブルイン行列（交番数順アダマール行列をドブルイン系列に基づいて並び替えた行列）
 */
vector<vector<int>> generateDebruijnMatrix(int N, const std::string& debruijnSeqFilePath, int targetLine, int lineStartIndex) {
	std::ifstream debruijnSeqFile(debruijnSeqFilePath);

  // ファイルのオープンに失敗した場合
  if (!debruijnSeqFile.is_open()) {
    throw std::runtime_error("ファイルを開けませんでした。");
  }

  // ドブルイン系列が文字列として格納される変数
  std::string debruijnSeq;
  int lineCount = 0;

  // ファイルから行ごとに読み込み、目標行のドブルイン系列を取得
  while (std::getline(debruijnSeqFile, debruijnSeq)) {
    lineCount++;

    if (lineCount == targetLine) {
      // 改行文字を削除（\rと\nの両方に対応）
      debruijnSeq.erase(std::remove(debruijnSeq.begin(), debruijnSeq.end(), '\r'), debruijnSeq.end());
      debruijnSeq.erase(std::remove(debruijnSeq.begin(), debruijnSeq.end(), '\n'), debruijnSeq.end());
      
      cout << "使用するドブルイン系列：" << debruijnSeq << endl;
      break;
    }
  }

  // 目標行が見つからなかった場合
  if (lineCount < targetLine) {
    throw std::runtime_error("エラー: 指定された行が見つかりませんでした。");
  }

  int windowSize;  // ドブルイン系列を切り出す窓サイズ（系列長が64なら6, 256なら8）

  // ドブルイン系列の長さに合わせた窓サイズを設定
  size_t seqLength = debruijnSeq.length();
  
  // 先頭にプレフィックス"00000000"がある場合の処理
  // ファイルフォーマット: プレフィックス(8文字) + データ(248文字) = 256文字
  // または、データのみ(64文字 or 256文字)
  if (seqLength == 256 && seqLength >= 8 && debruijnSeq.substr(0, 8) == "00000000") {
    // プレフィックスを含めて256文字の場合、プレフィックスはデータの一部として扱う
    // または、プレフィックスを削除して248文字のデータを使用する
    // ここでは、プレフィックスを含めた256文字全体を使用する
    windowSize = 8;
  } else if (seqLength == 64) {
    windowSize = 6;
  } else if (seqLength == 256) {
    windowSize = 8;
  } else if (seqLength == 248) {
    // プレフィックスを削除した後の248文字の場合
    // 248文字では256個の値（2^8）を表現できないため、
    // 循環的に拡張して256文字にする必要がある
    // または、プレフィックスを追加して256文字にする
    std::cerr << "警告: ドブルイン系列長が248文字です。256文字に拡張します。" << std::endl;
    // プレフィックス"00000000"を追加して256文字にする
    debruijnSeq = "00000000" + debruijnSeq;
    seqLength = 256;
    windowSize = 8;
  } else {
    std::cerr << "エラー: ドブルイン系列長が未対応の長さです (長さ: " << seqLength << ")" << std::endl;
    throw std::runtime_error("エラー: ドブルイン系列長が未対応の長さです。64または256文字（プレフィックス含む）である必要があります。");
  }
  
  // Nと系列長の整合性チェック
  if (N != static_cast<int>(seqLength)) {
    std::cerr << "警告: 行列サイズN(" << N << ")とドブルイン系列長(" << seqLength << ")が一致しません。" << std::endl;
  }

  // ドブルイン系列の窓サイズごとの値(10進数)を格納する配列
  vector<int> debruijnSeqWindowed(N, 0);

  // ドブルイン系列を複製して後ろにくっつける（窓サイズごとに取り出す際の循環を考慮するため）
  debruijnSeq += debruijnSeq;

  // ドブルイン系列を窓サイズごとに十進数へ変換
  for (int i = 0; i < N; i++) {
    // ドブルイン系列を窓サイズ分だけ文字列として取り出す
    std::string seqWindowed =
        debruijnSeq.substr(lineStartIndex + i, windowSize);
    // 文字列を2進数として解釈し、10進数の値へ変換
    debruijnSeqWindowed[i] = std::stoi(seqWindowed, 0, 2);
  }

  // 十進数に変換したドブルイン系列を出力
  cout << "十進数に変換したドブルイン系列：";
  for (int i = 0; i < N; i++) {
    cout << i << ":"<< debruijnSeqWindowed[i] << " ";
  }
  cout << endl;

  // ファイルを閉じる
  debruijnSeqFile.close();

	// 交番数順アダマール行列を生成
	vector<vector<int>> WalshHadamardMatrix = generateWalshHadamardMatrix(N);

  // NxNの行列を作成し、0で初期化
  vector<vector<int>> debruijnMatrix(N, vector<int>(N, 0));

  // 交番数順アダマール行列をドブルイン系列で並び替える
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      debruijnMatrix[j][i] = WalshHadamardMatrix[j][debruijnSeqWindowed[i]];
    }
  }

  return debruijnMatrix;
}

/**
 * @fn
 * グレースケール画像を読み込んでMat行列を出力
 * @param imageFilePath 画像のパス
 * @return 画像のMat行列
 */
cv::Mat loadGrayImageAsMatrix(const std::string& imageFilePath) {
  cv::Mat image = cv::imread(imageFilePath, cv::IMREAD_GRAYSCALE); // グレースケールで読み込む
  if (image.empty()) {
    throw std::runtime_error("エラー: 画像を読み込めませんでした。");
  }
  return image;
}

/**
 * @fn
 * 画像を直交変換する
 * @param inputImage 入力画像
 * @param orthogonalMatrix  直交行列
 * @return 直交変換後に得られる周波数領域の係数行列
 */
vector<vector<double>> orthogonalTransform(const cv::Mat& inputImage, const vector<vector<int>>& orthogonalMatrix) {
  // 直交行列の行サイズ（次数N）を取得
  size_t N = orthogonalMatrix.size();

  // 周波数領域の係数を格納する行列
  vector<vector<double>> freqMatrix(N, vector<double>(N, 0.0));

  // Eigenライブラリの行列型を使用
  Eigen::MatrixXd inputImageEigen(N, N);
  Eigen::MatrixXd orthogonalMatrixEigen(N, N);

  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      inputImageEigen(i, j) = inputImage.at<uchar>(i, j);
      orthogonalMatrixEigen(i, j) = orthogonalMatrix[i][j];
    }
  }

  // 行列演算を行い変換
  Eigen::MatrixXd freqMatrixEigen = (orthogonalMatrixEigen * inputImageEigen * orthogonalMatrixEigen.transpose()) / static_cast<double>(N);

  // Eigenからvectorへ
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      freqMatrix[i][j] = freqMatrixEigen(i, j);
    }
  }

  return freqMatrix;
}

/**
 * @fn
 * 係数行列を直交変換（逆変換）して、画像行列を生成する
 * 逆変換後に得られた浮動小数点数は、0~255の整数（画素値）に丸められる
 * @param embeddedFreqMatrix 情報が埋め込まれた周波数行列
 * @param orthogonalMatrix  直交行列
 * @return 逆変換後に得られる画像（行列）
 */
cv::Mat inverseOrthogonalTransform(const vector<vector<double>>& embeddedFreqMatrix, const vector<vector<int>>& orthogonalMatrix) {
  // 直交行列の行サイズ（次数N）を取得
  int N = static_cast<int>(orthogonalMatrix.size());

  // 出力されるステゴ画像
  cv::Mat stegoImage(N, N, CV_8UC1);

  // Eigenライブラリの行列型を使用
  Eigen::MatrixXd embeddedFreqMatrixEigen(N, N);
  Eigen::MatrixXd orthogonalMatrixEigen(N, N);

  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      embeddedFreqMatrixEigen(i, j) = embeddedFreqMatrix[i][j];
      orthogonalMatrixEigen(i, j) = orthogonalMatrix[i][j];
    }
  }

  // 行列演算を行い逆変換
  Eigen::MatrixXd stegoImageEigen = (orthogonalMatrixEigen.transpose() * embeddedFreqMatrixEigen * orthogonalMatrixEigen) / static_cast<double>(N);

  // 結果を四捨五入して整数に丸め、さらに0~255にクリッピングして画素値とする
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      int pixelValue = static_cast<int>(std::round(stegoImageEigen(i, j)));
      pixelValue = std::max(0, std::min(255, pixelValue));
      stegoImage.at<uchar>(i, j) = static_cast<uchar>(pixelValue);
    }
  }

  return stegoImage;
}


/**
 * @fn
 * 指定されたパスにディレクトリを作成
 * @param path 作成するディレクトリのパス
 */
void createDirectory(const std::string& path) {
    // 文字列からpathオブジェクトを作成
    std::filesystem::path dirPath(path);
    // ディレクトリが存在しない場合は作成
    if (!std::filesystem::exists(dirPath)) {
        if (std::filesystem::create_directories(dirPath)) {
            std::cout << "ディレクトリを正常に作成しました: " << path << std::endl;
        } else {
            std::cout << "ディレクトリの作成に失敗しました: " << path << std::endl;
        }
    }
}

/**
 * @fn
 * PSNR（Peak Signal-to-Noise Ratio）を計算する関数
 * @param img1 比較対象の画像1
 * @param img2 比較対象の画像2
 * @return PSNR[dB]（小数点以下第三位まで）
 */
double calculatePSNR(const cv::Mat& img1, const cv::Mat& img2) {
  cv::Mat diff;
  cv::absdiff(img1, img2, diff);
  diff.convertTo(diff, CV_32F);
  cv::Mat squaredDiff = diff.mul(diff);
  cv::Scalar mse = cv::mean(squaredDiff);

  double psnr;
  if (mse[0] <= 1e-10) {
    psnr = 100.0;  // PSNRは無限大（画像は同一）
  } else {
    double maxPixelValue = 255.0;
    psnr = 10.0 * log10((maxPixelValue * maxPixelValue) / mse[0]);
  }

  // 小数点以下第三位で四捨五入
  psnr = std::round(psnr * 1000.0) / 1000.0;

  return psnr;
}