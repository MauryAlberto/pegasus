fn randNum() {
    if(rand() > 0.5) {
        return 1;
    } else {
        return 0;
    }
}

mut grid = [];
for(mut i = 0; i < 400; i = i + 1) {
    grid.push(randNum());
}

immut GRID_ROW_SIZE = 20;
immut GRID_COL_SIZE = 20;

// temp grid to store updated grid before re-assigning to grid
mut temp = [];

fn countCells(row, col) {
    // needs to perform bounds checking so we don't index outside the range of the array
    // we check up, down, left, right, and the four corners
    immut directions = [
        [-1, -1], [-1, 0], [-1, 1],
        [0, -1],           [0, 1],
        [1, -1],  [1, 0],  [1, 1]
    ];

    mut count = 0;

    for(mut i = 0; i < directions.len(); i = i + 1) {
        immut dir = directions[i];
        immut newRow = row + dir[0];
        immut newCol = col + dir[1];

        if(newRow >= 0 and newRow < GRID_ROW_SIZE and newCol >= 0 and newCol < GRID_COL_SIZE) {
            immut index = newRow * GRID_COL_SIZE + newCol;
            if(grid[index] == 1) {
                count = count + 1;
            }
        }
    }
    return count;
}

// This function checks the current position in the grid to determine it's survivability
fn checkCell(row, col) {
    immut pos = row * GRID_COL_SIZE + col;
    immut alive = grid[pos] == 1;
    immut count = countCells(row, col);
    
    if(alive) {
        if(count == 2) {
            temp[pos] = 1;
        } else if(count == 3) {
            temp[pos] = 1;
        } else {
            temp[pos] = 0;
        }
    } else {
        if(count == 3) {
            temp[pos] = 1;
        } else {
            temp[pos] = 0;
        }
    }
}

fn delay() {
    for(mut i = 0; i < 5000; i = i + 1) {
        // do nothing
    }
}

fn printGrid() {
    for(mut i = 0; i < GRID_ROW_SIZE; i = i + 1) {
        for(mut j = 0; j < GRID_COL_SIZE; j = j + 1) {
            immut pos = i * GRID_COL_SIZE + j;
            write(grid[pos]);
            write(" ");
        }
        write("\n");
    }
}

fn main() {
    while(true) {
        printGrid();
        // reset temp to a fresh array of zeros
        temp = [];
        for(mut k = 0; k < 400; k = k + 1) {
            temp.push(0);
        }
        for(mut i = 0; i < GRID_ROW_SIZE; i = i + 1) {
            for(mut j = 0; j < GRID_COL_SIZE; j = j + 1) {
                checkCell(i, j);
            }
        }
        grid = temp;
        delay();
    }
}

main();