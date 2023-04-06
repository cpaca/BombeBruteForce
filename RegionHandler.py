from typing import List, Any

from z3 import *

from Deductions.Deduction import Deduction
from RegionTypes.RegionType import RegionType

"""
RegionHandler class.
This class takes in a set of regions and initializes as much information as possible.
(Actually, it initializes everything except max values for each cell)
"""


class RegionHandler:
    def __init__(self, regions: List[RegionType]):
        # Port of SECT 0 in testZ3.py
        self.ctx = Context()
        self.solver = Solver(ctx=self.ctx)

        # Port of SECT 1 in testZ3.py
        self.size = len(regions)
        self.regions = [None]*len(regions)
        for i in range(len(self.regions)):
            self.regions[i] = Int(chr(65+i), self.ctx)

        # Port of SECT 2 and SECT 3  in testZ3.py
        self.num_cells = 2**self.size
        self.cells = [None] * self.num_cells
        region_sums = self.regions[:]
        for i in range(1, len(self.cells)):
            # range starts at 1 so dont need to skip 0
            name = ""
            cell_regions = []
            # Note that SECT 2 uses chr(97), then chr(98), etc.
            # also note SECT 3 uses index 0, then index 1, etc.
            loop_num = 0
            # Don't modify i directly because we still need self.cells[i] = Int(name, ctx)
            num = i
            while num > 0:
                # This ports the if i // n % 2 but:
                # A) As a while loop so we can do 4 regions or 5 regions... in theory
                # (In practice you need NASA supercomputers for 4-regions and
                #    most likely some form of quantum computer which can do SAT for 5-regions)
                if num % 2:
                    name += chr(97 + loop_num)
                    # Record what regions this cell is apart of
                    # There's probably a more time-efficient way to do this
                    # involving invoking C-code
                    # but I'm gonna avoid premature optimization.
                    cell_regions += [loop_num]
                # Then check the next bit
                num //= 2
                loop_num += 1

            # Finalize info
            self.cells[i] = Int(name, ctx=self.ctx)
            # While we're in this for loop let's also do the cell restriction
            # Can't have negative mines in a cell.
            self.solver.add(self.cells[i] >= 0)
            # The 99 limit is a fill-in for "?" but will hopefully be detected by the strategy
            # and deleted if a stronger restriction comes in.
            self.solver.add(self.cells[i] <= 99)

            # Instead of setting region = cell + cell + cell ...
            # Do region - cell - cell - cell ... = 0
            for j in cell_regions:
                region_sums[j] -= self.cells[i]

        # and finally set all the region sums to 0
        for s in region_sums:
            self.solver.add(s == 0)

        # SECT 4
        for i in range(self.size):
            self.solver.add(regions[i].get_expr(self.regions[i]))

    def get_deduction(self):
        # It may be more efficient to do a binary search instead.
        # I'm not sure. I'm doing it one-at-a-time for now but I'll have to see.
        learned = Deduction(self.num_cells)
        for i in range(1, self.num_cells):
            for j in range(11):
                # from 0 to 10
                result = self.solver.check(self.cells[i] == j)
                # If it is SAT, then it is possible to have J mines in cell I
                # If it is UNSAT, then it is not possible to have J mines in cell I.
                if result != unsat:
                    # either sat or unknown, whatever
                    learned.add(i, j)
        return learned

    def test_cells(self, cell_limits: List[int]):
        self.solver.push()
        if len(cell_limits) != self.num_cells:
            raise ArgumentError
        for i in range(1, self.num_cells):
            limit = cell_limits[i]
            if limit == 11:
                continue  # Let 11 fill in for "?"
            self.solver.add(self.cells[i] <= cell_limits[i])
        if self.solver.check() == unsat:
            return None
        learned = self.get_deduction()
        self.solver.pop()
        return learned

    def recursive_test(self, cell_limits: List[int]):
        """
        Recursively tests a set of cell_limits. Note that if ??a?b?? is input, only the ?'s after the b will be
        swapped to integers and recursively tested. This is because we eventually get ??a?b?c, and if
        R(??a?b?c) modified the ? between b and c, it would be repeating something that ??a?b?? alreaady did.
        :param cell_limits: The limits of each cell.
        :return: A list containing deductions and more lists. For nonzero indices, the list will store either
        the recursive output of changing that digit with numbers or None. Since swapping index 0 is swapping cell 0
        (already known to be nonexistant), we save data by putting this recursive's test into index 0.
        TODO: Put optimizations in to remove recursive tests if they learn nothing vs their parent.
        """
        # Prepare output.
        out: List[Any] = [None]*self.num_cells
        # First, test the given set of cells.
        out[0] = self.test_cells(cell_limits)
        # Then, perform recursion:
        # Find the index of the last non-11 element of the list.
        # For reasons described in the pydoc, we don't want to modify elements before or including that one.
        # https://stackoverflow.com/a/41346563
        # Note that this is guaranteed to not throw an error since the standard is for cell_limits[0] to be 0.
        # so worst-case this will return 0
        last_index = [idx for idx, elem in enumerate(cell_limits) if elem != 11][-1]
        # Increment by 1 - last_index now represents the first index we can start modifying the values of.
        last_index += 1
        for idx in range(last_index, self.num_cells):
            # Note that if the recursive_test is all ??s, last_index is 1 here so we start modifying 1
            # Note that if the last element of recursive_test is not a ??, last_index = self_num_cells
            # and the range iterates over zero objects
            # Much more important note: cell_limits[idx] is always a ?.
            learned = [None]*11
            for num_mines in range(11):
                cell_limits[idx] = num_mines
                learned[num_mines] = self.recursive_test(cell_limits)
            # Reset back to a ?
            # This way we don't create and feed a clone of cell_limits into every function
            cell_limits[idx] = 11
            # Save the knowledge
            out[idx] = learned

        return out


