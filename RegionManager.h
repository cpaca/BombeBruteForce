//
// Created on 4/8/2023.
//

#ifndef BOMBEBRUTEFORCE_REGIONMANAGER_H
#define BOMBEBRUTEFORCE_REGIONMANAGER_H

#include <iostream>

#include "RegionTypes/RegionType.h"
#include "Deductions/Deduction.h"
#include "Deductions/DeductionManager.h"

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

    /**
     * Equivalent (but faster than) recursive test(), applying limits to position [index] and beyond.
     * At index = numCells, simply returns a DeductionManager* with no Deductions in it.
     * @param index The first index to start manipulating.
     * @param check Only check mine-values which are truthy in this Deduction.
     */
    DeductionManager* recursive_test(int index, const Deduction& check, DeductionManager* parentDM);
    DeductionManager* recursive_test(int index);

    /**
     * Gives the list of satModels.
     * Added because I vaguely feel like some satModels are being added twice, which shouldn't be possible.
     */
     std::ostream& getModels(std::ostream& stream);
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

    // vector<vector<int>> since we can't have a vector<int*>
    std::vector<std::vector<int>> satModels;
    std::vector<std::vector<int>> unsatModels;
    int* currLimits;

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

    /**
     * Do NOT run this if the last solver.check() state was UNSAT.
     * @return The current Z3 Model from solver.check()
     */
    std::vector<int> getAndSaveModel();

    static size_t nameToCellNum(const char* name);

    // Some debugging functions so I can call these instead of rewriting the code a dozen times
    bool limitsEquals(const int* compareTo);
    void printLimits();
};


#endif //BOMBEBRUTEFORCE_REGIONMANAGER_H
