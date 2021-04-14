//
// Created by 陶伟 on 2021/4/8.
//

#include "../src/dnn.h"
#include <iostream>
#include "fstream"
using combotree::OneLayerDnn;

void read_file(std::vector<float> &keys) {
    std::ifstream infile;
    infile.open("../asia_final_outfile.csv");
    std::string line_info;
    int line_num = 0;
    while(getline(infile, line_info)) {
        if(++line_num % 1000 == 0) {
            line_num = 0;
            std::stringstream input(line_info);
            std::string key;
            input >> key;
            float key_f = atof(key.c_str());
            key_f = key_f / 1e9;
//            std::cout << key_f << std::endl;
            keys.push_back(key_f);
        }
//        else break;
    }
    infile.close();
}

int main() {
    OneLayerDnn<float> net(1, 200, 1);
    std::vector<float>keys;
    read_file(keys);
    net.init(keys.begin(), keys.end());
    size_t pos = net.predict(keys[0]);
    std::cout << pos << std::endl;
}
