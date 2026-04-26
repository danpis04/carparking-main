#include "utils.h"

using namespace std;
float randomFloat(float a, float b) {
    if(a > b) {
        cerr << "in interval linf > lsup" << endl;
    }
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<float> distribution(a, b);
    return distribution(gen);
}

float euclideanDistance(const vector<float>& v1, const vector<float>& v2) {
    float sum = 0.0f;
    for (size_t i = 0; i < v1.size(); ++i) {
        sum += pow(v1[i] - v2[i], 2);
    }
    return sqrt(sum);
}

float anglediff(float x, float y) {
    if(x==y) return 0.0;
    x = fmod(x, 2*M_PI);
    if(x<0) x+=2*M_PI;
    y = fmod(y, 2*M_PI);
    if(y<0) y+=2*M_PI;
    float min_ = min(x, y);
    float max_ = max(x, y);
    return min((max_-min_), (float)(min_+2*M_PI-max_));
}

void printMatrix(const vector<vector<float>>& matrix) {
    for (const auto& row : matrix) {
        for (float element : row) {
            cout << element << "\t";
        }
        cout << endl;
    }
    cout << endl;
}

map<string, string> readConfig(const string& filePath) {

    map<string, string> config;
    ifstream file(filePath);
    if (!file.is_open()) {
        cout << "Somthing wrong in .conf opening" << endl;
        return config;
    }

    regex pattern(R"(^\s*([^\s#]+)\s*=\s*([^#]+?)\s*$)");
    smatch match;
    string line;

    while (getline(file, line)) {
        size_t commentPos = line.find('#');
        if (commentPos != string::npos) {
            line = line.substr(0, commentPos); 
        }

        line = regex_replace(line, regex(R"(\s+$)"), "");

        if (regex_match(line, match, pattern)) {
            config[match[1]] = match[2];
        }
    }

    return config;
}

vector<float> progression(int n, float unity) {
    vector<float> ret(n);
    float start = -(n/2)*unity;
    if(n%2==0) start += unity/2;
    for(int i=0; i<n; i++) ret[i] = start + i*unity;
    return ret;
}
