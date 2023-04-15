//
// Created on 4/8/2023.
//

#include <sstream>
#include <iostream>
#include <bitset>
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

DeductionManager* RegionManager::recursive_test(int index, const Deduction &check, DeductionManager* parentDM) { // NOLINT(misc-no-recursion)
    Deduction self = getDeduction(check);

    // when recursiveTest is done, parentDM will set this as an element so this we don't need to teach this about its parent
    auto* out = new DeductionManager(self);

    if(self.isUnsat()){
        // Self is unsat, therefore all children will also be unsat
        return out;
    }

    for(int cellNum = numCells-1; cellNum >= index; cellNum--){
        auto cell = *cells[cellNum];
        solver.push();

        // This is equivalent to if the limit on this cell was 99
        auto lastDeduction = self;

        // Save the cell limit so we can reload it later
        auto cellLimit = currLimits[cellNum];
        for(int limit = 10; limit >= 0; limit--){
            // By going from 10 to 0 instead of 0 to 11 (or i guess 10)
            // we don't need to call push and pop nearly as much
            // though I need to experiment with how much CPU time this saves since now the z3 solver has to
            // figure it out (though i don't doubt that their internals can do it faster than I can)
            auto cellCompare = cell <= limit;
            solver.add(cellCompare);

            // Update the limits
            currLimits[cellNum] = limit;

            /*
            int scan[] = {-1, 11, 11, 11, 1, 0, 12, 0};
            if(limitsEquals(scan)){
                std::cout << "Debug this\n";
                std::cout << std::bitset<3>(lastDeduction.getCellData(1)).to_string() << "\n";
                std::cout << lastDeduction.get(1, 0) << "\n";
                std::cout << cellNum + 1 << "\n";
            }
            */

            // Consider the following:
            // Inside all of these for-loops, we would be ...XYZ???
            // Outside the for loops (the "out" variable) would be ...XY????
            // The parent would be ...X?????
            // So if parent knows that ...X?Z??? is UNSAT, we don't need to calculate here
            if(parentDM != nullptr){
                // The ...X?Z??? DM is "superDM"
                auto superDM = parentDM->get(cellNum, limit);
                // I suspect if superDM is nullptr, then it would be unsat
                // then this wouldn't be here becuase it would've been caught by the unsat catcher
                if(superDM != nullptr){
                    // I was wrong.
                    // std::cerr << "You were wrong.\n";
                    auto superDeduction = superDM->getDeduction();
                    lastDeduction = lastDeduction && superDeduction;
                }
            }

            // cellNum + 1 because we don't want to manipulate cellNum twice.
            // Note that lastDeduction is always the result of a less-restrictive limitation on this cell
            // Either because the lastDeduction was from when the limit was one greater
            // or because it was from when the limit was 99.
            DeductionManager* recursiveOut = recursive_test(cellNum + 1, lastDeduction, out);
            /*
            if(limitsEquals(scan)){
                std::cout << recursiveOut->getDeduction().get(1,0) << "\n";
            }
            */

            lastDeduction = recursiveOut->getDeduction();
            if(lastDeduction.isUnsat()){
                // normally this is done when you delete the super-deduction
                // but we don't save this into the super-deduction, so
                delete recursiveOut;
                // Don't save this (aka don't do out->set, there's no valuable data here)
                break;
            }
            out->set(cellNum, limit, recursiveOut);
        }
        // and reset the currLimits
        currLimits[cellNum] = cellLimit;
        solver.pop();
    }

    return out;
}

DeductionManager *RegionManager::recursive_test(int index) { // NOLINT(misc-no-recursion)
    Deduction truthyDeduction = Deduction(numCells, true);
    return recursive_test(index, truthyDeduction, nullptr);
}

std::ostream &RegionManager::getModels(std::ostream& stream) {
    stream << "SAT models: \n";
    for(const auto& model:satModels){
        stream << "[";
        for(auto elem : model){
            stream << elem << ", ";
        }
        stream << "]\n";
    }
    stream << "\nUNSAT models: \n";
    for(const auto& model:unsatModels){
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
    Deduction data(numCells, false);
    // load data from satModels
    for(const auto& model : satModels){
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
    // Contemplate loading data from unsatModels
    if(data.isUnsat()){
        // See if the currLimits matches with one of the known UNSAT models
        for(const auto& model : unsatModels){
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
                return data;
            }
        }
    }
    else {
        // we already know it matches at least one SAT model
        // so it won't match any unsat models
    }

    for(size_t cellNum = 1; cellNum < numCells; cellNum++){
        auto cell = *cells[cellNum];

        // Only consider numMines if:
        // They're "true" in the parent.
        //   If they weren't true in the parent, they won't be true in the (more restrictive) child
        // They're "false" in the current-data
        //   If they were true in the current-data, they've already been considered (possibly from models)
        uint64_t parentMinesToConsider = oth.getCellData(cellNum);
        uint64_t selfMinesToNotConsider = data.getCellData(cellNum);
        uint64_t minesToConsider = parentMinesToConsider & (~selfMinesToNotConsider);

        // since rangeMax is inclusive, we use <= instead of <
        int numMines = -1; // start at -1 since we instantly increment to 0.
        while(true){
            numMines++;
            if(minesToConsider < (1 << numMines)){
                // nothing else to consider
                break;
            }
            if((minesToConsider & (1 << numMines)) == 0){
                // do not consider mines at position numMines
                continue;
            }
            auto cellLimit = currLimits[cellNum];
            if(cellLimit < numMines){
                // we're testing if the cell has == numMines mines
                // AND if it has < numMines mines.
                // obviously, falsy.
                continue;
            }
            auto assumption = cell == numMines;
            auto result = solver.check(1, &assumption);
            // If result is UNSAT then it is not possible for cell to have numMines mines
            // If result is NOT UNSAT then it is NOT (not possible for cell to have numMines mines)
            // If result is NOT UNSAT then it is possible for cell to have numMines mines
            if(result == unsat){
                // If result is UNSAT then it is not possible for cell to have numMines mines
                // Commented out because data now defaults to falsy
                // data.set(cellNum, numMines, false);

                // output some debug stuff
                // if you want to look for fast-falsy cases
                // std::cout << "UNSAT if cell " << cellNum << " has " << numMines << " mines: ";
                // printLimits();
            }
            else{
                const auto& model = getAndSaveModel();
                if(model.empty()){
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
            }
        }
    }
    if(data.isUnsat()){
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

bool RegionManager::limitsEquals(const int* compareTo) {
    // use 12s for ?s in this function so I can search for certain situations much easier
    for(int i = 1; i < numCells; i++){
        if(compareTo[i] == 12){
            continue;
        }
        if(currLimits[i] != compareTo[i]){
            return false;
        }
    }
    return true;
}

void RegionManager::printLimits() {
    std::cout << "[";
    for(int i = 1; i < numCells; i++){
        std::cout << currLimits[i] << ", ";
    }
    std::cout << "]\n";
}
