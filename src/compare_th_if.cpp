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
#include <random>

#include "Eigen/Dense"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/opencv.hpp"
#include "matrix_transform_utils.hpp"
#include "embed_by_threshold_adjust.hpp"

using std::cout;
using std::endl;
using std::vector;
using std::string;
using std::tuple;

// 反転処理
void applyRandomSignFlip(std::vector<std::vector<int>>& matrix, int flipCount) {
    int N = matrix.size();
    if (flipCount > N * N) flipCount = N * N;

    std::vector<std::pair<int,int>> coords;
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            coords.emplace_back(i,j);

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(coords.begin(), coords.end(), g);

    for (int k = 0; k < flipCount; ++k) {
        int i = coords[k].first;
        int j = coords[k].second;
        matrix[i][j] = -matrix[i][j];
    }
}

// 行をランダムに入れ替える関数
void applyRandomRowShuffle(std::vector<std::vector<int>>& matrix, int shuffleCount) {
    int N = matrix.size();
    if (shuffleCount > N) shuffleCount = N;

    std::vector<int> rowIndices(N);
    for (int i = 0; i < N; ++i) rowIndices[i] = i;

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(rowIndices.begin(), rowIndices.end(), g);

    // shuffleCountだけ入れ替え
    for (int k = 0; k < shuffleCount - 1; ++k) {
        std::swap(matrix[rowIndices[k]], matrix[rowIndices[k + 1]]);
    }
}

// 列をランダムに入れ替える関数
void applyRandomColShuffle(std::vector<std::vector<int>>& matrix, int shuffleCount) {
    int N = matrix.size();
    if (shuffleCount > N) shuffleCount = N;

    std::vector<int> colIndices(N);
    for (int i = 0; i < N; ++i) colIndices[i] = i;

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(colIndices.begin(), colIndices.end(), g);

    // shuffleCountだけ隣同士を入れ替える
    for (int k = 0; k < shuffleCount - 1; ++k) {
        for (int i = 0; i < N; ++i) {
            std::swap(matrix[i][colIndices[k]], matrix[i][colIndices[k + 1]]);
        }
    }
}

int main() {
    const int N = 256; // カバー画像サイズ

    // --- 入力画像リスト ---
    vector<tuple<string, string, string>> coverImages = {
        {"../input/img/cover_image/Lenna_grayscale_256.bmp", "Lenna_256", "Lenna_256"},
        {"../input/img/cover_image/barbara256.bmp", "barbara256", "barbara256"},
        {"../input/img/cover_image/airplane256.bmp", "airplane256", "airplane256"},
        // {"../input/img/cover_image/boat256.bmp", "boat256", "boat256"},
        {"../input/img/cover_image/cameraman256.bmp", "cameraman256", "cameraman256"},
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
        {"../input/img/secret_image/cameraman256.bmp", "cameraman256", "cameraman256"},
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
    int targetLineValue = 5;
    const int TH_min = 5, TH_max = 25, TH_step = 1;
    const double IF_RATIO_OF_THEORY = 0.98;
    std::vector<int> flipCounts = {5, 10, 20, 50, 100};

    // 出力ディレクトリ
    string outputDir = "../output/cover_image_comparison5";
    createDirectory(outputDir);

    string csvFilePath = outputDir + "/comparison_results_all.csv";
    std::ofstream csvFile(csvFilePath);
    if (!csvFile.is_open()) {
        std::cerr << "エラー: 全体CSVが開けません: " << csvFilePath << endl;
        return 1;
    }

    csvFile << "Cover Image,Secret Image,Matrix Type,FlipCount,TH,IF,Stego PSNR,Extracted PSNR" << endl;


    // 総組み合わせ数（進捗用）
    int TH_count = ((TH_max - TH_min) / TH_step) + 1;

    long long progress = 0;

    
    cout << "----------------------------------------" << endl;
    cout << "targetLine = " << lineStartIndex << " で実行開始" << endl;

    // ドブルイン行列生成
    vector<vector<int>> debruijnMatrix;
    try {
        debruijnMatrix = generateDebruijnMatrix(N, debruijnSeqFilePath, targetLineValue, lineStartIndex);
    } catch (const std::exception& e) {
        std::cerr << "Debruijn行列生成失敗 (targetLine=" << targetLineValue << "): " << e.what() << endl;
        return 1;
    }
    
    vector<tuple<string, vector<vector<int>>>> matrixTypes = {
        {"Debruijn_t" + std::to_string(targetLineValue), debruijnMatrix},
    };

    long long total_combinations = static_cast<long long>(TH_count)
                   * coverImages.size()
                   * secretImages.size()
                   * matrixTypes.size();
                 
    cout << "カバー画像数: " << coverImages.size()
         << ", 秘密画像数: " << secretImages.size() << endl;
    cout << "総組み合わせ数: " << total_combinations << endl << endl;   

    for (const auto& [coverPath, coverName, coverDisp] : coverImages) {
        for (const auto& [secretPath, secretName, secretDisp] : secretImages) {

            string imageOutputDir = outputDir + "/" + coverName + "/" + secretName;
            createDirectory(imageOutputDir);

            string imageCsvPath = imageOutputDir + "/comparison_results.csv";
            std::ofstream imageCsvFile(imageCsvPath);
            if (imageCsvFile.is_open()) {
                imageCsvFile << "Matrix Type,FlipCount,TH,IF,Stego PSNR,Extracted PSNR" << endl;
            }

            for (const auto& [matrixNameBase, orthMatrix] : matrixTypes) {
                string matrixOutputDirBase = imageOutputDir + "/" + matrixNameBase;
                createDirectory(matrixOutputDirBase);

                for (int TH = TH_min; TH <= TH_max; TH += TH_step) {
                    double IF = (static_cast<double>(TH) / 255.0) * IF_RATIO_OF_THEORY;
                    progress++;
                    if (progress % 20 == 0 || progress == total_combinations) {
                        cout << "進捗: " << progress << "/" << total_combinations
                            << " (" << (progress * 100 / total_combinations) << "%) "
                            << "(Cover=" << coverName << ", Secret=" << secretName << ")"
                            << endl;
                    }

                    try {
                        // 埋め込みはオリジナル行列
                        auto [stegoFile, stegoPSNR] = embedByThresholdAdjust(coverPath, secretPath, orthMatrix, TH, IF, matrixOutputDirBase);

                        // flipCountsごとに抽出
                        for (int flipCount : flipCounts) {
                            // 反転行列生成
                            // vector<vector<int>> modifiedMatrix = orthMatrix;
                            // applyRandomSignFlip(modifiedMatrix, flipCount);

                            // 行入れ替え版
                            // vector<vector<int>> modifiedMatrix = orthMatrix;
                            // applyRandomRowShuffle(modifiedMatrix, flipCount);

                            // 列入れ替え版
                            vector<vector<int>> modifiedMatrix = orthMatrix;
                            applyRandomColShuffle(modifiedMatrix, flipCount);
                          

                            string matrixNameFlipped = matrixNameBase + "_flip_" + std::to_string(flipCount);
                            string matrixOutputDir = imageOutputDir + "/" + matrixNameFlipped;
                            createDirectory(matrixOutputDir);

                            auto [extractedFile, extractedPSNR] = extractByThresholdAdjust(stegoFile, secretPath, modifiedMatrix, TH, IF, matrixOutputDir);

                            // CSV 出力
                            csvFile << coverDisp << "," << secretDisp << "," << matrixNameFlipped
                                    << "," << flipCount << "," << TH << "," 
                                    << std::fixed << std::setprecision(4) << IF
                                    << "," << std::fixed << std::setprecision(2) << stegoPSNR
                                    << "," << std::fixed << std::setprecision(2) << extractedPSNR
                                    << endl;

                            if (imageCsvFile.is_open()) {
                                imageCsvFile << matrixNameFlipped << "," << flipCount << "," << TH << ","
                                            << std::fixed << std::setprecision(4) << IF << ","
                                            << std::fixed << std::setprecision(2) << stegoPSNR << ","
                                            << std::fixed << std::setprecision(2) << extractedPSNR
                                            << endl;
                            }
                        } // flipCounts
                    } catch (const std::exception& e) {
                        std::cerr << "エラー (Cover=" << coverDisp << ", Secret=" << secretDisp
                                << ", TH=" << TH << ", IF=" << IF
                                << ", Matrix=" << matrixNameBase << "): "
                                << e.what() << endl;

                        for (int flipCount : flipCounts) {
                            string matrixNameFlipped = matrixNameBase + "_flip_" + std::to_string(flipCount);

                            csvFile << coverDisp << "," << secretDisp << "," << matrixNameFlipped
                                    << "," << flipCount << "," << TH << "," 
                                    << std::fixed << std::setprecision(4) << IF
                                    << ",ERROR,ERROR"
                                    << endl;

                            if (imageCsvFile.is_open()) {
                                imageCsvFile << matrixNameFlipped << "," << flipCount << "," << TH << ","
                                            << std::fixed << std::setprecision(4) << IF
                                            << ",ERROR,ERROR"
                                            << endl;
                            }
                        }
                    }
                } // TH
            } // matrixTypes

            if (imageCsvFile.is_open()) imageCsvFile.close();
            cout << "  " << secretDisp << " の処理完了: " << imageCsvPath << endl;
        } // secretImages
    } // coverImages

    csvFile.close();
    cout << "========================================" << endl;
    cout << "すべての処理が完了しました！" << endl;
    cout << "全体結果: " << csvFilePath << endl;

    return 0;
}
