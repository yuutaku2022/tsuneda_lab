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


/**
 * @fn
 * 閾値に基づいて係数行列に埋め込みを行う（2022年の論文で提案されたHadamard変換を利用した手法）
 * @param coverImageFilePath カバー画像のパス
 * @param secretImageFilePath 秘密画像のパス
 * @param orthogonalMatrix 直交行列
 * @param TH 閾値
 * @param IF Insert Factor
 * @param outputPath 保存先ディレクトリのパス
 * @return [ステゴ画像のパス, ステゴ画像のPSNR]（タプル）
 */
std::tuple<std::string, double> embedByThresholdAdjust(const std::string& coverImageFilePath, const std::string& secretImageFilePath, const vector<vector<int>>& orthogonalMatrix, int TH, double IF, const std::string& outputPath) {
	// カバー画像と秘密画像を読み込む
	cv::Mat coverImage = loadGrayImageAsMatrix(coverImageFilePath);
	cv::Mat secretImage = loadGrayImageAsMatrix(secretImageFilePath);

	// カバー画像を直交変換して、係数行列を得る
	vector<vector<double>> freqMatrix = orthogonalTransform(coverImage, orthogonalMatrix);

	// 秘密画像の幅と高さを抽出
  const int secretImageWidth = secretImage.cols;   // 秘密画像の幅
  const int secretImageHeight = secretImage.rows;  // 秘密画像の高さ

	// 係数行列に秘密画像を埋め込む
	vector<vector<double>> embeddedFreqMatrix = freqMatrix;
	for (int i = 0; i < secretImageHeight; i++) {
    for (int j = 0; j < secretImageWidth; j++) {
			double s = secretImage.at<uchar>(i,j) * IF;
			// 正の係数
      if (embeddedFreqMatrix[i][j] > 0) {
        if (embeddedFreqMatrix[i][j] <= TH) 
          embeddedFreqMatrix[i][j] = 0;
        else {
					embeddedFreqMatrix[i][j] = embeddedFreqMatrix[i][j] - fmod(embeddedFreqMatrix[i][j], TH);
				}
        // 埋め込み
        embeddedFreqMatrix[i][j] += s;
        
      // 負の係数  
      } else {
        if (embeddedFreqMatrix[i][j] >= -TH)
          embeddedFreqMatrix[i][j] = 0; 
        else {
					embeddedFreqMatrix[i][j] = embeddedFreqMatrix[i][j] - fmod(embeddedFreqMatrix[i][j], TH);
				}
        
        embeddedFreqMatrix[i][j] -= s;
      }
		}
	}

	// 係数行列を逆変換してステゴ画像を生成
	cv::Mat stegoImage = inverseOrthogonalTransform(embeddedFreqMatrix, orthogonalMatrix);

	// ステゴ画像のPSNRを計算
	double stegoImagePsnr = calculatePSNR(coverImage, stegoImage);


	/***** ステゴ画像を保存 *****/

	// ステゴ画像を保存するディレクトリを生成
	createDirectory(outputPath);

	// PSNRの小数点以下第一位までを文字列として格納
	std::ostringstream stegoImagePsnr_stream;
	stegoImagePsnr_stream << std::fixed << std::setprecision(1) << stegoImagePsnr << "dB";
	std::string stegoImagePsnr_str = stegoImagePsnr_stream.str();

	// Thを文字列化
	std::string TH_str = std::to_string(TH);

	// IFを文字列化
	std::ostringstream IF_stream;
	IF_stream << std::fixed << std::setprecision(3) << IF;
	std::string IF_str = IF_stream.str();

	std::string stegoImageFilePath = outputPath + "/" + std::to_string(secretImageWidth) + "x" + std::to_string(secretImageHeight) + "_stego_TH=" + TH_str + "_IF=" + IF_str + "_" + stegoImagePsnr_str + ".bmp";
	cv::imwrite(stegoImageFilePath, stegoImage);

	return {stegoImageFilePath, stegoImagePsnr};
}


/**
 * @fn
 * ステゴ画像から復元画像を抽出する
 * @param stegoImageFilePath ステゴ画像のパス
 * @param secretImageFilePath 秘密画像のパス
 * @param orthogonalMatrix 直交行列
 * @param TH 閾値
 * @param IF Insert Factor
 * @param outputPath 保存先ディレクトリのパス
 * @return [復元画像のパス, 復元画像のPSNR]（タプル）
 */
std::tuple<std::string, double> extractByThresholdAdjust(const std::string& stegoImageFilePath, const std::string& secretImageFilePath, const vector<vector<int>>& orthogonalMatrix, int TH, double IF, const std::string& outputPath) {
	// ステゴ画像と秘密画像を読み込む
	cv::Mat stegoImage = loadGrayImageAsMatrix(stegoImageFilePath);
	cv::Mat secretImage = loadGrayImageAsMatrix(secretImageFilePath);

	// ステゴ画像を直交変換して、係数行列を得る
	vector<vector<double>> stegoFreqMatrix = orthogonalTransform(stegoImage, orthogonalMatrix);


	// 秘密画像の幅と高さを抽出
  const int secretImageWidth = secretImage.cols;
  const int secretImageHeight = secretImage.rows;


  // 抽出する秘密画像のMat行列
	cv::Mat extractedSecretImage(secretImageHeight, secretImageWidth, CV_8UC1);

	// 係数行列から秘密画像を抽出する
	double es;
	for (int i = 0; i < secretImageHeight; i++) {
    for (int j = 0; j < secretImageWidth; j++) {
			double stegoFreqElement_abs = std::abs(stegoFreqMatrix[i][j]);
			if (stegoFreqElement_abs <= TH)
				es = stegoFreqElement_abs;
      else {
        double I_stegoFreqElement_abs = static_cast<int>(stegoFreqElement_abs); 
        double R_stegoFreqElement_abs = I_stegoFreqElement_abs - fmod(I_stegoFreqElement_abs, TH);
        es = stegoFreqElement_abs - R_stegoFreqElement_abs;
      }
      
      es /= IF;
			extractedSecretImage.at<uchar>(i,j) = static_cast<int>(es);
		}
	}

	// ステゴ画像のPSNRを計算
	double extractedSecretImagePsnr = calculatePSNR(secretImage, extractedSecretImage);


	/***** 復元画像を保存 *****/

	// PSNRの小数点以下第一位までを文字列として格納
	std::ostringstream extractedSecretImagePsnr_stream;
	extractedSecretImagePsnr_stream << std::fixed << std::setprecision(1) << extractedSecretImagePsnr << "dB";
	std::string extractedSecretImagePsnr_str = extractedSecretImagePsnr_stream.str();

	std::ostringstream TH_stream;
	std::string TH_str = std::to_string(TH);

	std::ostringstream IF_stream;
	IF_stream << std::fixed << std::setprecision(3) << IF;
	std::string IF_str = IF_stream.str();

	std::string extractedSecretImageFilePath = outputPath + "/" + std::to_string(secretImageWidth) + "x" + std::to_string(secretImageHeight) + "_secret_TH=" + TH_str + "_IF=" + IF_str + "_" + extractedSecretImagePsnr_str + ".bmp";
	cv::imwrite(extractedSecretImageFilePath, extractedSecretImage);

	return {extractedSecretImageFilePath, extractedSecretImagePsnr};
}