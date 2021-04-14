//
// Created by 陶伟 on 2021/4/8.
//

#ifndef COMBOTREE_DNN_H
#define COMBOTREE_DNN_H

#endif //COMBOTREE_DNN_H
#include <torch/torch.h>
#include <ATen/ATen.h>
#include <iostream>
#include <vector>
#include <algorithm>
//#include "matplotlibcpp.h"
// Define a new Module.
namespace combotree {
    template <class key_t>
struct OneLayerDnn : torch::nn::Module {
    OneLayerDnn(size_t n_features, size_t n_hidden, size_t n_output) {
        // Construct and register two Linear submodules.
        fc1 = register_module("fc1", torch::nn::Linear(n_features, n_hidden));
        fc2 = register_module("fc2", torch::nn::Linear(n_hidden, n_output));
        // max_num = 0;
        length = 0;
    }

    // Implement the Net's algorithm.
    torch::Tensor forward(torch::Tensor x) {
        // Use one of many tensor manipulation functions.
        x = torch::relu(fc1->forward(x));
        // x = torch::relu(fc2->forward(x));
        x = fc2->forward(x);
        return x;
    }



    int predict(key_t key) {
        std::vector<key_t> x({key});
        auto t = torch::from_blob(x.data(), x.size(), torch::kFloat);
        t = t.reshape({-1, 1});
        torch::Tensor prediction = this->forward(t);
//        std::cout << "predict2: " << prediction[0][0] << std::endl;
        std::vector<float> y(prediction.data_ptr<float>(), prediction.data_ptr<float>() + prediction.numel());
        return (int)(y[0] * length);
    }

    void init(typename std::vector<key_t>::iterator begin, typename std::vector<key_t>::iterator end) {
        std::vector<key_t> raw_keys;
        raw_keys.assign(begin, end);
        length = raw_keys.size();
        std::sort(raw_keys.begin(), raw_keys.end());
        std::vector<float> keys(length);
        max_num = *std::max_element(raw_keys.begin(), raw_keys.end());
        for(size_t i = 0; i < length; i++) {
            keys[i] = ((float)raw_keys[i])/max_num;
        }
        auto x = torch::from_blob(keys.data(), keys.size(), torch::kFloat);
        std::vector<float> cdf(length);
        for(int i = 0; i < length; i++) {
            cdf[i] = ((float)i)/length;
        }
        auto y = torch::from_blob(cdf.data(), cdf.size(), torch::kFloat);
        x = x.reshape({-1, 1});
        y = y.reshape({-1, 1});
        torch::optim::SGD optimizer(this->parameters(), /*lr=*/0.1);
        for(size_t t2 = 0; t2 <= 400; t2++) {
            optimizer.zero_grad();
            torch::Tensor prediction = this->forward(x);
            torch::Tensor loss = torch::mse_loss(prediction, y);
            loss.backward();
            optimizer.step();
  //          if(t2 % 5 == 0) {
    //            std::cout << "loss: " << loss.item<double>() << std::endl;
      //      }
 //           if(t2 == 400) {
 //               optimizer.zero_grad();
//                prediction = this->forward(x);
//                std::vector<float> y2(prediction.data_ptr<float>(), prediction.data_ptr<float>() + prediction.numel());
//                std::cout << "length: " << length << std::endl;
//                std::cout << "predict:" << prediction[0][0] << std::endl;
//                std::cout << "real: " << y[0][0]  << std::endl;
 //           }
        }

    }
    // Use one of many "standard library" modules.
    torch::nn::Linear fc1{nullptr}, fc2{nullptr};
    key_t max_num;
    size_t length;
};
}
