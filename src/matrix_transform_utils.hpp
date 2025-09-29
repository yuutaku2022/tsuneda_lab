#ifndef MATRIX_TRANSFORM_UTILS_HPP
#define MATRIX_TRANSFORM_UTILS_HPP

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

using std::cout;
using std::endl;
using std::vector;


vector<vector<int>> generateSylvesterHadamardMatrix(int N);

vector<vector<int>> generateWalshHadamardMatrix(int N);

vector<vector<int>> generateDebruijnMatrix(int N, const std::string& debruijnSeqFilePath, int targetLine, int lineStartIndex);

cv::Mat loadGrayImageAsMatrix(const std::string& imageFilePath);

vector<vector<double>> orthogonalTransform(const cv::Mat& inputImage, const vector<vector<int>>& orthogonalMatrix);

cv::Mat inverseOrthogonalTransform(const vector<vector<double>>& embeddedFreqMatrix, const vector<vector<int>>& orthogonalMatrix);

void createDirectory(const std::string& path);

double calculatePSNR(const cv::Mat& img1, const cv::Mat& img2);

#endif