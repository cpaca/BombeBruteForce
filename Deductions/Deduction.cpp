//
// Created on 4/9/2023.
//

#include <sstream>
#include <iostream>
#include "Deduction.h"

Deduction::Deduction(size_t numCells) : Deduction(numCells, false) {

}

Deduction::Deduction(size_t numCells, bool def) :
    numCells(numCells) {
    cellStates = new bool*[numCells];
    cellStates[0] = nullptr; // No data for the 0 cell
    for(int i = 1; i < numCells; i++){
        bool* cell = new bool[11];
        for(int j = 0; j < 11; j++){
            cell[j] = def; // not sure if this is necessary
        }
        cellStates[i] = cell;
    }
}

Deduction::~Deduction() {
    for(int i = 0; i < numCells; i++){
        delete[] cellStates[i];
    }
    delete[] cellStates;
}

Deduction& Deduction::operator=(const Deduction &rhs) {
    if(&rhs == this){
        return *this;
    }
    if(numCells != rhs.numCells){
        throw std::invalid_argument("RHS of operator= doesn't have the same number of cells.");
    }
    for(int cellNum = 1; cellNum < numCells; cellNum++){
        for(int limit = 0; limit < 11; limit++){
            cellStates[cellNum][limit] = rhs.cellStates[cellNum][limit];
        }
    }
    return *this;
}

Deduction::Deduction(const Deduction &oth) :
    numCells(oth.numCells) {

    cellStates = new bool*[numCells];
    cellStates[0] = nullptr; // No data for the 0 cell
    for(int i = 1; i < numCells; i++){
        bool* cell = new bool[11];
        for(int j = 0; j < 11; j++){
            cell[j] = oth.get(i,j);
        }
        cellStates[i] = cell;
    }
}

bool Deduction::get(size_t cell, size_t mines) const {
    return cellStates[cell][mines];
}

void Deduction::set(size_t cell, size_t mines, bool state) {
    cellStates[cell][mines] = state;
}

std::string Deduction::toLongStr(const std::string& pre, const Deduction& parent) const {
    std::stringstream out;

    for(int cell = 1; cell < numCells; cell++){
        bool haveInformation = false;
        for(int mines = 0; mines < 11; mines++){
            if(parent.get(cell, mines) != this->get(cell, mines)){
                haveInformation = true;
                break;
            }
        }
        if(!haveInformation){
            // no new information over parent
            // at least, for this cell.
            continue;
        }

        out << pre;
        out << "Possible mine values for cell " << cell << ": [";
        for(int j = 0; j < 11; j++){
            if(get(cell, j)){
                // there are j mines in [cell]
                out << j << ", ";
            }
        }
        out << "]" << std::endl;
    }

    return out.str();
}

std::string Deduction::toLongStr(const std::string& pre) const {
    // there's probably a faster/more efficient way to write this
    // but I think this is the only way to do it that avoids code reuse
    // and also avoids taking extra cpu time in toLongStr(pre, parent) cases
    return toLongStr("", Deduction(numCells, true));
}

std::string Deduction::toLongStr() const {
    return toLongStr("");
}

bool Deduction::isUnsat() const {
    for(int i = 1; i < numCells; i++){
        for(int j = 0; j < 11; j++){
            if(get(i,j)){
                // true for at least one value
                return false;
            }
        }
    }
    // true for no values
    return true;
}

int Deduction::getMinMinesInCell(size_t cellNum) const {
    for(int i = 0; i < 11; i++){
        if(get(cellNum, i)){
            // there could be this many mines in this cell
            // and we know all lesser-mine-counts aren't possible
            return i;
        }
    }
    // no mine counts are possible at all
    // possibly a result of "remove redundant deductions"? idk
    return -1;
}

int Deduction::getMaxMinesInCell(size_t cellNum) const {
    for(int i = 10; i >= 0; i--){
        if(get(cellNum, i)){
            return i;
        }
    }
    // no mine counts are possible at all
    // possibly a result of "remove redundant deductions"? idk
    return -1;
}
