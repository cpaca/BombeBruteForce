//
// Created on 4/8/2023.
//

#ifndef BOMBEBRUTEFORCE_EQUALSREGIONTYPE_H
#define BOMBEBRUTEFORCE_EQUALSREGIONTYPE_H


#include "RegionType.h"

class EqualsRegionType : public RegionType{
public:
    /**
     * Same as EqualsRegionType(&validCount, 1)
     * Used for if exactly 1 mine is in the region, or exactly 2 mines, or whatever
     * @param validCount The number of mines this region has
     */
    explicit EqualsRegionType(int validCount);
    /**
     * Sets this region to have a number of mines exactly equal to one of the values in validCounts.
     * So if validCounts is [1, 3, 5], then the getExpr of this region will return that this has either exactly 1 mine,
     * exactly 3 mines, or exactly 5 mines, but no other value.
     * @param validCounts The numbers of valid mines.
     * @param arrSize The size of the validCounts array.
     */
    EqualsRegionType(const int validCounts[], size_t arrSize);

    ~EqualsRegionType();
    EqualsRegionType operator=(const EqualsRegionType& oth) = delete;
    EqualsRegionType(EqualsRegionType& oth) = delete;

    z3::expr getExpr(const z3::expr& var) override;
private:
    int* validCounts{}; // Can't be 0 or null terminated since it's possible this could be fed a 0/2 region
    size_t countSize{};
};


#endif //BOMBEBRUTEFORCE_EQUALSREGIONTYPE_H
