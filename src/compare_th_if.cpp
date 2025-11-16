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
 * (IFの決定方法を TH に基づく動的計算に変更)
 */
int main() {
    // カバー画像のサイズ
    const int N = 256;

    // 読み込むカバー画像のパス
    std::string coverImageFilePath = "../input/img/cover_image/Lenna_grayscale_256.bmp";

    // 読み込む秘密画像のリスト
    vector<std::tuple<std::string, std::string, std::string>> secretImages = {
        {"../input/img/secret_image/secret_grayscale_64x64.bmp", "secret_grayscale_64x64", "secret_grayscale_64x64"},
        {"../input/img/secret_image/secret_grayscale_128x128.bmp", "secret_grayscale_128x128", "secret_grayscale_128x128"},
        {"../input/img/secret_image/secret_grayscale_256x256.bmp", "secret_grayscale_256x256", "secret_grayscale_256x256"},
        {"../input/img/secret_image/airplane256.bmp", "airplane256", "airplane256"},
        {"../input/img/secret_image/barbara256.bmp", "barbara256", "barbara256"},
        {"../input/img/secret_image/boat256.bmp", "boat256", "boat256"},
        {"../input/img/secret_image/cameraman256.bmp", "cameraman256", "cameraman256"},
        {"../input/img/secret_image/mandrill256.bmp", "mandrill256", "mandrill256"},
    };

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
    vector<vector<int>> WalshHadamardMatrix    = generateWalshHadamardMatrix(N);
    vector<vector<int>> debruijnMatrix         = generateDebruijnMatrix(N, debruijnSeqFilePath, targetLine, lineStartIndex);

    // 出力ディレクトリ
    std::string outputDir = "../output/th_if_comparison";
    createDirectory(outputDir);

    // THの範囲とステップを定義
    const int TH_min  = 10;
    const int TH_max  = 50;
    const int TH_step = 5;

    // THに対するIFの比率
    const double IF_RATIO_OF_THEORY = 0.95;

    // THとIFの組み合わせ数
    int TH_count = ((TH_max - TH_min) / TH_step) + 1;
    int combinations_per_matrix = TH_count;
    int total_combinations = combinations_per_matrix * 3 * secretImages.size();

    // 全体CSVファイル
    std::string csvFilePath = outputDir + "/comparison_results.csv";
    std::ofstream csvFile(csvFilePath);

    csvFile << "Secret Image,Matrix Type,TH,IF,Stego PSNR,Extracted PSNR" << endl;

    cout << "THとIFの組み合わせをテストしています..." << endl;
    cout << "テスト画像数: " << secretImages.size() << endl;
    cout << "TH範囲: " << TH_min << " ～ " << TH_max << " (ステップ: " << TH_step << ")" << endl;
    cout << "IF: TH / 255.0 * " << IF_RATIO_OF_THEORY << " (動的計算)" << endl;
    cout << "TH値の数: " << TH_count << endl;
    cout << "1画像あたりの組み合わせ数: " << combinations_per_matrix << " × 3種類 = " 
         << (combinations_per_matrix * 3) << endl;
    cout << "総組み合わせ数: " << total_combinations << endl << endl;

    // 行列セット
    vector<std::tuple<std::string, vector<vector<int>>>> matrixTypes = {
        {"Sylvester_Hadamard", SylvesterHadamardMatrix},
        {"Walsh_Hadamard",     WalshHadamardMatrix},
        {"Debruijn",           debruijnMatrix}
    };

    int progress = 0;

    // 各秘密画像について実行
    for (const auto& [secretImageFilePath, secretImageName, secretImageDisplayName] : secretImages) {

        cout << "========================================" << endl;
        cout << "処理中: " << secretImageDisplayName << endl;
        cout << "========================================" << endl;

        std::string imageOutputDir = outputDir + "/" + secretImageName;
        createDirectory(imageOutputDir);

        std::string imageCsvFilePath = imageOutputDir + "/comparison_results.csv";
        std::ofstream imageCsvFile(imageCsvFilePath);

        imageCsvFile << "Matrix Type,TH,IF,Stego PSNR,Extracted PSNR" << endl;

        // 各行列タイプについて実行
        for (const auto& [matrixTypeName, orthogonalMatrix] : matrixTypes) {

            std::string matrixOutputDir = imageOutputDir + "/" + matrixTypeName;
            createDirectory(matrixOutputDir);

            for (int TH = TH_min; TH <= TH_max; TH += TH_step) {

                double IF = (static_cast<double>(TH) / 255.0) * IF_RATIO_OF_THEORY;

                progress++;
                if (progress % 10 == 0 || progress == total_combinations) {
                    cout << "進捗: " << progress << "/" << total_combinations
                         << " (" << (progress * 100 / total_combinations) << "%)" << endl;
                }

                try {
                    // 埋め込み
                    auto [stegoImageFilePath, stegoImagePSNR] =
                        embedByThresholdAdjust(
                            coverImageFilePath,
                            secretImageFilePath,
                            orthogonalMatrix,
                            TH, IF,
                            matrixOutputDir
                        );

                    // 抽出
                    auto [extractedImageFilePath, extractedImagePSNR] =
                        extractByThresholdAdjust(
                            stegoImageFilePath,
                            secretImageFilePath,
                            orthogonalMatrix,
                            TH, IF,
                            matrixOutputDir
                        );

                    // 全体CSV
                    csvFile << secretImageDisplayName << ","
                            << matrixTypeName << ","
                            << TH << ","
                            << std::fixed << std::setprecision(3) << IF << ","
                            << std::fixed << std::setprecision(2) << stegoImagePSNR << ","
                            << std::fixed << std::setprecision(2) << extractedImagePSNR
                            << endl;

                    // 画像ごとCSV
                    imageCsvFile << matrixTypeName << ","
                                 << TH << ","
                                 << std::fixed << std::setprecision(3) << IF << ","
                                 << std::fixed << std::setprecision(2) << stegoImagePSNR << ","
                                 << std::fixed << std::setprecision(2) << extractedImagePSNR
                                 << endl;

                } catch (const std::exception& e) {

                    std::cerr << "エラー (画像=" << secretImageDisplayName
                              << ", TH=" << TH << ", IF=" << IF
                              << ", Matrix=" << matrixTypeName << "): "
                              << e.what() << endl;

                    csvFile << secretImageDisplayName << ","
                            << matrixTypeName << "," << TH << ","
                            << std::fixed << std::setprecision(3) << IF
                            << ",ERROR,ERROR" << endl;

                    imageCsvFile << matrixTypeName << ","
                                 << TH << ","
                                 << std::fixed << std::setprecision(3) << IF
                                 << ",ERROR,ERROR" << endl;
                }
            }
        }

        imageCsvFile.close();
        cout << secretImageDisplayName << " の処理が完了しました。" << endl;
        cout << "結果は " << imageCsvFilePath << " に保存されました。" << endl << endl;
    }

    csvFile.close();
    cout << "========================================" << endl;
    cout << "すべての処理が完了しました！" << endl;
    cout << "全体の結果は " << csvFilePath << " に保存されました。" << endl;
    cout << "各画像の結果は " << outputDir << "/{画像名}/comparison_results.csv に保存されました。" << endl;

    return 0;
}
