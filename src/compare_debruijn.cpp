// main.cpp
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <tuple>
#include <stdexcept>

#include "Eigen/Dense"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/opencv.hpp"

#include "matrix_transform_utils.hpp"
#include "embed_by_threshold_adjust.hpp"

using std::cout;
using std::endl;
using std::vector;

/**
 * createDirectory: 深い階層を含めてフォルダを作成するラッパー
 */
void createDirectory(const std::string& path) {
    try {
        std::filesystem::create_directories(path);
    } catch (const std::exception& e) {
        std::cerr << "ディレクトリ作成エラー (" << path << "): " << e.what() << std::endl;
        throw;
    }
}

int main() {
    // カバー画像のサイズ
    const int N = 256;

    // --- 入力画像リスト（必要に応じてパスを調整） ---
    vector<std::tuple<std::string, std::string, std::string>> coverImages = {
        {"../input/img/cover_image/Lenna_grayscale_256.bmp", "Lenna_256", "Lenna_256"},
        {"../input/img/cover_image/barbara256.bmp", "barbara256", "barbara256"},
        {"../input/img/cover_image/airplane256.bmp", "airplane256", "airplane256"},
        {"../input/img/cover_image/boat256.bmp", "boat256", "boat256"},
        {"../input/img/cover_image/cameraman256.bmp", "cameraman256", "cameraman256"},
        {"../input/img/cover_image/mandrill256.bmp", "mandrill256", "mandrill256"},
        {"../input/img/cover_image/LAX.bmp", "LAX", "LAX"},
        {"../input/img/cover_image/Text.bmp", "Text", "Text"}
    };

    vector<std::tuple<std::string, std::string, std::string>> secretImages = {
        {"../input/img/secret_image/secret_grayscale_64x64.bmp", "secret_64x64", "secret_64x64"},
        {"../input/img/secret_image/secret_grayscale_128x128.bmp", "secret_128x128", "secret_128x128"},
        {"../input/img/secret_image/secret_grayscale_256x256.bmp", "secret_256x256", "secret_256x256"},
        {"../input/img/secret_image/airplane256.bmp", "airplane256", "airplane256"},
        {"../input/img/secret_image/barbara256.bmp", "barbara256", "barbara256"},
        {"../input/img/secret_image/boat256.bmp", "boat256", "boat256"},
        {"../input/img/secret_image/cameraman256.bmp", "cameraman256", "cameraman256"},
        {"../input/img/secret_image/LAX.bmp", "LAX", "LAX"},
        {"../input/img/secret_image/Text.bmp", "Text", "Text"},
        {"../input/img/secret_image/mandrill256.bmp", "mandrill256", "mandrill256"}
    };

    // ドブルイン系列のdatファイルのパス（Nに応じて設定）
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

    // ----- 実験パラメータ -----
    int lineStartIndex = 0; // debruijn行列生成の開始インデックス（必要に応じて変更）
    // THの範囲とステップ
    const int TH_min = 5;
    const int TH_max = 25;
    const int TH_step = 1;
    const double IF_RATIO_OF_THEORY = 0.98;

    // 試す targetLine の候補（必要なら変更）
    vector<int> targetLineCandidates = {5, 10, 20, 50};

    // 出力ディレクトリと全体CSV
    std::string outputDir = "../output/cover_image_comparison";
    createDirectory(outputDir);
    std::string csvFilePath = outputDir + "/comparison_results_all.csv";
    std::ofstream csvFile(csvFilePath);
    if (!csvFile.is_open()) {
        std::cerr << "エラー: 出力CSVが開けません: " << csvFilePath << std::endl;
        return 1;
    }

    // CSVヘッダー（Cover Image, Secret Image, Matrix Type, targetLine, TH, IF, Stego PSNR, Extracted PSNR）
    csvFile << "Cover Image,Secret Image,Matrix Type,TargetLine,TH,IF,Stego PSNR,Extracted PSNR" << std::endl;

    // 計算する総組み合わせ数（進捗用）
    int TH_count = ((TH_max - TH_min) / TH_step) + 1;
    long long total_combinations = static_cast<long long>(TH_count)
                                   * static_cast<long long>(targetLineCandidates.size())
                                   * static_cast<long long>(coverImages.size())
                                   * static_cast<long long>(secretImages.size());

    cout << "カバー画像数: " << coverImages.size() << ", 秘密画像数: " << secretImages.size() << endl;
    cout << "TH 範囲: " << TH_min << " ～ " << TH_max << " (step " << TH_step << ")" << endl;
    cout << "targetLine 個数: " << targetLineCandidates.size() << endl;
    cout << "総組み合わせ数: " << total_combinations << endl << endl;

    long long progress = 0;

    // targetLineごとに debruijn 行列だけを生成して実行
    for (int targetLineValue : targetLineCandidates) {

        cout << "----------------------------------------" << endl;
        cout << "targetLine = " << targetLineValue << " で実行開始" << endl;
        cout << "----------------------------------------" << endl;

        // ドブルイン行列を生成（関数は matrix_transform_utils.hpp 側にある想定）
        vector<vector<int>> debruijnMatrix;
        try {
            debruijnMatrix = generateDebruijnMatrix(N, debruijnSeqFilePath, targetLineValue, lineStartIndex);
        } catch (const std::exception& e) {
            std::cerr << "Debruijn 行列生成に失敗 (targetLine=" << targetLineValue << "): " << e.what() << std::endl;
            continue; // この targetLine はスキップして次へ
        }

        // matrixTypes はドブルインのみ（名前に targetLine を含めて衝突回避）
        vector<std::tuple<std::string, vector<vector<int>>>> matrixTypes = {
            { "Debruijn_t" + std::to_string(targetLineValue), debruijnMatrix }
        };

        // カバー画像ループ
        for (const auto& [coverImageFilePath, coverImageName, coverImageDisplayName] : coverImages) {

            // 秘密画像ループ
            for (const auto& [secretImageFilePath, secretImageName, secretImageDisplayName] : secretImages) {

                // 出力ディレクトリ（cover/secret/matrix）
                std::string imageOutputDir = outputDir + "/" + coverImageName + "/" + secretImageName;
                createDirectory(imageOutputDir);

                std::string imageCsvFilePath = imageOutputDir + "/comparison_results.csv";
                std::ofstream imageCsvFile(imageCsvFilePath);
                if (!imageCsvFile.is_open()) {
                    std::cerr << "警告: 個別CSVが開けません: " << imageCsvFilePath << std::endl;
                    // 続行は可能だがファイルが無いと個別CSV出力はスキップ
                } else {
                    imageCsvFile << "Matrix Type,TH,IF,Stego PSNR,Extracted PSNR" << std::endl;
                }

                // matrix（ここは1つだけ）
                for (const auto& [matrixTypeName, orthogonalMatrix] : matrixTypes) {

                    std::string matrixOutputDir = imageOutputDir + "/" + matrixTypeName;
                    createDirectory(matrixOutputDir);

                    // THループ
                    for (int TH = TH_min; TH <= TH_max; TH += TH_step) {

                        double IF = (static_cast<double>(TH) / 255.0) * IF_RATIO_OF_THEORY;

                        progress++;
                        if (progress % 20 == 0 || progress == total_combinations) {
                            cout << "進捗: " << progress << "/" << total_combinations
                                 << " (" << (progress * 100 / total_combinations) << "%) "
                                 << "(Cover=" << coverImageName << ", Secret=" << secretImageName
                                 << ", TargetLine=" << targetLineValue << ")" << endl;
                        }

                        try {
                            // embedByThresholdAdjust / extractByThresholdAdjust は embed_by_threshold_adjust.hpp にある想定
                            auto [stegoImageFilePath, stegoImagePSNR] =
                                embedByThresholdAdjust(
                                    coverImageFilePath,
                                    secretImageFilePath,
                                    orthogonalMatrix,
                                    TH, IF,
                                    matrixOutputDir
                                );

                            auto [extractedImageFilePath, extractedImagePSNR] =
                                extractByThresholdAdjust(
                                    stegoImageFilePath,
                                    secretImageFilePath,
                                    orthogonalMatrix,
                                    TH, IF,
                                    matrixOutputDir
                                );

                            // 全体CSV出力
                            csvFile << coverImageDisplayName << ","
                                    << secretImageDisplayName << ","
                                    << matrixTypeName << ","
                                    << targetLineValue << ","
                                    << TH << ","
                                    << std::fixed << std::setprecision(4) << IF << ","
                                    << std::fixed << std::setprecision(2) << stegoImagePSNR << ","
                                    << std::fixed << std::setprecision(2) << extractedImagePSNR
                                    << std::endl;

                            // 個別CSV出力（開いていれば）
                            if (imageCsvFile.is_open()) {
                                imageCsvFile << matrixTypeName << ","
                                             << TH << ","
                                             << std::fixed << std::setprecision(4) << IF << ","
                                             << std::fixed << std::setprecision(2) << stegoImagePSNR << ","
                                             << std::fixed << std::setprecision(2) << extractedImagePSNR
                                             << std::endl;
                            }

                        } catch (const std::exception& e) {
                            std::cerr << "エラー (Cover=" << coverImageDisplayName
                                      << ", Secret=" << secretImageDisplayName
                                      << ", TH=" << TH << ", IF=" << IF
                                      << ", Matrix=" << matrixTypeName << ", TargetLine=" << targetLineValue << "): "
                                      << e.what() << std::endl;

                            // エラーログをCSVに記録
                            csvFile << coverImageDisplayName << ","
                                    << secretImageDisplayName << ","
                                    << matrixTypeName << ","
                                    << targetLineValue << ","
                                    << TH << ","
                                    << std::fixed << std::setprecision(4) << IF
                                    << ",ERROR,ERROR" << std::endl;

                            if (imageCsvFile.is_open()) {
                                imageCsvFile << matrixTypeName << ","
                                             << TH << ","
                                             << std::fixed << std::setprecision(4) << IF
                                             << ",ERROR,ERROR" << std::endl;
                            }
                        }
                    } // THループ
                } // matrixTypesループ

                if (imageCsvFile.is_open()) imageCsvFile.close();

                cout << "  " << secretImageDisplayName << " の処理が完了しました。" << std::endl;
                cout << "  結果は " << imageCsvFilePath << " に保存されました。" << std::endl << std::endl;
            } // secretImages
        } // coverImages
    } // targetLineCandidates

    csvFile.close();

    cout << "========================================" << std::endl;
    cout << "すべての処理が完了しました！" << std::endl;
    cout << "全体の結果は " << csvFilePath << " に保存されました。" << std::endl;

    return 0;
}
