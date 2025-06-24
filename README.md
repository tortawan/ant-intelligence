# Ant Intelligence: A Swarm Simulation Project

!          [](/images/ant_working.gif)

This repository showcases **Ant Intelligence**, a sophisticated simulation project I developed to investigate emergent behavior and swarm intelligence principles. The system integrates a high-performance C++ simulation core with a user-friendly Python GUI for intuitive experiment control and comprehensive data analysis.

## Overview

At its heart, this project simulates ant-like agents navigating a two-dimensional environment. The agents follow straightforward rules, resulting in complex self-organizing behaviors, notably the clustering of diverse objects such as food, waste, and eggs. This project explores how simple localized interactions and minimal memory can yield intricate large-scale patterns, essential in the fields of Artificial Life and Swarm Intelligence.

## Project Structure

The project directory is organized as follows:

```
ant-intelligence/
├── src/
│   ├── Ant.cpp
│   ├── ConsoleApp_ffmpeg.cpp
│   └── Ground.cpp
├── include/
│   └── ant_intelligence/
│       ├── Ant.h
│       ├── Config.h
│       ├── Ground.h
│       ├── Objects.h
│       └── Utils.h
├── ConsoleApp_controller.py
├── ConsoleApp_ffmpeg.sln
├── tests/
│   └── test_ant_movement.cpp
├── ConsoleApp_ffmpeg/
│   ├── cluster_evolution_plot.png
│   └── ground_data.csv
└── AntTest/
    └── AntTest.cpp
```

## Key Features

- **High-Performance Core**: Engineered with modern C++ and parallelized using OpenMP for efficiency and scalability.
- **Advanced Agent Behavior**: Agents utilize memory, directional inertia, and interaction cooldowns, governed by probabilistic models based on their immediate surroundings.
- **Systematic Experimentation**: Parameter sweeping capabilities for variables like `similarityThreshold` and `interactionCooldown` facilitate rigorous scientific exploration.
- **Data-Driven Insights**: Outputs comprehensive data logs in CSV format for further analysis and visualization.
- **Interactive Python GUI**: Built with Tkinter and ttkbootstrap, the Python interface simplifies simulation configuration, real-time monitoring, and data visualization.
- **Robust Testing Framework**: Includes thorough unit tests to verify agent mechanics, ensuring simulation accuracy and reliability.

## Project Engineering

I personally architected and developed all project components, deliberately selecting each technology for its specific advantages:

- **C++ Core**: Chosen for performance-critical tasks, enabling efficient multi-threaded simulations.
- **Python GUI**: Selected for rapid development and user interaction, providing a seamless experience by managing the C++ core through subprocess control.

This integration highlights my proficiency in software architecture, performance programming, GUI development, and scientific software engineering.

## How the Simulation Works

- **Initialization**: Random distribution of objects (Food, Waste, Eggs) and Ant agents across the simulation grid.
- **Movement**: Agents navigate based on inertia, more likely to maintain previous directions.
- **Memory and Interaction**: Agents observe their environment, update short-term memory, and use probabilistic models to pick up or drop objects, driven by local clustering densities.
- **Agent Communication**: Ant interactions influence directional behavior and memory similarity, employing cooldown periods to regulate interactions.
- **Emergent Patterns**: Simple rules lead to the spontaneous clustering of similar objects without centralized coordination.

## Build and Run Instructions

### Prerequisites

- **Compiler**: C++17-compatible compiler with OpenMP (e.g., g++).
- **Python 3.x** with libraries:
  ```bash
  pip install ttkbootstrap pandas
  ```

### Compile C++ Core

Navigate to the project's root directory:

```bash
g++ -std=c++17 -fopenmp -O3 -Iinclude src/*.cpp -o ConsoleApp_ffmpeg
```

(You may rename the executable as desired.)

### Launch Python GUI

Start the Python controller:

```bash
python ConsoleApp_controller.py
```

Through the GUI, users can easily set simulation parameters, monitor progress, and analyze results.

---

This project is a testament to my expertise in creating robust, scalable, and user-centric scientific software solutions, reflecting both technical competence and innovative problem-solving abilities suitable for advanced roles in software development, computational biology, and artificial intelligence.

