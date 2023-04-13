//
// Created on 4/8/2023.
//

#include <sstream>
#include <iostream>
#include "RegionManager.h"
#include "Deductions/Deduction.h"

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

    // Stuff to help with timing management.
    // These arrays are probably too big but I don't wanna have to change it later if it's too small.
    recursionTimes = new uint64_t[20];
    for(int i = 0; i < 20; i++){
        recursionTimes[i] = 0;
    }

    deductionTimes = new uint64_t[20];
    for(int i = 0; i < 20; i++){
        deductionTimes[i] = 0;
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

    delete[] recursionTimes;
    delete[] deductionTimes;

    // Clean up all the models
    // Each int* in models was generated in getAndSaveModels so we need to clean them up
    for(int* model : models){
        delete[] model;
    }
}

void RegionManager::test(int *cellLimits) {
    solver.push();
    restrict(cellLimits);
    std::cout << "Solver state:\n";
    std::cout << solver << "\n";
    std::cout << "SMT2 solver state:\n";
    std::cout << solver.to_smt2() << "\n";
    if(solver.check() == unsat){
        std::cout << "Test output is UNSAT\n";
    }
    Deduction data = getDeduction();
    std::cout << data.toLongStr();

    solver.pop();
}

void RegionManager::restrict(int *cellLimits) {
    for(size_t i = 1; i < numCells; i++){
        auto cell = *cells[i];
        solver.add(cell <= cellLimits[i]);
    }
}

DeductionManager* RegionManager::recursive_test(int index, const Deduction &check) { // NOLINT(misc-no-recursion)
    recursionTimes[0] -= clock();
    Deduction self = getDeduction(check);
    recursionTimes[0] += clock();

    recursionTimes[1] -= clock();
    auto* out = new DeductionManager(self);
    recursionTimes[1] += clock();

    // not all of these indices are necessary but I keep them all just in case.

    for(int cellNum = index; cellNum < numCells; cellNum++){
        recursionTimes[2] -= clock();
        auto cell = *cells[cellNum];
        recursionTimes[2] += clock();

        recursionTimes[3] -= clock();
        solver.push();
        recursionTimes[3] += clock();

        // This is equivalent to if the limit on this cell was 99
        recursionTimes[7] -= clock();
        auto lastDeduction = self;
        recursionTimes[7] += clock();
        for(int limit = 10; limit >= 0; limit--){
            // By going from 10 to 0 instead of 0 to 11 (or i guess 10)
            // we don't need to call push and pop nearly as much
            // though I need to experiment with how much CPU time this saves since now the z3 solver has to
            // figure it out (though i don't doubt that their internals can do it faster than I can)
            recursionTimes[4] -= clock();
            solver.add(cell <= limit);
            recursionTimes[4] += clock();

            // cellNum + 1 because we don't want to manipulate cellNum twice.
            // Note that lastDeduction is always the result of a less-restrictive limitation on this cell
            // Either because the lastDeduction was from when the limit was one greater
            // or because it was from when the limit was 99.
            DeductionManager* recursiveOut = recursive_test(cellNum + 1, lastDeduction);

            recursionTimes[8] -= clock();
            lastDeduction = recursiveOut->getDeduction();
            recursionTimes[8] += clock();

            recursionTimes[5] -= clock();
            out->set(cellNum, limit, recursiveOut);
            recursionTimes[5] += clock();
        }
        recursionTimes[6] -= clock();
        solver.pop();
        recursionTimes[6] += clock();
    }
    return out;
}

DeductionManager *RegionManager::recursive_test(int index) { // NOLINT(misc-no-recursion)
    Deduction truthyDeduction = Deduction(numCells, true);
    return recursive_test(index, truthyDeduction);
}

std::ostream &RegionManager::getClockStr(std::ostream &stream) {
    stream << "getDeduction() time: " << recursionTimes[0] << "\n";
    stream << "new DeductionManager time: " << recursionTimes[1] << "\n";
    stream << "[auto cell = ] time: " << recursionTimes[2] << "\n";
    stream << "solver.push() time: " << recursionTimes[3] << "\n";
    stream << "[lastDeduction = self] time: " << recursionTimes[7] << "\n";
    stream << "solver.add() time: " << recursionTimes[4] << "\n";
    stream << "[lastDeduction = recursiveOut->getDeduction()] time: " << recursionTimes[8] << "\n";
    stream << "out->set() time: " << recursionTimes[5] << "\n";
    stream << "solver.pop() time: " << recursionTimes[6] << "\n";
    stream << "\n";
    stream << "getClockStr() data: \n";
    stream << "Deduction init time: " << deductionTimes[0] << "\n";
    stream << "[auto cell] time: " << deductionTimes[1] << "\n";
    stream << "[oth] known-falsy time: " << deductionTimes[2] << "\n";
    stream << "oth.get() falsy values: " << dataGetFalsy << "\n";
    stream << "oth.get() truthy values: " << dataGetTruthy << "\n";
    stream << "Deduction known-truthy time: " << deductionTimes[3] << "\n";
    stream << "Model reduced check calls by: " << modelTruthy << "\n";
    stream << "Model falsy: " << modelFalsy << "\n";
    stream << "[auto assumption] time: " << deductionTimes[4] << "\n";
    stream << "solver.check() time: " << deductionTimes[5] << "\n";
    stream << "getAndSaveModel() time: " << deductionTimes[6] << "\n";
    stream << "[int* model] processing time: " << deductionTimes[7] << "\n";
    stream << std::endl;
    return stream;
}

Deduction RegionManager::getDeduction() {
    Deduction out(numCells, true);
    return getDeduction(out);
}

Deduction RegionManager::getDeduction(const Deduction &oth) {
    deductionTimes[0] -= clock();
    Deduction data(numCells, false);
    deductionTimes[0] += clock();

    for(size_t cellNum = 1; cellNum < numCells; cellNum++){
        deductionTimes[1] -= clock();
        auto cell = *cells[cellNum];
        deductionTimes[1] += clock();

        for(int numMines = 0; numMines < 11; numMines++){
            deductionTimes[2] -= clock();
            if(!oth.get(cellNum, numMines)){
                deductionTimes[2] += clock();
                dataGetFalsy++;
                // Super-deduction knows that this isn't true
                // So it can't be true here, either.
                continue;
            }
            deductionTimes[2] += clock();
            dataGetTruthy++;

            deductionTimes[3] -= clock();
            if(data.get(cellNum, numMines)){
                deductionTimes[3] += clock();
                modelTruthy++;
                // We already know this to be true from other optimizations.
                continue;
            }
            deductionTimes[3] += clock();
            modelFalsy++;

            deductionTimes[4] -= clock();
            auto assumption = cell == numMines;
            deductionTimes[4] += clock();
            deductionTimes[5] -= clock();
            auto result = solver.check(1, &assumption);
            deductionTimes[5] += clock();
            // If result is UNSAT then it is not possible for cell to have numMines mines
            // If result is NOT UNSAT then it is NOT (not possible for cell to have numMines mines)
            // If result is NOT UNSAT then it is possible for cell to have numMines mines
            if(result == unsat){
                // If result is UNSAT then it is not possible for cell to have numMines mines
                // Commented out because data now defaults to falsy
                // data.set(cellNum, numMines, false);
            }
            else{
                deductionTimes[6] -= clock();
                auto model = getAndSaveModel();
                deductionTimes[6] += clock();

                deductionTimes[7] -= clock();
                if(model == nullptr){
                    modelNullptr++;
                    // well.
                    // default behavior
                    // We know it COULD be numMines mines
                    data.set(cellNum, numMines, true);
                }
                else{
                    // Use the model to update all information.
                    // since we know the model to be accurate (we just used getAndSaveModel())
                    for(int i = 1; i < numCells; i++){
                        // Cell number i could have model[i] mines in it.
                        // In theory.
                        data.set(i, model[i], true);
                    }
                }
                deductionTimes[7] += clock();
            }
        }
    }
    return data;
}

int *RegionManager::getAndSaveModel() {
    auto solverModel = solver.get_model();
    int* model = new int[numCells];
    // fill with 0s just in case.
    for(int i = 0; i < numCells; i++){
        model[i] = -1;
    }

    for(int i = 0; i < solverModel.size(); i++){
        auto var = solverModel[i];
        auto name = var.name().str();
        auto modelCellNum = nameToCellNum(name.c_str());
        if(modelCellNum < 1){
            // If modelCellNum is invalid, it'll be 0, and we shouldn't modify model[]
            continue;
        }
        auto valueExpr = solverModel.get_const_interp(var);
        auto value = valueExpr.as_int64();
        model[modelCellNum] = value;
    }

    // Validate model
    for(int i = 1; i < numCells; i++){
        if(model[i] == -1){
            // invalid model
            delete[] model;
            return nullptr;
        }
    }

    // Save model.
    models.push_back(model);
    return model;
}

size_t RegionManager::nameToCellNum(const char *name){
    // Note: This returns 0 for invalid cell nums
    // because size_t is an UNSIGNED long, so -1 is the biggest number

    size_t cellNum = 0;
    for(int ptrIndex = 0; name[ptrIndex] != '\0'; ptrIndex++){
        // aka until the end of the thing
        char letter = name[ptrIndex];
        if(letter >= 'a' && letter <= 'z'){
            // If this is falsy then most likely this is a region variable.
            cellNum = cellNum | (1 << (letter - 'a'));
        }
        else{
            // Most likely a region variable?
            return 0;
        }
    }

    if(cellNum == 0){
        // I don't know.
        return 0;
    }
    return cellNum;
}
