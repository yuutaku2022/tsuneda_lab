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

#include "Eigen/Dense"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/opencv.hpp"

#include "matrix_transform_utils.hpp"

using std::cout;
using std::endl;
using std::vector;

const int BITS_PER_PIXEL = 8;  // 1ピクセルあたりのビット数
const int BITS_PER_CHANNEL = 8;  // 各チャネルにおける1ピクセルあたりのビット数

/**
 * @fn
 * 埋め込みを行う位置をtrueとするマスク行列を作成する（矩形領域）
 * @param freqMatrix 周波数領域の係数行列
 * @param secretImage 秘密画像
 * @param regionTopLeftX 埋め込みを行う領域の左上端のX座標
 * @param regionTopLeftY 埋め込みを行う領域の左上端のY座標
 * @param bitPosVec 変更したい係数行列の整数部分のビット位置が格納された配列（例: [0,1]:最下位ビットと第２ビット）（要素数が2の累乗の場合のみ許容）
 * @return 埋め込み位置をtrueとするブール値行列
 */
vector<vector<bool>> makeEmbeddedBoolMaskByRectangle(vector<vector<double>>& freqMatrix, cv::Mat& secretImage, int regionTopLeftX, int regionTopLeftY, vector<int>& bitPosVec) {
  // 係数行列のサイズを取得
  int rows = static_cast<int>(freqMatrix.size());
  int cols = static_cast<int>(freqMatrix[0].size());

  // 係数行列と同サイズかつ、要素をブール値とするvector行列を作成
  vector<vector<bool>> embeddedBoolMask(rows, vector<bool>(cols, false));

  // 埋め込みビット総数
  const int totalEmbeddedBits = static_cast<int>(secretImage.total()) * BITS_PER_CHANNEL;
  // 埋め込み要素の総数
  int totalEmbeddedElements = static_cast<int>(totalEmbeddedBits / bitPosVec.size());

  // 埋め込み領域の幅と高さを計算
  int logBase2Width = static_cast<int>(std::log2(totalEmbeddedElements) / 2) + (static_cast<int>(std::log2(totalEmbeddedElements)) % 2);
  int logBase2Height = static_cast<int>(std::log2(totalEmbeddedElements) / 2);
  const int regionWidth = static_cast<int>(pow(2, logBase2Width));
  const int regionHeight = static_cast<int>(pow(2, logBase2Height));

	cout << "埋め込み領域の幅： " << regionWidth << ", " << "埋め込み領域の高さ: " << regionHeight << endl;

  // 埋め込みを行う位置をtrueとするマスク行列を作成
  for (int i = 0; i < regionHeight; i++) {
    for (int j = 0; j < regionWidth; j++) {
      embeddedBoolMask[i + regionTopLeftY][j + regionTopLeftX] = true;
    }
  }

  return embeddedBoolMask;
}

/**
 * @fn
 * 埋め込みを行う位置をtrueとするマスク行列を作成する（行列全体を等間隔に指定）
 * @param freqMatrix 周波数領域の係数行列
 * @param secretImage 秘密画像
 * @param bitPosVec 変更したい係数行列の整数部分のビット位置が格納された配列（例: [0,1]:最下位ビットと第２ビット）（現時点では要素数が2の累乗の場合のみ許容される）
 * @return 埋め込み位置をtrueとするブール値行列
 */
vector<vector<bool>> makeEmbeddedBoolMaskByWholeArea(vector<vector<double>>& freqMatrix, cv::Mat& secretImage, vector<int>& bitPosVec) {
  // 係数行列のサイズを取得
  int rows = static_cast<int>(freqMatrix.size());
  int cols = static_cast<int>(freqMatrix[0].size());

  // 係数行列と同サイズかつ、要素をブール値とするvector行列を作成
  vector<vector<bool>> embeddedBoolMask(rows, vector<bool>(cols, false));

  // 埋め込みビット総数
  const int totalEmbeddedBits = static_cast<int>(secretImage.total()) * BITS_PER_CHANNEL;
  // 埋め込み要素の総数
  int totalEmbeddedElements = static_cast<int>(totalEmbeddedBits / bitPosVec.size());

  // 埋め込み周期を計算
	const int embedInterval = (rows * cols) / totalEmbeddedElements;

	cout << "埋め込み要素の周期： " << embedInterval << endl;

	// 埋め込みを行う位置をtrueとするマスク行列を作成
	int count = 0;
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
			// 一定の周期でtrue
			if ((i * cols + j) % embedInterval == 0) {
      	embeddedBoolMask[i][j] = true;
				count++;

				// 埋め込み要素の総数分を指定したら抜ける
				if (count == totalEmbeddedElements) {
					break;
				} 
			}
    }
  }

  return embeddedBoolMask;
}

/**
 * @fn
 * 埋め込みを行う位置をtrueとするマスク行列を作成する（行列の四隅を指定）
 * @param freqMatrix 周波数領域の係数行列
 * @param secretImage 秘密画像
 * @param bitPosVec 変更したい係数行列の整数部分のビット位置が格納された配列（例: [0,1]:最下位ビットと第２ビット）（現時点では要素数が2の累乗の場合のみ許容される）
 * @return 埋め込み位置をtrueとするブール値行列
 */
vector<vector<bool>> makeEmbeddedBoolMaskByCornerArea(vector<vector<double>>& freqMatrix, cv::Mat& secretImage, vector<int>& bitPosVec) {
  // 係数行列のサイズを取得
  int rows = static_cast<int>(freqMatrix.size());
  int cols = static_cast<int>(freqMatrix[0].size());

  // 係数行列と同サイズかつ、要素をブール値とするvector行列を作成
  vector<vector<bool>> embeddedBoolMask(rows, vector<bool>(cols, false));

  // 埋め込みビット総数
  const int totalEmbeddedBits = static_cast<int>(secretImage.total()) * BITS_PER_CHANNEL;
  // 埋め込み要素の総数
  int totalEmbeddedElements = static_cast<int>(totalEmbeddedBits / bitPosVec.size());

	// 四隅の各領域における幅と高さを計算
  int logBase2Width = static_cast<int>(std::log2(totalEmbeddedElements) / 2) + (static_cast<int>(std::log2(totalEmbeddedElements)) % 2);
  int logBase2Height = static_cast<int>(std::log2(totalEmbeddedElements) / 2);
  const int regionWidth = static_cast<int>(pow(2, logBase2Width)) / 2;
  const int regionHeight = static_cast<int>(pow(2, logBase2Height)) / 2;

	cout << "埋め込み領域の幅： " << regionWidth << ", " << "埋め込み領域の高さ: " << regionHeight << endl;

	// 埋め込みを行う位置をtrueとするマスク行列を作成
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
			// 左上
			if (i < regionHeight && j < regionWidth) {
				embeddedBoolMask[i][j] = true;
				continue;
			} 
			// 右上
			if (i < regionHeight && j >= cols - regionWidth) {
				embeddedBoolMask[i][j] = true;
				continue;
			} 
			// 左下
			if (i >= rows - regionHeight && j < regionWidth) {
				embeddedBoolMask[i][j] = true;
				continue;
			} 
			// 右下
			if (i >= rows - regionHeight && j >= cols - regionWidth) {
				embeddedBoolMask[i][j] = true;
				continue;
			} 
    }
  }

  return embeddedBoolMask;
}

/** 
 * @fn
 * 埋め込み領域の文字列から、埋め込みを行う位置をtrueとするマスク行列を生成
 * @param freqMatrix 周波数領域の係数行列
 * @param secretImage 秘密画像
 * @param embeddedAreaString 埋め込み領域の文字列
 * @param bitPosVec 変更したい係数行列の整数部分のビット位置が格納された配列（例: [0,1]:最下位ビットと第２ビット）（現時点では要素数が2の累乗の場合のみ許容される）
 * @return 埋め込みを行う位置をtrueとするマスク行列
 */
vector<vector<bool>> embeddedAreaString_to_boolMaskMatrix(vector<vector<double>>& freqMatrix, cv::Mat& secretImage, std::string& embeddedAreaString, vector<int>& bitPosVec) {
	// 係数行列の行サイズ・列サイズを取得
  int row = static_cast<int>(freqMatrix.size()); 
  int col = static_cast<int>(freqMatrix[0].size());
	// マスク行列を生成
	std::string str = embeddedAreaString;
	vector<vector<bool>> embeddedBoolMask;
	if (str == "係数行列の左上の領域(サイズ128x64)") {
		int regionTopLeftX = 0, regionTopLeftY = 0;
		embeddedBoolMask = makeEmbeddedBoolMaskByRectangle(freqMatrix, secretImage, regionTopLeftX, regionTopLeftY, bitPosVec);
	} else if (str == "係数行列の右上の領域(サイズ128x64)") {
		int regionTopLeftX = row - 128, regionTopLeftY = 0;
		embeddedBoolMask = makeEmbeddedBoolMaskByRectangle(freqMatrix, secretImage, regionTopLeftX, regionTopLeftY, bitPosVec);
	} else if (str == "係数行列の左下の領域(サイズ128x64)") {
		int regionTopLeftX = 0, regionTopLeftY = col - 64;
		embeddedBoolMask = makeEmbeddedBoolMaskByRectangle(freqMatrix, secretImage, regionTopLeftX, regionTopLeftY, bitPosVec);
	} else if (str == "係数行列の右下の領域(サイズ128x64)") {
		int regionTopLeftX = row - 128, regionTopLeftY = col - 64;
		embeddedBoolMask = makeEmbeddedBoolMaskByRectangle(freqMatrix, secretImage, regionTopLeftX, regionTopLeftY, bitPosVec);
	} else if (str == "係数行列の左上の領域(サイズ64x64)") {
		int regionTopLeftX = 0, regionTopLeftY = 0;
		embeddedBoolMask = makeEmbeddedBoolMaskByRectangle(freqMatrix, secretImage, regionTopLeftX, regionTopLeftY, bitPosVec);
	} else if (str == "係数行列の右上の領域(サイズ64x64)") {
		int regionTopLeftX = row - 64, regionTopLeftY = 0;
		embeddedBoolMask = makeEmbeddedBoolMaskByRectangle(freqMatrix, secretImage, regionTopLeftX, regionTopLeftY, bitPosVec);
	} else if (str == "係数行列の左下の領域(サイズ64x64)") {
		int regionTopLeftX = 0, regionTopLeftY = col - 64;
		embeddedBoolMask = makeEmbeddedBoolMaskByRectangle(freqMatrix, secretImage, regionTopLeftX, regionTopLeftY, bitPosVec);
	} else if (str == "係数行列の右下の領域(サイズ64x64)") {
		int regionTopLeftX = row - 64, regionTopLeftY = col - 64;
		embeddedBoolMask = makeEmbeddedBoolMaskByRectangle(freqMatrix, secretImage, regionTopLeftX, regionTopLeftY, bitPosVec);
	} else if (str == "係数行列の四隅(サイズ64x32)" || str == "係数行列の四隅(サイズ32x32)") {
		embeddedBoolMask = makeEmbeddedBoolMaskByCornerArea(freqMatrix, secretImage, bitPosVec);
	} else if ( str == "係数行列全体(周期8)" || str == "係数行列全体(周期16)") {
		embeddedBoolMask = makeEmbeddedBoolMaskByWholeArea(freqMatrix, secretImage, bitPosVec);
	}

	return embeddedBoolMask;
}

/**
 * @fn
 * 秘密画像の画素値をビットに分解する
 * @param secretImage 秘密画像
 * @return 秘密画像の全ビットが格納されたvectorの1次元配列
 */
vector<std::bitset<1>> makeBinarySecretImageVec(cv::Mat& secretImage) {
  // 秘密画像の画素値の全ビットを1次元配列のvectorに格納する
  vector<std::bitset<1>> binarySecretImageVec(secretImage.total() * BITS_PER_CHANNEL);
  for (int i = 0; i < secretImage.rows; i++) {
    for (int j = 0; j < secretImage.cols; j++) {
      // 画素値を取得
      uchar pixel = secretImage.at<uchar>(i, j);
      // 画素値を8ビットの2進数に変換
      std::bitset<8> pixelBits(pixel);
      // 格納
      for (int bitCount = 0; bitCount < BITS_PER_CHANNEL; bitCount++) {
        binarySecretImageVec[(i * secretImage.cols + j) * BITS_PER_CHANNEL + bitCount] = static_cast<int>(pixelBits[bitCount]);
      }
    }
  }

  return binarySecretImageVec;
}

/**
 * @fn
 * マスク行列を元に、係数行列に埋め込みを行う
 * @param freqMatrix 周波数領域の係数行列
 * @param binarySecretImageVec 秘密画像の全ビットが格納されたvectorの1次元配列
 * @param embeddedBoolMask 埋め込み位置をtrueとするブール値行列
 * @param bitPosVec 変更したい係数行列の整数部分のビット位置が格納された配列（例: [0, 1]:最下位ビットと第２ビット）（現時点では要素数が2の累乗の場合のみ許容される）
 * @return 埋め込み処理が行われた周波数領域の係数行列
 */
vector<vector<double>> embedInFreqDomainByBoolMask(vector<vector<double>>& freqMatrix, vector<std::bitset<1>>& binarySecretImageVec, vector<vector<bool>>& embeddedBoolMask, vector<int>& bitPosVec) {
  // 情報を埋め込まれた係数行列
  vector<vector<double>> embeddedFreqMatrix = freqMatrix;

  // 係数行列のサイズを取得
  int rows = static_cast<int>(freqMatrix.size());
  int cols = static_cast<int>(freqMatrix[0].size());

  // 埋め込みビットカウント数
  int embedCount = 0;

  // 係数行列にビットを格納
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      // falseなら飛ばす
      if (embeddedBoolMask[i][j] == false) {
        continue;
      }

      // 符号を全て正にする
      int sign = 1;
      if (freqMatrix[i][j] < 0) {
        sign = -1;
      }
      double absFreqElement = sign * freqMatrix[i][j];

      // 整数部分を取得
      int freqElementIntPart = static_cast<int>(absFreqElement);
      // 小数部分を取得
      double freqElementDecimalPart = absFreqElement - freqElementIntPart;

      // 整数部分を32ビットの2進数に変換
      std::bitset<32> binaryFreqElementIntPart(freqElementIntPart);

      // 周波数行列の整数部分の第bitPosVec[k]ビット目を2値vectorの値に置き換えて情報を埋め込む
      for (int k = 0; k < bitPosVec.size(); k++) {
        binaryFreqElementIntPart[bitPosVec[k]] = binarySecretImageVec[embedCount][0];
        embedCount++;
      }

      // 情報を埋め込まれた整数部分を10進数に変換
      int embeddedFreqElementIntPart = static_cast<int>(binaryFreqElementIntPart.to_ulong());

      // 整数部分と小数部分を合成
      embeddedFreqMatrix[i][j] = sign * (embeddedFreqElementIntPart + freqElementDecimalPart);

      // 全てのビットを埋め込んだら終了
      if (embedCount == binarySecretImageVec.size()) {
        return embeddedFreqMatrix;
      }
    }
  }

  throw std::runtime_error("エラー：すべてのビットが埋め込まれませんでした");
}

/**
 * @fn
 * 係数行列から秘密画像の抽出
 * 埋め込みの際に利用したブール行列を使用する
 * @param stegoFreqMatrix 秘密画像が埋め込まれた周波数領域の係数行列
 * @param secretImageWidth 秘密画像の幅
 * @param secretImageHeight 秘密画像の高さ
 * @param embeddedBoolMask 埋め込み位置をtrueとするブール値行列
 * @param bitPosVec
 * 係数行列の整数部分の、埋め込みが行われたビット位置が格納された配列
 * @return 埋め込み済みの係数行列から抽出された秘密画像
 */
cv::Mat extractFromFreqDomainByBoolMask(vector<vector<double>>& stegoFreqMatrix, int secretImageWidth, int secretImageHeight, vector<vector<bool>>& embeddedBoolMask, vector<int>& bitPosVec) {
  // 抽出する秘密画像のMat行列
  cv::Mat extractedSecretImage(secretImageHeight, secretImageWidth, CV_8UC1);

  // 係数行列のサイズを取得
  int stegoRows = static_cast<int>(stegoFreqMatrix.size());
  int stegoCols = static_cast<int>(stegoFreqMatrix[0].size());

  // 抽出
  int extractCount = 0;  // 抽出ビットカウント
  int bitCount = 0;      // 8ビットカウント
  std::bitset<8> pixelBits;
  int row = 0, col = 0;  // 抽出画像のピクセル位置
  for (int i = 0; i < stegoRows; i++) {
    for (int j = 0; j < stegoCols; j++) {

      if (embeddedBoolMask[i][j] == false) {
        continue;
      }

      // 符号を全て正にする
      int sign = 1;
      if (stegoFreqMatrix[i][j] < 0) {
        sign = -1;
      }
      double absFreqElement = sign * stegoFreqMatrix[i][j];

      // 整数部分を取得
      int freqElementIntPart = static_cast<int>(absFreqElement);

      // 整数部分を32ビットの2進数に変換
      std::bitset<32> binaryFreqElementIntPart(freqElementIntPart);

      // 周波数行列の整数部分の第bitPosVec[k]ビット目を抽出（bitCountがループ中に8を超えるパターンでは上手くいかないため、例えばbitPosVecの要素数が3のときは使えない→要改善）
      for (int k = 0; k < bitPosVec.size(); k++) {
        // LSBからMSBにかけて格納
        pixelBits[bitCount] = binaryFreqElementIntPart[bitPosVec[k]];
        bitCount++;
        extractCount++;
      }

      // 8ビット抽出したら、2進数を10進数整数に変換し、画素値として設定
      if (bitCount == BITS_PER_CHANNEL) {
        int pixelValue = static_cast<int>(pixelBits.to_ulong());
        extractedSecretImage.at<uchar>(row, col) = pixelValue;
        bitCount = 0;
        col++;
        if (col == extractedSecretImage.cols) {
          col = 0;
          row++;
        }
      }

      // 全てのビットを抽出したら終了
      if (extractCount == extractedSecretImage.total() * BITS_PER_CHANNEL) {
        return extractedSecretImage;
      }
    }
  }

  throw std::runtime_error("エラー：すべてのビットが抽出できませんでした");
}

/**
 * @fn
 * カバー画像を直交変換して得られた係数行列の要素をビット反転して秘密画像の画素値を埋め込む
 * @param coverImageFilePath カバー画像のパス
 * @param secretImageFilePath 秘密画像のパス
 * @param orthogonalMatrix 直交行列
 * @param embeddedAreaString 埋め込み領域（文字列）
 * @param bitPosVec 埋め込みビット位置（1次元vector配列）
 * @param outputPath 保存先ディレクトリのパス
 * @return [ステゴ画像のパス, ステゴ画像のPSNR]（タプル）
 */
std::tuple<std::string, double> embedByBitInversion(const std::string& coverImageFilePath, const std::string& secretImageFilePath, const vector<vector<int>>& orthogonalMatrix, std::string& embeddedAreaString, vector<int>& bitPosVec, const std::string& outputPath) {
	// カバー画像と秘密画像を読み込む
	cv::Mat coverImage = loadGrayImageAsMatrix(coverImageFilePath);
	cv::Mat secretImage = loadGrayImageAsMatrix(secretImageFilePath);

	// 秘密画像の画素値をビットに分解して１次元vector配列に格納
	vector<std::bitset<1>> binarySecretImageVec = makeBinarySecretImageVec(secretImage);

	// カバー画像を直交変換して、係数行列を得る
	vector<vector<double>> freqMatrix = orthogonalTransform(coverImage, orthogonalMatrix);

	// 埋め込み領域の文字列から、trueとなるマスク行列を生成する
	vector<vector<bool>> embeddedBoolMask = embeddedAreaString_to_boolMaskMatrix(freqMatrix, secretImage, embeddedAreaString, bitPosVec);

	// 係数行列に秘密画像のビットを埋め込む（マスク行列の要素がtrueなら対応する係数のビットを反転させ、falseなら変更なし）
	vector<vector<double>> embeddedFreqMatrix = embedInFreqDomainByBoolMask(freqMatrix, binarySecretImageVec, embeddedBoolMask, bitPosVec);

	// 係数行列を逆変換してステゴ画像を生成
	cv::Mat stegoImage = inverseOrthogonalTransform(embeddedFreqMatrix, orthogonalMatrix);

	// ステゴ画像のPSNRを計算
	double stegoImagePsnr = calculatePSNR(coverImage, stegoImage);


	/***** ステゴ画像を保存 *****/

	// ステゴ画像を保存するディレクトリを生成
	createDirectory(outputPath);
	// 埋め込みビット位置を文字列として格納
	std::string embeddedBitPosString = "";
	for (int i = 0; i < bitPosVec.size(); i++) {
		embeddedBitPosString += "b";
		embeddedBitPosString += std::to_string(bitPosVec[i]); // 例: bitPos == 0のときb0
		if (i + 1 < bitPosVec.size()) {
			embeddedBitPosString += ",";
		} 
	}
	// PSNRの小数点以下第三位までを文字列として格納
	std::ostringstream stegoImagePsnr_stream;
	stegoImagePsnr_stream << std::fixed << std::setprecision(3) << stegoImagePsnr << "dB";
	std::string stegoImagePsnr_str = stegoImagePsnr_stream.str();

	std::string stegoImageFilePath = outputPath + "/stego_" + embeddedBitPosString + "_" + stegoImagePsnr_str + ".bmp";
	cv::imwrite(stegoImageFilePath, stegoImage);

	return {stegoImageFilePath, stegoImagePsnr};
}


/**
 * @fn
 * ステゴ画像から復元画像を抽出する
 * @param stegoImageFilePath ステゴ画像のパス
 * @param secretImageFilePath 秘密画像のパス
 * @param orthogonalMatrix 直交行列
 * @param embeddedAreaString 埋め込み領域（文字列）
 * @param bitPosVec 埋め込みビット位置（1次元vector配列）
 * @param outputPath 保存先ディレクトリのパス
 * @return [復元画像のパス, 復元画像のPSNR]（タプル）
 */
std::tuple<std::string, double> extractByBitInversion(const std::string& stegoImageFilePath, const std::string& secretImageFilePath, const vector<vector<int>>& orthogonalMatrix, std::string& embeddedAreaString, vector<int>& bitPosVec, const std::string& outputPath) {
	// ステゴ画像と秘密画像を読み込む
	cv::Mat stegoImage = loadGrayImageAsMatrix(stegoImageFilePath);
	cv::Mat secretImage = loadGrayImageAsMatrix(secretImageFilePath);

	// ステゴ画像を直交変換して、係数行列を得る
	vector<vector<double>> freqMatrix = orthogonalTransform(stegoImage, orthogonalMatrix);

	// 埋め込み領域の文字列から、trueとなるマスク行列を生成する
	vector<vector<bool>> embeddedBoolMask = embeddedAreaString_to_boolMaskMatrix(freqMatrix, secretImage, embeddedAreaString, bitPosVec);

	// 係数行列から秘密画像を抽出する
  const int secretImageWidth = secretImage.cols;
  const int secretImageHeight = secretImage.rows;
	cv::Mat extractedSecretImage = extractFromFreqDomainByBoolMask(freqMatrix, secretImageWidth, secretImageHeight, embeddedBoolMask, bitPosVec);

	// ステゴ画像のPSNRを計算
	double extractedSecretImagePsnr = calculatePSNR(secretImage, extractedSecretImage);


	/***** 復元画像を保存 *****/

	// 埋め込みビット位置を文字列として格納
	std::string embeddedBitPosString = "";
	for (int i = 0; i < bitPosVec.size(); i++) {
		embeddedBitPosString += "b";
		embeddedBitPosString += std::to_string(bitPosVec[i]);
		if (i + 1 < bitPosVec.size()) {
			embeddedBitPosString += ",";
		} 
	}
	// PSNRの小数点以下第三位までを文字列として格納
	std::ostringstream extractedSecretImagePsnr_stream;
	extractedSecretImagePsnr_stream << std::fixed << std::setprecision(3) << extractedSecretImagePsnr << "dB";
	std::string extractedSecretImagePsnr_str = extractedSecretImagePsnr_stream.str();

	std::string extractedSecretImageFilePath = outputPath + "/secret_" + embeddedBitPosString + "_" + extractedSecretImagePsnr_str + ".bmp";
	cv::imwrite(extractedSecretImageFilePath, extractedSecretImage);

	return {extractedSecretImageFilePath, extractedSecretImagePsnr};

}
