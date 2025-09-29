#ifndef EMBED_BY_THRESHOLD_ADJUST_HPP
#define EMBED_BY_THRESHOLD_ADJUST_HPP

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


std::tuple<std::string, double> embedByThresholdAdjust(const std::string& coverImageFilePath, const std::string& secretImageFilePath, const vector<vector<int>>& orthogonalMatrix, int TH, double IF, const std::string& outputPath);

std::tuple<std::string, double> extractByThresholdAdjust(const std::string& stegoImageFilePath, const std::string& secretImageFilePath, const vector<vector<int>>& orthogonalMatrix, int TH, double IF, const std::string& outputPath);

#endif