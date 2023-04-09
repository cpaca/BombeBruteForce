//
// Created on 4/8/2023.
//

#ifndef BOMBEBRUTEFORCE_REGIONMANAGER_H
#define BOMBEBRUTEFORCE_REGIONMANAGER_H


#include "RegionTypes/RegionType.h"

class RegionManager {
public:
    RegionManager(RegionType** regionTypes, size_t numRegions);

    ~RegionManager();
    RegionManager operator=(const RegionManager& oth) = delete;
    RegionManager(RegionManager& oth) = delete;

    // Returns "void" for now so I can just print the output and see what I get
    // but in the future I'll need to port the Deduction class.
    void test(int* cellLimits);
private:
    z3::context ctx;
    z3::solver solver;
    z3::expr** regions;
    size_t size;
    z3::expr** cells;
    /**
     * Note: numCells is actually the number of cells +1.
     * This is because index 0 would be represented by the cell not covered by ANY region
     */
    size_t numCells;
};


#endif //BOMBEBRUTEFORCE_REGIONMANAGER_H
