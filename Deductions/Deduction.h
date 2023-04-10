//
// Created on 4/9/2023.
//

#ifndef BOMBEBRUTEFORCE_DEDUCTION_H
#define BOMBEBRUTEFORCE_DEDUCTION_H


#include <string>

/**
 * This class stores all of the information about a Deduction.
 * Convention:
 * If position (cell, mines) is True, then there COULD be [mines] mines in cell [cell]
 * It's not guaranteed that there is, but there COULD be.
 * If position (cell, mines) is False, then there DEFINITELY IS NOT [mines] mines in cell [cell]
 */
class Deduction {
public:
    /**
     * Equivalent to Deduction(numCells, false)
     * @param numCells The number of cells to keep info about.
     */
    explicit Deduction(size_t numCells);
    Deduction(size_t numCells, bool def);

    ~Deduction();
    Deduction operator=(const Deduction& oth) = delete;
    Deduction(const Deduction& oth);

    bool get(size_t cell, size_t mines) const;
    void set(size_t cell, size_t mines, bool state);

    /**
     * Converts this deduction into a string format.
     * This is called "to long str" because the string is quite long
     * but good for user-readability
     * @return This deduction as a string.
     */
    std::string toLongStr() const;

    /**
     * Basically the same as asking if every possible value of get() will return false
     * If isUnsat(), then get(c, m) will always return false
     * If not isUnsat(), then get(c, m) will return true for at least one input value
     * @return Whether or not this deduction can be considered "UNSAT"
     */
    bool isUnsat() const;
private:
    // Whether Cell C has N mines is a bool
    // So each item of N mines in each cell C is a bool
    // So each set of 0, 1, 2, ... mines in cell C is a bool*
    // So each cell is a bool*
    // So all cells together are a bool**
    bool** cellStates;
    size_t numCells;
};


#endif //BOMBEBRUTEFORCE_DEDUCTION_H
