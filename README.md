# Traffic Simulation in a Directed Graph

## Group Members
- **Student 1**: Algorithm implementation and graph logic.
- **Student 2**: GUI development and visualization.
- **Student 3**: Process management and synchronization.
- **Student 4**: IPC and scheduling.

## Project Description
This project simulates a traffic system where multiple passengers (represented as processes) move simultaneously through a directed graph. The simulation incorporates core operating system concepts such as process management, synchronization, IPC, and scheduling.

## Background Story
The graph represents a city's metro network where trains move between stations based on real-time traffic data and pathfinding algorithms.

## Compilation and Execution Commands

### Milestone 1: Graph Logic
- **Compilation**: `make milestone1`
- **Execution**: `./dijkstra <file_name>`
- **Description**: Implements Dijkstra's algorithm to find the shortest path in a weighted directed graph.

### Milestone 2: GUI Visualization
- **Compilation**: `make milestone2`
- **Execution**: `./sim <file_name>`
- **Description**: Displays the graph using the raylib library, showing nodes, directed edges, and weights.

### Milestone 3: Movement Animation
- **Compilation**: `make milestone3`
- **Execution**: `./sim <file_name>`
- **Description**: Adds animation of an entity moving along the shortest path calculated by Dijkstra. Includes Play/Stop functionality and realistic timing (waiting at nodes and jumping on edges).

### Milestone 4: Multiple Processes
- **Compilation**: `make milestone4`
- **Execution**: `./sim <file_name>`
- **Description**: Introduces multiple travelers moving simultaneously. Each traveler is managed as a separate child process created using `fork()`. The parent process handles the GUI and route calculations.

### Milestone 5: Inter-Process Communication (IPC)
- **Compilation**: `make milestone5`
- **Execution**: `./sim <file_name>`
- **Description**: Child processes become autonomous, calculating their own routes and reporting their status to the parent using **Pipes**. The parent process updates the GUI and prints detailed logs to the terminal. Pipes were chosen for their simplicity and effectiveness in one-way communication from multiple children to a single parent.

### Milestone 6: Node Synchronization
- **Compilation**: `make milestone6`
- **Execution**: `./sim <file_name>`
- **Description**: Implements a locking mechanism using **Semaphores** (in shared memory) to ensure that no more than one traveler is inside a node at any given time. Travelers arriving at a node while it's occupied wait outside, visualized in the GUI.

### Features Added (Current Session)
- **Smooth Movement**: Entities now move smoothly between nodes using interpolation.
- **Dynamic Colors**: The first traveler is colored **Red** and the second is **Green** for better identification.
- **Start/Stop Control**: A button was added to the GUI to pause and resume the simulation at any time.

## General
- **Clean Project**: `make clean`
- **Execution for all Milestones (2-6)**: `./sim <file_name>`
