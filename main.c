#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <emscripten/emscripten.h>

#define GRID_SIZE 20
#define MAX_OPEN_LIST 400

typedef struct Node {
    int x, y;
    int g, h, f;
    int walkable; // 1 = yes, 0 = barrier
    int in_open; // For open list tracking
    struct Node* parent;
} Node;

Node grid[GRID_SIZE][GRID_SIZE];
int visited[GRID_SIZE][GRID_SIZE];
Node* open_list[MAX_OPEN_LIST];
int open_count = 0;
Node* path_nodes[GRID_SIZE * GRID_SIZE];
int path_length = 0;
int dx[4] = {0, 0, -1, 1};
int dy[4] = {-1, 1, 0, 0};

int is_inside(int x, int y) {
    return x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE;
}

int heuristic(Node* a, Node* b) {
    return abs(a->x - b->x) + abs(a->y - b->y);
}

void add_open(Node* n) {
    open_list[open_count++] = n;
    n->in_open = 1;
}

void remove_open(int index) {
    open_list[index]->in_open = 0;
    for (int i = index; i < open_count - 1; i++)
        open_list[i] = open_list[i + 1];
    open_count--;
}

int find_lowest_f() {
    int lowest = 0;
    for (int i = 1; i < open_count; i++)
        if (open_list[i]->f < open_list[lowest]->f)
            lowest = i;
    return lowest;
}

void reset_pathfinding_data() {
    open_count = 0;
    path_length = 0;
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            grid[x][y].g = 0;
            grid[x][y].h = 0;
            grid[x][y].f = 0;
            grid[x][y].in_open = 0;
            grid[x][y].parent = NULL;
            visited[x][y] = 0;
        }
    }
}

void reset_grid() {
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            grid[x][y].x = x;
            grid[x][y].y = y;
            grid[x][y].walkable = 1;
        }
    }
    reset_pathfinding_data();
}

EMSCRIPTEN_KEEPALIVE void init_grid() {
    reset_grid();
}

EMSCRIPTEN_KEEPALIVE void set_barrier(int x, int y) {
    if (is_inside(x, y)) {
        grid[x][y].walkable = !grid[x][y].walkable;
    }
}

EMSCRIPTEN_KEEPALIVE void clear_barriers() {
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            grid[x][y].walkable = 1;
        }
    }
}

EMSCRIPTEN_KEEPALIVE int run_astar(int startX, int startY, int endX, int endY) {
    reset_pathfinding_data();
    if (!is_inside(startX, startY) || !is_inside(endX, endY)) return 0;
    Node* start = &grid[startX][startY];
    Node* end = &grid[endX][endY];

    start->g = 0;
    start->h = heuristic(start, end);
    start->f = start->g + start->h;
    add_open(start);

    while (open_count > 0) {
        int index = find_lowest_f();
        Node* current = open_list[index];
        remove_open(index);

        visited[current->x][current->y] = 1;

        if (current == end) {
            Node* p = end;
            while (p && path_length < GRID_SIZE * GRID_SIZE) {
                path_nodes[path_length++] = p;
                p = p->parent;
            }
            for (int i = 0; i < path_length / 2; i++) {
                Node* tmp = path_nodes[i];
                path_nodes[i] = path_nodes[path_length - 1 - i];
                path_nodes[path_length - 1 - i] = tmp;
            }
            return 1;
        }
        for (int i = 0; i < 4; i++) {
            int nx = current->x + dx[i];
            int ny = current->y + dy[i];
            if (!is_inside(nx, ny)) continue;
            Node* neighbor = &grid[nx][ny];
            if (!neighbor->walkable || visited[nx][ny]) continue;
            int tentative_g = current->g + 1;
            if (!neighbor->in_open || tentative_g < neighbor->g) {
                neighbor->g = tentative_g;
                neighbor->h = heuristic(neighbor, end);
                neighbor->f = neighbor->g + neighbor->h;
                neighbor->parent = current;
                if (!neighbor->in_open) add_open(neighbor);
            }
        }
    }
    return 0;
}

EMSCRIPTEN_KEEPALIVE int run_dijkstra(int startX, int startY, int endX, int endY) {
    reset_pathfinding_data();
    if (!is_inside(startX, startY) || !is_inside(endX, endY)) return 0;
    Node* start = &grid[startX][startY];
    Node* end = &grid[endX][endY];

    start->g = 0;
    start->f = 0;
    add_open(start);

    while (open_count > 0) {
        int index = find_lowest_f();
        Node* current = open_list[index];
        remove_open(index);

        visited[current->x][current->y] = 1;

        if (current == end) {
            Node* p = end;
            while (p && path_length < GRID_SIZE * GRID_SIZE) {
                path_nodes[path_length++] = p;
                p = p->parent;
            }
            for (int i = 0; i < path_length / 2; i++) {
                Node* tmp = path_nodes[i];
                path_nodes[i] = path_nodes[path_length - 1 - i];
                path_nodes[path_length - 1 - i] = tmp;
            }
            return 1;
        }
        for (int i = 0; i < 4; i++) {
            int nx = current->x + dx[i];
            int ny = current->y + dy[i];
            if (!is_inside(nx, ny)) continue;
            Node* neighbor = &grid[nx][ny];
            if (!neighbor->walkable || visited[nx][ny]) continue;
            int tentative_g = current->g + 1;
            if (!neighbor->in_open || tentative_g < neighbor->g) {
                neighbor->g = tentative_g;
                neighbor->f = tentative_g;
                neighbor->parent = current;
                if (!neighbor->in_open) add_open(neighbor);
            }
        }
    }
    return 0;
}

EMSCRIPTEN_KEEPALIVE int run_bfs(int startX, int startY, int endX, int endY) {
    reset_pathfinding_data();
    if (!is_inside(startX, startY) || !is_inside(endX, endY)) return 0;
    Node* start = &grid[startX][startY];
    Node* end = &grid[endX][endY];

    start->g = 0;
    add_open(start);
    visited[startX][startY] = 1;

    while (open_count > 0) {
        int index = 0; // Queue
        Node* current = open_list[index];
        remove_open(index);

        if (current == end) {
            Node* p = end;
            while (p && path_length < GRID_SIZE * GRID_SIZE) {
                path_nodes[path_length++] = p;
                p = p->parent;
            }
            for (int i = 0; i < path_length / 2; i++) {
                Node* tmp = path_nodes[i];
                path_nodes[i] = path_nodes[path_length - 1 - i];
                path_nodes[path_length - 1 - i] = tmp;
            }
            return 1;
        }
        for (int i = 0; i < 4; i++) {
            int nx = current->x + dx[i];
            int ny = current->y + dy[i];
            if (!is_inside(nx, ny)) continue;
            Node* neighbor = &grid[nx][ny];
            if (!neighbor->walkable || visited[nx][ny]) continue;
            neighbor->g = current->g + 1;
            neighbor->parent = current;
            visited[nx][ny] = 1;
            add_open(neighbor);
        }
    }
    return 0;
}

EMSCRIPTEN_KEEPALIVE int run_dfs(int startX, int startY, int endX, int endY) {
    reset_pathfinding_data();
    if (!is_inside(startX, startY) || !is_inside(endX, endY)) return 0;
    Node* start = &grid[startX][startY];
    Node* end = &grid[endX][endY];

    start->g = 0;
    add_open(start);
    visited[startX][startY] = 1;

    while (open_count > 0) {
        int index = open_count - 1; // Stack
        Node* current = open_list[index];
        remove_open(index);

        if (current == end) {
            Node* p = end;
            while (p && path_length < GRID_SIZE * GRID_SIZE) {
                path_nodes[path_length++] = p;
                p = p->parent;
            }
            for (int i = 0; i < path_length / 2; i++) {
                Node* tmp = path_nodes[i];
                path_nodes[i] = path_nodes[path_length - 1 - i];
                path_nodes[path_length - 1 - i] = tmp;
            }
            return 1;
        }
        for (int i = 0; i < 4; i++) {
            int nx = current->x + dx[i];
            int ny = current->y + dy[i];
            if (!is_inside(nx, ny)) continue;
            Node* neighbor = &grid[nx][ny];
            if (!neighbor->walkable || visited[nx][ny]) continue;
            neighbor->g = current->g + 1;
            neighbor->parent = current;
            visited[nx][ny] = 1;
            add_open(neighbor);
        }
    }
    return 0;
}

// Greedy Best-First Search
EMSCRIPTEN_KEEPALIVE int run_greedy(int startX, int startY, int endX, int endY) {
    reset_pathfinding_data();
    if (!is_inside(startX, startY) || !is_inside(endX, endY)) return 0;
    Node* start = &grid[startX][startY];
    Node* end = &grid[endX][endY];

    start->g = 0;
    start->h = heuristic(start, end);
    start->f = start->h; // Only heuristic
    add_open(start);

    while (open_count > 0) {
        int index = find_lowest_f();
        Node* current = open_list[index];
        remove_open(index);
        visited[current->x][current->y] = 1;
        if (current == end) {
            Node* p = end;
            while (p && path_length < GRID_SIZE * GRID_SIZE) {
                path_nodes[path_length++] = p;
                p = p->parent;
            }
            for (int i = 0; i < path_length / 2; i++) {
                Node* tmp = path_nodes[i];
                path_nodes[i] = path_nodes[path_length - 1 - i];
                path_nodes[path_length - 1 - i] = tmp;
            }
            return 1;
        }
        for (int i = 0; i < 4; i++) {
            int nx = current->x + dx[i];
            int ny = current->y + dy[i];
            if (!is_inside(nx, ny)) continue;
            Node* neighbor = &grid[nx][ny];
            if (!neighbor->walkable || visited[nx][ny]) continue;
            int heuristic_val = heuristic(neighbor, end);
            if (!neighbor->in_open) {
                neighbor->g = current->g + 1; // not used for f, but for traceback
                neighbor->h = heuristic_val;
                neighbor->f = heuristic_val;
                neighbor->parent = current;
                add_open(neighbor);
            }
        }
    }
    return 0;
}

// ============================================
// WEIGHTED A*
// ============================================
EMSCRIPTEN_KEEPALIVE int run_weighted_astar(int startX, int startY, int endX, int endY) {
    reset_pathfinding_data();
    if (!is_inside(startX, startY) || !is_inside(endX, endY)) return 0;
    Node* start = &grid[startX][startY];
    Node* end = &grid[endX][endY];

    start->g = 0;
    start->h = heuristic(start, end);
    start->f = start->g + 1.5 * start->h; // Weight = 1.5 (trades optimality for speed)
    add_open(start);

    while (open_count > 0) {
        int index = find_lowest_f();
        Node* current = open_list[index];
        remove_open(index);

        visited[current->x][current->y] = 1;

        if (current == end) {
            Node* p = end;
            while (p && path_length < GRID_SIZE * GRID_SIZE) {
                path_nodes[path_length++] = p;
                p = p->parent;
            }
            for (int i = 0; i < path_length / 2; i++) {
                Node* tmp = path_nodes[i];
                path_nodes[i] = path_nodes[path_length - 1 - i];
                path_nodes[path_length - 1 - i] = tmp;
            }
            return 1;
        }
        
        for (int i = 0; i < 4; i++) {
            int nx = current->x + dx[i];
            int ny = current->y + dy[i];
            if (!is_inside(nx, ny)) continue;
            Node* neighbor = &grid[nx][ny];
            if (!neighbor->walkable || visited[nx][ny]) continue;
            int tentative_g = current->g + 1;
            if (!neighbor->in_open || tentative_g < neighbor->g) {
                neighbor->g = tentative_g;
                neighbor->h = heuristic(neighbor, end);
                neighbor->f = neighbor->g + 1.5 * neighbor->h; // Weighted heuristic
                neighbor->parent = current;
                if (!neighbor->in_open) add_open(neighbor);
            }
        }
    }
    return 0;
}

// ============================================
// BIDIRECTIONAL BFS
// ============================================
int visited_forward[GRID_SIZE][GRID_SIZE];
int visited_backward[GRID_SIZE][GRID_SIZE];
Node* parent_forward[GRID_SIZE][GRID_SIZE];
Node* parent_backward[GRID_SIZE][GRID_SIZE];

EMSCRIPTEN_KEEPALIVE int run_bidirectional(int startX, int startY, int endX, int endY) {
    reset_pathfinding_data();
    if (!is_inside(startX, startY) || !is_inside(endX, endY)) return 0;
    
    Node* start = &grid[startX][startY];
    Node* end = &grid[endX][endY];
    
    // Initialize arrays
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            visited_forward[x][y] = 0;
            visited_backward[x][y] = 0;
            parent_forward[x][y] = NULL;
            parent_backward[x][y] = NULL;
        }
    }
    
    // Two queues
    Node* queue_forward[MAX_OPEN_LIST];
    Node* queue_backward[MAX_OPEN_LIST];
    int front_f = 0, rear_f = 0;
    int front_b = 0, rear_b = 0;
    
    // Start both searches
    queue_forward[rear_f++] = start;
    queue_backward[rear_b++] = end;
    visited_forward[startX][startY] = 1;
    visited_backward[endX][endY] = 1;
    visited[startX][startY] = 1;
    visited[endX][endY] = 1;
    
    Node* meeting_point = NULL;
    
    while (front_f < rear_f && front_b < rear_b) {
        // Expand forward search
        if (front_f < rear_f) {
            Node* current = queue_forward[front_f++];
            
            for (int i = 0; i < 4; i++) {
                int nx = current->x + dx[i];
                int ny = current->y + dy[i];
                if (!is_inside(nx, ny)) continue;
                Node* neighbor = &grid[nx][ny];
                if (!neighbor->walkable || visited_forward[nx][ny]) continue;
                
                visited_forward[nx][ny] = 1;
                visited[nx][ny] = 1;
                parent_forward[nx][ny] = current;
                neighbor->g = current->g + 1;
                queue_forward[rear_f++] = neighbor;
                
                // Check if we met backward search
                if (visited_backward[nx][ny]) {
                    meeting_point = neighbor;
                    goto build_path;
                }
            }
        }
        
        // Expand backward search
        if (front_b < rear_b) {
            Node* current = queue_backward[front_b++];
            
            for (int i = 0; i < 4; i++) {
                int nx = current->x + dx[i];
                int ny = current->y + dy[i];
                if (!is_inside(nx, ny)) continue;
                Node* neighbor = &grid[nx][ny];
                if (!neighbor->walkable || visited_backward[nx][ny]) continue;
                
                visited_backward[nx][ny] = 1;
                visited[nx][ny] = 1;
                parent_backward[nx][ny] = current;
                neighbor->g = current->g + 1;
                queue_backward[rear_b++] = neighbor;
                
                // Check if we met forward search
                if (visited_forward[nx][ny]) {
                    meeting_point = neighbor;
                    goto build_path;
                }
            }
        }
    }
    
    return 0; // No path found

build_path:
    if (!meeting_point) return 0;
    
    // Build path from start to meeting point
    Node* p = meeting_point;
    Node* temp_path[GRID_SIZE * GRID_SIZE];
    int temp_len = 0;
    
    while (p) {
        temp_path[temp_len++] = p;
        p = parent_forward[p->x][p->y];
    }
    
    // Reverse first half
    for (int i = 0; i < temp_len; i++) {
        path_nodes[path_length++] = temp_path[temp_len - 1 - i];
    }
    
    // Build path from meeting point to end
    p = parent_backward[meeting_point->x][meeting_point->y];
    while (p) {
        path_nodes[path_length++] = p;
        p = parent_backward[p->x][p->y];
    }
    
    return 1;
}

// ============================================
// MAZE GENERATOR - RANDOM
// ============================================
EMSCRIPTEN_KEEPALIVE void generate_random_maze(int density) {
    // density = 0-100 (percentage of barriers)
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            // Don't block if it's start (0,0) or end (19,19)
            if ((x == 0 && y == 0) || (x == GRID_SIZE-1 && y == GRID_SIZE-1)) {
                grid[x][y].walkable = 1;
            } else {
                // Random chance based on density
                grid[x][y].walkable = (rand() % 100) >= density;
            }
        }
    }
}

EMSCRIPTEN_KEEPALIVE int get_node_walkable(int x, int y) {
    if (is_inside(x, y)) return grid[x][y].walkable;
    return 0;
}

EMSCRIPTEN_KEEPALIVE int get_path_length() { return path_length; }
EMSCRIPTEN_KEEPALIVE int get_path_x(int index) { return path_nodes[index]->x; }
EMSCRIPTEN_KEEPALIVE int get_path_y(int index) { return path_nodes[index]->y; }
EMSCRIPTEN_KEEPALIVE int is_visited(int x, int y) { if (is_inside(x, y)) return visited[x][y]; return 0; }
EMSCRIPTEN_KEEPALIVE int get_node_g(int x, int y) { if (is_inside(x, y)) return grid[x][y].g; return -1; }
