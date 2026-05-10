# OS Project 

## Group Members
- **Student 1**: Mohamad Mhesen
- **Student 2**: Omar Talgi
- **Student 3**: Nagham Idies

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

## General
- **Clean Project**: `make clean`
