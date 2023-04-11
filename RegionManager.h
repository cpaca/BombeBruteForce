//
// Created on 4/8/2023.
//

#ifndef BOMBEBRUTEFORCE_REGIONMANAGER_H
#define BOMBEBRUTEFORCE_REGIONMANAGER_H


#include "RegionTypes/RegionType.h"
#include "Deductions/Deduction.h"

class RegionManager {
public:
    RegionManager(RegionType** regionTypes, size_t numRegions);

    ~RegionManager();
    RegionManager operator=(const RegionManager& oth) = delete;
    RegionManager(RegionManager& oth) = delete;

    // Returns "void" for now so I can just print the output and see what I get
    // but in the future I'll need to port the Deduction class.
    // Equivalent to restrict(cellLimits), printing some debug stuff, then printing getDeduction().toLongStr()
    void test(int* cellLimits);

    // Applies certain limits to the solver
    // More or less used for debugging stuff out.
    // Treats 11s as ?s.
    void restrict(int* cellLimits);
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

    /**
     * Calculates the information available with the system loaded into the solver.
     * @return A deduction with all of the known information.
     */
    Deduction getDeduction();
    /**
     * Calculates the information available with the system loaded into the solver.
     * Note: Will only consider values which are "true" in the deduction.
     * @param oth The deduction to "trim down"
     * @return A deduction with all information true in the input deduction and true in this system.
     */
    Deduction getDeduction(const Deduction& oth);
};


#endif //BOMBEBRUTEFORCE_REGIONMANAGER_H
