#ifndef EMBED_BY_BIT_INVERSION_HPP
#define EMBED_BY_BIT_INVERSION_HPP

vector<vector<bool>> makeEmbeddedBoolMaskByRectangle(vector<vector<double>>& freqMatrix, cv::Mat& secretImage, int regionTopLeftX, int regionTopLeftY, vector<int>& bitPosVec);

vector<vector<bool>> makeEmbeddedBoolMaskByWholeArea(vector<vector<double>>& freqMatrix, cv::Mat& secretImage, vector<int>& bitPosVec);

vector<vector<bool>> makeEmbeddedBoolMaskByCornerArea(vector<vector<double>>& freqMatrix, cv::Mat& secretImage, vector<int>& bitPosVec);

vector<vector<bool>> embeddedAreaString_to_boolMaskMatrix(vector<vector<double>>& freqMatrix, cv::Mat& secretImage, std::string& embeddedAreaString, vector<int>& bitPosVec);

vector<std::bitset<1>> makeBinarySecretImageVec(cv::Mat& secretImage);

vector<vector<double>> embedInFreqDomainByBoolMask(vector<vector<double>>& freqMatrix, vector<std::bitset<1>>& binarySecretImageVec, vector<vector<bool>>& embeddedBoolMask, vector<int>& bitPosVec);

cv::Mat extractFromFreqDomainByBoolMask(vector<vector<double>>& stegoFreqMatrix, int secretImageWidth, int secretImageHeight, vector<vector<bool>>& embeddedBoolMask, vector<int>& bitPosVec);

std::tuple<std::string, double> embedByBitInversion(const std::string& coverImageFilePath, const std::string& secretImageFilePath, const vector<vector<int>>& orthogonalMatrix, std::string& embeddedAreaString, vector<int>& bitPosVec, const std::string& outputPath);

std::tuple<std::string, double> extractByBitInversion(const std::string& stegoImageFilePath, const std::string& secretImageFilePath, const vector<vector<int>>& orthogonalMatrix, std::string& embeddedAreaString, vector<int>& bitPosVec, const std::string& outputPath);

#endif