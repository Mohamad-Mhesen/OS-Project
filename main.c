#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <math.h>

#if defined(MILESTONE2) || defined(MILESTONE3)
#include "raylib.h"
#include "raymath.h"
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

#ifdef MILESTONE3
typedef enum { STATE_STOPPED, STATE_PLAYING, STATE_WAITING_NODE, STATE_MOVING_EDGE, STATE_FINISHED } AnimState;
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

#if defined(MILESTONE2) || defined(MILESTONE3)
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

#if !defined(MILESTONE2) && !defined(MILESTONE3)
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
