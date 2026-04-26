#include "q_learning.h"

using namespace std;
QLearningModel::QLearningModel() {}

QLearningModel::QLearningModel(int state_count, int action_count, double lr_max, double lr_min, double lr_half_life, float discount_factor, double er_max, double er_min, double er_half_life): 
        q_table(state_count, vector<float>(action_count, 0)), 
        state_count(state_count), 
        action_count(action_count), 
        lr_max(lr_max),
        lr_min(lr_min),
        lr_half_life(lr_half_life),
        discount_factor(discount_factor), 
        er_max(er_max),
        er_min(er_min),
        er_half_life(er_half_life)
{
        lr = lr_max;
        er = er_max;
        lr_ratio = pow(0.5, 1.0/lr_half_life);
        er_ratio = pow(0.5, 1.0/er_half_life);
        reset();
}

QLearningModel::QLearningModel(map<string, string> config) {
    //state_count = stoi(config["X_DIVIDE"]) * stoi(config["Y_DIVIDE"]) * stoi(config["THETA_DIVIDE"]);
    state_count = stoi(config["N_STATES"]);
    action_count = stoi(config["N_ACTIONS"]);

    lr_max = stof(config["Q_LR_MAX"]);
    lr_min = stof(config["Q_LR_MIN"]);
    lr_half_life = stoi(config["Q_LR_HL"]);
    discount_factor = stof(config["DISCOUNT_FACTOR"]);
    er_max = stof(config["ER_MAX"]);
    er_min = stof(config["ER_MIN"]);
    er_half_life = stof(config["ER_HL"]);

    lr = lr_max;
    er = er_max;
    lr_ratio = pow(0.5, 1.0/lr_half_life);
    er_ratio = pow(0.5, 1.0/er_half_life);
    cout << lr_ratio << endl;
    cout << er_ratio << endl;

    q_table.resize(state_count); 

    for (int i = 0; i < state_count; ++i) {
        q_table[i].resize(action_count, 0.0f);
    }


    reset();

}

void QLearningModel::reset() {
    random_device rd;
    rng.seed(rd());

    uniform_real_distribution<float> distribution(0.0, 1.0);
    for (int i = 0; i < state_count; ++i) {
        for (int j = 0; j < action_count; ++j) {
            q_table[i][j] = distribution(rng);
        }
    }

}

int QLearningModel::chooseAction(int state, bool eval) {
    if(state >= state_count || state < 0) {
        cerr << "non valid state " << state << endl;
        throw 0;
    }
    uniform_real_distribution<float> distribution(0.0, 1.0);
    if (distribution(rng) < er && !eval) {
        uniform_int_distribution<int> actionDistribution(0, q_table[state].size() - 1);
        return actionDistribution(rng);
    } else {
        auto maxAction = max_element(q_table[state].begin(), q_table[state].end());
        return distance(q_table[state].begin(), maxAction);
    }
}

float QLearningModel::train(int state, int action, float reward, int nextState) {
    if(state >= state_count) {
        cerr << "non valid state " << state << endl;
        throw 0;
    }
    if(nextState >= state_count) {
        cerr << "non valid state " << nextState << endl;
        throw 0;
    }
    if(action >= action_count) {
        cerr << "non valid action " << action << endl;
        throw 0;
    }

    float maxNextQValue = *max_element(q_table[nextState].begin(), q_table[nextState].end());
    float tdr = reward + discount_factor * maxNextQValue - q_table[state][action];
    q_table[state][action] += lr * tdr;

    return tdr;
}



bool QLearningModel::storeWeights(const string& filename) {
    string filepath = "parameters/" + filename; 

    ofstream file(filepath);
    if (!file.is_open()) {
        cerr << "Failed to open file: " << filepath << endl;
        return false;
    }

    for (const auto& qrow : q_table) {
        for (const auto& value : qrow) {
            file << value << " ";
        }
        file << endl;
    }
    cout << "q_table stored in " << filename << endl;

    file.close();
    return true;
}

bool QLearningModel::loadWeights(const string& filename) {
    string filepath = "parameters/" + filename; 

    ifstream file(filepath);
    if (!file.is_open()) {
        cerr << "Failed to open file: " << filename << endl;
        return false;
    }
    q_table.clear();
    vector<float> neuronWeights;
    float weight;
    while (file >> weight) {
        neuronWeights.push_back(weight);
        if (neuronWeights.size() == (unsigned int)(action_count)) {
            q_table.push_back(neuronWeights);
            neuronWeights.clear();
        }
    }
    cout << "q_table loaded from " << filename << endl;
    file.close();
    return true;
}
