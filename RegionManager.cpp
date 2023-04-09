//
// Created on 4/8/2023.
//

#include <sstream>
#include <iostream>
#include "RegionManager.h"

using namespace z3;

RegionManager::RegionManager(RegionType** regionTypes, size_t numRegions) :
    solver(ctx),
    size(numRegions),
    numCells(1 << size) {
    // Heads-up, valgrind will lie and say that the above line, the one that says numCells(1<<size) is leaking memory
    // It's not.
    // solver(ctx) is the one leaking memory.
    // With my current execution, Z3 is leaking about 305 kilobytes of memory.

    // SECT [n] referrs to things in testZ3.py and RegionHandler.py
    // note that those are .py so they're in the py_code folder

    // SECT 0 initializes ctx (automatically done) and solver (in the member initializer list)

    // SECT 1 initializes regionTypes
    // Size is auto-declared in the member initializer list
    // So we need to initialize regionTypes
    // First initialize the region-array
    regions = new expr*[size];
    // Then initialize each element of the region-array:
    for(size_t i = 0; i < size; i++){
        char* name = new char[2] {static_cast<char>('A'+i), '\0'};
        auto region = new expr(ctx.int_const(name));
        delete[] name;
        regions[i] = region;
    }

    // SECT 2 initializes cells
    // I decided to split SECT 2 and SECT 3 because it makes the code somewhat better
    // Initialize the array of cells:
    cells = new expr*[numCells];
    // and make sure the zero cell is null:
    cells[0] = nullptr;
    // Initialize each cell:
    for(size_t i = 1; i < numCells; i++){
        // Initialize the actual cell variable:
        std::stringstream cellName;
        size_t bits = i;
        char regionName = 'a';
        while(bits > 0){
            if(bits%2 == 1){
                cellName << regionName;
            }
            regionName++;
            bits >>= 1;
        }

        // It's probably faster to just use a c-string to begin with
        // so I don't feel like writing that out until it's actually a significant # of clock cycles
        auto cell = new expr(ctx.int_const(cellName.str().c_str()));
        // Save the cell
        cells[i] = cell;

        // Update the solver with the cell min and maxes. Cell maxes are provided because one of the solver tactics
        // requires a min and a max, so maybe having both will make the solver run slightly faster?
        // And mins are provided because of course we can't have negative mines in a cell
        solver.add(*cell >= 0);
        solver.add(*cell <= 99);
    }

    // SECT 3 initializes the region sums
    // More specifically, it declares that region A is cells a + ab + ac + abc
    // Initialize regionSums as a copy of region:
    auto** regionSums = new expr*[size];
    for(size_t i = 0; i < size; i++){
        regionSums[i] = new expr(*regions[i]);
    }
    // Make sure the regionSums have their cells set correctly:
    for(size_t i = 1; i < numCells; i++){
        size_t bits = i;
        size_t regionNum = 0;
        auto cell = *cells[i];
        while(bits > 0){
            if(bits%2 == 1){
                expr region = *regionSums[regionNum];
                std::cout << region << std::endl;
                std::cout << cell << std::endl;
                region = region - cell;
                regionSums[regionNum][0] = region;
            }
            bits >>= 1;
            regionNum++;
        }
    }
    // and tell the solver that the regionSums are all zero
    // aka that region - cell - cell - cell = 0
    // aka that region = cell + cell + cell
    for(size_t i = 0; i < size; i++){
        auto sum = *regionSums[i];
        solver.add(sum == 0);
    }
    // clean up the memory
    for(size_t i = 0; i < size; i++){
        delete regionSums[i];
    }
    delete[] regionSums;

    // SECT 4
    // Getting the data from the RegionType parameter
    for(size_t i = 0; i < size; i++){
        RegionType* regionType = regionTypes[i];
        expr regionVar = *regions[i];
        expr regionExpr = regionType->getExpr(regionVar);
        solver.add(regionExpr);
        // A small optimization.
        solver.add(regionVar <= regionType->getMaxMines());
    }
}

RegionManager::~RegionManager() {
    // Valgrind will say there's memory leaks.
    // Ignore them, z3++.h and z3::solver solver(ctx) are the ones doing the leaking.
    // Currently, valgrind measures about 304,568 bytes leaked.
    for(size_t i = 0; i < size; i++){
        delete regions[i];
    }
    delete[] regions;

    for(size_t i = 1; i < numCells; i++){
        delete cells[i];
    }
    delete[] cells;
}

void RegionManager::test(int *cellLimits) {
    solver.push();
    for(size_t i = 1; i < numCells; i++){
        auto cell = *cells[i];
        solver.add(cell <= cellLimits[i]);
    }
    std::cout << "Solver state:\n";
    std::cout << solver << "\n";
    std::cout << "SMT2 solver state:\n";
    std::cout << solver.to_smt2() << "\n";
    if(solver.check() == unsat){
        std::cout << "Test output is UNSAT\n";
    }
    for(size_t cellNum = 1; cellNum < numCells; cellNum++){
        std::vector<int> vals;
        auto cell = *cells[cellNum];
        for(int numMines = 0; numMines < 11; numMines++){
            auto assumption = cell == numMines;
            auto result = solver.check(1, &assumption);
            // If result is UNSAT then it is not possible for cell to have numMines mines
            // If result is NOT UNSAT then it is NOT (not possible for cell to have numMines mines)
            // If result is NOT UNSAT then it is possible for cell to have numMines mines
            if(result != unsat){
                vals.push_back(numMines);
            }
        }
        std::cout << "Possible mine values for cell " << cell << ": [";
        for(int i : vals){
            std::cout << i << ", ";
        }
        std::cout << "]" << std::endl;
    }

    solver.pop();
}
