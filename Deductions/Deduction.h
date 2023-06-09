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
    Deduction& operator=(const Deduction& rhs);
    Deduction(const Deduction& oth);

    bool get(size_t cell, size_t mines) const;
    void set(size_t cell, size_t mines, bool state);

    /**
     * Converts this deduction into a string format.
     * This is called "to long str" because the string is quite long
     * but good for user-readability
     * @return This deduction as a string.
     */
    /**
     * Converts this deduction to a string format.
     * This is called "to long str" because the output string will be quite long, but user-readible.
     * The "pre" variable string is placed before each line: So toLongStr(" ") will output a " " at the start
     * of each line.
     * toLongStr will not return any information that is already given by the "parent".
     * So if the parent knows cell 1 has either [0,1,2], and this Deduction is the same, then toLongStr(..., parent)
     * will not return information about Cell 1
     * @param pre The string to place before each line
     * @return This deduction as a long, user-readible string.
     */
    std::string toLongStr(const std::string& pre, const Deduction& parent) const;
    std::string toLongStr(const std::string& pre) const;
    std::string toLongStr() const;

    /**
     * Basically the same as asking if every possible value of get() will return false
     * If isUnsat(), then get(c, m) will always return false
     * If not isUnsat(), then get(c, m) will return true for at least one input value
     * @return Whether or not this deduction can be considered "UNSAT"
     */
    bool isUnsat() const;

    size_t getNumCells() const {return numCells;};

    // Used for certain optimizations.
    int getMinMinesInCell(size_t cellNum) const;
    int getMaxMinesInCell(size_t cellNum) const;

    uint64_t getCellData(size_t cellNum) const;
    void setCellData(size_t cellNum, uint64_t data);
private:
    // Whether Cell C has N mines is a bool
    // So each item of N mines in each cell C is a bool
    // So each set of 0, 1, 2, ... mines in cell C is a bool*
    // So each cell is a bool*
    // So all cells together are a bool**
    // REFACTOR NOTE:
    // (bool*) can be represented as an unsigned int if you do bit-operations right
    // due to limitations, we can only go to uint_64, but that will only be an issue if we want more than 64 mines
    // in a single cell
    uint64_t* cellStates;
    size_t numCells;
};

Deduction operator&&(const Deduction& lhs, const Deduction& rhs);

#endif //BOMBEBRUTEFORCE_DEDUCTION_H
