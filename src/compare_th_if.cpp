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
 * カバー画像と秘密画像を切り替えながらTHとIFを変えて実行し、結果をCSVに出力する
 */
int main() {
    // カバー画像のサイズ
    const int N = 256;

    // <--- 変更: 読み込むカバー画像のリストを定義
    vector<std::tuple<std::string, std::string, std::string>> coverImages = {
        {"../input/img/cover_image/Lenna_grayscale_256.bmp", "Lenna_256", "Lenna_256"},
        {"../input/img/cover_image/barbara256.bmp", "barbara256", "barbara256"},
        {"../input/img/cover_image/airplane256.bmp", "airplane256", "airplane256"},
        {"../input/img/cover_image/boat256.bmp", "boat256", "boat256"},
        {"../input/img/cover_image/cameraman256.bmp", "cameraman256", "cameraman256"},
        {"../input/img/cover_image/mandrill256.bmp", "mandrill256", "mandrill256"},
        {"../input/img/cover_image/LAX.bmp", "LAX", "LAX"},
        {"../input/img/cover_image/Text.bmp", "Text", "Text"}
        // 必要に応じて他のカバー画像（256x256）を追加
    };

    // 読み込む秘密画像のリスト
    vector<std::tuple<std::string, std::string, std::string>> secretImages = {
        {"../input/img/secret_image/secret_grayscale_64x64.bmp", "secret_64x64", "secret_64x64"},
        {"../input/img/secret_image/secret_grayscale_128x128.bmp", "secret_128x128", "secret_128x128"},
        {"../input/img/secret_image/secret_grayscale_256x256.bmp", "secret_256x256", "secret_256x256"},
        {"../input/img/secret_image/airplane256.bmp", "airplane256", "airplane256"}, // カバー画像と重複するためコメントアウト
        {"../input/img/secret_image/barbara256.bmp", "barbara256", "barbara256"},
        {"../input/img/secret_image/boat256.bmp", "boat256", "boat256"},
        {"../input/img/secret_image/cameraman256.bmp", "cameraman256", "cameraman256"},
        {"../input/img/secret_image/LAX.bmp", "LAX", "LAX"},
        {"../input/img/secret_image/Text.bmp", "Text", "Text"},
        {"../input/img/secret_image/mandrill256.bmp", "mandrill256", "mandrill256"}, // カバー画像と重複するためコメントアウト
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
    vector<vector<int>> WalshHadamardMatrix     = generateWalshHadamardMatrix(N);
    vector<vector<int>> debruijnMatrix          = generateDebruijnMatrix(N, debruijnSeqFilePath, targetLine, lineStartIndex);

    // 出力ディレクトリ
    std::string outputDir = "../output/cover_image_comparison"; // <--- 変更: 出力ディレクトリ名
    createDirectory(outputDir);

    // THの範囲とステップを定義
    const int TH_min = 5;  
    const int TH_max = 25;  
    const int TH_step = 1;   

    // THに対するIFの比率
    const double IF_RATIO_OF_THEORY = 0.98;

    // THとIFの組み合わせ数
    int TH_count = ((TH_max - TH_min) / TH_step) + 1;
    int combinations_per_matrix = TH_count;
    // <--- 変更: total_combinations に coverImages.size() を追加
    int total_combinations = combinations_per_matrix * 3 * secretImages.size() * coverImages.size();

    // 全体CSVファイル
    std::string csvFilePath = outputDir + "/comparison_results_all.csv"; // <--- 変更: ファイル名
    std::ofstream csvFile(csvFilePath);

    // <--- 変更: CSVヘッダーに Cover Image を追加
    csvFile << "Cover Image,Secret Image,Matrix Type,TH,IF,Stego PSNR,Extracted PSNR" << endl;

    cout << "カバー画像、秘密画像、THの組み合わせをテストしています..." << endl;
    cout << "テストカバー画像数: " << coverImages.size() << endl;
    cout << "テスト秘密画像数: " << secretImages.size() << endl;
    cout << "TH範囲: " << TH_min << " ～ " << TH_max << " (ステップ: " << TH_step << ")" << endl;
    cout << "IF: TH / 255.0 * " << IF_RATIO_OF_THEORY << " (動的計算)" << endl;
    cout << "総組み合わせ数: " << total_combinations << endl << endl;

    // 行列セット
    vector<std::tuple<std::string, vector<vector<int>>>> matrixTypes = {
        {"Sylvester_Hadamard", SylvesterHadamardMatrix},
        {"Walsh_Hadamard",     WalshHadamardMatrix},
        {"Debruijn",           debruijnMatrix}
    };

    int progress = 0;

    // <--- 追加: 各カバー画像について実行 (一番外側のループ)
    for (const auto& [coverImageFilePath, coverImageName, coverImageDisplayName] : coverImages) {
        
        cout << "****************************************" << endl;
        cout << "処理中カバー画像: " << coverImageDisplayName << endl;
        cout << "****************************************" << endl;

        // 各秘密画像について実行
        for (const auto& [secretImageFilePath, secretImageName, secretImageDisplayName] : secretImages) {

            cout << "========================================" << endl;
            cout << "  処理中秘密画像: " << secretImageDisplayName << endl;
            cout << "========================================" << endl;

            // <--- 変更: 出力ディレクトリ構造 (カバー画像/秘密画像/行列)
            std::string imageOutputDir = outputDir + "/" + coverImageName + "/" + secretImageName;
            createDirectory(imageOutputDir); // (注: createDirectoryが深い階層も一括作成できる必要があります)

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
                    if (progress % 20 == 0 || progress == total_combinations) { // <--- 変更: 進捗表示の頻度調整
                        cout << "進捗: " << progress << "/" << total_combinations
                             << " (" << (progress * 100 / total_combinations) << "%)" 
                             << " (Cover: " << coverImageName << ", Secret: " << secretImageName << ")" << endl;
                    }

                    try {
                        // 埋め込み
                        auto [stegoImageFilePath, stegoImagePSNR] =
                            embedByThresholdAdjust(
                                coverImageFilePath, // <--- 変更: ループ変数を使用
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

                        // <--- 変更: 全体CSVに Cover Image を追加
                        csvFile << coverImageDisplayName << ","
                                << secretImageDisplayName << ","
                                << matrixTypeName << ","
                                << TH << ","
                                << std::fixed << std::setprecision(4) << IF << "," // <--- 変更: IFの精度
                                << std::fixed << std::setprecision(2) << stegoImagePSNR << ","
                                << std::fixed << std::setprecision(2) << extractedImagePSNR
                                << endl;

                        // 画像ごとCSV
                        imageCsvFile << matrixTypeName << ","
                                     << TH << ","
                                     << std::fixed << std::setprecision(4) << IF << "," // <--- 変更: IFの精度
                                     << std::fixed << std::setprecision(2) << stegoImagePSNR << ","
                                     << std::fixed << std::setprecision(2) << extractedImagePSNR
                                     << endl;

                    } catch (const std::exception& e) {

                        std::cerr << "エラー (Cover=" << coverImageDisplayName
                                  << ", Secret=" << secretImageDisplayName
                                  << ", TH=" << TH << ", IF=" << IF
                                  << ", Matrix=" << matrixTypeName << "): "
                                  << e.what() << endl;

                        // <--- 変更: エラーログにも Cover Image を追加
                        csvFile << coverImageDisplayName << ","
                                << secretImageDisplayName << ","
                                << matrixTypeName << "," << TH << ","
                                << std::fixed << std::setprecision(4) << IF
                                << ",ERROR,ERROR" << endl;

                        imageCsvFile << matrixTypeName << ","
                                     << TH << ","
                                     << std::fixed << std::setprecision(4) << IF
                                     << ",ERROR,ERROR" << endl;
                    }
                }
            }

            imageCsvFile.close();
            cout << "  " << secretImageDisplayName << " の処理が完了しました。" << endl;
            cout << "  結果は " << imageCsvFilePath << " に保存されました。" << endl << endl;
        }
    } // <--- 追加: カバー画像のループ閉じ

    csvFile.close();
    cout << "========================================" << endl;
    cout << "すべての処理が完了しました！" << endl;
    cout << "全体の結果は " << csvFilePath << " に保存されました。" << endl;

    return 0;
}