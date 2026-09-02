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

#include "matrix_transform_utils.hpp"
#include "embed_by_threshold_adjust.hpp"

using std::cout;
using std::endl;
using std::vector;
using std::string;
using std::tuple;

int main() {
    const int N = 256; // カバー画像サイズ

    // --- 入力画像リスト ---
    vector<tuple<string, string, string>> coverImages = {
        {"../input/img/cover_image/Lenna_grayscale_256.bmp", "Lenna_256", "Lenna_256"},
        {"../input/img/cover_image/barbara256.bmp", "barbara256", "barbara256"},
        {"../input/img/cover_image/airplane256.bmp", "airplane256", "airplane256"},
        // {"../input/img/cover_image/boat256.bmp", "boat256", "boat256"},
        // {"../input/img/cover_image/cameraman256.bmp", "cameraman256", "cameraman256"},
        // {"../input/img/cover_image/mandrill256.bmp", "mandrill256", "mandrill256"},
        // {"../input/img/cover_image/LAX.bmp", "LAX", "LAX"},
        // {"../input/img/cover_image/Text.bmp", "Text", "Text"}
    };

    vector<tuple<string, string, string>> secretImages = {
        {"../input/img/secret_image/secret_grayscale_64x64.bmp", "secret_64x64", "secret_64x64"},
        {"../input/img/secret_image/secret_grayscale_128x128.bmp", "secret_128x128", "secret_128x128"},
        {"../input/img/secret_image/secret_grayscale_256x256.bmp", "secret_256x256", "secret_256x256"},
        {"../input/img/secret_image/airplane256.bmp", "airplane256", "airplane256"},
        {"../input/img/secret_image/barbara256.bmp", "barbara256", "barbara256"},
        // {"../input/img/secret_image/boat256.bmp", "boat256", "boat256"},
        // {"../input/img/secret_image/cameraman256.bmp", "cameraman256", "cameraman256"},
        // {"../input/img/secret_image/LAX.bmp", "LAX", "LAX"},
        // {"../input/img/secret_image/Text.bmp", "Text", "Text"},
        // {"../input/img/secret_image/mandrill256.bmp", "mandrill256", "mandrill256"}
    };

    // ドブルイン系列ファイル
    string debruijnSeqFilePath;
    switch (N) {
        case 64:  debruijnSeqFilePath = "../input/debruijn/deb64.dat"; break;
        case 256: debruijnSeqFilePath = "../input/debruijn/deb256.dat"; break;
        default:
            std::cerr << "エラー: ドブルイン系列ファイルが未対応サイズです。" << endl;
            return 1;
    }

    // 実験パラメータ
    int lineStartIndex = 0;
    const int TH_min = 5, TH_max = 25, TH_step = 1;
    const double IF_RATIO_OF_THEORY = 0.98;
    vector<int> targetLineCandidates = {5, 10, 20, 50};

    // 出力ディレクトリ
    string outputDir = "../output/cover_image_comparison";
    createDirectory(outputDir);

    string csvFilePath = outputDir + "/comparison_results_all.csv";
    std::ofstream csvFile(csvFilePath);
    if (!csvFile.is_open()) {
        std::cerr << "エラー: 全体CSVが開けません: " << csvFilePath << endl;
        return 1;
    }

    csvFile << "Cover Image,Secret Image,Matrix Type,TargetLine,TH,IF,Stego PSNR,Extracted PSNR" << endl;

    // 総組み合わせ数（進捗用）
    int TH_count = ((TH_max - TH_min) / TH_step) + 1;
    long long total_combinations = static_cast<long long>(TH_count)
                                 * static_cast<long long>(targetLineCandidates.size())
                                 * static_cast<long long>(coverImages.size())
                                 * static_cast<long long>(secretImages.size());

    cout << "カバー画像数: " << coverImages.size()
         << ", 秘密画像数: " << secretImages.size() << endl;
    cout << "総組み合わせ数: " << total_combinations << endl << endl;

    long long progress = 0;

    // targetLineごとの処理
    for (int targetLineValue : targetLineCandidates) {
        cout << "----------------------------------------" << endl;
        cout << "targetLine = " << targetLineValue << " で実行開始" << endl;

        // ドブルイン行列生成
        vector<vector<int>> debruijnMatrix;
        try {
            debruijnMatrix = generateDebruijnMatrix(N, debruijnSeqFilePath, targetLineValue, lineStartIndex);
        } catch (const std::exception& e) {
            std::cerr << "Debruijn行列生成失敗 (targetLine=" << targetLineValue << "): " << e.what() << endl;
            continue;
        }

        vector<tuple<string, vector<vector<int>>>> matrixTypes = {
            {"Debruijn_t" + std::to_string(targetLineValue), debruijnMatrix}
        };

        for (const auto& [coverPath, coverName, coverDisp] : coverImages) {
            for (const auto& [secretPath, secretName, secretDisp] : secretImages) {

                string imageOutputDir = outputDir + "/" + coverName + "/" + secretName;
                createDirectory(imageOutputDir);

                string imageCsvPath = imageOutputDir + "/comparison_results.csv";
                std::ofstream imageCsvFile(imageCsvPath);
                if (imageCsvFile.is_open()) {
                    imageCsvFile << "Matrix Type,TH,IF,Stego PSNR,Extracted PSNR" << endl;
                }

                for (const auto& [matrixName, orthMatrix] : matrixTypes) {
                    string matrixOutputDir = imageOutputDir + "/" + matrixName;
                    createDirectory(matrixOutputDir);

                    for (int TH = TH_min; TH <= TH_max; TH += TH_step) {
                        double IF = (static_cast<double>(TH) / 255.0) * IF_RATIO_OF_THEORY;
                        progress++;
                        if (progress % 20 == 0 || progress == total_combinations) {
                            cout << "進捗: " << progress << "/" << total_combinations
                                 << " (" << (progress * 100 / total_combinations) << "%) "
                                 << "(Cover=" << coverName << ", Secret=" << secretName
                                 << ", TargetLine=" << targetLineValue << ")" << endl;
                        }

                        try {
                            auto [stegoFile, stegoPSNR] = embedByThresholdAdjust(coverPath, secretPath, orthMatrix, TH, IF, matrixOutputDir);
                            auto [extractedFile, extractedPSNR] = extractByThresholdAdjust(stegoFile, secretPath, orthMatrix, TH, IF, matrixOutputDir);

                            csvFile << coverDisp << "," << secretDisp << "," << matrixName << "," << targetLineValue
                                    << "," << TH << "," << std::fixed << std::setprecision(4) << IF
                                    << "," << std::fixed << std::setprecision(2) << stegoPSNR
                                    << "," << std::fixed << std::setprecision(2) << extractedPSNR << endl;

                            if (imageCsvFile.is_open()) {
                                imageCsvFile << matrixName << "," << TH << ","
                                             << std::fixed << std::setprecision(4) << IF << ","
                                             << std::fixed << std::setprecision(2) << stegoPSNR << ","
                                             << std::fixed << std::setprecision(2) << extractedPSNR << endl;
                            }
                        } catch (const std::exception& e) {
                            std::cerr << "エラー (Cover=" << coverDisp << ", Secret=" << secretDisp
                                      << ", TH=" << TH << ", IF=" << IF
                                      << ", Matrix=" << matrixName << ", TargetLine=" << targetLineValue << "): "
                                      << e.what() << endl;

                            csvFile << coverDisp << "," << secretDisp << "," << matrixName << "," << targetLineValue
                                    << "," << TH << "," << std::fixed << std::setprecision(4) << IF
                                    << ",ERROR,ERROR" << endl;

                            if (imageCsvFile.is_open()) {
                                imageCsvFile << matrixName << "," << TH << ","
                                             << std::fixed << std::setprecision(4) << IF
                                             << ",ERROR,ERROR" << endl;
                            }
                        }
                    } // TH
                } // matrixTypes

                if (imageCsvFile.is_open()) imageCsvFile.close();
                cout << "  " << secretDisp << " の処理完了: " << imageCsvPath << endl;
            } // secretImages
        } // coverImages
    } // targetLineCandidates

    csvFile.close();
    cout << "========================================" << endl;
    cout << "すべての処理が完了しました！" << endl;
    cout << "全体結果: " << csvFilePath << endl;

    return 0;
}
