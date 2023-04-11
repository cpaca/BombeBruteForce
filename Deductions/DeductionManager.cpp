//
// Created on 4/11/2023.
//

#include <sstream>

#include "DeductionManager.h"

DeductionManager::DeductionManager(const Deduction& baseDeduction) :
    self(baseDeduction),
    numCells(self.getNumCells()){

    children = new DeductionManager**[numCells];

    children[0] = nullptr; // cell 0 is always invalid
    for(size_t cellNum = 1; cellNum < numCells; cellNum++){
        auto cellManagers = new DeductionManager*[11];
        for(size_t numMines = 0; numMines < 11; numMines++){
            cellManagers[numMines] = nullptr;
        }
        children[cellNum] = cellManagers;
    }
}

DeductionManager::~DeductionManager() {
    // cell 0 was set to null so no need to delete
    for(size_t cell = 1; cell < numCells; cell++){
        // clean up any lingering pointers in here
        for(size_t mines = 0; mines < 11; mines++){
            // had to double check but deleting nullpointer is safe so this either deletes a deductionmanager
            // or "deletes a nullptr" ie does nothing
            delete children[cell][mines];
        }
        delete[] children[cell];
    }
    delete[] children;
}

void DeductionManager::set(int cell, int mines, const Deduction& d) {
    if(d.isUnsat()){
        // don't save an unsat deduction
        return;
    }
    children[cell][mines] = new DeductionManager(d);
}

void DeductionManager::set(int cell, int mines, DeductionManager* d){
    // don't even run checks just override the existing DeductionManager.
    children[cell][mines] = d;
}

std::string DeductionManager::toLongStr(const std::string &pre, const Deduction &parent) const { // NOLINT(misc-no-recursion)
    std::stringstream out;
    bool printed = false;

    auto selfData = self.toLongStr(pre + " ", parent);
    if(!selfData.empty()){
        out << pre << "Self result:\n";
        out << selfData;
        out << pre << "End of self result" << std::endl;
        printed = true;
    }
    for(int cellNum = 1; cellNum < numCells; cellNum++){
        auto cell = children[cellNum];
        std::stringstream cellOut; // haha, pronounced "sellout"
        bool cellHasData = false;

        cellOut << pre << "Information for cell " << cellNum << ": \n";
        for(int limit = 0; limit < 11; limit++){
            auto limitData = cell[limit];
            if(limitData != nullptr) {
                auto limitDataStr = limitData->toLongStr(pre + " " + " ", self);
                if(!limitDataStr.empty()) {
                    cellOut << pre << " Information if limit <= " << limit << "\n";
                    cellOut << limitDataStr;
                    cellOut << pre << " End of information if limit <= " << limit << "\n";
                    cellHasData = true;
                }
            }
        }
        cellOut << pre << "End of information for cell " << cellNum << std::endl;

        if(cellHasData){
            if(!printed){
                out << pre << "Self has no special data.";
                printed = true;
            }
            out << cellOut.str();
        }
    }
    return out.str();
}

std::string DeductionManager::toLongStr() const {
    return toLongStr("", Deduction(numCells, true));
}
