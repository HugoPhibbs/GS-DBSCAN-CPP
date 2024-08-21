//
// Created by hphi344 on 21/08/24.
//

#ifndef SDBSCAN_RUN_UTILS_H
#define SDBSCAN_RUN_UTILS_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "../json.hpp"
#include "algo_utils.h"
#include "GsDBSCAN.h"

using json = nlohmann::json;


namespace GsDBSCAN::run_utils {

    inline json cleanArgs(json args) {
        json newArgs;

        newArgs["datasetFilename"] = args["--datasetFilename"];
        newArgs["outFile"] = args["--outFile"];

        newArgs["n"] = std::stoi((std::string) args["--n"]);
        newArgs["d"] = std::stoi((std::string) args["--d"]);

        newArgs["D"] = std::stoi((std::string) args["--D"]);
        newArgs["minPts"] = std::stoi((std::string) args["--minPts"]);
        newArgs["k"] = std::stoi((std::string) args["--k"]);
        newArgs["m"] = std::stoi((std::string) args["--m"]);
        newArgs["eps"] = std::stof((std::string) args["--eps"]);

        newArgs["distanceMetric"] = args["--distanceMetric"];

        newArgs["clusterBlockSize"] = std::stoi((std::string) args["--clusterBlockSize"]);

        return newArgs;
    }

    inline json parseArgs(int argc, char *argv[]) {
        json args;

        // Parse arguments
        for (int i = 1; i < argc; i += 2) {
            std::string key = argv[i];
            if (i + 1 < argc) {
                std::string value = argv[i + 1];
                args[key] = value;
            } else {
                throw std::runtime_error("Error: Missing value for argument: " + key);
            }
        }

        return cleanArgs(args);
    }

    inline float *loadCSVDatasetToDevice(std::string filename, int n, int d) {
        std::vector<float> X_vec = GsDBSCAN::algo_utils::loadCsvColumnToVector<float>(filename, 0);
        float *X_h = X_vec.data();
        float *X_d = GsDBSCAN::algo_utils::copyHostToDevice(X_h, n * d);
        return X_d;
    }

    inline float *loadBinDatasetToDevice(std::string filename, int n, int d) {
        std::vector<float> X_vec = GsDBSCAN::algo_utils::loadBinFileToVector<float>(filename);
        float *X_h = X_vec.data();
        float *X_d = GsDBSCAN::algo_utils::copyHostToDevice(X_h, n * d);
        return X_d;
    }

    inline void writeResults(json &args, json &times, int *clusterLabels, int *typeLabels) {
        std::ofstream file(args["outFile"]);
        json combined;
        combined["args"] = args;
        combined["times"] = times;

        std::vector<int> clusterLabelsVec(clusterLabels, clusterLabels + (size_t) args["n"]);
        std::vector<int> typeLabelsVec(typeLabels, typeLabels + (size_t) args["n"]);
        combined["clusterLabels"] = clusterLabelsVec;
        combined["typeLabels"] = typeLabelsVec;

        json result = json::array(); // Array of JSON objects, so Pandas can read it
        result.push_back(combined);

        if (file.is_open()) {
            file << result.dump(4);
            file.close();
        } else {
            throw std::runtime_error("Error: Unable to open file: " + (string) args["outFile"]);
        }
    }

    inline std::tuple<int *, int *, json>
    main_helper(std::string datasetFileName, int n, int d, int D, int minPts, int k, int m, float eps, float alpha,
                std::string distanceMetric, int clusterBlockSize) {
        float *X_d = loadBinDatasetToDevice(datasetFileName, n, d);

        auto [clusterLabels, typeLabels, times] = performGsDbscan(
                X_d,
                n,
                d,
                D,
                minPts,
                k,
                m,
                eps,
                alpha,
                distanceMetric,
                clusterBlockSize,
                true
        );

        cudaFree(X_d);

        return std::tie(clusterLabels, typeLabels, times);
    }
}


#endif //SDBSCAN_RUN_UTILS_H
