//
// Created by cppac on 4/8/2023.
//

#include "EqualsRegionType.h"

// I didn't like having this in the header file even if it's an empty code block.
EqualsRegionType::EqualsRegionType(int validCount) : EqualsRegionType(&validCount, 1) {

}

EqualsRegionType::EqualsRegionType(const int *validCounts, size_t arrSize) : countSize(arrSize){
    if(countSize == 0){
        throw std::invalid_argument("EqualsRegionType needs to be equal to at least one number.");
    }
    this->validCounts = new int[arrSize];
    for(size_t i = 0; i < countSize; i++){
        this->validCounts[i] = validCounts[i];
    }
}

EqualsRegionType::~EqualsRegionType() {
    delete[] validCounts;
}

z3::expr EqualsRegionType::getExpr(const z3::expr& var) {
    // Constructor guarantees that countSize will always be nonzero
    z3::expr out = (var == this->validCounts[0]);
    for(size_t i = 1; i < countSize; i++){
        out = out || (var == this->validCounts[i]);
    }
    return out;
}
