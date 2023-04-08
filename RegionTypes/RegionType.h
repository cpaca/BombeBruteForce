//
// Created on 4/8/2023.
//

#ifndef BOMBEBRUTEFORCE_REGIONTYPE_H
#define BOMBEBRUTEFORCE_REGIONTYPE_H


#include <string>
#include <z3++.h>

class RegionType {
public:
    /**
     * Constructs an expression that matches this region, from the [var].
     * For example, a 3+ region always has 3 or more mines, so it should return (var >= 3)
     * @param var The Z3 Expression that names this region
     * @return An expression that makes the region always true.
     */
    virtual z3::expr getExpr(const z3::expr& var) = 0;
    /**
     * Returns the maximum # of mines that can be in this Region.
     * Used for certain optimizations, but as a side-effect will never let a region have more than 99 mines.
     */
    virtual int getMaxMines();
};


#endif //BOMBEBRUTEFORCE_REGIONTYPE_H
