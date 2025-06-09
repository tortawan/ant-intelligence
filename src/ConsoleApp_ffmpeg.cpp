/**
 * @file ConsoleApp_ffmpeg.cpp
 * @brief Main executable for running the ant intelligence simulation.
 *
 * This application initializes a Ground, populates it with ants and objects,
 * and runs the simulation for a specified number of iterations over multiple
 * experimental runs. It sweeps through different 'similarityThreshold' values
 * to test their effect on object clustering.
 *
 * The results, including average cluster size and interaction counts, are
 * logged to a CSV file for later analysis.
 *
 * @section usage Usage
 * The program is controlled via command-line arguments:
 * --width <int>                Width of the simulation grid.
 * --length <int>               Length of the simulation grid.
 * --ants <int>                 Number of ants in the simulation.
 * --experiments <int>          Number of times to run the simulation for each threshold.
 * --iterations <int>           Number of steps in each simulation run.
 * --memory_size <int>          The memory capacity of each ant.
 * --threshold_start <int>      The starting value for the similarity threshold sweep.
 * --threshold_end <int>        The ending value for the similarity threshold sweep.
 * --threshold_interval <int>   The step size for the threshold sweep.
 * --prob_relu_low <float>      The lower bound for the pick/drop probability function.
 * --prob_relu_high <float>     The upper bound for the pick/drop probability function.
 * --csv_filename <string>      Name of the output data file.
 * --video <true|false>         Enable or disable video recording of the simulation.
 */

#define NOMINMAX

#include <opencv2/opencv.hpp>
#include <vector>
#include <unordered_map>
#include <random>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <thread>
#include <fstream>
#include <iostream>
#include <omp.h>
#include <windows.h>
#include <cstdlib>
#include <memory>
#include "ant_intelligence/Ant.h"
#include "ant_intelligence/Ground.h"
#include "ant_intelligence/Objects.h"
#include "ant_intelligence/Config.h"

// Function to set thread affinity to performance cores
bool SetThreadAffinityToPerformanceCores() {
    DWORD_PTR processAffinityMask = 0;
    DWORD_PTR systemAffinityMask = 0;

    if (!GetProcessAffinityMask(GetCurrentProcess(), &processAffinityMask, &systemAffinityMask)) {
        std::cerr << "Failed to get process affinity mask. Error code: " << GetLastError() << std::endl;
        return false;
    }

    // For i9 13900HX, let's assume the first 8 cores are performance cores
    DWORD_PTR performanceCoreMask = 0xFF; // binary: 11111111

    DWORD_PTR newAffinityMask = performanceCoreMask & processAffinityMask;
    if (newAffinityMask == 0) {
        std::cerr << "No performance cores available within the process affinity mask." << std::endl;
        return false;
    }

    if (!SetThreadAffinityMask(GetCurrentThread(), newAffinityMask)) {
        std::cerr << "Failed to set thread affinity mask. Error code: " << GetLastError() << std::endl;
        return false;
    }

    return true;
}

int main(int argc, char* argv[]) {
    // Seed for random number generation
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    // Default values
    int ground_width = AIConfig::DEFAULT_GROUND_WIDTH;
    int ground_length = AIConfig::DEFAULT_GROUND_LENGTH;
    int num_ants = AIConfig::DEFAULT_NUM_ANTS;
    int num_iter = AIConfig::DEFAULT_NUM_EXPERIMENTS;
    int num_iteration = AIConfig::DEFAULT_ITERATIONS;
    int memory_size = AIConfig::DEFAULT_MEMORY_SIZE;           // Ant memory size
    int threshold_start = AIConfig::DEFAULT_THRESHOLD_START;   // Start similarity threshold
    int threshold_end = AIConfig::DEFAULT_THRESHOLD_END;       // End similarity threshold
    int threshold_interval = AIConfig::DEFAULT_THRESHOLD_INTERVAL; // Interval for similarity threshold sweep
    std::vector<double> prob_relu(AIConfig::DEFAULT_PROB_RELU.begin(), AIConfig::DEFAULT_PROB_RELU.end());
    bool enable_visual = false;
    std::string csvFilename = "ground_data.csv";  // Default CSV filename


    auto start_time = std::chrono::high_resolution_clock::now();

    // Parse command-line arguments 
    for (int i = 1; i < argc; i += 2) {
        std::string arg = argv[i];
        if (i + 1 < argc) {
            if (arg == "--width") {
                ground_width = std::atoi(argv[i + 1]);
            }
            else if (arg == "--length") {
                ground_length = std::atoi(argv[i + 1]);
            }
            else if (arg == "--ants") {
                num_ants = std::atoi(argv[i + 1]);
            }
            else if (arg == "--experiments") {
                num_iter = std::atoi(argv[i + 1]);
            }
            else if (arg == "--iterations") {
                num_iteration = std::atoi(argv[i + 1]);
            }
            else if (arg == "--memory_size") {
                memory_size = std::atoi(argv[i + 1]);
            }
            else if (arg == "--threshold_start") {
                threshold_start = std::atoi(argv[i + 1]);
            }
            else if (arg == "--threshold_end") {
                threshold_end = std::atoi(argv[i + 1]);
            }
            else if (arg == "--threshold_interval") {
                threshold_interval = std::atoi(argv[i + 1]);
            }
            else if (arg == "--prob_relu_low") {
                prob_relu[0] = std::atof(argv[i + 1]);
            }
            else if (arg == "--prob_relu_high") {
                prob_relu[1] = std::atof(argv[i + 1]);
            }
            else if (arg == "--csv_filename") { // New parameter
                csvFilename = argv[i + 1];
            }
            else if (arg == "--video") {
                std::string val = argv[i + 1];
                if (val == "true" || val == "1") {
                    enable_visual = true;
                }
                else if (val == "false" || val == "0") {
                    enable_visual = false;
                }
            }


        }
    }


    // Print parameters
    std::cout << "Ground width: " << ground_width << std::endl;
    std::cout << "Ground length: " << ground_length << std::endl;
    std::cout << "Number of ants: " << num_ants << std::endl;
    std::cout << "Number of experiments: " << num_iter << std::endl;
    std::cout << "Iterations per experiment: " << num_iteration << std::endl;
    std::cout << "Memory size: " << memory_size << std::endl;
    std::cout << "Threshold sweep: from " << threshold_start << " to " << threshold_end
        << " with interval " << threshold_interval << std::endl;
    std::cout << "Prob_relu: [" << prob_relu[0] << ", " << prob_relu[1] << "]" << std::endl;
    std::cout << "Video enabled: " << (enable_visual ? "Yes" : "No") << std::endl;
    std::cout << "Multi Files " << std::endl;

    // Normalize probability distribution for ant movement
    std::vector<double> prob = { 12, 5, 2, 1, 0.1, 1, 2, 5 };
    double prob_sum = std::accumulate(prob.begin(), prob.end(), 0.0);
    for (auto& p : prob) {
        p /= prob_sum;
    }

    // Probability distribution for objects
    std::unordered_map<std::shared_ptr<Object>, double> obj_dict = {
        {std::make_shared<Food>(),  0.05},
        {std::make_shared<Egg>(),   0.05},
        {std::make_shared<Waste>(), 0.05},
        {nullptr,                  0.85}
    };

    // Open output file with header (now with an extra column for Memory)
    std::ofstream file(csvFilename);
    if (!file.is_open()) {
        std::cerr << "Could not open the output file for writing" << std::endl;
        return -1;
    }

    // Write metadata header
    file << "# Simulation Parameters:\n";
    file << "# Grid Width: " << ground_width << "\n";
    file << "# Ground Length: " << ground_length << "\n";
    file << "# Number of Ants: " << num_ants << "\n";
    file << "# Number of Experiments: " << num_iter << "\n";
    file << "# Iterations per Experiment: " << num_iteration << "\n";
    file << "# Memory Size: " << memory_size << "\n";
    file << "# Threshold Start: " << threshold_start << "\n";
    file << "# Threshold End: " << threshold_end << "\n";
    file << "# Threshold Interval: " << threshold_interval << "\n";
    file << "# Prob Relu Low: " << prob_relu[0] << "\n";
    file << "# Prob Relu High: " << prob_relu[1] << "\n";

    // Updated header to include InteractionCount
    file << "Threshold,Iteration,Run,ClusterSize,Memory,InteractionCount\n";

    // Video output setup
    cv::VideoWriter video;
    if (enable_visual) {
        video.open(
            "ground_simulation.mp4",
            cv::VideoWriter::fourcc('m', 'p', '4', 'v'),
            60,
            cv::Size(ground_length * 6, ground_width * 6)
        );
        if (!video.isOpened()) {
            std::cerr << "Could not open the output video file for writing" << std::endl;
            return -1;
        }
    }

    // Data collection: sweep threshold values from threshold_start to threshold_end with threshold_interval
    for (int threshold = threshold_start; threshold <= threshold_end; threshold += threshold_interval) {
        std::vector<std::vector<double>> all_cluster_sizes(num_iter);

        std::cout << "Starting experiments for Similarity Threshold = " << threshold << std::endl;
        auto threshold_start_time = std::chrono::high_resolution_clock::now();

#pragma omp parallel
        {
            if (!SetThreadAffinityToPerformanceCores()) {
                std::cerr << "Warning: Failed to set thread affinity for thread " << omp_get_thread_num() << std::endl;
           }
#pragma omp single
            {
                std::cout << "Number of threads in use: " << omp_get_num_threads() << std::endl;
            }
#pragma omp for schedule(dynamic, 1)
            for (int j = 0; j < num_iter; ++j) {
                std::random_device rd;
                std::mt19937 local_gen(rd() + omp_get_thread_num());

#pragma omp critical
                {
                    std::cout << "Threshold " << threshold << " - Run number " << j + 1
                        << " starting on thread " << omp_get_thread_num() << std::endl;
                }
                // Construct Ground with current threshold; ensure that Ground uses memory_size as needed.
                Ground ground(ground_width, ground_length, prob, prob_relu, threshold);
                // Optionally, set the memory size in Ground or pass it to Ants here.
                ground.addObject(obj_dict);
                for (int i = 0; i < num_ants; ++i) {
                    ground.addAnt(memory_size);
                }
#pragma omp critical
                {
                    ground.countObjects();
                    if (enable_visual) {
                        ground.showGround("Ground before operation", video);
                    }
                }
                for (int i = 0; i < num_iteration; ++i) {
                    if (enable_visual && i % 500 == 0) {
#pragma omp critical
                        {
                            ground.showGround("Ant Movement", video);
                        }
                    }
                    ground.moveAnts();
                    ground.assignWork();
                    ground.handleAntInteractions(i);

                    if (i % 10000 == 0) {
                        double temp = ground.averageClusterSize();
#pragma omp critical
                        {
                            all_cluster_sizes[j].push_back(temp);
                            // Log the average cluster size, memory, and interaction count
                            if (!ground.getAgents().empty()) {
                                const Ant& ant = ground.getAgents().front();
                                file << threshold << "," << i << "," << j + 1 << "," << temp
                                    << "," << ant.getMemoryString() << ","
                                    << ground.getInteractionCount() << "\n";

                                std::cout << "Threshold " << threshold << " - Logged cluster size: "
                                    << temp << " at iteration " << i
                                    << " for run " << j + 1
                                    << " on thread " << omp_get_thread_num()
                                    << " - Interaction count: " << ground.getInteractionCount() << std::endl;
                            }
                            else {
                                file << threshold << "," << i << "," << j + 1 << "," << temp
                                    << ",," << ground.getInteractionCount() << "\n";

                                std::cout << "Threshold " << threshold << " - Logged cluster size: "
                                    << temp << " at iteration " << i
                                    << " for run " << j + 1
                                    << " on thread " << omp_get_thread_num()
                                    << " - Interaction count: " << ground.getInteractionCount() << std::endl;
                            }
                        }
                    }
                }
#pragma omp critical
                {
                    if (enable_visual) {
                        ground.showGround("Ground after operation", video);
                    }
                    std::cout << "Threshold " << threshold << " - Run number "
                        << j + 1 << " completed on thread " << omp_get_thread_num()
                        << " - Final interaction count: " << ground.getInteractionCount() << std::endl;
                }
            }
        }
        auto threshold_end_time = std::chrono::high_resolution_clock::now();
        auto threshold_duration = std::chrono::duration_cast<std::chrono::milliseconds>(threshold_end_time - threshold_start_time);
        std::cout << "Threshold " << threshold << " experiments complete. Duration: "
            << threshold_duration.count() << " milliseconds" << std::endl;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "Total execution time: " << total_duration.count() << " milliseconds" << std::endl;

    file.close();
    if (enable_visual) {
        video.release();
    }
    std::cout << "Simulation complete. Data written to " << csvFilename << std::endl;
    std::cout << "Please run the Python script to generate the confidence band plot." << std::endl;

    return 0;
}
