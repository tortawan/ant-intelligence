/**
 * @file ConsoleApp_ffmpeg.cpp
 * @brief Main executable for running the ant intelligence simulation.
 *
 * This application initializes a Ground, populates it with ants and objects,
 * and runs the simulation for a specified number of iterations over multiple
 * experimental runs. It sweeps through different 'similarityThreshold' and
 * 'interactionCooldown' values to test their effect on object clustering.
 *
 * The results are logged to a CSV file for later analysis.
 * If enabled, it also generates an MP4 video of the simulation.
 */

#define NOMINMAX

#include "ant_intelligence/Ant.h"
#include "ant_intelligence/Config.h"
#include "ant_intelligence/Ground.h"
#include "ant_intelligence/Objects.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <numeric>
#include <cstdlib>
#include <memory>
#include <map>
#include <stdexcept>
#include <omp.h>
#include <sstream>
#include <cstdio> // Added for the remove() function

#ifdef _WIN32
#include <windows.h>
#endif

 // Conditionally include OpenCV headers only if this is NOT a test build.
#ifndef IS_TEST_BUILD
#include <opencv2/opencv.hpp>
#endif

 // A simple struct to hold all simulation parameters.
struct SimParameters {
    int width = AIConfig::DEFAULT_GROUND_WIDTH;
    int length = AIConfig::DEFAULT_GROUND_LENGTH;
    int num_ants = AIConfig::DEFAULT_NUM_ANTS;
    int num_experiments = AIConfig::DEFAULT_NUM_EXPERIMENTS;
    int num_iterations = AIConfig::DEFAULT_ITERATIONS;
    int memory_size = AIConfig::DEFAULT_MEMORY_SIZE;
    int threshold_start = AIConfig::DEFAULT_THRESHOLD_START;
    int threshold_end = AIConfig::DEFAULT_THRESHOLD_END;
    int threshold_interval = AIConfig::DEFAULT_THRESHOLD_INTERVAL;
    int cooldown_start = AIConfig::DEFAULT_COOLDOWN_START;
    int cooldown_end = AIConfig::DEFAULT_COOLDOWN_END;
    int cooldown_interval = AIConfig::DEFAULT_COOLDOWN_INTERVAL;
    std::vector<double> prob_relu = { AIConfig::DEFAULT_PROB_RELU[0], AIConfig::DEFAULT_PROB_RELU[1] };
    bool enable_visual = AIConfig::DEFAULT_VIDEO_ENABLED;
    std::string csv_filename = "ground_data.csv";
};

// Function to parse command-line arguments into the parameters struct.
void parse_arguments(int argc, char* argv[], SimParameters& params) {
    std::map<std::string, std::string> args;
    for (int i = 1; i < argc; i += 2) {
        if (i + 1 < argc) {
            args[argv[i]] = argv[i + 1];
        }
    }

    try {
        if (args.count("--width")) params.width = std::stoi(args["--width"]);
        if (args.count("--length")) params.length = std::stoi(args["--length"]);
        if (args.count("--ants")) params.num_ants = std::stoi(args["--ants"]);
        if (args.count("--experiments")) params.num_experiments = std::stoi(args["--experiments"]);
        if (args.count("--iterations")) params.num_iterations = std::stoi(args["--iterations"]);
        if (args.count("--memory_size")) params.memory_size = std::stoi(args["--memory_size"]);
        if (args.count("--threshold_start")) params.threshold_start = std::stoi(args["--threshold_start"]);
        if (args.count("--threshold_end")) params.threshold_end = std::stoi(args["--threshold_end"]);
        if (args.count("--threshold_interval")) params.threshold_interval = std::stoi(args["--threshold_interval"]);
        if (args.count("--cooldown_start")) params.cooldown_start = std::stoi(args["--cooldown_start"]);
        if (args.count("--cooldown_end")) params.cooldown_end = std::stoi(args["--cooldown_end"]);
        if (args.count("--cooldown_interval")) params.cooldown_interval = std::stoi(args["--cooldown_interval"]);
        if (args.count("--prob_relu_low")) params.prob_relu[0] = std::stod(args["--prob_relu_low"]);
        if (args.count("--prob_relu_high")) params.prob_relu[1] = std::stod(args["--prob_relu_high"]);
        if (args.count("--csv_filename")) params.csv_filename = args["--csv_filename"];
        if (args.count("--video")) {
            std::string val = args["--video"];
            params.enable_visual = (val == "true" || val == "1");
        }
    }
    catch (const std::invalid_argument& e) {
        std::cerr << "Error: Invalid argument type provided. " << e.what() << std::endl;
        exit(1);
    }
    catch (const std::out_of_range& e) {
        std::cerr << "Error: Argument value out of range. " << e.what() << std::endl;
        exit(1);
    }
}

// Function to print the simulation parameters.
void print_parameters(const SimParameters& params) {
    std::cout << "--- Simulation Parameters ---" << std::endl;
    std::cout << "  Grid Dimensions: " << params.width << "x" << params.length << std::endl;
    std::cout << "  Number of Ants: " << params.num_ants << std::endl;
    std::cout << "  Number of Experiments: " << params.num_experiments << std::endl;
    std::cout << "  Iterations per Experiment: " << params.num_iterations << std::endl;
    std::cout << "  Ant Memory Size: " << params.memory_size << std::endl;
    std::cout << "  Threshold Sweep: " << params.threshold_start << " to " << params.threshold_end
        << " (step " << params.threshold_interval << ")" << std::endl;
    std::cout << "  Cooldown Sweep: " << params.cooldown_start << " to " << params.cooldown_end
        << " (step " << params.cooldown_interval << ")" << std::endl;
    std::cout << "  Pick/Drop Probability Range: [" << params.prob_relu[0] << ", " << params.prob_relu[1] << "]" << std::endl;
    std::cout << "  Video Enabled: " << (params.enable_visual ? "Yes" : "No") << std::endl;
    std::cout << "  Output CSV: " << params.csv_filename << std::endl;
    std::cout << "-----------------------------" << std::endl;
}

// Function to write the header of the CSV data file.
void write_csv_header(std::ofstream& file) {
    file << "Cooldown,Threshold,Run,Iteration,ClusterSize,InteractionCount\n";
}


int main(int argc, char* argv[]) {
    SimParameters params;
    parse_arguments(argc, argv, params);
    print_parameters(params);

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

    // Robust File Handling Step 1: Write header and close immediately.
    std::ofstream header_writer(params.csv_filename, std::ios_base::trunc);
    if (!header_writer.is_open()) {
        std::cerr << "Error: Could not open the output file '" << params.csv_filename << "' for writing header." << std::endl;
        return -1;
    }
    write_csv_header(header_writer);
    header_writer.close();

    auto total_start_time = std::chrono::high_resolution_clock::now();
    std::cout << "\nStarting simulation with " << omp_get_max_threads() << " threads." << std::endl;

    // Main simulation loop: sweep through cooldown and threshold values
    for (int cooldown = params.cooldown_start; cooldown <= params.cooldown_end; cooldown += params.cooldown_interval) {
        for (int threshold = params.threshold_start; threshold <= params.threshold_end; threshold += params.threshold_interval) {

            std::cout << "Running experiments for Cooldown = " << cooldown << ", Threshold = " << threshold << "..." << std::endl;

            // Parallelize the experimental runs for the current parameter combination
#pragma omp parallel for schedule(dynamic, 1)
            for (int j = 0; j < params.num_experiments; ++j) {
                // Create a unique temporary filename for each experiment run.
                std::string temp_filename = "temp_data_C" + std::to_string(cooldown)
                    + "_T" + std::to_string(threshold)
                    + "_R" + std::to_string(j + 1) + ".csv";

                std::ofstream temp_file(temp_filename);

                // Initialize the simulation environment
                Ground ground(params.width, params.length, prob, params.prob_relu, threshold, cooldown);
                ground.addObject(obj_dict);
                for (int i = 0; i < params.num_ants; ++i) {
                    ground.addAnt(params.memory_size);
                }

                // === VIDEO WRITER SETUP (START) ===
#ifndef IS_TEST_BUILD
                cv::VideoWriter video;
                if (params.enable_visual) {
                    // Create a unique video filename for the experiment
                    std::string video_filename = "simulation_C" + std::to_string(cooldown)
                        + "_T" + std::to_string(threshold)
                        + "_R" + std::to_string(j + 1) + ".mp4";
                    int scale = 6; // Must match the scale used in showGround()
                    cv::Size frame_size(params.length * scale, params.width * scale);
                    // Use 'm', 'p', '4', 'v' for MP4 file format
                    video.open(video_filename, cv::VideoWriter::fourcc('m', 'p', '4', 'v'), 120, frame_size, true);
                    if (!video.isOpened()) {
#pragma omp critical
                        {
                            std::cerr << "Error: Could not open video file for writing: " << video_filename << std::endl;
                        }
                    }
                }
#endif
                // === VIDEO WRITER SETUP (END) ===


                // Run the simulation
                for (int i = 0; i < params.num_iterations; ++i) {
                    ground.moveAnts();
                    ground.assignWork();
                    ground.handleAntInteractions(i);

                    // === SHOW/SAVE FRAME (START) ===
#ifndef IS_TEST_BUILD
                    if (params.enable_visual && video.isOpened()) {
                        // The window name is provided, and the video writer is passed
                        ground.showGround("Ant Simulation", video);
                    }
#endif
                    // === SHOW/SAVE FRAME (END) ===


                    if (i % 10000 == 0) {
                        double avg_cluster_size = ground.averageClusterSize();
                        int interaction_count = ground.getInteractionCount();

                        temp_file << cooldown << "," << threshold << "," << j + 1 << "," << i << ","
                            << avg_cluster_size << "," << interaction_count << "\n";

#pragma omp critical
                        {
                            std::cout << "C: " << cooldown << ", T: " << threshold
                                << ", Exp: " << j + 1
                                << ", Iter: " << i << "/" << params.num_iterations
                                << ", Cluster: " << avg_cluster_size
                                << ", Interact: " << interaction_count << std::endl;
                        }
                    }
                }
                temp_file.close();
                // VideoWriter is automatically released by its destructor when it goes out of scope.
            }

            // Robust File Handling Step 2: Open in append mode, aggregate data, and close immediately.
            std::ofstream aggregator(params.csv_filename, std::ios_base::app);
            if (!aggregator.is_open()) {
                std::cerr << "Error: Could not open the output file '" << params.csv_filename << "' for appending results." << std::endl;
                continue; // Skip to the next loop iteration
            }

            // The aggregation loop now reconstructs the unique filenames to read and append them.
            for (int run = 1; run <= params.num_experiments; ++run) {
                std::string temp_filename = "temp_data_C" + std::to_string(cooldown)
                    + "_T" + std::to_string(threshold)
                    + "_R" + std::to_string(run) + ".csv";

                std::ifstream temp_file(temp_filename);
                if (temp_file.is_open()) {
                    if (temp_file.peek() != std::ifstream::traits_type::eof()) {
                        aggregator << temp_file.rdbuf();
                    }
                    temp_file.close();
                    remove(temp_filename.c_str());
                }
            }
            aggregator.close();
        }
    }

    auto total_end_time = std::chrono::high_resolution_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::seconds>(total_end_time - total_start_time);
    std::cout << "\nTotal execution time: " << total_duration.count() << " seconds" << std::endl;

    std::cout << "Simulation complete. Data written to " << params.csv_filename << std::endl;

    return 0;
}
