#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#if defined(MILESTONE2) || defined(MILESTONE3) || defined(MILESTONE4) || defined(MILESTONE5) || defined(MILESTONE6) || defined(MILESTONE7)
#include "raylib.h"
#include "raymath.h"
#endif
#if defined(MILESTONE4) || defined(MILESTONE5) || defined(MILESTONE6) || defined(MILESTONE7)
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#endif

#if defined(MILESTONE5) || defined(MILESTONE6) || defined(MILESTONE7)
#include <fcntl.h>
#endif

#ifdef MILESTONE6
#include <semaphore.h>
#include <sys/mman.h>
#endif

#define INF INT_MAX

typedef struct {
    int to;
    int weight;
} Edge;

typedef struct {
    Edge* edges;
    int count;
    int capacity;
    float x, y; // Added for GUI positioning
} Node;

#if defined(MILESTONE3) || defined(MILESTONE4) || defined(MILESTONE5) || defined(MILESTONE6) || defined(MILESTONE7)
typedef enum {
    STATE_STOPPED,
    STATE_PLAYING,
    STATE_WAITING_NODE,
    STATE_MOVING_EDGE,
    STATE_FINISHED
} AnimState;

#if defined(MILESTONE4) || defined(MILESTONE5) || defined(MILESTONE6) || defined(MILESTONE7)
typedef struct {
    pid_t pid;
    int startNode;
    int endNode;

    int* path;
    int pathCount;

    int currentPathIdx;
    int currentJump;

    float timer;
    Vector2 position;
    Color color;

    AnimState state;
    int waitingAtNode; // -1 if not waiting, else node index (Milestone 6)
    Vector2 targetPosition; // Milestone 5/6 smooth movement
    int nextNodeIdx; // Milestone 5/6 smooth movement
    bool pendingFinish; // Added to handle delayed finish after smooth movement
    int remainingDistance; // Added for SJF (Milestone 7)
} Traveler;
#endif
#endif

void add_edge(Node* nodes, int src, int dst, int weight) {
    if (nodes[src].count == nodes[src].capacity) {
        nodes[src].capacity = nodes[src].capacity == 0 ? 4 : nodes[src].capacity * 2;
        nodes[src].edges = realloc(nodes[src].edges, nodes[src].capacity * sizeof(Edge));
    }
    nodes[src].edges[nodes[src].count].to = dst;
    nodes[src].edges[nodes[src].count].weight = weight;
    nodes[src].count++;
}

void print_path(int* parent, int j) {
    if (parent[j] == -1) {
        printf("%d", j);
        return;
    }
    print_path(parent, parent[j]);
    printf(" -> %d", j);
}
#if defined(MILESTONE2) || defined(MILESTONE3) || defined(MILESTONE4) || defined(MILESTONE5) || defined(MILESTONE6) || defined(MILESTONE7)
void DrawArrow(Vector2 start, Vector2 end, int weight) {
    float angle = atan2f(end.y - start.y, end.x - start.x);
    float arrowSize = 15.0f;
    float nodeRadius = 25.0f;

    // Shorten the line so it starts and ends at the edge of the node circles
    Vector2 lineStart = {
        start.x + cosf(angle) * nodeRadius,
        start.y + sinf(angle) * nodeRadius
    };
    Vector2 lineEnd = {
        end.x - cosf(angle) * nodeRadius,
        end.y - sinf(angle) * nodeRadius
    };

    // Draw the main edge line
    DrawLineEx(lineStart, lineEnd, 3.0f, DARKGRAY);

    // Calculate the points for a sharper, more visible arrowhead
    // The tip of the arrow is exactly at lineEnd
    Vector2 p1 = {
        lineEnd.x - cosf(angle - 0.5f) * arrowSize,
        lineEnd.y - sinf(angle - 0.5f) * arrowSize
    };
    Vector2 p2 = {
        lineEnd.x - cosf(angle + 0.5f) * arrowSize,
        lineEnd.y - sinf(angle + 0.5f) * arrowSize
    };

    // Draw the arrowhead as a filled triangle
    DrawTriangle(lineEnd, p1, p2, MAROON);
    // Draw an outline for the triangle to make it even clearer
    DrawTriangleLines(lineEnd, p1, p2, BLACK);

    // Draw weight
    char weightText[16];
    sprintf(weightText, "%d", weight);
    Vector2 midPoint = { (lineStart.x + lineEnd.x) / 2, (lineStart.y + lineEnd.y) / 2 };

    // Offset weight text slightly to the side of the line for better readability
    midPoint.x += cosf(angle + PI/2) * 20;
    midPoint.y += sinf(angle + PI/2) * 20;

    DrawText(weightText, midPoint.x - 5, midPoint.y - 5, 18, DARKBLUE);
}
#endif


int build_path(Node* nodes, int N, int start, int end, int** outPath) {
    int* dist = malloc(N * sizeof(int));
    int* parent = malloc(N * sizeof(int));
    bool* visited = calloc(N, sizeof(bool));

    for (int i = 0; i < N; i++) {
        dist[i] = INF;
        parent[i] = -1;
    }

    dist[start] = 0;

    for (int count = 0; count < N - 1; count++) {
        int u = -1;
        int min_dist = INF;

        for (int v = 0; v < N; v++) {
            if (!visited[v] && dist[v] <= min_dist) {
                min_dist = dist[v];
                u = v;
            }
        }

    if (u == -1 || dist[u] == INF) break;
    visited[u] = true;

    for (int i = 0; i < nodes[u].count; i++) {
        int v = nodes[u].edges[i].to;
        int weight = nodes[u].edges[i].weight;

        if (dist[u] != INF && dist[u] + weight < dist[v]) {
            dist[v] = dist[u] + weight;
            parent[v] = u;
        }
    }
    }

    if (dist[end] == INF) {
        free(dist);
        free(parent);
        free(visited);
        *outPath = NULL;
        return 0;
    }

    int pathCount = 0;
    int curr = end;

    while (curr != -1) {
        pathCount++;
        curr = parent[curr];
    }

    int* path = malloc(pathCount * sizeof(int));
    curr = end;

    for (int i = pathCount - 1; i >= 0; i--) {
        path[i] = curr;
        curr = parent[curr];
    }

    free(dist);
    free(parent);
    free(visited);

    *outPath = path;
    return pathCount;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file_name>\n", argv[0]);
        return 1;
    }

    FILE* fp = NULL;
#ifdef MILESTONE7
    if (argc >= 3) {
        fp = fopen(argv[2], "r");
    } else {
        fp = fopen(argv[1], "r");
    }
#else
    fp = fopen(argv[1], "r");
#endif
    if (fp == NULL) {
        perror("Error opening file");
        return 1;
    }

    int N, M;
    if (fscanf(fp, "%d %d", &N, &M) != 2) {
        fprintf(stderr, "Invalid input format\n");
        fclose(fp);
        return 1;
    }

    if (N <= 0) {
        fprintf(stderr, "Invalid number of nodes\n");
        fclose(fp);
        return 1;
    }

    Node* nodes = calloc(N, sizeof(Node));
    for (int i = 0; i < M; i++) {
        int src, dst, weight;
        if (fscanf(fp, "%d %d %d", &src, &dst, &weight) != 3) {
            fprintf(stderr, "Invalid edge format\n");
            goto cleanup;
        }
        if (weight < 0) {
            fprintf(stderr, "Negative numbers constitute invalid input\n");
            goto cleanup;
        }
        if (src < 0 || src >= N || dst < 0 || dst >= N) {
            fprintf(stderr, "Invalid node index\n");
            goto cleanup;
        }
        add_edge(nodes, src, dst, weight);
    }

#if defined(MILESTONE4) || defined(MILESTONE5) || defined(MILESTONE6) || defined(MILESTONE7)
    int travelerCount;
    if (fscanf(fp, "%d", &travelerCount) != 1) {
        fprintf(stderr, "Invalid travelers count\n");
        goto cleanup;
    }

    int* startNodes = malloc(travelerCount * sizeof(int));
    int* endNodes = malloc(travelerCount * sizeof(int));

    for (int i = 0; i < travelerCount; i++) {
        if (fscanf(fp, "%d %d", &startNodes[i], &endNodes[i]) != 2) {
            fprintf(stderr, "Invalid traveler format\n");
            goto cleanup;
        }

        if (startNodes[i] < 0 || startNodes[i] >= N ||
            endNodes[i] < 0 || endNodes[i] >= N) {
            fprintf(stderr, "Invalid traveler node\n");
            goto cleanup;
            }
    }
#else
    int start_node, end_node;
    if (fscanf(fp, "%d %d", &start_node, &end_node) != 2) {
        fprintf(stderr, "Invalid start/end node format\n");
        goto cleanup;
    }

    if (start_node < 0 || start_node >= N || end_node < 0 || end_node >= N) {
        fprintf(stderr, "Invalid start or end node\n");
        goto cleanup;
    }

    if (start_node == end_node) {
        printf("%d\n0\n", start_node);
#ifndef MILESTONE2
        goto cleanup;
#endif
    }
#endif
#if !defined(MILESTONE2) && !defined(MILESTONE3) && !defined(MILESTONE4) && !defined(MILESTONE5) && !defined(MILESTONE6) && !defined(MILESTONE7)
    int* dist = malloc(N * sizeof(int));
    int* parent = malloc(N * sizeof(int));
    bool* visited = calloc(N, sizeof(bool));

    for (int i = 0; i < N; i++) {
        dist[i] = INF;
        parent[i] = -1;
    }

    dist[start_node] = 0;

    for (int count = 0; count < N - 1; count++) {
        int u = -1;
        int min_dist = INF;

        for (int v = 0; v < N; v++) {
            if (!visited[v] && dist[v] <= min_dist) {
                min_dist = dist[v];
                u = v;
            }
        }

        if (u == -1 || dist[u] == INF) break;

        visited[u] = true;

        for (int i = 0; i < nodes[u].count; i++) {
            int v = nodes[u].edges[i].to;
            int weight = nodes[u].edges[i].weight;
            if (!visited[v] && dist[u] + weight < dist[v]) {
                dist[v] = dist[u] + weight;
                parent[v] = u;
            }
        }
    }

    if (dist[end_node] == INF) {
        printf("No path found\n");
    } else {
        print_path(parent, end_node);
        printf("\n%d\n", dist[end_node]);
    }

    free(dist);
    free(parent);
    free(visited);
#elif defined(MILESTONE2)
    // GUI Implementation for Milestone 2
    const int screenWidth = 800;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "Traffic Simulation - Graph View");

    // Calculate positions in a circle
    float centerX = screenWidth / 2.0f;
    float centerY = screenHeight / 2.0f;
    float radius = 200.0f;

    for (int i = 0; i < N; i++) {
        float angle = (2 * PI * i) / N;
        nodes[i].x = centerX + cosf(angle) * radius;
        nodes[i].y = centerY + sinf(angle) * radius;
    }

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawText("Graph Visualization", 10, 10, 20, DARKGRAY);

        // Draw Edges
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < nodes[i].count; j++) {
                int to = nodes[i].edges[j].to;
                int weight = nodes[i].edges[j].weight;
                Vector2 start = { nodes[i].x, nodes[i].y };
                Vector2 end = { nodes[to].x, nodes[to].y };
                DrawArrow(start, end, weight);
            }
        }

        // Draw Nodes
        for (int i = 0; i < N; i++) {
            DrawCircle(nodes[i].x, nodes[i].y, 25, SKYBLUE);
            DrawCircleLines(nodes[i].x, nodes[i].y, 25, BLUE);
            char id[12];
            sprintf(id, "%d", i);
            DrawText(id, nodes[i].x - MeasureText(id, 20)/2, nodes[i].y - 10, 20, BLACK);
        }

        EndDrawing();
    }

    CloseWindow();
#elif defined(MILESTONE4)
    Color colors[] = { RED, BLUE, GREEN, ORANGE, PURPLE, MAROON, DARKGREEN, PINK };
    Traveler* travelers = calloc(travelerCount, sizeof(Traveler));

    const int screenWidth = 800;
    const int screenHeight = 600;

    float centerX = screenWidth / 2.0f;
    float centerY = screenHeight / 2.0f;
    float radius = 200.0f;

    for (int i = 0; i < N; i++) {
        float angle = (2 * PI * i) / N;
        nodes[i].x = centerX + cosf(angle) * radius;
        nodes[i].y = centerY + sinf(angle) * radius;
    }

    for (int i = 0; i < travelerCount; i++) {
        travelers[i].startNode = startNodes[i];
        travelers[i].endNode = endNodes[i];
        travelers[i].pathCount = build_path(nodes, N, startNodes[i], endNodes[i], &travelers[i].path);
        travelers[i].currentPathIdx = 0;
        travelers[i].currentJump = 0;
        travelers[i].timer = 0.0f;
        if (i == 0) travelers[i].color = RED;
        else if (i == 1) travelers[i].color = GREEN;
        else travelers[i].color = colors[i % 8];
        travelers[i].state = STATE_PLAYING;
        travelers[i].position = (Vector2){ nodes[startNodes[i]].x, nodes[startNodes[i]].y };

        pid_t pid = fork();

        if (pid == 0) {
#if defined(MILESTONE4)
            printf("[%d] started\n", getpid());
            fflush(stdout);
#endif
            while (1) {
                pause();
            }
            exit(0);
        } else if (pid > 0) {
            travelers[i].pid = pid;
        } else {
            perror("fork failed");
        }
    }

    InitWindow(screenWidth, screenHeight, "Milestone 4 - Multiple Travelers");
    SetTargetFPS(60);

    bool paused = false;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        Rectangle btnRect = { 10, 40, 120, 40 };
        Vector2 mousePos = GetMousePosition();
        bool hovered = CheckCollisionPointRec(mousePos, btnRect);

        bool allFinished = true;
        for (int t = 0; t < travelerCount; t++) {
            if (travelers[t].state != STATE_FINISHED) {
                allFinished = false;
                break;
            }
        }

        if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (allFinished) {
                // Restart simulation
                for (int t = 0; t < travelerCount; t++) {
                    if (travelers[t].pid > 0) {
                        kill(travelers[t].pid, SIGKILL);
                        waitpid(travelers[t].pid, NULL, 0);
                    }
                    travelers[t].currentPathIdx = 0;
                    travelers[t].currentJump = 0;
                    travelers[t].timer = 0.0f;
                    travelers[t].state = STATE_PLAYING;
                    travelers[t].position = (Vector2){ nodes[travelers[t].startNode].x, nodes[travelers[t].startNode].y };

                    pid_t pid = fork();
                    if (pid == 0) {
#ifdef MILESTONE4
                        printf("[%d] started\n", getpid());
                        fflush(stdout);
#endif
                        while (1) pause();
                        exit(0);
                    } else if (pid > 0) {
                        travelers[t].pid = pid;
                    }
                }
                paused = false;
            } else {
                paused = !paused;
            }
        }

        for (int t = 0; t < travelerCount; t++) {
            Traveler* tr = &travelers[t];

            if (tr->state != STATE_FINISHED && tr->pathCount > 1) {
                if (!paused && !allFinished && (tr->state == STATE_MOVING_EDGE || tr->state == STATE_PLAYING)) {
                    if (tr->state == STATE_PLAYING) tr->state = STATE_MOVING_EDGE;

                    int u = tr->path[tr->currentPathIdx];
                    int v = tr->path[tr->currentPathIdx + 1];

                    int weight = 1;
                    for (int i = 0; i < nodes[u].count; i++) {
                        if (nodes[u].edges[i].to == v) {
                            weight = nodes[u].edges[i].weight;
                            break;
                        }
                    }

                    tr->timer += dt;

                    if (tr->timer >= 0.3f) {
                        float overshoot = tr->timer - 0.3f;
                        tr->timer = overshoot;
                        tr->currentJump++;

                        if (tr->currentJump >= weight) {
                            tr->currentPathIdx++;
                            tr->currentJump = 0;
                            tr->timer = 0.0f; // Reset on node arrival

                            if (tr->currentPathIdx >= tr->pathCount - 1) {
                                // Set state to finished
                                tr->state = STATE_FINISHED;
                                tr->position = (Vector2){
                                    nodes[tr->path[tr->currentPathIdx]].x,
                                    nodes[tr->path[tr->currentPathIdx]].y
                                };
                            } else {
                                tr->state = STATE_WAITING_NODE;
                            }
                        }
                    }

                    if (tr->state != STATE_FINISHED) {
                        Vector2 start = { nodes[u].x, nodes[u].y };
                        Vector2 end = { nodes[v].x, nodes[v].y };

                        float jumpProgress = tr->timer / 0.3f;
                        if (jumpProgress > 1.0f) jumpProgress = 1.0f;

                        float totalProgress = ((float)tr->currentJump + jumpProgress) / weight;
                        if (totalProgress > 1.0f) totalProgress = 1.0f;

                        tr->position.x = start.x + (end.x - start.x) * totalProgress;
                        tr->position.y = start.y + (end.y - start.y) * totalProgress;

                        // إضافة تأثير القفزة (Arc effect) لكل قفزة صغيرة
                        // نستخدم sin لعمل حركة مقوسة
                        float hopHeight = sinf(jumpProgress * PI) * 20.0f;
                        float angle = atan2f(end.y - start.y, end.x - start.x);
                        // نزيح الموقع بشكل عمودي على اتجاه الحركة
                        tr->position.x -= sinf(angle) * hopHeight;
                        tr->position.y += cosf(angle) * hopHeight;
                    }
                } else if (!paused && tr->state == STATE_WAITING_NODE) {
                    tr->timer += dt;
                    tr->position = (Vector2){ nodes[tr->path[tr->currentPathIdx]].x, nodes[tr->path[tr->currentPathIdx]].y };
                    if (tr->timer >= 1.0f) {
                        tr->state = STATE_MOVING_EDGE;
                        tr->timer = 0.0f;
                        tr->currentJump = 0;
                    }
                }
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawText("Milestone 4: Multiple Processes and Parent Process", 10, 10, 20, DARKGRAY);

        // Button
        DrawRectangleRec(btnRect, hovered ? GRAY : LIGHTGRAY);
        DrawRectangleLinesEx(btnRect, 3, DARKGRAY);
        const char* btnLabel = (paused || allFinished) ? "START" : "STOP";
        int textWidth = MeasureText(btnLabel, 20);
        DrawText(btnLabel, btnRect.x + (btnRect.width - textWidth) / 2, btnRect.y + (btnRect.height - 20) / 2, 20, BLACK);

        for (int i = 0; i < N; i++) {
            for (int j = 0; j < nodes[i].count; j++) {
                DrawArrow(
                    (Vector2){ nodes[i].x, nodes[i].y },
                    (Vector2){ nodes[nodes[i].edges[j].to].x, nodes[nodes[i].edges[j].to].y },
                    nodes[i].edges[j].weight
                );
            }
        }

        for (int i = 0; i < N; i++) {
            DrawCircle(nodes[i].x, nodes[i].y, 25, SKYBLUE);
            DrawCircleLines(nodes[i].x, nodes[i].y, 25, BLUE);

            char id[12];
            sprintf(id, "%d", i);
            DrawText(id, nodes[i].x - MeasureText(id, 20) / 2, nodes[i].y - 10, 20, BLACK);
        }

        for (int t = 0; t < travelerCount; t++) {
            Color c = travelers[t].color;
            if (travelers[t].state == STATE_WAITING_NODE) c = GRAY;
            DrawCircleV(travelers[t].position, 15, c);
            DrawCircleLinesV(travelers[t].position, 15, BLACK);
            if (travelers[t].state == STATE_FINISHED) {
                DrawText("FINISHED", travelers[t].position.x - 30, travelers[t].position.y - 30, 10, travelers[t].color);
            }
        }

        EndDrawing();
    }

    CloseWindow();

    for (int i = 0; i < travelerCount; i++) {
        if (travelers[i].pid > 0) {
            if (travelers[i].state == STATE_FINISHED) {
                printf("[%d] finished\n", travelers[i].pid);
            }
            kill(travelers[i].pid, SIGTERM);
            waitpid(travelers[i].pid, NULL, 0);
            }
            free(travelers[i].path);
        }

#ifdef MILESTONE7
        for(int i=0; i<N; i++) free(nodeQueues[i]);
        free(nodeQueues);
        free(nodeQueueSizes);
        free(nodeOccupant);
#endif

        free(travelers);
    free(startNodes);
    free(endNodes);

#elif defined(MILESTONE5) || defined(MILESTONE6) || defined(MILESTONE7)
    typedef struct {
        pid_t pid;
        int travelerIndex;
        int currentNode;
        int nextNode;
        int weight; // For edge movement
        int state; // 0: moving_edge, 1: waiting_node (M6/7), 2: arrived_node, 3: finished
        int remainingDistance; // For SJF (M7)
    } IPCMessage;

    typedef enum { SCHED_FCFS, SCHED_SJF } SchedType;
    SchedType currentSched = SCHED_FCFS;
    if (argc >= 3) {
        if (strcmp(argv[1], "fcfs") == 0) currentSched = SCHED_FCFS;
        else if (strcmp(argv[1], "sjf") == 0) currentSched = SCHED_SJF;
    }
    
    // Node occupant: -1 if free, else travelerIndex
    int* nodeOccupant = malloc(N * sizeof(int));
    for(int i=0; i<N; i++) nodeOccupant[i] = -1;

    // Waiting queues for each node (array of traveler indices)
    int** nodeQueues = malloc(N * sizeof(int*));
    int* nodeQueueSizes = calloc(N, sizeof(int));
    for(int i=0; i<N; i++) nodeQueues[i] = malloc(travelerCount * sizeof(int));

    Color colors[] = { RED, BLUE, GREEN, ORANGE, PURPLE, MAROON, DARKGREEN, PINK };
    Traveler* travelers = calloc(travelerCount, sizeof(Traveler));
    int (*pipes)[2] = malloc(travelerCount * sizeof(int[2]));

#ifdef MILESTONE6
    sem_t* node_sems = mmap(NULL, N * sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    for (int i = 0; i < N; i++) {
        sem_init(&node_sems[i], 1, 1);
    }
#endif

    const int screenWidth = 800;
    const int screenHeight = 600;

    float centerX = screenWidth / 2.0f;
    float centerY = screenHeight / 2.0f;
    float radius = 200.0f;

    for (int i = 0; i < N; i++) {
        float angle = (2 * PI * i) / N;
        nodes[i].x = centerX + cosf(angle) * radius;
        nodes[i].y = centerY + sinf(angle) * radius;
    }

    for (int i = 0; i < travelerCount; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe failed");
            continue;
        }

        travelers[i].startNode = startNodes[i];
        travelers[i].endNode = endNodes[i];
        if (i == 0) travelers[i].color = RED;
        else if (i == 1) travelers[i].color = GREEN;
        else travelers[i].color = colors[i % 8];
        travelers[i].position = (Vector2){ nodes[startNodes[i]].x, nodes[startNodes[i]].y };
        travelers[i].targetPosition = travelers[i].position;
        travelers[i].state = STATE_PLAYING;
        travelers[i].waitingAtNode = -1;

        pid_t pid = fork();

        if (pid == 0) {
            close(pipes[i][0]);

            int* childPath = NULL;
            int childPathCount = build_path(nodes, N, startNodes[i], endNodes[i], &childPath);

            for (int p = 0; p < childPathCount; p++) {
                int curr = childPath[p];
                int next = (p + 1 < childPathCount) ? childPath[p + 1] : -1;

                IPCMessage msg;
                msg.pid = getpid();
                msg.travelerIndex = i;
                msg.currentNode = curr;
                msg.nextNode = next;
                msg.weight = 0;

#ifdef MILESTONE6
                // Signal waiting for node
                msg.state = 1; // Waiting
                write(pipes[i][1], &msg, sizeof(IPCMessage));
                sem_wait(&node_sems[curr]);
#endif
#ifdef MILESTONE7
                // Calculate remaining distance for SJF
                int remDist = 0;
                for (int rp = p; rp < childPathCount - 1; rp++) {
                    int c_curr = childPath[rp];
                    int c_next = childPath[rp+1];
                    for (int e = 0; e < nodes[c_curr].count; e++) {
                        if (nodes[c_curr].edges[e].to == c_next) {
                            remDist += nodes[c_curr].edges[e].weight;
                            break;
                        }
                    }
                }
                msg.remainingDistance = remDist;
                msg.state = 1; // Waiting
                write(pipes[i][1], &msg, sizeof(IPCMessage));
                
                // Wait for SIGCONT from parent
                raise(SIGSTOP);
#endif
                // Arrived at node
                msg.state = 2; // Arrived
                write(pipes[i][1], &msg, sizeof(IPCMessage));
                
                sleep(1); // Stay in node for 1 second

                if (next != -1) {
                    // Moving towards next node
                    int w = 1;
                    for (int e = 0; e < nodes[curr].count; e++) {
                        if (nodes[curr].edges[e].to == next) {
                            w = nodes[curr].edges[e].weight;
                            break;
                        }
                    }
                    msg.state = 0; // Moving Edge
                    msg.weight = w;
                    write(pipes[i][1], &msg, sizeof(IPCMessage));
#ifdef MILESTONE7
                    // Release node before moving (Milestone 7)
                    // The parent will handle waking up the next traveler
                    msg.state = 4; // Node Released
                    write(pipes[i][1], &msg, sizeof(IPCMessage));
#endif
                    for(int j=0; j<w; j++) {
                        usleep(300000); // 0.3s per jump
                    }
                } else {
                    msg.state = 3; // Finished
                    write(pipes[i][1], &msg, sizeof(IPCMessage));
#ifdef MILESTONE7
                    msg.state = 4; // Node Released
                    write(pipes[i][1], &msg, sizeof(IPCMessage));
#endif
                }
#ifdef MILESTONE6
                sem_post(&node_sems[curr]);
#endif
            }

            free(childPath);
            close(pipes[i][1]);
            exit(0);
        } else if (pid > 0) {
            travelers[i].pid = pid;
            close(pipes[i][1]);
            fcntl(pipes[i][0], F_SETFL, O_NONBLOCK);
        } else {
            perror("fork failed");
        }
    }

    InitWindow(screenWidth, screenHeight, 
#ifdef MILESTONE6
               "Milestone 6 - Node Synchronization"
#else
               "Milestone 5 - IPC"
#endif
    );
    SetTargetFPS(60);

    bool paused = false;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        
        Rectangle btnRect = { 10, 40, 120, 40 };
        Vector2 mousePos = GetMousePosition();
        bool hovered = CheckCollisionPointRec(mousePos, btnRect);

        bool allFinished = true;
        for (int i = 0; i < travelerCount; i++) {
            if (travelers[i].state != STATE_FINISHED) {
                allFinished = false;
                break;
            }
        }

        if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (allFinished) {
                // Restart simulation
                for (int i = 0; i < travelerCount; i++) {
                    if (travelers[i].pid > 0) {
                        kill(travelers[i].pid, SIGKILL);
                        waitpid(travelers[i].pid, NULL, 0);
                    }
                    // Drain old pipe data
                    IPCMessage dummy;
                    while (read(pipes[i][0], &dummy, sizeof(IPCMessage)) > 0);

                    travelers[i].position = (Vector2){ nodes[travelers[i].startNode].x, nodes[travelers[i].startNode].y };
                    travelers[i].targetPosition = travelers[i].position;
                    travelers[i].state = STATE_PLAYING;
                    travelers[i].waitingAtNode = -1;
                    travelers[i].pendingFinish = false;

                    pid_t pid = fork();
                    if (pid == 0) {
                        close(pipes[i][0]);
                        int* childPath = NULL;
                        int childPathCount = build_path(nodes, N, startNodes[i], endNodes[i], &childPath);
                        for (int p = 0; p < childPathCount; p++) {
                            int curr = childPath[p];
                            int next = (p + 1 < childPathCount) ? childPath[p + 1] : -1;
                            IPCMessage msg;
                            msg.pid = getpid();
                            msg.travelerIndex = i;
                            msg.currentNode = curr;
                            msg.nextNode = next;
                            msg.weight = 0;
#ifdef MILESTONE6
                            msg.state = 1; write(pipes[i][1], &msg, sizeof(IPCMessage));
                            sem_wait(&node_sems[curr]);
#endif
                            msg.state = 2; write(pipes[i][1], &msg, sizeof(IPCMessage));
                            sleep(1);
                            if (next != -1) {
                                int w = 1;
                                for (int e = 0; e < nodes[curr].count; e++) {
                                    if (nodes[curr].edges[e].to == next) { w = nodes[curr].edges[e].weight; break; }
                                }
                                msg.state = 0; msg.weight = w;
                                write(pipes[i][1], &msg, sizeof(IPCMessage));
                                usleep(w * 300000);
                            } else {
                                msg.state = 3; write(pipes[i][1], &msg, sizeof(IPCMessage));
                            }
#ifdef MILESTONE6
                            sem_post(&node_sems[curr]);
#endif
                        }
                        free(childPath); close(pipes[i][1]); exit(0);
                    } else if (pid > 0) {
                        travelers[i].pid = pid;
                    }
                }
                paused = false;
            } else {
                paused = !paused;
            }
        }

        for (int i = 0; i < travelerCount; i++) {
            IPCMessage msg;

            if (!paused && !allFinished) {
                while (read(pipes[i][0], &msg, sizeof(IPCMessage)) > 0) {
                    if (msg.state == 0) { // Moving Edge
                        travelers[msg.travelerIndex].state = STATE_MOVING_EDGE;
                        travelers[msg.travelerIndex].targetPosition = 
                            (Vector2){ nodes[msg.nextNode].x, nodes[msg.nextNode].y };
                        // Store weight to calculate speed
                        travelers[msg.travelerIndex].currentJump = msg.weight;
                        travelers[msg.travelerIndex].timer = 0.0f; // Track total time for this edge
                    } else if (msg.state == 1) { // Waiting (M6/7)
                        travelers[msg.travelerIndex].waitingAtNode = msg.currentNode;
                        travelers[msg.travelerIndex].state = STATE_WAITING_NODE;
                        travelers[msg.travelerIndex].position = 
                            (Vector2){ nodes[msg.currentNode].x, nodes[msg.currentNode].y };
#ifdef MILESTONE7
                        travelers[msg.travelerIndex].remainingDistance = msg.remainingDistance;
                        // Add to node queue
                        int n = msg.currentNode;
                        nodeQueues[n][nodeQueueSizes[n]++] = msg.travelerIndex;
                        
                        // If node is free, trigger scheduler
                        if (nodeOccupant[n] == -1) {
                            // Find next to wake up
                            int nextToWake = -1;
                            if (currentSched == SCHED_FCFS) {
                                nextToWake = nodeQueues[n][0];
                                // Shift queue
                                for(int q=0; q<nodeQueueSizes[n]-1; q++) nodeQueues[n][q] = nodeQueues[n][q+1];
                                nodeQueueSizes[n]--;
                            } else {
                                // SJF
                                int minIdx = 0;
                                for(int q=1; q<nodeQueueSizes[n]; q++) {
                                    if (travelers[nodeQueues[n][q]].remainingDistance < travelers[nodeQueues[n][minIdx]].remainingDistance) {
                                        minIdx = q;
                                    }
                                }
                                nextToWake = nodeQueues[n][minIdx];
                                // Remove from queue
                                for(int q=minIdx; q<nodeQueueSizes[n]-1; q++) nodeQueues[n][q] = nodeQueues[n][q+1];
                                nodeQueueSizes[n]--;
                            }
                            if (nextToWake != -1) {
                                nodeOccupant[n] = nextToWake;
                                kill(travelers[nextToWake].pid, SIGCONT);
                            }
                        }
#endif
                    } else if (msg.state == 2) { // Arrived
                        travelers[msg.travelerIndex].waitingAtNode = -1;
                        travelers[msg.travelerIndex].state = STATE_PLAYING;
                        travelers[msg.travelerIndex].position =
                            (Vector2){ nodes[msg.currentNode].x, nodes[msg.currentNode].y };
                        travelers[msg.travelerIndex].targetPosition = travelers[msg.travelerIndex].position;
                        travelers[msg.travelerIndex].timer = 0.0f;
                        
                        if (msg.nextNode != -1) {
                            printf("[PID=%d] arrived at node %d | next node: %d\n",
                                   msg.pid, msg.currentNode, msg.nextNode);
                        } else {
                            printf("[PID=%d] arrived at node %d | DESTINATION\n", msg.pid, msg.currentNode);
                        }
                        fflush(stdout);
                    } else if (msg.state == 3) { // Finished
                        travelers[msg.travelerIndex].pendingFinish = true;
                    }
#ifdef MILESTONE7
                    else if (msg.state == 4) { // Node Released
                        int n = msg.currentNode;
                        nodeOccupant[n] = -1;
                        if (nodeQueueSizes[n] > 0) {
                            int nextToWake = -1;
                            if (currentSched == SCHED_FCFS) {
                                nextToWake = nodeQueues[n][0];
                                for(int q=0; q<nodeQueueSizes[n]-1; q++) nodeQueues[n][q] = nodeQueues[n][q+1];
                                nodeQueueSizes[n]--;
                            } else {
                                // SJF
                                int minIdx = 0;
                                for(int q=1; q<nodeQueueSizes[n]; q++) {
                                    if (travelers[nodeQueues[n][q]].remainingDistance < travelers[nodeQueues[n][minIdx]].remainingDistance) {
                                        minIdx = q;
                                    }
                                }
                                nextToWake = nodeQueues[n][minIdx];
                                for(int q=minIdx; q<nodeQueueSizes[n]-1; q++) nodeQueues[n][q] = nodeQueues[n][q+1];
                                nodeQueueSizes[n]--;
                            }
                            if (nextToWake != -1) {
                                nodeOccupant[n] = nextToWake;
                                kill(travelers[nextToWake].pid, SIGCONT);
                            }
                        }
                    }
#endif
                }

                // Smooth position update
                if (travelers[i].state != STATE_FINISHED && travelers[i].state != STATE_WAITING_NODE) {
                    float dist = Vector2Distance(travelers[i].position, travelers[i].targetPosition);
                    if (dist > 1.0f) {
                        float speed = 100.0f;
                        float totalWeight = 1.0f;
                        if (travelers[i].state == STATE_MOVING_EDGE && travelers[i].currentJump > 0) {
                            totalWeight = (float)travelers[i].currentJump;
                            // Remaining distance / Remaining time
                            float totalTime = totalWeight * 0.3f;
                            float remainingTime = totalTime - travelers[i].timer;
                            if (remainingTime < 0.01f) remainingTime = 0.01f;
                            speed = dist / remainingTime;
                        }
                        travelers[i].timer += dt;
                        float step = speed * dt;
                        if (step > dist) {
                            step = dist;
                        }
                        Vector2 dir = Vector2Normalize(Vector2Subtract(travelers[i].targetPosition, travelers[i].position));
                        Vector2 nextPos = Vector2Add(travelers[i].position, Vector2Scale(dir, step));
                        travelers[i].position = nextPos;
                    } else {
                        // Close enough, snap to target
                        travelers[i].position = travelers[i].targetPosition;
                        if (travelers[i].pendingFinish) {
                            travelers[i].state = STATE_FINISHED;
                            printf("[PID=%d] finished\n", travelers[i].pid);
                            fflush(stdout);
                        }
                    }
                }
            }

        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawText(
#ifdef MILESTONE7
            TextFormat("Scheduler: %s", currentSched == SCHED_FCFS ? "FCFS" : "SJF"),
#elif defined(MILESTONE6)
            "Milestone 6: Node Synchronization (1 traveler per node)",
#else
            "Milestone 5: IPC between processes",
#endif
            10, 10, 20, DARKGRAY);
#ifdef MILESTONE7
        DrawText("Milestone 7: Advanced Scheduling", 10, 40, 20, DARKGRAY);
#endif

        // Button
        DrawRectangleRec(btnRect, hovered ? GRAY : LIGHTGRAY);
        DrawRectangleLinesEx(btnRect, 3, DARKGRAY);
        const char* btnLabel = (paused || allFinished) ? "START" : "STOP";
        int textWidth = MeasureText(btnLabel, 20);
        DrawText(btnLabel, btnRect.x + (btnRect.width - textWidth) / 2, btnRect.y + (btnRect.height - 20) / 2, 20, BLACK);

        for (int i = 0; i < N; i++) {
            for (int j = 0; j < nodes[i].count; j++) {
                DrawArrow(
                    (Vector2){ nodes[i].x, nodes[i].y },
                    (Vector2){ nodes[nodes[i].edges[j].to].x, nodes[nodes[i].edges[j].to].y },
                    nodes[i].edges[j].weight
                );
            }
        }

        for (int i = 0; i < N; i++) {
            DrawCircle(nodes[i].x, nodes[i].y, 25, SKYBLUE);
            DrawCircleLines(nodes[i].x, nodes[i].y, 25, BLUE);

            char id[12];
            sprintf(id, "%d", i);
            DrawText(id, nodes[i].x - MeasureText(id, 20) / 2, nodes[i].y - 10, 20, BLACK);
        }

        for (int i = 0; i < travelerCount; i++) {
            if (travelers[i].state == STATE_FINISHED) continue;
            
            Vector2 pos = travelers[i].position;
            Color col = travelers[i].color;

            // تطبيق تأثير القفزة بصرياً فقط عند الرسم لضمان عدم تأثر الحسابات المنطقية
            if (travelers[i].state == STATE_MOVING_EDGE) {
                // محاكاة التقدم في القفزة بناءً على المسافة المتبقية للهدف
                float distToTarget = Vector2Distance(pos, travelers[i].targetPosition);
                // تقدير طول القفزة الواحدة (الوزن الكلي هو travelers[i].currentJump)
                // إذا كان الوزن 1، فإن المسافة الكلية هي المسافة بين العقدتين
                // سنستخدم جيب الزاوية لعمل قوس
                float jumpProgress = fmodf(distToTarget, 50.0f) / 50.0f; // قفزة كل 50 بكسل تقريباً
                float hopHeight = sinf(jumpProgress * PI) * 15.0f;
                
                // حساب زاوية الحركة للإزاحة العمودية
                Vector2 diff = Vector2Subtract(travelers[i].targetPosition, pos);
                float angle = atan2f(diff.y, diff.x);
                pos.x -= sinf(angle) * hopHeight;
                pos.y += cosf(angle) * hopHeight;
            }

            if (travelers[i].state == STATE_WAITING_NODE) {
                int nodeIdx = travelers[i].waitingAtNode;
                // Offset position to show waiting outside
                pos.x = nodes[nodeIdx].x + 35;
                pos.y = nodes[nodeIdx].y + 35;
                // Draw with a different color or outline to indicate waiting
                DrawCircleV(pos, 12, col);
                DrawCircleLinesV(pos, 12, RED);
                DrawText("WAIT", pos.x - 12, pos.y - 5, 10, MAROON);
            } else if (travelers[i].state == STATE_MOVING_EDGE) {
                // Moving between nodes
                DrawCircleV(pos, 15, col);
                DrawCircleLinesV(pos, 15, BLACK);
            } else {
                // Staying in node
                DrawCircleV(pos, 15, col);
                DrawCircleLinesV(pos, 15, BLACK);
            }
        }

        EndDrawing();
    }

    CloseWindow();

    for (int i = 0; i < travelerCount; i++) {
        close(pipes[i][0]);
        waitpid(travelers[i].pid, NULL, 0);
    }

#ifdef MILESTONE6
    for (int i = 0; i < N; i++) {
        sem_destroy(&node_sems[i]);
    }
    munmap(node_sems, N * sizeof(sem_t));
#endif

    free(pipes);
    free(travelers);
    free(startNodes);
    free(endNodes);


#else
    // GUI Implementation for Milestone 3
    int* dist = malloc(N * sizeof(int));
    int* parent = malloc(N * sizeof(int));
    bool* visited = calloc(N, sizeof(bool));
    for (int i = 0; i < N; i++) { dist[i] = INF; parent[i] = -1; }
    dist[start_node] = 0;
    for (int count = 0; count < N - 1; count++) {
        int u = -1; int min_dist = INF;
        for (int v = 0; v < N; v++) if (!visited[v] && dist[v] <= min_dist) { min_dist = dist[v]; u = v; }
        if (u == -1 || dist[u] == INF) break;
        visited[u] = true;
        for (int i = 0; i < nodes[u].count; i++) {
            int v = nodes[u].edges[i].to; int w = nodes[u].edges[i].weight;
            if (!visited[v] && dist[u] + w < dist[v]) { dist[v] = dist[u] + w; parent[v] = u; }
        }
    }

    int* path = NULL;
    int pathCount = 0;
    if (dist[end_node] != INF) {
        int curr = end_node;
        while (curr != -1) { pathCount++; curr = parent[curr]; }
        path = malloc(pathCount * sizeof(int));
        curr = end_node;
        for (int i = pathCount - 1; i >= 0; i--) { path[i] = curr; curr = parent[curr]; }
    }

    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "Traffic Simulation - Animation");

    float centerX = screenWidth / 2.0f;
    float centerY = screenHeight / 2.0f;
    float radius = 200.0f;
    for (int i = 0; i < N; i++) {
        float angle = (2 * PI * i) / N;
        nodes[i].x = centerX + cosf(angle) * radius;
        nodes[i].y = centerY + sinf(angle) * radius;
    }

    AnimState state = STATE_STOPPED;
    int currentPathIdx = 0; // Index in 'path' array
    int currentJump = 0;
    float timer = 0.0f;
    Vector2 entityPos = { nodes[start_node].x, nodes[start_node].y };

    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // Logic
        Rectangle btnRect = { 10, 40, 100, 30 };
        if (CheckCollisionPointRec(GetMousePosition(), btnRect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (state == STATE_STOPPED) {
                state = STATE_PLAYING;
            } else if (state == STATE_FINISHED) {
                state = STATE_PLAYING;
                currentPathIdx = 0;
                currentJump = 0;
                timer = 0.0f;
                entityPos = (Vector2){ nodes[start_node].x, nodes[start_node].y };
            } else {
                state = STATE_STOPPED;
            }
        }

        if (state == STATE_PLAYING) {
            if (pathCount <= 1) {
                state = STATE_FINISHED;
            } else if (currentPathIdx < pathCount - 1) {
                state = STATE_MOVING_EDGE;
            } else {
                state = STATE_FINISHED;
            }
        }

        if (state == STATE_MOVING_EDGE) {
            timer += dt;
            int u = path[currentPathIdx];
            int v = path[currentPathIdx + 1];
            int weight = 0;
            for(int i=0; i<nodes[u].count; i++) if(nodes[u].edges[i].to == v) { weight = nodes[u].edges[i].weight; break; }

                    if (timer >= 0.3f) {
                        float overshoot = timer - 0.3f;
                        timer = overshoot;
                        currentJump++;
                        if (currentJump >= weight) {
                            currentPathIdx++;
                            currentJump = 0;
                            timer = 0.0f;
                            if (currentPathIdx >= pathCount - 1) {
                                state = STATE_FINISHED;
                                entityPos = (Vector2){ nodes[path[currentPathIdx]].x, nodes[path[currentPathIdx]].y };
                            }
                            else { state = STATE_WAITING_NODE; }
                        }
                    }

            // Smooth Jump-based Interpolation
            if (state == STATE_MOVING_EDGE) {
                Vector2 start = { nodes[u].x, nodes[u].y };
                Vector2 end = { nodes[v].x, nodes[v].y };

                // Calculate progress: completed jumps + current jump progress
                float jumpProgress = timer / 0.3f;
                if (jumpProgress > 1.0f) jumpProgress = 1.0f;

                float totalProgress = ((float)currentJump + jumpProgress) / weight;
                if (totalProgress > 1.0f) totalProgress = 1.0f;

                entityPos.x = start.x + (end.x - start.x) * totalProgress;
                entityPos.y = start.y + (end.y - start.y) * totalProgress;

                // إضافة تأثير القفزة (Arc effect)
                float hopHeight = sinf(jumpProgress * PI) * 20.0f;
                float angle = atan2f(end.y - start.y, end.x - start.x);
                entityPos.x -= sinf(angle) * hopHeight;
                entityPos.y += cosf(angle) * hopHeight;
            }
        } else if (state == STATE_WAITING_NODE) {
            timer += dt;
            entityPos = (Vector2){ nodes[path[currentPathIdx]].x, nodes[path[currentPathIdx]].y };
            if (timer >= 1.0f) {
                state = STATE_MOVING_EDGE;
                timer = 0.0f;
                currentJump = 0;
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("Milestone 3: Animation", 10, 10, 20, DARKGRAY);

        // Button
        DrawRectangleRec(btnRect, LIGHTGRAY);
        DrawRectangleLinesEx(btnRect, 2, GRAY);
        const char* btnText = (state == STATE_STOPPED || state == STATE_FINISHED) ? "PLAY" : "STOP";
        DrawText(btnText, btnRect.x + 25, btnRect.y + 5, 20, BLACK);

        for (int i = 0; i < N; i++) {
            for (int j = 0; j < nodes[i].count; j++) {
                DrawArrow((Vector2){nodes[i].x, nodes[i].y}, (Vector2){nodes[nodes[i].edges[j].to].x, nodes[nodes[i].edges[j].to].y}, nodes[i].edges[j].weight);
            }
        }
        for (int i = 0; i < N; i++) {
            DrawCircle(nodes[i].x, nodes[i].y, 25, SKYBLUE);
            DrawCircleLines(nodes[i].x, nodes[i].y, 25, BLUE);
            char id[12]; sprintf(id, "%d", i);
            DrawText(id, nodes[i].x - MeasureText(id, 20)/2, nodes[i].y - 10, 20, BLACK);
        }

        if (state != STATE_STOPPED || (state == STATE_STOPPED && currentPathIdx < pathCount)) {
            DrawCircleV(entityPos, 15, RED);
            DrawCircleLinesV(entityPos, 15, MAROON);
        }

        if (state == STATE_FINISHED) {
            DrawText("Destination Reached!", screenWidth/2 - 100, 50, 20, GREEN);
        } else if (dist[end_node] == INF) {
             DrawText("No Path Found", screenWidth/2 - 60, 50, 20, RED);
        }

        EndDrawing();
    }
    CloseWindow();
    free(dist); free(parent); free(visited); if(path) free(path);
#endif

cleanup:
    for (int i = 0; i < N; i++) {
        if (nodes[i].edges) free(nodes[i].edges);
    }
    free(nodes);
    fclose(fp);
    return 0;
}
