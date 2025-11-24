Module.onRuntimeInitialized = () => {
    Module.ccall('init_grid', null, [], []);
    
    const GRID_SIZE = 20;
    const gridDivs = [];
    let start = { x: 0, y: 0 };
    let end = { x: GRID_SIZE - 1, y: GRID_SIZE - 1 };
    let currentMode = 'barriers';
    let mouseDown = false;
    let isRunning = false;
    
    const gridContainer = document.getElementById('grid');
    const setStartBtn = document.getElementById('setStartBtn');
    const setEndBtn = document.getElementById('setEndBtn');
    const setBarriersBtn = document.getElementById('setBarriersBtn');
    const modeButtons = [setStartBtn, setEndBtn, setBarriersBtn];
    const runtimeDisplay = document.getElementById('runtime');
    const clearBtn = document.getElementById('clearBtn');
    const clearBarriersBtn = document.getElementById('clearBarriersBtn');
    const resetBtn = document.getElementById('resetBtn');
    
    const algoButtons = [
        document.getElementById('runBtn'),
        document.getElementById('runDijkstraBtn'),
        document.getElementById('runBfsBtn'),
        document.getElementById('runDfsBtn'),
        document.getElementById('runGreedyBtn'),
        document.getElementById('runWeightedAstarBtn'),      
        document.getElementById('runBidirectionalBtn')   
    ];
    
    gridContainer.style.gridTemplateColumns = `repeat(${GRID_SIZE}, 28px)`;
    gridContainer.style.gridTemplateRows = `repeat(${GRID_SIZE}, 28px)`;
    
    // Toast notification function
    function showToast(title, message, type = 'error') {
        const existingToast = document.querySelector('.toast');
        if (existingToast) existingToast.remove();
        
        const toast = document.createElement('div');
        toast.className = `toast ${type}`;
        
        const icon = type === 'error' ? '❌' : '✅';
        
        toast.innerHTML = `
            <div class="toast-icon">${icon}</div>
            <div class="toast-content">
                <div class="toast-title">${title}</div>
                <div class="toast-message">${message}</div>
            </div>
        `;
        
        document.body.appendChild(toast);
        
        setTimeout(() => {
            toast.classList.add('hide');
            setTimeout(() => toast.remove(), 300);
        }, 4000);
    }
    
    function updateMode(newMode) {
        currentMode = newMode;
        modeButtons.forEach(btn => btn.classList.remove('active'));
        if (newMode === 'start') setStartBtn.classList.add('active');
        else if (newMode === 'end') setEndBtn.classList.add('active');
        else setBarriersBtn.classList.add('active');
    }
    
    setStartBtn.addEventListener('click', () => updateMode('start'));
    setEndBtn.addEventListener('click', () => updateMode('end'));
    setBarriersBtn.addEventListener('click', () => updateMode('barriers'));
    
    function setPoint(cell, x, y) {
        if (cell.classList.contains('barrier')) return;
        
        if (currentMode === 'start') {
            if (x === end.x && y === end.y) return;
            const oldStartCell = gridDivs[start.y * GRID_SIZE + start.x];
            oldStartCell.classList.remove('start');
            start = { x, y };
            cell.classList.add('start');
        } else if (currentMode === 'end') {
            if (x === start.x && y === start.y) return;
            const oldEndCell = gridDivs[end.y * GRID_SIZE + end.x];
            oldEndCell.classList.remove('end');
            end = { x, y };
            cell.classList.add('end');
        }
    }
    
    // Create grid
    for (let y = 0; y < GRID_SIZE; y++) {
        for (let x = 0; x < GRID_SIZE; x++) {
            const cell = document.createElement('div');
            cell.classList.add('cell');
            
            if (x === start.x && y === start.y) cell.classList.add('start');
            if (x === end.x && y === end.y) cell.classList.add('end');
            
            cell.addEventListener('mousedown', (e) => {
                e.preventDefault();
                if (currentMode === 'barriers') {
                    mouseDown = true;
                    toggleBarrier(cell, x, y);
                } else {
                    setPoint(cell, x, y);
                }
            });
            
            cell.addEventListener('mouseenter', () => {
                if (mouseDown && currentMode === 'barriers') toggleBarrier(cell, x, y);
            });
            
            cell.addEventListener('mouseup', () => { mouseDown = false; });
            
            gridDivs.push(cell);
            gridContainer.appendChild(cell);
        }
    }
    
    document.body.addEventListener('mouseup', () => { mouseDown = false; });
    
    function toggleBarrier(cell, x, y) {
        if (cell.classList.contains('start') || cell.classList.contains('end')) return;
        Module.ccall('set_barrier', null, ['number', 'number'], [x, y]);
        cell.classList.toggle('barrier');
    }
    
    // Clear path button
    clearBtn.addEventListener('click', () => {
        clearPath();
        runtimeDisplay.classList.remove('show');
        showToast('Path Cleared', 'Visualization cleared successfully', 'success');
    });
    
    // Clear barriers button
    clearBarriersBtn.addEventListener('click', () => {
        gridDivs.forEach((c, idx) => {
            const y = Math.floor(idx / GRID_SIZE);
            const x = idx % GRID_SIZE;
            if (!c.classList.contains('start') && !c.classList.contains('end')) {
                c.classList.remove('barrier');
            }
        });
        Module.ccall('clear_barriers', null, [], []);
        showToast('Barriers Cleared', 'All barriers have been removed', 'success');
    });
    
    // Reset grid button
    resetBtn.addEventListener('click', () => {
        // Clear all visualizations
        gridDivs.forEach(c => {
            c.classList.remove('visited', 'path', 'barrier');
            const oldText = c.querySelector('.weight');
            if (oldText) oldText.remove();
        });
        
        // Reset start and end to corners
        const oldStart = gridDivs[start.y * GRID_SIZE + start.x];
        const oldEnd = gridDivs[end.y * GRID_SIZE + end.x];
        oldStart.classList.remove('start');
        oldEnd.classList.remove('end');
        
        start = { x: 0, y: 0 };
        end = { x: GRID_SIZE - 1, y: GRID_SIZE - 1 };
        
        gridDivs[0].classList.add('start');
        gridDivs[GRID_SIZE * GRID_SIZE - 1].classList.add('end');
        
        // Reset C grid
        Module.ccall('init_grid', null, [], []);
        
        runtimeDisplay.classList.remove('show');
        enableAlgorithmButtons();
        
        showToast('Grid Reset', 'Everything has been reset to default', 'success');
    });
    
    function disableAlgorithmButtons() {
        algoButtons.forEach(btn => btn.disabled = true);
        isRunning = true;
    }
    
    function enableAlgorithmButtons() {
        algoButtons.forEach(btn => btn.disabled = false);
        isRunning = false;
    }
    
    async function drawVisited() {
        let visitedNodes = [];
        let maxG = 0;
        
        for (let y = 0; y < GRID_SIZE; y++) {
            for (let x = 0; x < GRID_SIZE; x++) {
                if (Module.ccall('is_visited', 'number', ['number', 'number'], [x, y])) {
                    const g = Module.ccall('get_node_g', 'number', ['number', 'number'], [x, y]);
                    visitedNodes.push({ x, y, g });
                    if (g > maxG) maxG = g;
                }
            }
        }
        
        for (let g = 0; g <= maxG; g++) {
            const nodesInWave = visitedNodes.filter(node => node.g === g);
            if (nodesInWave.length > 0) {
                for (const node of nodesInWave) {
                    const cell = gridDivs[node.y * GRID_SIZE + node.x];
                    if (!cell.classList.contains('start') && !cell.classList.contains('end')) {
                        cell.classList.add('visited');
                    }
                }
                await new Promise(r => setTimeout(r, 80));
            }
        }
    }
    
    async function drawPath() {
        const length = Module.ccall('get_path_length', 'number');
        for (let i = 0; i < length; i++) {
            const x = Module.ccall('get_path_x', 'number', ['number'], [i]);
            const y = Module.ccall('get_path_y', 'number', ['number'], [i]);
            const cell = gridDivs[y * GRID_SIZE + x];
            if (!cell.classList.contains('start') && !cell.classList.contains('end')) {
                cell.classList.add('path');
            }
            await new Promise(r => setTimeout(r, 25));
        }
    }

    function clearPath() {
        gridDivs.forEach(c => {
            c.classList.remove('visited', 'path');
            const oldText = c.querySelector('.weight');
            if (oldText) oldText.remove();
        });
        runtimeDisplay.classList.remove('show');
    }

    
    document.getElementById('showWeightsBtn').addEventListener('click', () => {
        for (let y = 0; y < GRID_SIZE; y++) {
            for (let x = 0; x < GRID_SIZE; x++) {
                const cell = gridDivs[y * GRID_SIZE + x];
                const oldText = cell.querySelector('.weight');
                if (oldText) oldText.remove();
                
                const visited = Module.ccall('is_visited', 'number', ['number', 'number'], [x, y]);
                if (visited) {
                    const g = Module.ccall('get_node_g', 'number', ['number', 'number'], [x, y]);
                    const weightDiv = document.createElement('div');
                    weightDiv.classList.add('weight');
                    weightDiv.textContent = g;
                    cell.appendChild(weightDiv);
                }
            }
        }
    });
    
    async function runAlgorithm(algoName, algoDisplayName) {
        if (isRunning) return;
        
        disableAlgorithmButtons();
        
        gridDivs.forEach(c => {
            c.classList.remove('visited', 'path');
            const oldText = c.querySelector('.weight');
            if (oldText) oldText.remove();
        });
        
        const startTime = performance.now();
        
        const pathFound = Module.ccall(algoName, 'number', ['number', 'number', 'number', 'number'], 
                    [start.x, start.y, end.x, end.y]);
        
        const endTime = performance.now();
        const runtime = (endTime - startTime);
        
        // EXACT runtime display with 3 decimal places (microsecond precision)
        const runtimeStr = runtime.toFixed(3) + 'ms';
        
        // Count nodes visited
        let nodesVisited = 0;
        for (let y = 0; y < GRID_SIZE; y++) {
            for (let x = 0; x < GRID_SIZE; x++) {
                if (Module.ccall('is_visited', 'number', ['number', 'number'], [x, y])) {
                    nodesVisited++;
                }
            }
        }
        
        if (pathFound) {
            await drawVisited();
            await drawPath();
            
            const pathLength = Module.ccall('get_path_length', 'number');
            runtimeDisplay.textContent = `✨ ${algoDisplayName} | Runtime: ${runtimeStr} | Nodes explored: ${nodesVisited} | Path: ${pathLength} steps`;
            runtimeDisplay.className = 'show';
            showToast('Path Found!', `${algoDisplayName} explored ${nodesVisited} nodes in ${runtimeStr}`, 'success');
        } else {
            await drawVisited();
            runtimeDisplay.textContent = `⚠️ ${algoDisplayName} | Runtime: ${runtimeStr} | Nodes explored: ${nodesVisited} | No path found`;
            runtimeDisplay.className = 'show';
            showToast('No Path Found', `${algoDisplayName} explored ${nodesVisited} nodes but found no path`, 'error');
        }
        
        enableAlgorithmButtons();
    }

    
    document.getElementById('runBtn').addEventListener('click', async () => {
        await runAlgorithm('run_astar', 'A* Search');
    });
    
    document.getElementById('runDijkstraBtn').addEventListener('click', async () => {
        await runAlgorithm('run_dijkstra', "Dijkstra's Algorithm");
    });
    
    document.getElementById('runBfsBtn').addEventListener('click', async () => {
        await runAlgorithm('run_bfs', 'Breadth-First Search');
    });
    
    document.getElementById('runDfsBtn').addEventListener('click', async () => {
        await runAlgorithm('run_dfs', 'Depth-First Search');
    });

    document.getElementById('runGreedyBtn').addEventListener('click', async () => {
    await runAlgorithm('run_greedy', 'Greedy Best-First Search');
    });

    document.getElementById('runWeightedAstarBtn').addEventListener('click', async () => {
    await runAlgorithm('run_weighted_astar', 'Weighted A* (ε=1.5)');
    });

    document.getElementById('runBidirectionalBtn').addEventListener('click', async () => {
        await runAlgorithm('run_bidirectional', 'Bidirectional BFS');
    });

    document.getElementById('mazeRandomBtn').addEventListener('click', () => {
        clearPath();
        Module.ccall('generate_random_maze', null, ['number'], [30]); // 30% barriers
        
        // Update grid display
        gridDivs.forEach((cell, idx) => {
            const x = idx % GRID_SIZE;
            const y = Math.floor(idx / GRID_SIZE);
            const walkable = Module.ccall('get_node_walkable', 'number', ['number', 'number'], [x, y]);
            
            if (!walkable && !cell.classList.contains('start') && !cell.classList.contains('end')) {
                cell.classList.add('barrier');
            } else {
                cell.classList.remove('barrier');
            }
        });
        
        showToast('Maze Generated', 'Random barriers (30% density) created!', 'success');
    });
    
    updateMode('barriers');
};
