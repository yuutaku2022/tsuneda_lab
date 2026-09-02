#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "Eigen/Dense"
#include "opencv2/opencv.hpp"

// 画像を読み込み、グレースケール化した後に 64x64 の Eigen 行列に変換する。
Eigen::MatrixXd loadGrayscaleImage(const std::string& imagePath, int size) {
    cv::Mat image = cv::imread(imagePath, cv::IMREAD_GRAYSCALE);
    if (image.empty()) {
        throw std::runtime_error("画像を読み込めませんでした: " + imagePath);
    }

    cv::Mat resized;
    cv::resize(image, resized, cv::Size(size, size), 0, 0, cv::INTER_AREA);

    Eigen::MatrixXd matrix(size, size);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            matrix(y, x) = static_cast<double>(resized.at<uchar>(y, x));
        }
    }
    return matrix;
}

// Sylvester の再帰で N x N のハダマード行列を生成する。
Eigen::MatrixXd generateSylvesterHadamard(int N) {
    if (N == 1) {
        Eigen::MatrixXd H(1, 1);
        H(0, 0) = 1.0;
        return H;
    }
    Eigen::MatrixXd Hprev = generateSylvesterHadamard(N / 2);
    Eigen::MatrixXd H(N, N);
    H.topLeftCorner(N / 2, N / 2) = Hprev;
    H.topRightCorner(N / 2, N / 2) = Hprev;
    H.bottomLeftCorner(N / 2, N / 2) = Hprev;
    H.bottomRightCorner(N / 2, N / 2) = -Hprev;
    return H;
}

// 1 行の交番数 (sequency) を計算する。符号が変わる回数を数える。
int computeSequency(const Eigen::VectorXd& row) {
    int count = 0;
    for (int i = 1; i < row.size(); ++i) {
        if (row(i) != row(i - 1)) {
            ++count;
        }
    }
    return count;
}

// ハダマード行列を交番数順に並び替え、行と列の両方を揃えて対称にする。
Eigen::MatrixXd reorderWalshSequency(const Eigen::MatrixXd& H) {
    const int N = H.rows();
    std::vector<std::pair<int, int>> sequencyIndex;
    sequencyIndex.reserve(N);

    for (int i = 0; i < N; ++i) {
        sequencyIndex.emplace_back(computeSequency(H.row(i)), i);
    }

    std::stable_sort(sequencyIndex.begin(), sequencyIndex.end());

    Eigen::MatrixXd ordered(N, N);
    for (int i = 0; i < N; ++i) {
        ordered.row(i) = H.row(sequencyIndex[i].second);
    }

    Eigen::MatrixXd result(N, N);
    for (int j = 0; j < N; ++j) {
        result.col(j) = ordered.col(sequencyIndex[j].second);
    }

    return result;
}

// 交番数順アダマール行列を生成し、正規化する。
Eigen::MatrixXd generateNormalisedWalshHadamard(int N) {
    Eigen::MatrixXd H = generateSylvesterHadamard(N);
    Eigen::MatrixXd Hseq = reorderWalshSequency(H);
    return Hseq / std::sqrt(static_cast<double>(N));
}

// ドブルイン系列テキストを読み込み、全ての改行と空白を除去する。
std::string loadDebruijnSequence(const std::string& filePath) {
    std::ifstream input(filePath);
    if (!input.is_open()) {
        throw std::runtime_error("ドブルイン系列ファイルを開けませんでした: " + filePath);
    }

    std::ostringstream buffer;
    std::string line;
    while (std::getline(input, line)) {
        for (char c : line) {
            if (c == '0' || c == '1') {
                buffer << c;
            }
        }
    }

    std::string sequence = buffer.str();
    if (sequence.empty()) {
        throw std::runtime_error("ドブルイン系列の内容が空です: " + filePath);
    }
    return sequence;
}

// ドブルイン行列を生成する。Walsh 行列の列をドブルイン系列の窓に従って並べ替える。
Eigen::MatrixXd generateDebruijnMatrix(int N, const std::string& debruijnPath, const Eigen::MatrixXd& walsh) {
    const int windowSize = static_cast<int>(std::round(std::log2(static_cast<double>(N))));
    std::string sequence = loadDebruijnSequence(debruijnPath);

    if (sequence.size() < static_cast<size_t>(windowSize + N - 1)) {
        sequence += sequence;
    }

    Eigen::MatrixXd D(N, N);
    for (int col = 0; col < N; ++col) {
        std::string window = sequence.substr(col, windowSize);
        int index = std::stoi(window, nullptr, 2);
        if (index < 0 || index >= N) {
            throw std::runtime_error("ドブルイン系列から不正なインデックスが生成されました。");
        }
        D.col(col) = walsh.col(index);
    }

    return D;
}

// 指定範囲を超えないように値をクランプする。
double clampValue(double value, double minVal, double maxVal) {
    return std::max(minVal, std::min(maxVal, value));
}

// 値を -1000..0..1000 の範囲で青-白-赤にマッピングする。
cv::Vec3b mapValueToHeatmapColor(double value, double minVal, double maxVal) {
    const double clipped = clampValue(value, minVal, maxVal);
    const double normalized = (clipped - minVal) / (maxVal - minVal);
    const double midPoint = (-minVal) / (maxVal - minVal);

    if (normalized <= midPoint) {
        double ratio = normalized / midPoint;
        // 負側: 青から白
        uchar blue = 255;
        uchar green = static_cast<uchar>(255.0 * ratio);
        uchar red = static_cast<uchar>(255.0 * ratio);
        return cv::Vec3b(blue, green, red);
    }

    double ratio = (normalized - midPoint) / (1.0 - midPoint);
    // 正側: 白から赤
    uchar blue = static_cast<uchar>(255.0 * (1.0 - ratio));
    uchar green = static_cast<uchar>(255.0 * (1.0 - ratio));
    uchar red = 255;
    return cv::Vec3b(blue, green, red);
}

// Eigen 行列をヒートマップとして OpenCV 画像に変換する。
cv::Mat createHeatmap(const Eigen::MatrixXd& matrix, double minVal, double maxVal) {
    const int rows = matrix.rows();
    const int cols = matrix.cols();
    cv::Mat heatmap(rows, cols, CV_8UC3);

    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            heatmap.at<cv::Vec3b>(y, x) = mapValueToHeatmapColor(matrix(y, x), minVal, maxVal);
        }
    }
    return heatmap;
}

// カラーバーを描画する。上が maxVal、下が minVal になる。
cv::Mat createColorbar(int height, int width, double minVal, double maxVal) {
    cv::Mat bar(height, width, CV_8UC3);
    for (int y = 0; y < height; ++y) {
        double value = maxVal - (static_cast<double>(y) / static_cast<double>(height - 1)) * (maxVal - minVal);
        cv::Vec3b color = mapValueToHeatmapColor(value, minVal, maxVal);
        for (int x = 0; x < width; ++x) {
            bar.at<cv::Vec3b>(y, x) = color;
        }
    }

    // 目盛を描画
    const int margin = 6;
    const int fontFace = cv::FONT_HERSHEY_SIMPLEX;
    const double fontScale = 0.4;
    const int thickness = 1;
    std::vector<double> labels = {maxVal, 0.0, minVal};
    for (double value : labels) {
        int y = static_cast<int>(std::round((maxVal - value) / (maxVal - minVal) * (height - 1)));
        std::ostringstream ss;
        ss << static_cast<int>(value);
        std::string text = ss.str();
        int baseline = 0;
        cv::Size textSize = cv::getTextSize(text, fontFace, fontScale, thickness, &baseline);
        int tx = width - textSize.width - margin;
        int ty = std::max(0, std::min(height - 1, y + textSize.height / 2));
        cv::putText(bar, text, cv::Point(tx, ty), fontFace, fontScale, cv::Scalar(0, 0, 0), thickness, cv::LINE_AA);
    }

    return bar;
}

// 画像にラベルを描画する。
void drawLabel(cv::Mat& canvas, const std::string& text, int x, int y) {
    const int fontFace = cv::FONT_HERSHEY_SIMPLEX;
    const double fontScale = 1.0;
    const int thickness = 2;
    const cv::Scalar color(0, 0, 0);
    cv::putText(canvas, text, cv::Point(x, y), fontFace, fontScale, color, thickness, cv::LINE_AA);
}

int main() {
    try {
        const int size = 64;
        const std::string inputImagePath = "input/img/cover_image/Lenna_grayscale_256.bmp";
        const std::string debruijnPath = "input/debruijn/deb64.dat";
        const double colorbarMin = -1000.0;
        const double colorbarMax = 1000.0;

        // 画像読み込みと 64x64 行列への変換
        Eigen::MatrixXd sourceImageMatrix = loadGrayscaleImage(inputImagePath, size);

        // 交番数順アダマール行列を生成
        Eigen::MatrixXd walsh = generateNormalisedWalshHadamard(size);

        // ドブルイン行列を生成（交番数順 Walsh 行列の列並べ替え）
        Eigen::MatrixXd debruijn = generateDebruijnMatrix(size, debruijnPath, walsh);

        // 2 次元直交変換を適用
        Eigen::MatrixXd hadamardCoefficients = walsh * sourceImageMatrix * walsh.transpose();
        Eigen::MatrixXd debruijnCoefficients = debruijn * sourceImageMatrix * debruijn.transpose();

        // ヒートマップを生成
        cv::Mat hadamardHeatmap = createHeatmap(hadamardCoefficients, colorbarMin, colorbarMax);
        cv::Mat debruijnHeatmap = createHeatmap(debruijnCoefficients, colorbarMin, colorbarMax);
        cv::Mat colorbar = createColorbar(520, 40, colorbarMin, colorbarMax);

        // 元画像を BGR に変換して描画サイズを調整
        cv::Mat originalGray = cv::imread(inputImagePath, cv::IMREAD_GRAYSCALE);
        cv::Mat originalResized;
        cv::resize(originalGray, originalResized, cv::Size(320, 320), 0, 0, cv::INTER_AREA);
        cv::Mat originalBgr;
        cv::cvtColor(originalResized, originalBgr, cv::COLOR_GRAY2BGR);

        // 最終キャンバスを作成
        cv::Mat canvas(800, 1600, CV_8UC3, cv::Scalar(255, 255, 255));

        // 描画位置を設定
        const int leftX = 20;
        const int rightX = 1040;
        const int topY = 120;
        const int heatmapSize = 520;
        const int colorbarXOffset = 536;
        const int centerX = 580;
        const int originalY = 40;

        // ヒートマップをリサイズして描画
        cv::Mat hadamardResized;
        cv::resize(hadamardHeatmap, hadamardResized, cv::Size(heatmapSize, heatmapSize), 0, 0, cv::INTER_NEAREST);
        cv::Mat debruijnResized;
        cv::resize(debruijnHeatmap, debruijnResized, cv::Size(heatmapSize, heatmapSize), 0, 0, cv::INTER_NEAREST);

        hadamardResized.copyTo(canvas(cv::Rect(leftX, topY, heatmapSize, heatmapSize)));
        debruijnResized.copyTo(canvas(cv::Rect(rightX, topY, heatmapSize, heatmapSize)));
        colorbar.copyTo(canvas(cv::Rect(leftX + colorbarXOffset, topY, colorbar.cols, colorbar.rows)));
        colorbar.copyTo(canvas(cv::Rect(rightX + colorbarXOffset, topY, colorbar.cols, colorbar.rows)));

        // 元画像を中央上に描画
        originalBgr.copyTo(canvas(cv::Rect(centerX + 60, originalY, originalBgr.cols, originalBgr.rows)));

        // ラベル描画
        drawLabel(canvas, "Hadamard Transform", leftX, topY - 20);
        drawLabel(canvas, "Debruijn Transform", rightX, topY - 20);
        drawLabel(canvas, "Original Image", centerX + 60, originalY - 10);

        drawLabel(canvas, "Colorbar: -1000 to 1000", leftX + colorbarXOffset, topY + heatmapSize + 30);
        drawLabel(canvas, "Colorbar: -1000 to 1000", rightX + colorbarXOffset, topY + heatmapSize + 30);

        // 枠線を描画
        cv::rectangle(canvas, cv::Rect(leftX - 2, topY - 2, heatmapSize + 4, heatmapSize + 4), cv::Scalar(0, 0, 0), 2);
        cv::rectangle(canvas, cv::Rect(rightX - 2, topY - 2, heatmapSize + 4, heatmapSize + 4), cv::Scalar(0, 0, 0), 2);
        cv::rectangle(canvas, cv::Rect(centerX + 60 - 2, originalY - 2, originalBgr.cols + 4, originalBgr.rows + 4), cv::Scalar(0, 0, 0), 2);

        // 画像を保存
        const std::string outputPath = "result.png";
        if (!cv::imwrite(outputPath, canvas)) {
            throw std::runtime_error("画像ファイルを保存できませんでした: " + outputPath);
        }

        std::cout << "result.png を出力しました。" << std::endl;
        return 0;
    }
    catch (const std::exception& ex) {
        std::cerr << "エラー: " << ex.what() << std::endl;
        return 1;
    }
}
