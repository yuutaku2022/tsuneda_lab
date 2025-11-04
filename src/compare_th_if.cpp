#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <tuple>

#include "Eigen/Dense"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/opencv.hpp"

#include "matrix_transform_utils.hpp"
#include "embed_by_threshold_adjust.hpp"

using std::cout;
using std::endl;
using std::vector;

/**
 * THとIFを変えて実行し、結果をCSVに出力する
 */
int main() {
	// カバー画像のサイズ
	const int N = 256;

	// 読み込むカバー画像のパス
	std::string coverImageFilePath = "../input/img/cover_image/Lenna_grayscale_256.bmp";

	// 読み込む秘密画像のパス
	std::string secretImageFilePath = "../input/img/secret_image/secret_grayscale_64x64.bmp";

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
	const int targetLine = 10;
	const int lineStartIndex = 0;

	// 直交行列を生成
	vector<vector<int>> SylvesterHadamardMatrix = generateSylvesterHadamardMatrix(N);
	vector<vector<int>> WalshHadamardMatrix = generateWalshHadamardMatrix(N);
	vector<vector<int>> debruijnMatrix = generateDebruijnMatrix(N, debruijnSeqFilePath, targetLine, lineStartIndex);

	// 出力ディレクトリ
	std::string outputDir = "../output/th_if_comparison";
	createDirectory(outputDir);

	// THとIFの範囲とステップを定義
	// これらの値を変更することで、テスト範囲と細かさを調整できます
	const int TH_min = 10;      // THの最小値
	const int TH_max = 50;      // THの最大値
	const int TH_step = 5;      // THのステップ（刻み幅）
	
	const double IF_min = 0.05;   // IFの最小値
	const double IF_max = 0.30;   // IFの最大値
	const double IF_step = 0.01;  // IFのステップ（刻み幅）
	// 例: TH_step=1, IF_step=0.01 にするとより細かくテストできます

	// THとIFの組み合わせ数を計算
	int TH_count = ((TH_max - TH_min) / TH_step) + 1;
	int IF_count = static_cast<int>((IF_max - IF_min) / IF_step) + 1;
	int total_combinations = TH_count * IF_count * 3; // 3種類の行列

	// CSVファイルを開く
	std::string csvFilePath = outputDir + "/comparison_results.csv";
	std::ofstream csvFile(csvFilePath);
	
	// CSVヘッダーを書き込み
	csvFile << "Matrix Type,TH,IF,Stego PSNR,Extracted PSNR" << endl;

	cout << "THとIFの組み合わせをテストしています..." << endl;
	cout << "TH範囲: " << TH_min << " ～ " << TH_max << " (ステップ: " << TH_step << ")" << endl;
	cout << "IF範囲: " << IF_min << " ～ " << IF_max << " (ステップ: " << IF_step << ")" << endl;
	cout << "TH値の数: " << TH_count << endl;
	cout << "IF値の数: " << IF_count << endl;
	cout << "総組み合わせ数: " << total_combinations << " (3種類の行列)" << endl;

	// 各行列タイプについて実行
	vector<std::tuple<std::string, vector<vector<int>>>> matrixTypes = {
		{"Sylvester_Hadamard", SylvesterHadamardMatrix},
		{"Walsh_Hadamard", WalshHadamardMatrix},
		{"Debruijn", debruijnMatrix}
	};

	int progress = 0;

	for (const auto& [matrixTypeName, orthogonalMatrix] : matrixTypes) {
		std::string matrixOutputDir = outputDir + "/" + matrixTypeName;
		createDirectory(matrixOutputDir);

		// THの範囲をfor文で細かく刻む
		for (int TH = TH_min; TH <= TH_max; TH += TH_step) {
			// IFの範囲をfor文で細かく刻む
			for (double IF = IF_min; IF <= IF_max + 0.001; IF += IF_step) { // 0.001は浮動小数点誤差対策
				progress++;
				if (progress % 10 == 0 || progress == total_combinations) {
					cout << "進捗: " << progress << "/" << total_combinations << " (" 
					     << (progress * 100 / total_combinations) << "%)" << endl;
				}

				try {
					// 埋め込み
					auto [stegoImageFilePath, stegoImagePSNR] = 
						embedByThresholdAdjust(coverImageFilePath, secretImageFilePath, 
						                      orthogonalMatrix, TH, IF, matrixOutputDir);

					// 抽出
					auto [extractedImageFilePath, extractedImagePSNR] = 
						extractByThresholdAdjust(stegoImageFilePath, secretImageFilePath, 
						                        orthogonalMatrix, TH, IF, matrixOutputDir);

					// CSVに結果を書き込み
					csvFile << matrixTypeName << "," 
					        << TH << "," 
					        << std::fixed << std::setprecision(3) << IF << "," 
					        << std::fixed << std::setprecision(2) << stegoImagePSNR << "," 
					        << std::fixed << std::setprecision(2) << extractedImagePSNR 
					        << endl;

				} catch (const std::exception& e) {
					std::cerr << "エラー (TH=" << TH << ", IF=" << IF 
					          << ", Matrix=" << matrixTypeName << "): " 
					          << e.what() << endl;
					csvFile << matrixTypeName << "," << TH << "," 
					        << std::fixed << std::setprecision(3) << IF << ",ERROR,ERROR" 
					        << endl;
				}
			}
		}
	}

	csvFile.close();
	cout << "完了！結果は " << csvFilePath << " に保存されました。" << endl;

	return 0;
}
