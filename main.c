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
// ALGORITHM: BEAM SEARCH
// ============================================
// A variation of A* that prunes the open list to a fixed width.
// It uses qsort to sort nodes by F-score.

int compare_nodes(const void* a, const void* b) {
    Node* nodeA = *(Node**)a;
    Node* nodeB = *(Node**)b;
    return nodeA->f - nodeB->f;
}

EMSCRIPTEN_KEEPALIVE int run_beam_search(int startX, int startY, int endX, int endY) {
    reset_pathfinding_data();
    if (!is_inside(startX, startY) || !is_inside(endX, endY)) return 0;

    Node* start = &grid[startX][startY];
    Node* end = &grid[endX][endY];
    int BEAM_WIDTH = 3; // Very narrow beam to show the effect clearly

    start->g = 0;
    start->h = heuristic(start, end);
    start->f = start->h;
    add_open(start);

    while (open_count > 0) {
        // 1. Sort Open List to put best nodes at the start (index 0)
        qsort(open_list, open_count, sizeof(Node*), compare_nodes);

        // 2. PRUNING: If we have more than BEAM_WIDTH, cut the rest off
        if (open_count > BEAM_WIDTH) {
            // Reset 'in_open' flag for nodes we are dropping
            for (int i = BEAM_WIDTH; i < open_count; i++) {
                open_list[i]->in_open = 0;
            }
            open_count = BEAM_WIDTH; // Hard cut
        }

        // 3. Pop the best node (which is now at index 0 after sort)
        Node* current = open_list[0];
        
        // Manual remove from index 0 (shift everything down)
        current->in_open = 0;
        for(int i=0; i < open_count - 1; i++) {
            open_list[i] = open_list[i+1];
        }
        open_count--;

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

            // Standard A* logic
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

// ============================================
// ALGORITHM: BIDIRECTIONAL A* (FIXED)
// ============================================

// 1. Global Arrays for the Backward Search
// (The Forward search uses the standard 'grid' struct)
int g_back[GRID_SIZE][GRID_SIZE];
int f_back[GRID_SIZE][GRID_SIZE];
int visited_back[GRID_SIZE][GRID_SIZE];
Node* parent_back[GRID_SIZE][GRID_SIZE];

// 2. Helper to reset backward data
void reset_bi_astar_data() {
    for(int x=0; x<GRID_SIZE; x++) {
        for(int y=0; y<GRID_SIZE; y++) {
            g_back[x][y] = 999999;
            f_back[x][y] = 999999;
            visited_back[x][y] = 0;
            parent_back[x][y] = NULL;
        }
    }
}

// 3. The Main Algorithm
EMSCRIPTEN_KEEPALIVE int run_bi_astar(int startX, int startY, int endX, int endY) {
    reset_pathfinding_data(); // Clear Forward/Standard data
    reset_bi_astar_data();    // Clear Backward data

    if (!is_inside(startX, startY) || !is_inside(endX, endY)) return 0;

    Node* start = &grid[startX][startY];
    Node* end = &grid[endX][endY];

    // --- SETUP FORWARD SEARCH ---
    Node* open_fwd[MAX_OPEN_LIST];
    int count_fwd = 0;
    
    start->g = 0;
    start->h = heuristic(start, end);
    start->f = start->g + start->h;
    start->in_open = 1;
    open_fwd[count_fwd++] = start;
    visited[startX][startY] = 1;

    // --- SETUP BACKWARD SEARCH ---
    Node* open_bwd[MAX_OPEN_LIST];
    int count_bwd = 0;

    g_back[endX][endY] = 0;
    f_back[endX][endY] = heuristic(end, start); // Heuristic aims at Start
    open_bwd[count_bwd++] = end;
    visited_back[endX][endY] = 1;
    visited[endX][endY] = 1; // Mark visited so visualizer sees it

    Node* meeting_node = NULL;

    while (count_fwd > 0 && count_bwd > 0) {
        
        // --- EXPAND FORWARD ---
        int best_fwd = 0;
        for(int i=1; i<count_fwd; i++) {
            if(open_fwd[i]->f < open_fwd[best_fwd]->f) best_fwd = i;
        }
        Node* curr_fwd = open_fwd[best_fwd];
        
        // Remove from list
        for(int i=best_fwd; i<count_fwd-1; i++) open_fwd[i] = open_fwd[i+1];
        count_fwd--;
        curr_fwd->in_open = 0;

        visited[curr_fwd->x][curr_fwd->y] = 1;

        for(int i=0; i<4; i++) {
            int nx = curr_fwd->x + dx[i];
            int ny = curr_fwd->y + dy[i];
            if(!is_inside(nx, ny)) continue;
            Node* neighbor = &grid[nx][ny];
            if(!neighbor->walkable) continue;

            // Check Collision with Backward Search
            if(visited_back[nx][ny]) {
                meeting_node = neighbor;
                neighbor->parent = curr_fwd; 
                goto build_bi_path;
            }

            // A* Update
            int tent_g = curr_fwd->g + 1;
            if(!visited[nx][ny] || tent_g < neighbor->g) {
                neighbor->g = tent_g;
                neighbor->h = heuristic(neighbor, end);
                neighbor->f = neighbor->g + neighbor->h;
                neighbor->parent = curr_fwd;
                visited[nx][ny] = 1;
                
                int in_list = 0;
                for(int k=0; k<count_fwd; k++) if(open_fwd[k] == neighbor) in_list = 1;
                if(!in_list) open_fwd[count_fwd++] = neighbor;
            }
        }

        // --- EXPAND BACKWARD ---
        int best_bwd = 0;
        for(int i=1; i<count_bwd; i++) {
            Node* n = open_bwd[i];
            Node* best = open_bwd[best_bwd];
            if(f_back[n->x][n->y] < f_back[best->x][best->y]) best_bwd = i;
        }
        Node* curr_bwd = open_bwd[best_bwd];

        // Remove from list
        for(int i=best_bwd; i<count_bwd-1; i++) open_bwd[i] = open_bwd[i+1];
        count_bwd--;

        visited[curr_bwd->x][curr_bwd->y] = 1;

        for(int i=0; i<4; i++) {
            int nx = curr_bwd->x + dx[i];
            int ny = curr_bwd->y + dy[i];
            if(!is_inside(nx, ny)) continue;
            Node* neighbor = &grid[nx][ny];
            if(!neighbor->walkable) continue;

            // Check Collision with Forward Search
            if(visited[nx][ny] && neighbor->parent) { 
                meeting_node = neighbor; 
                parent_back[nx][ny] = curr_bwd; 
                goto build_bi_path;
            }

            // A* Update (using local arrays)
            int tent_g = g_back[curr_bwd->x][curr_bwd->y] + 1;
            if(!visited_back[nx][ny] || tent_g < g_back[nx][ny]) {
                g_back[nx][ny] = tent_g;
                
                // *** THIS IS THE FIX ***
                neighbor->g = tent_g; // Sync G-score so visualizer animates it correctly
                // ***********************

                f_back[nx][ny] = tent_g + heuristic(neighbor, start);
                parent_back[nx][ny] = curr_bwd;
                visited_back[nx][ny] = 1;
                visited[nx][ny] = 1; // Mark global for visuals
                
                int in_list = 0;
                for(int k=0; k<count_bwd; k++) if(open_bwd[k] == neighbor) in_list = 1;
                if(!in_list) open_bwd[count_bwd++] = neighbor;
            }
        }
    }
    return 0;

build_bi_path:
    if (!meeting_node) return 0;

    // 1. Trace Forward (Start -> Meeting)
    Node* p = meeting_node;
    Node* temp_path[GRID_SIZE * GRID_SIZE];
    int temp_len = 0;
    
    while (p) {
        temp_path[temp_len++] = p;
        p = p->parent;
    }
    // Reverse to get Start -> Meeting
    for (int i = 0; i < temp_len; i++) {
        path_nodes[path_length++] = temp_path[temp_len - 1 - i];
    }

    // 2. Trace Backward (Meeting -> End)
    p = parent_back[meeting_node->x][meeting_node->y];
    while (p) {
        path_nodes[path_length++] = p;
        p = parent_back[p->x][p->y];
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
