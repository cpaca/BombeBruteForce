//
// Created on 4/11/2023.
//

#ifndef BOMBEBRUTEFORCE_DEDUCTIONMANAGER_H
#define BOMBEBRUTEFORCE_DEDUCTIONMANAGER_H


#include "Deduction.h"

class DeductionManager {
public:
    explicit DeductionManager(const Deduction& baseDeduction);

    ~DeductionManager();
    DeductionManager operator=(const DeductionManager& oth) = delete;
    DeductionManager(const DeductionManager& oth) = delete;

    void set(int cell, int mines, const Deduction& d);
    void set(int cell, int mines, DeductionManager* d);

    /**
     * Equivalent to repeated callings of self.toLongStr()
     * With extra notes about "this is cell 1", "this is cell 2",
     * and notes about "this is if cell <= k"
     *
     * Note: This function is recursive. And Clang-tidy keeps complaining about it.
     */
    std::string toLongStr(const std::string& pre, const Deduction& parent) const;
    std::string toLongStr() const;
private:
    /**
     * The deduction that this DeducitonManager knows about.
     */
    Deduction self;
    size_t numCells;

    /**
     * The child-deductions of this DeductionManager.
     * children[0] represents cell 0 (ie invalid)
     * children[1] represents cell 1, children[2] represents cell 2
     * etc., with children[n] representing cell n.
     *
     * children[1][0] represents if cell 1 <= 0, children[1][1] represents if cell 1 is <= 1
     * children[1][2] represents if cell 1 <= 2, children[1][3] represents if cell 1 is <= 3
     * etc., with children[c][m] representing what happens if cell [c] can have at most [m] mines.
     *
     * The third pointer is to allow for some Deductions to be set to NULL
     * For example, you might want a NULL DeductionManager if its Deduction is NULL.
     */
    DeductionManager*** children;
};


#endif //BOMBEBRUTEFORCE_DEDUCTIONMANAGER_H
