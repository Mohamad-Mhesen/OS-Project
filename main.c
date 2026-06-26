#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>

#include "raylib.h"
#include "raymath.h"

#define INF INT_MAX
#define MOVE_TIME_PER_WEIGHT 0.30f

/* ---------- Graph ---------- */
typedef struct {
    int to;
    int weight;
} Edge;

typedef struct {
    Edge *edges;
    int count;
    int capacity;
    float x, y;
} Node;

void add_edge(Node *nodes, int src, int dst, int weight) {
    if (nodes[src].count == nodes[src].capacity) {
        int newCap = nodes[src].capacity == 0 ? 4 : nodes[src].capacity * 2;
        Edge *tmp = realloc(nodes[src].edges, newCap * sizeof(Edge));
        if (!tmp) {
            perror("realloc");
            exit(1);
        }
        nodes[src].edges = tmp;
        nodes[src].capacity = newCap;
    }
    nodes[src].edges[nodes[src].count].to = dst;
    nodes[src].edges[nodes[src].count].weight = weight;
    nodes[src].count++;
}

int get_edge_weight(Node *nodes, int u, int v) {
    for (int i = 0; i < nodes[u].count; i++) {
        if (nodes[u].edges[i].to == v) return nodes[u].edges[i].weight;
    }
    return 1;
}

/* Dijkstra: returns number of nodes in path, and stores malloc'ed path in outPath */
int build_path(Node *nodes, int N, int start, int end, int **outPath) {
    int *dist = malloc(N * sizeof(int));
    int *parent = malloc(N * sizeof(int));
    bool *visited = calloc(N, sizeof(bool));

    if (!dist || !parent || !visited) {
        perror("malloc");
        exit(1);
    }

    for (int i = 0; i < N; i++) {
        dist[i] = INF;
        parent[i] = -1;
    }
    dist[start] = 0;

    for (int c = 0; c < N - 1; c++) {
        int u = -1;
        int best = INF;

        for (int i = 0; i < N; i++) {
            if (!visited[i] && dist[i] < best) {
                best = dist[i];
                u = i;
            }
        }

        if (u == -1) break;
        visited[u] = true;

        for (int i = 0; i < nodes[u].count; i++) {
            int v = nodes[u].edges[i].to;
            int w = nodes[u].edges[i].weight;
            if (!visited[v] && dist[u] != INF && dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                parent[v] = u;
            }
        }
    }

    if (dist[end] == INF) {
        *outPath = NULL;
        free(dist);
        free(parent);
        free(visited);
        return 0;
    }

    int count = 0;
    for (int cur = end; cur != -1; cur = parent[cur]) count++;

    int *path = malloc(count * sizeof(int));
    if (!path) {
        perror("malloc path");
        exit(1);
    }

    int cur = end;
    for (int i = count - 1; i >= 0; i--) {
        path[i] = cur;
        cur = parent[cur];
    }

    *outPath = path;
    free(dist);
    free(parent);
    free(visited);
    return count;
}

/* ---------- IPC ---------- */
typedef enum {
    MSG_MOVE = 1,
    MSG_ARRIVED = 2,
    MSG_FINISHED = 3,
    MSG_NO_PATH = 4
} MessageType;

typedef struct {
    int travelerIndex;
    pid_t pid;
    MessageType type;
    int currentNode;
    int nextNode;
    int weight;
} IPCMessage;

typedef enum {
    STATE_WAITING,
    STATE_MOVING,
    STATE_FINISHED,
    STATE_NO_PATH
} TravelerState;

typedef struct {
    pid_t pid;
    int startNode;
    int endNode;
    int currentNode;
    int nextNode;
    int edgeWeight;
    float timer;
    Vector2 position;
    Vector2 fromPosition;
    Vector2 targetPosition;
    Color color;
    TravelerState state;
} Traveler;

void write_msg_or_exit(int fd, IPCMessage msg) {
    ssize_t n = write(fd, &msg, sizeof(msg));
    if (n != (ssize_t)sizeof(msg)) {
        perror("write pipe");
        exit(1);
    }
}

void traveler_child_process(int index, int writeFd, Node *nodes, int N, int start, int end) {
    close(STDIN_FILENO);

    int *path = NULL;
    int pathCount = build_path(nodes, N, start, end, &path);

    IPCMessage msg;
    msg.travelerIndex = index;
    msg.pid = getpid();
    msg.currentNode = start;
    msg.nextNode = -1;
    msg.weight = 0;

    if (pathCount == 0) {
        msg.type = MSG_NO_PATH;
        write_msg_or_exit(writeFd, msg);
        close(writeFd);
        exit(0);
    }

    msg.type = MSG_ARRIVED;
    msg.currentNode = path[0];
    msg.nextNode = pathCount > 1 ? path[1] : -1;
    write_msg_or_exit(writeFd, msg);

    for (int i = 0; i < pathCount - 1; i++) {
        int u = path[i];
        int v = path[i + 1];
        int w = get_edge_weight(nodes, u, v);

        msg.type = MSG_MOVE;
        msg.currentNode = u;
        msg.nextNode = v;
        msg.weight = w;
        write_msg_or_exit(writeFd, msg);

        usleep((useconds_t)(w * MOVE_TIME_PER_WEIGHT * 1000000.0f));

        msg.type = MSG_ARRIVED;
        msg.currentNode = v;
        msg.nextNode = (i + 2 < pathCount) ? path[i + 2] : -1;
        msg.weight = 0;
        write_msg_or_exit(writeFd, msg);
    }

    msg.type = MSG_FINISHED;
    msg.currentNode = end;
    msg.nextNode = -1;
    msg.weight = 0;
    write_msg_or_exit(writeFd, msg);

    free(path);
    close(writeFd);
    exit(0);
}

/* ---------- Drawing ---------- */
void DrawArrow(Vector2 start, Vector2 end, int weight) {
    float angle = atan2f(end.y - start.y, end.x - start.x);
    float nodeRadius = 25.0f;
    float arrowSize = 14.0f;

    Vector2 a = { start.x + cosf(angle) * nodeRadius, start.y + sinf(angle) * nodeRadius };
    Vector2 b = { end.x - cosf(angle) * nodeRadius, end.y - sinf(angle) * nodeRadius };

    DrawLineEx(a, b, 3.0f, DARKGRAY);

    Vector2 p1 = { b.x - cosf(angle - 0.5f) * arrowSize, b.y - sinf(angle - 0.5f) * arrowSize };
    Vector2 p2 = { b.x - cosf(angle + 0.5f) * arrowSize, b.y - sinf(angle + 0.5f) * arrowSize };
    DrawTriangle(b, p1, p2, MAROON);
    DrawTriangleLines(b, p1, p2, BLACK);

    char txt[16];
    snprintf(txt, sizeof(txt), "%d", weight);
    Vector2 mid = { (a.x + b.x) / 2.0f, (a.y + b.y) / 2.0f };
    mid.x += cosf(angle + PI / 2.0f) * 18.0f;
    mid.y += sinf(angle + PI / 2.0f) * 18.0f;
    DrawText(txt, (int)mid.x, (int)mid.y, 18, DARKBLUE);
}

void update_traveler_from_msg(Traveler *travelers, Node *nodes, IPCMessage msg) {
    Traveler *t = &travelers[msg.travelerIndex];

    if (msg.type == MSG_MOVE) {
        t->state = STATE_MOVING;
        t->currentNode = msg.currentNode;
        t->nextNode = msg.nextNode;
        t->edgeWeight = msg.weight;
        t->timer = 0.0f;
        t->fromPosition = (Vector2){ nodes[msg.currentNode].x, nodes[msg.currentNode].y };
        t->targetPosition = (Vector2){ nodes[msg.nextNode].x, nodes[msg.nextNode].y };
        t->position = t->fromPosition;
        printf("[PID=%d] traveler %d moves %d -> %d, weight=%d\n",
               msg.pid, msg.travelerIndex, msg.currentNode, msg.nextNode, msg.weight);
    } else if (msg.type == MSG_ARRIVED) {
        t->state = STATE_WAITING;
        t->currentNode = msg.currentNode;
        t->nextNode = msg.nextNode;
        t->position = (Vector2){ nodes[msg.currentNode].x, nodes[msg.currentNode].y };
        t->fromPosition = t->position;
        t->targetPosition = t->position;
        if (msg.nextNode >= 0) {
            printf("[PID=%d] traveler %d arrived at node %d, next=%d\n",
                   msg.pid, msg.travelerIndex, msg.currentNode, msg.nextNode);
        } else {
            printf("[PID=%d] traveler %d arrived at destination node %d\n",
                   msg.pid, msg.travelerIndex, msg.currentNode);
        }
    } else if (msg.type == MSG_FINISHED) {
        t->state = STATE_FINISHED;
        t->currentNode = msg.currentNode;
        t->position = (Vector2){ nodes[msg.currentNode].x, nodes[msg.currentNode].y };
        printf("[PID=%d] traveler %d finished\n", msg.pid, msg.travelerIndex);
    } else if (msg.type == MSG_NO_PATH) {
        t->state = STATE_NO_PATH;
        printf("[PID=%d] traveler %d: no path found\n", msg.pid, msg.travelerIndex);
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    int N, M;
    if (fscanf(fp, "%d %d", &N, &M) != 2 || N <= 0 || M < 0) {
        fprintf(stderr, "Invalid input: first line should be N M\n");
        fclose(fp);
        return 1;
    }

    Node *nodes = calloc(N, sizeof(Node));
    if (!nodes) {
        perror("calloc nodes");
        fclose(fp);
        return 1;
    }

    for (int i = 0; i < M; i++) {
        int src, dst, weight;
        if (fscanf(fp, "%d %d %d", &src, &dst, &weight) != 3) {
            fprintf(stderr, "Invalid edge line\n");
            return 1;
        }
        if (src < 0 || src >= N || dst < 0 || dst >= N || weight < 0) {
            fprintf(stderr, "Invalid edge values\n");
            return 1;
        }
        add_edge(nodes, src, dst, weight);
    }

    int travelerCount;
    if (fscanf(fp, "%d", &travelerCount) != 1 || travelerCount <= 0) {
        fprintf(stderr, "Invalid travelers count\n");
        return 1;
    }

    int *starts = malloc(travelerCount * sizeof(int));
    int *ends = malloc(travelerCount * sizeof(int));
    if (!starts || !ends) {
        perror("malloc travelers");
        return 1;
    }

    for (int i = 0; i < travelerCount; i++) {
        if (fscanf(fp, "%d %d", &starts[i], &ends[i]) != 2) {
            fprintf(stderr, "Invalid traveler line\n");
            return 1;
        }
        if (starts[i] < 0 || starts[i] >= N || ends[i] < 0 || ends[i] >= N) {
            fprintf(stderr, "Invalid traveler nodes\n");
            return 1;
        }
    }
    fclose(fp);

    const int screenWidth = 900;
    const int screenHeight = 650;
    float centerX = screenWidth / 2.0f;
    float centerY = screenHeight / 2.0f + 20.0f;
    float radius = 230.0f;

    for (int i = 0; i < N; i++) {
        float angle = 2.0f * PI * i / N;
        nodes[i].x = centerX + cosf(angle) * radius;
        nodes[i].y = centerY + sinf(angle) * radius;
    }

    Traveler *travelers = calloc(travelerCount, sizeof(Traveler));
    int (*pipes)[2] = malloc(travelerCount * sizeof(int[2]));
    if (!travelers || !pipes) {
        perror("malloc simulation data");
        return 1;
    }

    Color colors[] = { RED, BLUE, GREEN, ORANGE, PURPLE, MAROON, DARKGREEN, PINK, SKYBLUE, GOLD };

    for (int i = 0; i < travelerCount; i++) {
        travelers[i].startNode = starts[i];
        travelers[i].endNode = ends[i];
        travelers[i].currentNode = starts[i];
        travelers[i].nextNode = -1;
        travelers[i].position = (Vector2){ nodes[starts[i]].x, nodes[starts[i]].y };
        travelers[i].fromPosition = travelers[i].position;
        travelers[i].targetPosition = travelers[i].position;
        travelers[i].color = colors[i % 10];
        travelers[i].state = STATE_WAITING;

        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return 1;
        }

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            return 1;
        }

        if (pid == 0) {
            close(pipes[i][0]);
            traveler_child_process(i, pipes[i][1], nodes, N, starts[i], ends[i]);
        }

        close(pipes[i][1]);
        travelers[i].pid = pid;

        int flags = fcntl(pipes[i][0], F_GETFL, 0);
        fcntl(pipes[i][0], F_SETFL, flags | O_NONBLOCK);
    }

    InitWindow(screenWidth, screenHeight, "Milestone 5 - IPC using Pipes");
    SetTargetFPS(60);

    bool paused = false;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        Rectangle btn = { 10, 40, 120, 40 };
        Vector2 mouse = GetMousePosition();
        bool hover = CheckCollisionPointRec(mouse, btn);

        if (hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            paused = !paused;
            for (int i = 0; i < travelerCount; i++) {
                if (travelers[i].state != STATE_FINISHED && travelers[i].state != STATE_NO_PATH) {
                    kill(travelers[i].pid, paused ? SIGSTOP : SIGCONT);
                }
            }
        }

        if (!paused) {
            for (int i = 0; i < travelerCount; i++) {
                IPCMessage msg;
                while (read(pipes[i][0], &msg, sizeof(msg)) == (ssize_t)sizeof(msg)) {
                    update_traveler_from_msg(travelers, nodes, msg);
                }
            }
        }

        for (int i = 0; i < travelerCount; i++) {
            Traveler *t = &travelers[i];
            if (!paused && t->state == STATE_MOVING) {
                t->timer += dt;
                float total = t->edgeWeight * MOVE_TIME_PER_WEIGHT;
                if (total <= 0.0f) total = MOVE_TIME_PER_WEIGHT;
                float alpha = t->timer / total;
                if (alpha > 1.0f) alpha = 1.0f;
                t->position = Vector2Lerp(t->fromPosition, t->targetPosition, alpha);
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawText("Milestone 5: Parent/Child communication using pipe", 10, 10, 20, DARKGRAY);
        DrawRectangleRec(btn, hover ? GRAY : LIGHTGRAY);
        DrawRectangleLinesEx(btn, 2, DARKGRAY);
        DrawText(paused ? "START" : "STOP", 38, 50, 20, BLACK);

        for (int i = 0; i < N; i++) {
            for (int j = 0; j < nodes[i].count; j++) {
                int to = nodes[i].edges[j].to;
                int w = nodes[i].edges[j].weight;
                DrawArrow((Vector2){nodes[i].x, nodes[i].y},
                          (Vector2){nodes[to].x, nodes[to].y}, w);
            }
        }

        for (int i = 0; i < N; i++) {
            DrawCircle((int)nodes[i].x, (int)nodes[i].y, 25, SKYBLUE);
            DrawCircleLines((int)nodes[i].x, (int)nodes[i].y, 25, BLUE);
            char id[16];
            snprintf(id, sizeof(id), "%d", i);
            DrawText(id, (int)(nodes[i].x - MeasureText(id, 20) / 2), (int)(nodes[i].y - 10), 20, BLACK);
        }

        for (int i = 0; i < travelerCount; i++) {
            Traveler *t = &travelers[i];
            if (t->state == STATE_NO_PATH) continue;

            Vector2 pos = t->position;
            if (t->state == STATE_MOVING) {
                float hop = sinf(fminf(t->timer / MOVE_TIME_PER_WEIGHT, 1.0f) * PI) * 12.0f;
                Vector2 diff = Vector2Subtract(t->targetPosition, t->fromPosition);
                float angle = atan2f(diff.y, diff.x);
                pos.x -= sinf(angle) * hop;
                pos.y += cosf(angle) * hop;
            }

            DrawCircleV(pos, 15, t->color);
            DrawCircleLinesV(pos, 15, BLACK);

            char label[20];
            snprintf(label, sizeof(label), "T%d", i);
            DrawText(label, (int)pos.x - 10, (int)pos.y - 32, 14, BLACK);

            if (t->state == STATE_FINISHED) {
                DrawText("DONE", (int)pos.x - 20, (int)pos.y + 22, 12, DARKGREEN);
            }
        }

        EndDrawing();
    }

    for (int i = 0; i < travelerCount; i++) {
        if (travelers[i].state != STATE_FINISHED && travelers[i].state != STATE_NO_PATH) {
            kill(travelers[i].pid, SIGKILL);
        }
        waitpid(travelers[i].pid, NULL, 0);
        close(pipes[i][0]);
    }

    CloseWindow();

    for (int i = 0; i < N; i++) free(nodes[i].edges);
    free(nodes);
    free(starts);
    free(ends);
    free(travelers);
    free(pipes);

    return 0;
}
