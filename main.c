#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <math.h>

#if defined(MILESTONE2) || defined(MILESTONE3) || defined(MILESTONE4) || defined(MILESTONE5)
#include "raylib.h"
#include "raymath.h"
#endif
#if defined(MILESTONE4) || defined(MILESTONE5)
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#endif

#ifdef MILESTONE5
#include <fcntl.h>
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

#if defined(MILESTONE3) || defined(MILESTONE4) || defined(MILESTONE5)
typedef enum {
    STATE_STOPPED,
    STATE_PLAYING,
    STATE_WAITING_NODE,
    STATE_MOVING_EDGE,
    STATE_FINISHED
} AnimState;

#if defined(MILESTONE4) || defined(MILESTONE5)typedef struct {

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
#if defined(MILESTONE2) || defined(MILESTONE3) || defined(MILESTONE4)
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

            if (!visited[v] && dist[u] != INF && dist[u] + weight < dist[v]) {
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

    FILE* fp = fopen(argv[1], "r");
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

#if defined(MILESTONE4) || defined(MILESTONE5)
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
#if !defined(MILESTONE2) && !defined(MILESTONE3) && !defined(MILESTONE4) && !defined(MILESTONE5)
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
        travelers[i].color = colors[i % 8];
        travelers[i].state = STATE_PLAYING;
        travelers[i].position = (Vector2){ nodes[startNodes[i]].x, nodes[startNodes[i]].y };

        pid_t pid = fork();

        if (pid == 0) {
            printf("[%d] started\n", getpid());
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

    bool allFinished = false;

    while (!WindowShouldClose() && !allFinished) {
        float dt = GetFrameTime();
        allFinished = true;

        for (int t = 0; t < travelerCount; t++) {
            Traveler* tr = &travelers[t];

            if (tr->state != STATE_FINISHED && tr->pathCount > 1) {
                allFinished = false;

                if (tr->currentPathIdx < tr->pathCount - 1) {
                    tr->state = STATE_MOVING_EDGE;

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
                        tr->timer -= 0.3f;
                        tr->currentJump++;

                        if (tr->currentJump >= weight) {
                            tr->currentPathIdx++;
                            tr->currentJump = 0;
                            tr->timer = 0.0f;

                            if (tr->currentPathIdx >= tr->pathCount - 1) {
                                tr->state = STATE_FINISHED;
                                tr->position = (Vector2){
                                    nodes[tr->path[tr->currentPathIdx]].x,
                                    nodes[tr->path[tr->currentPathIdx]].y
                                };

                                kill(tr->pid, SIGTERM);
                                printf("[%d] finished\n", tr->pid);
                            }
                        }
                    }

                    if (tr->state != STATE_FINISHED) {
                        Vector2 start = { nodes[u].x, nodes[u].y };
                        Vector2 end = { nodes[v].x, nodes[v].y };

                        float jumpProgress = tr->timer / 0.3f;
                        if (jumpProgress > 1.0f) jumpProgress = 1.0f;

                        float totalProgress = ((float)tr->currentJump + jumpProgress) / weight;

                        tr->position.x = start.x + (end.x - start.x) * totalProgress;
                        tr->position.y = start.y + (end.y - start.y) * totalProgress;
                    }
                }
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawText("Milestone 4: Multiple Processes and Parent Process", 10, 10, 20, DARKGRAY);

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
            DrawCircleV(travelers[t].position, 15, travelers[t].color);
            DrawCircleLinesV(travelers[t].position, 15, BLACK);
        }

        EndDrawing();
    }

    CloseWindow();

    for (int i = 0; i < travelerCount; i++) {
        if (travelers[i].pid > 0) {
            kill(travelers[i].pid, SIGTERM);
            waitpid(travelers[i].pid, NULL, 0);
        }
        free(travelers[i].path);
    }

    free(travelers);
    free(startNodes);
    free(endNodes);

#elif defined(MILESTONE5)
        typedef struct {
        pid_t pid;
        int travelerIndex;
        int currentNode;
        int nextNode;
        int finished;
    } IPCMessage;

    Color colors[] = { RED, BLUE, GREEN, ORANGE, PURPLE, MAROON, DARKGREEN, PINK };
    Traveler* travelers = calloc(travelerCount, sizeof(Traveler));
    int (*pipes)[2] = malloc(travelerCount * sizeof(int[2]));

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
        travelers[i].color = colors[i % 8];
        travelers[i].position = (Vector2){ nodes[startNodes[i]].x, nodes[startNodes[i]].y };
        travelers[i].state = STATE_PLAYING;

        pid_t pid = fork();

        if (pid == 0) {
            close(pipes[i][0]);

            int* childPath = NULL;
            int childPathCount = build_path(nodes, N, startNodes[i], endNodes[i], &childPath);

            for (int p = 0; p < childPathCount; p++) {
                IPCMessage msg;
                msg.pid = getpid();
                msg.travelerIndex = i;
                msg.currentNode = childPath[p];
                msg.nextNode = (p + 1 < childPathCount) ? childPath[p + 1] : -1;
                msg.finished = (p == childPathCount - 1);

                write(pipes[i][1], &msg, sizeof(IPCMessage));
                sleep(1);
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

    InitWindow(screenWidth, screenHeight, "Milestone 5 - IPC");
    SetTargetFPS(60);

    bool allFinished = false;

    while (!WindowShouldClose() && !allFinished) {
        allFinished = true;

        for (int i = 0; i < travelerCount; i++) {
            IPCMessage msg;

            while (read(pipes[i][0], &msg, sizeof(IPCMessage)) > 0) {
                travelers[msg.travelerIndex].position =
                    (Vector2){ nodes[msg.currentNode].x, nodes[msg.currentNode].y };

                if (msg.finished) {
                    travelers[msg.travelerIndex].state = STATE_FINISHED;
                    printf("[PID=%d] arrived at node %d | DESTINATION\n", msg.pid, msg.currentNode);
                    printf("[PID=%d] finished\n", msg.pid);
                } else {
                    printf("[PID=%d] arrived at node %d | next node: %d\n",
                           msg.pid, msg.currentNode, msg.nextNode);
                }
            }

            if (travelers[i].state != STATE_FINISHED) {
                allFinished = false;
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawText("Milestone 5: IPC between processes", 10, 10, 20, DARKGRAY);

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
            DrawCircleV(travelers[i].position, 15, travelers[i].color);
            DrawCircleLinesV(travelers[i].position, 15, BLACK);
        }

        EndDrawing();
    }

    CloseWindow();

    for (int i = 0; i < travelerCount; i++) {
        close(pipes[i][0]);
        waitpid(travelers[i].pid, NULL, 0);
    }

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
                timer -= 0.3f;
                currentJump++;
                if (currentJump >= weight) {
                    currentPathIdx++;
                    if (currentPathIdx == pathCount - 1) {
                        state = STATE_FINISHED;
                        entityPos = (Vector2){ nodes[path[currentPathIdx]].x, nodes[path[currentPathIdx]].y };
                    }
                    else { state = STATE_WAITING_NODE; timer = 0.0f; }
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

                entityPos.x = start.x + (end.x - start.x) * totalProgress;
                entityPos.y = start.y + (end.y - start.y) * totalProgress;

                // Add a "hop" effect during the jump timer
                float hopHeight = sinf(jumpProgress * PI) * 30.0f;
                entityPos.y -= hopHeight;
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
