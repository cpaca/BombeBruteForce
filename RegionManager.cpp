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

    currLimits = new int[numCells];
    for(int i = 1; i < numCells; i++){
        currLimits[i] = 11;
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

    delete[] currLimits;
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
        currLimits[i] = cellLimits[i];
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

    recursionTimes[10] -= clock();
    if(self.isUnsat()){
        recursionTimes[10] += clock();
        // Self is unsat, therefore all children will also be unsat
        return out;
    }
    recursionTimes[10] += clock();

    for(int cellNum = numCells-1; cellNum >= index; cellNum--){
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

        // Save the cell limit so we can reload it later
        auto cellLimit = currLimits[cellNum];
        for(int limit = 10; limit >= 0; limit--){
            // By going from 10 to 0 instead of 0 to 11 (or i guess 10)
            // we don't need to call push and pop nearly as much
            // though I need to experiment with how much CPU time this saves since now the z3 solver has to
            // figure it out (though i don't doubt that their internals can do it faster than I can)
            recursionTimes[4] -= clock();
            solver.add(cell <= limit);
            recursionTimes[4] += clock();

            // Update the limits
            currLimits[cellNum] = limit;

            // cellNum + 1 because we don't want to manipulate cellNum twice.
            // Note that lastDeduction is always the result of a less-restrictive limitation on this cell
            // Either because the lastDeduction was from when the limit was one greater
            // or because it was from when the limit was 99.
            DeductionManager* recursiveOut = recursive_test(cellNum + 1, lastDeduction);

            recursionTimes[8] -= clock();
            lastDeduction = recursiveOut->getDeduction();
            recursionTimes[8] += clock();

            recursionTimes[9] -= clock();
            if(lastDeduction.isUnsat()){
                recursionTimes[9] += clock();
                // normally this is done when you delete the super-deduction
                // but we don't save this into the super-deduction, so
                delete recursiveOut;
                // Don't save this (aka don't do out->set, there's no valuable data here)
                break;
            }
            recursionTimes[9] += clock();

            recursionTimes[5] -= clock();
            out->set(cellNum, limit, recursiveOut);
            recursionTimes[5] += clock();
        }
        // and reset the currLimits
        currLimits[cellNum] = cellLimit;

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
    stream << "getClockStr() data: \n";
    stream << "getDeduction() time: " << recursionTimes[0] << "\n";
    stream << "new DeductionManager time: " << recursionTimes[1] << "\n";
    stream << "self.isUnsat() time:" << recursionTimes[10] << "\n";
    stream << "[auto cell = ] time: " << recursionTimes[2] << "\n";
    stream << "solver.push() time: " << recursionTimes[3] << "\n";
    stream << "[lastDeduction = self] time: " << recursionTimes[7] << "\n";
    stream << "solver.add() time: " << recursionTimes[4] << "\n";
    stream << "[lastDeduction = recursiveOut->getDeduction()] time: " << recursionTimes[8] << "\n";
    stream << "lastDeduction.isUnsat() time: " << recursionTimes[9] << "\n";
    stream << "out->set() time: " << recursionTimes[5] << "\n";
    stream << "solver.pop() time: " << recursionTimes[6] << "\n";
    stream << "\n";
    stream << "Deduction init time: " << deductionTimes[0] << "\n";
    stream << "[auto cell] time: " << deductionTimes[1] << "\n";
    stream << "[oth] range-finding time: " << deductionTimes[9] << "\n";
    stream << "[oth] known-falsy time: " << deductionTimes[2] << "\n";
    stream << "oth.get() falsy values: " << dataGetFalsy << "\n";
    stream << "oth.get() truthy values: " << dataGetTruthy << "\n";
    stream << "Deduction known-truthy time: " << deductionTimes[3] << "\n";
    stream << "Fast-falsy time: " << deductionTimes[10] << "\n";
    stream << "Fast-falsy: " << fastFalsy << "\n";
    stream << "No fast-falsy: " << noFastFalsy << "\n";
    stream << "Model reduced check calls by: " << modelTruthy << "\n";
    stream << "Model falsy: " << modelFalsy << "\n";
    stream << "[for model in satModels] loop time: " << deductionTimes[8] << "\n";
    stream << "[for model in unsatModels] loop time: " << deductionTimes[10] << "\n";
    stream << "Unsat models caught: " << unsatModelsCaught << "\n";
    stream << "Unsat models known: " << unsatModels.size() << "\n";
    stream << "[auto assumption] time: " << deductionTimes[4] << "\n";
    stream << "solver.check() time: " << deductionTimes[5] << "\n";
    stream << "getAndSaveModel() time: " << deductionTimes[6] << "\n";
    stream << "[int* model] processing time: " << deductionTimes[7] << "\n";
    stream << std::endl;
    return stream;
}

std::ostream &RegionManager::getModels(std::ostream& stream) {
    for(const auto& model:satModels){
        stream << "[";
        for(auto elem : model){
            stream << elem << ", ";
        }
        stream << "]\n";
    }
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

    deductionTimes[8] -= clock();
    // load data from satModels
    for(auto model : satModels){
        bool validModel = true;
        for(int i = 1; i < numCells; i++){
            if(model[i] > currLimits[i]){
                // Not valid for these limits.
                validModel = false;
                break;
            }
        }
        if(validModel){
            // Model is valid for these limits.
            for(int i = 1; i < numCells; i++){
                data.set(i, model[i], true);
            }
        }
    }
    deductionTimes[8] += clock();

    deductionTimes[10] -= clock();
    // Contemplate loading data from unsatModels
    if(data.isUnsat()){
        // See if the currLimits matches with one of the known UNSAT models
        for(auto model : unsatModels){
            bool modelMatches = true;
            for(int i = 1; i < numCells; i++){
                if(model[i] >= currLimits[i]){
                    // the UNSAT model is less than or equally restrictive than this model
                    // so the unsat model has seen more and gotten the same conclusion
                }
                else{
                    // the UNSAT model has seen less. Maybe seeing more will get a different conclusion.
                    modelMatches = false;
                    break;
                }
            }

            if(modelMatches){
                // The current limits matches the UNSAT model.
                // So the current limits are UNSAT
                // so return the deduction
                deductionTimes[10] += clock();
                unsatModelsCaught++;
                return data;
            }
        }
    }
    else{
        // we already know it matches at least one SAT model
        // so it won't match any unsat models
    }
    deductionTimes[10] += clock();

    for(size_t cellNum = 1; cellNum < numCells; cellNum++){
        deductionTimes[1] -= clock();
        auto cell = *cells[cellNum];
        deductionTimes[1] += clock();

        deductionTimes[9] -= clock();
        int rangeMin = oth.getMinMinesInCell(cellNum);
        int rangeMax = oth.getMaxMinesInCell(cellNum); // note that oth.get(cellNum, rangeMax) is TRUE (assuming rangeMax isn't -1)
        deductionTimes[9] += clock();

        // since rangeMax is inclusive, we use <= instead of <
        for(int numMines = rangeMin; numMines <= rangeMax; numMines++){
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

            deductionTimes[10] -= clock();
            auto cellLimit = currLimits[cellNum];
            if(cellLimit < numMines){
                deductionTimes[10] += clock();
                // we're testing if the cell has == numMines mines
                // AND if it has < numMines mines.
                // obviously, falsy.
                fastFalsy++;
                continue;
            }
            deductionTimes[10] += clock();
            noFastFalsy++;

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

                // output some debug stuff
                // if you want to look for fast-falsy cases
                /*
                std::cout << "[";
                for(int i = 1; i < numCells; i++){
                    std::cout << currLimits[i] << ", ";
                }
                std::cout << "]\n";
                //*/

                /*
                int scan[] = {-1, 11, 11, 11, 0, 11, 0, 11};
                bool read = true;
                for(int i = 1; i < 8; i++){
                    if(currLimits[i] != scan[i]){
                        read = false;
                        break;
                    }
                }

                if(read){
                    std::cout << "\n" << solver.unsat_core() << "\n";
                    std::cout << "\n" << solver.assertions() << "\n";
                }
                //*/
            }
            else{
                deductionTimes[6] -= clock();
                auto model = getAndSaveModel();
                deductionTimes[6] += clock();

                deductionTimes[7] -= clock();
                if(model.empty()){
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
    if(data.isUnsat()){
        // Some debug information.
        /*
        std::cout << "[";
        for(int i = 1; i < numCells; i++){
            std::cout << currLimits[i] << ", ";
        }
        std::cout << "]\n";

        int scan[] = {11, 11, 11, 11, 0, 11, 0, 11};
        bool read = true;
        for(int i = 1; i < 8; i++){
            if(scan[i] == 11){
                // doesn't matter
                continue;
            }
            if(currLimits[i] != scan[i]){
                read = false;
                break;
            }
        }

        if(read){
            // std::cout << "\n" << solver.unsat_core() << "\n";
            // std::cout << "\n" << solver.assertions() << "\n";
        }
        */

        // This model is definitely unsat
        // Save this model
        std::vector<int> model(8);
        // This model is represented in currLimits instead of in a z3.model class.
        for(int i = 0; i < 8; i++){
            model[i] = currLimits[i];
        }
        unsatModels.push_back(model);
    }
    return data;
}

std::vector<int> RegionManager::getAndSaveModel() {
    auto solverModel = solver.get_model();
    std::vector<int> model(8);
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
            model.clear();
            return model;
        }
    }

    // Save model.
    satModels.push_back(model);
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
