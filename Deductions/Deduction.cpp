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
    cellStates = new uint64_t[numCells];
    uint64_t setTo = 0;
    if(def){
        // set them all to 1 (mine exists here) instead
        setTo = ~setTo;
        setTo &= (1 << 11)-1; // we don't record information about 11 mines and beyond
    }
    for(int i = 1; i < numCells; i++){
        cellStates[i] = setTo;
    }
}

Deduction::~Deduction() {
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
        cellStates[cellNum] = rhs.cellStates[cellNum];
    }
    return *this;
}

Deduction::Deduction(const Deduction &oth) :
    numCells(oth.numCells) {

    cellStates = new uint64_t[numCells];
    for(int i = 1; i < numCells; i++){
        cellStates[i] = oth.cellStates[i];
    }
}

bool Deduction::get(size_t cell, size_t mines) const {
    // if it's 0 then no mine (return 0 != 0 -> false)
    // if it's 1 then mine (return 1 != 0 -> true)
    return (cellStates[cell] & (1 << mines)) != 0;
}

void Deduction::set(size_t cell, size_t mines, bool state) {
    uint64_t offset = 1 << mines;
    if(state){
        // set it to 1
        cellStates[cell] |= offset;
    }
    else{
        // set it to 0
        // XXXXX & ~(00100) = XXXXX & 11011 = XX0XX
        cellStates[cell] &= ~offset;
    }
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
    return 0;
}

int Deduction::getMaxMinesInCell(size_t cellNum) const {
    for(int i = 10; i >= 0; i--){
        if(get(cellNum, i)){
            return i;
        }
    }
    // no mine counts are possible at all
    // possibly a result of "remove redundant deductions"? idk
    return 10;
}

uint64_t Deduction::getCellData(size_t cellNum) const {
    return cellStates[cellNum];
}

void Deduction::setCellData(size_t cellNum, uint64_t data) {
    cellStates[cellNum] = data;
}

Deduction operator&&(const Deduction &lhs, const Deduction &rhs) {
    if(lhs.getNumCells() != rhs.getNumCells()){
        return {lhs.getNumCells(), false}; // can't combine, all-false
    }
    Deduction out(lhs);
    for(int i = 1; i < lhs.getNumCells(); i++){
        out.setCellData(i, out.getCellData(i) & rhs.getCellData(i));
    }
    return out;
}
