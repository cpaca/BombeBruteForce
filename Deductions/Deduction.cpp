//
// Created on 4/9/2023.
//

#include <sstream>
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

bool Deduction::get(size_t cell, size_t mines) {
    return cellStates[cell][mines];
}

void Deduction::set(size_t cell, size_t mines, bool state) {
    cellStates[cell][mines] = state;
}

std::string Deduction::toLongStr() {
    std::stringstream out;

    for(int i = 1; i < numCells; i++){
        out << "Possible mine values for cell " << i << ": [";
        for(int j = 0; j < 11; j++){
            if(get(i,j)){
                // there are j mines in cell i
                out << j << ", ";
            }
        }
        out << "]" << std::endl;
    }

    return out.str();
}
