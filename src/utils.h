#ifndef UTILS_H
#define UTILS_H
#include <random>
#include <iostream>
#include <fstream>
#include <regex>
#include <string>
#include <map>
#include <vector>
#include <algorithm>

// Function to generate a random float between a and b
float randomFloat(float a, float b);

// Distance
float euclideanDistance(const std::vector<float>& v1, const std::vector<float>& v2);
float anglediff(float x, float y);
void printMatrix(const std::vector<std::vector<float>>& matrix);

std::map<std::string, std::string> readConfig(const std::string& filePath);

std::vector<float> progression(int n, float unity);
#endif // UTILS_H
