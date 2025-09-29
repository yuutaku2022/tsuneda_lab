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
#include "embed_by_bit_inversion.hpp"
#include "embed_by_threshold_adjust.hpp"

using std::cout;
using std::endl;
using std::vector;

int main() {
	// カバー画像の横幅および縦幅(64 or 256)
	const int N = 256;

	// 読み込むカバー画像のパス
  std::string coverImageFilePath = "../input/img/cover_image/Lenna_grayscale_256.bmp";

	// 読み込む秘密画像のパス
  std::string secretImageFilePath = "../input/img/secret_image/secret_grayscale_32x32.bmp";
  // std::string secretImageFilePath = "../input/img/secret_image/barbara32.bmp";

	// 埋め込みビット位置（1bit埋め込み）
	vector<vector<int>> bitPositions_1bit = {
		{0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}
	};
	// 埋め込みビット位置（2bit埋め込み）
	vector<vector<int>> bitPositions_2bit = {
		{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6}, {6, 7}
	};
	
	// タプルを要素にもつvector配列（タプルの0番目：係数行列の埋め込み領域の文字列, 1番目：埋め込みビット位置の二次元vector配列）
	vector<std::tuple<std::string, vector<vector<int>>>> embeddedAreaInfo = {
		{"係数行列の左上の領域(サイズ128x64)", bitPositions_1bit},	// 矩形領域 1bitずつ
		{"係数行列の右上の領域(サイズ128x64)", bitPositions_1bit},	// 矩形領域 1bitずつ
		{"係数行列の左下の領域(サイズ128x64)", bitPositions_1bit},	// 矩形領域 1bitずつ
		{"係数行列の右下の領域(サイズ128x64)", bitPositions_1bit},	// 矩形領域 1bitずつ
		{"係数行列の左上の領域(サイズ64x64)", bitPositions_2bit},		// 矩形領域 2bitずつ
		{"係数行列の右上の領域(サイズ64x64)", bitPositions_2bit},		// 矩形領域 2bitずつ
		{"係数行列の左下の領域(サイズ64x64)", bitPositions_2bit},		// 矩形領域 2bitずつ
		{"係数行列の右下の領域(サイズ64x64)", bitPositions_2bit},		// 矩形領域 2bitずつ
		{"係数行列の四隅(サイズ64x32)", bitPositions_1bit},					// 係数行列の四隅 1bitずつ
		{"係数行列の四隅(サイズ32x32)", bitPositions_2bit},					// 係数行列の四隅 2bitずつ
		{"係数行列全体(周期8)", bitPositions_1bit},									// 係数行列全体 1bitずつ
		{"係数行列全体(周期16)", bitPositions_2bit},								// 係数行列全体 2bitずつ
	};


	/***** 係数行列の生成 *****/

  // 再帰的にアダマール行列を生成
  vector<vector<int>> SylvesterHadamardMatrix = generateSylvesterHadamardMatrix(N);
  // 交番数順アダマール行列を生成
  vector<vector<int>> WalshHadamardMatrix = generateWalshHadamardMatrix(N);

	// ドブルイン系列のdatファイルのパス
  std::string debruijnSeqFilePath;
  switch (N) {
    case 64:
      debruijnSeqFilePath = "../input/debruijn/deb64.dat";
      break;
    case 256:
      debruijnSeqFilePath = "../input/debruijn/deb256.dat";
      break;
    default:
      std::cerr << "エラー: ドブルイン系列のファイルが開けませんでした。" << std::endl;
      return 1;
  }
	const int targetLine = 10;     // ドブルイン系列の指定する行
  const int lineStartIndex = 0;  // ドブルイン系列の開始位置

  // ドブルイン行列（交番数順アダマール行列をドブルイン系列に基づいて並び替えた行列）を生成
  vector<vector<int>> debruijnMatrix = generateDebruijnMatrix(N, debruijnSeqFilePath, targetLine, lineStartIndex);


	/***** ビット反転を使った埋め込み・抽出の準備 *****/

	// ステゴ画像のファイルパスを格納する二次元vector配列
	vector<vector<std::string>> stegoImageByBitInversion_WalshHadamard_filePathMatrix(embeddedAreaInfo.size());
	vector<vector<std::string>> stegoImageByBitInversion_debruijn_filePathMatrix(embeddedAreaInfo.size());

	// ステゴ画像のPSNRを格納する二次元vector配列
	vector<vector<double>> stegoImageByBitInversion_WalshHadamard_psnrMatrix(embeddedAreaInfo.size());
	vector<vector<double>> stegoImageByBitInversion_debruijn_psnrMatrix(embeddedAreaInfo.size());

	// 復元画像のファイルパスを格納する二次元vector配列
	vector<vector<std::string>> extractedImageByBitInversion_WalshHadamard_filePathMatrix(embeddedAreaInfo.size());
	vector<vector<std::string>> extractedImageByBitInversion_debruijn_filePathMatrix(embeddedAreaInfo.size());

	// 復元画像のPSNRを格納する二次元vector配列
	vector<vector<double>> extractedImageByBitInversion_WalshHadamard_psnrMatrix(embeddedAreaInfo.size());
	vector<vector<double>> extractedImageByBitInversion_debruijn_psnrMatrix(embeddedAreaInfo.size());


	/***** ビット反転を使った埋め込み・抽出 *****/

	for (size_t i = 0; i < embeddedAreaInfo.size(); i++) {
		// タプルから情報を取得
    const auto& entry = embeddedAreaInfo[i];
		std::string embeddedAreaStr = std::get<0>(entry); // 埋め込み領域の文字列
		vector<vector<int>> bitPositions = std::get<1>(entry); // 埋め込みビット位置の二次元vector配列

		// 保存先のディレクトリパス
		std::string outputPath_bitInv_h, outputPath_bitInv_d;
		outputPath_bitInv_h = "../output1/係数をビット反転して埋め込み/交番数順アダマール/" + embeddedAreaStr;
		outputPath_bitInv_d = "../output1/係数をビット反転して埋め込み/ドブルイン/" + embeddedAreaStr;
		for (vector<int>& bitPosVec : bitPositions) {			
			// ビット反転を使った埋め込み・抽出（交番数順アダマール行列を使用）
			auto [stegoImageFilePath_bitInv_h, stegoImagePSNR_bitInv_h] = embedByBitInversion(coverImageFilePath, secretImageFilePath, WalshHadamardMatrix, embeddedAreaStr, bitPosVec, outputPath_bitInv_h);
			stegoImageByBitInversion_WalshHadamard_filePathMatrix[i].push_back(stegoImageFilePath_bitInv_h);
			stegoImageByBitInversion_WalshHadamard_psnrMatrix[i].push_back(stegoImagePSNR_bitInv_h);
			auto [extractedImageFilePath_bitInv_h, extractedImagePSNR_bitInv_h] = extractByBitInversion(stegoImageFilePath_bitInv_h, secretImageFilePath, WalshHadamardMatrix, embeddedAreaStr, bitPosVec, outputPath_bitInv_h);
			extractedImageByBitInversion_WalshHadamard_filePathMatrix[i].push_back(extractedImageFilePath_bitInv_h);
			extractedImageByBitInversion_WalshHadamard_psnrMatrix[i].push_back(extractedImagePSNR_bitInv_h);
			
			// ビット反転を使った埋め込み・抽出（ドブルイン行列を使用）
			auto [stegoImageFilePath_bitInv_d, stegoImagePSNR_bitInv_d] = embedByBitInversion(coverImageFilePath, secretImageFilePath, debruijnMatrix, embeddedAreaStr, bitPosVec, outputPath_bitInv_d);
			stegoImageByBitInversion_debruijn_filePathMatrix[i].push_back(stegoImageFilePath_bitInv_d);
			stegoImageByBitInversion_debruijn_psnrMatrix[i].push_back(stegoImagePSNR_bitInv_d);
			auto [extractedImageFilePath_bitInv_d, extractedImagePSNR_bitInv_d] = extractByBitInversion(stegoImageFilePath_bitInv_d, secretImageFilePath, debruijnMatrix, embeddedAreaStr, bitPosVec, outputPath_bitInv_d);
			extractedImageByBitInversion_debruijn_filePathMatrix[i].push_back(extractedImageFilePath_bitInv_d);
			extractedImageByBitInversion_debruijn_psnrMatrix[i].push_back(extractedImagePSNR_bitInv_d);
		}
	};


	/***** 閾値に基づく埋め込み・抽出（特演で紹介したの論文の手法） *****/

	// パラメータセットをタプルで定義（TH, IF, secretImagePath）
	vector<std::tuple<int, double, std::string, std::string, std::string>> parameterSets = {
		{50, 0.19, "../input/img/secret_image/secret_grayscale_64x64.bmp", "全画素値127の画像(サイズ:$64 \\times 64$)", "secret_grayscale"},
		{30, 0.117, "../input/img/secret_image/secret_grayscale_128x128.bmp", "全画素値127の画像(サイズ:$128 \\times 128$)", "secret_grayscale"},
		{15, 0.058, "../input/img/secret_image/secret_grayscale_256x256.bmp", "全画素値127の画像(サイズ:$256 \\times 256$)", "secret_grayscale"},
		{15, 0.058, "../input/img/secret_image/airplane256.bmp", "airplane(サイズ:$256 \\times 256$)", "airplane"},
		{15, 0.058, "../input/img/secret_image/barbara256.bmp", "Barbara(サイズ:$256 \\times 256$)", "barbara"},
		{15, 0.058, "../input/img/secret_image/boat256.bmp", "boat(サイズ:$256 \\times 256$)", "boat"},
		{15, 0.058, "../input/img/secret_image/cameraman256.bmp", "cameraman(サイズ:$256 \\times 256$)", "cameraman"},
		{15, 0.058, "../input/img/secret_image/mandrill256.bmp", "mandrill(サイズ:$256 \\times 256$)", "mandrill"},
	};


	for (const auto& [TH, IF, secretImageFilePath, secretImageCaption, secretImageStr] : parameterSets) {
		std::string outputPath_thAdjust_s, outputPath_thAdjust_w, outputPath_thAdjust_d;
		outputPath_thAdjust_s = "../output1/閾値に基づいて係数行列に埋め込み/再帰的アダマール/" + secretImageStr;
		outputPath_thAdjust_w = "../output1/閾値に基づいて係数行列に埋め込み/交番数順アダマール/" + secretImageStr;
		outputPath_thAdjust_d = "../output1/閾値に基づいて係数行列に埋め込み/ドブルイン/" + secretImageStr;

		// 閾値に基づく埋め込み・抽出（再帰的アダマール行列を使用）
		auto [stegoImageFilePath_thAdjust_s, stegoImagePSNR_thAdjust_s] = embedByThresholdAdjust(coverImageFilePath, secretImageFilePath, SylvesterHadamardMatrix, TH, IF, outputPath_thAdjust_s);
		auto [extractedImageFilePath_thAdjust_s, extractedImagePSNR_thAdjust_s] = extractByThresholdAdjust(stegoImageFilePath_thAdjust_s, secretImageFilePath, SylvesterHadamardMatrix, TH, IF, outputPath_thAdjust_s);

		// 閾値に基づく埋め込み・抽出（交番数順アダマール行列を使用）
		auto [stegoImageFilePath_thAdjust_w, stegoImagePSNR_thAdjust_w] = embedByThresholdAdjust(coverImageFilePath, secretImageFilePath, WalshHadamardMatrix, TH, IF, outputPath_thAdjust_w);
		auto [extractedImageFilePath_thAdjust_w, extractedImagePSNR_thAdjust_w] = extractByThresholdAdjust(stegoImageFilePath_thAdjust_w, secretImageFilePath, WalshHadamardMatrix, TH, IF, outputPath_thAdjust_w);

		// 閾値に基づく埋め込み・抽出（ドブルイン行列を使用）
		auto [stegoImageFilePath_thAdjust_d, stegoImagePSNR_thAdjust_d] = embedByThresholdAdjust(coverImageFilePath, secretImageFilePath, debruijnMatrix, TH, IF, outputPath_thAdjust_d);
		auto [extractedImageFilePath_thAdjust_d, extractedImagePSNR_thAdjust_d] = extractByThresholdAdjust(stegoImageFilePath_thAdjust_d, secretImageFilePath, debruijnMatrix, TH, IF, outputPath_thAdjust_d);
	}


	return 0;
}