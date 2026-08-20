#include "nn/Training/Checkpoint.hpp"
#include <fstream>
#include <iostream>

namespace nn {

bool Checkpoint::save(const std::string& filepath, const std::vector<Tensor*>& parameters) {
    std::ofstream out(filepath, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Checkpoint::save: Failed to open file " << filepath << std::endl;
        return false;
    }

    // Write number of parameters
    int numParams = static_cast<int>(parameters.size());
    out.write(reinterpret_cast<const char*>(&numParams), sizeof(int));

    for (const Tensor* param : parameters) {
        // Write rank
        int rank = param->rank();
        out.write(reinterpret_cast<const char*>(&rank), sizeof(int));

        // Write shape
        const auto& shape = param->shape();
        out.write(reinterpret_cast<const char*>(shape.data()), rank * sizeof(int));

        // Write flat data size
        int size = param->size();
        out.write(reinterpret_cast<const char*>(&size), sizeof(int));

        // Write raw data
        out.write(reinterpret_cast<const char*>(param->rawData()), size * sizeof(float));
    }

    return true;
}

bool Checkpoint::load(const std::string& filepath, const std::vector<Tensor*>& parameters) {
    std::ifstream in(filepath, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "Checkpoint::load: Failed to open file " << filepath << std::endl;
        return false;
    }

    int numParams = 0;
    in.read(reinterpret_cast<char*>(&numParams), sizeof(int));
    if (numParams != static_cast<int>(parameters.size())) {
        std::cerr << "Checkpoint::load: Parameter count mismatch. File has " << numParams 
                  << ", expected " << parameters.size() << std::endl;
        return false;
    }

    for (Tensor* param : parameters) {
        int rank = 0;
        in.read(reinterpret_cast<char*>(&rank), sizeof(int));
        if (rank != param->rank()) {
            std::cerr << "Checkpoint::load: Rank mismatch. File has " << rank 
                      << ", expected " << param->rank() << std::endl;
            return false;
        }

        std::vector<int> shape(rank);
        in.read(reinterpret_cast<char*>(shape.data()), rank * sizeof(int));
        if (shape != param->shape()) {
            std::cerr << "Checkpoint::load: Shape mismatch." << std::endl;
            return false;
        }

        int size = 0;
        in.read(reinterpret_cast<char*>(&size), sizeof(int));
        if (size != param->size()) {
            std::cerr << "Checkpoint::load: Size mismatch." << std::endl;
            return false;
        }

        in.read(reinterpret_cast<char*>(param->rawData()), size * sizeof(float));
    }

    return true;
}

} // namespace nn
