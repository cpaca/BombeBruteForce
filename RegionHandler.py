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

        # Code added for optimization reasons
        # Since we can at least take a guess at the max # of mines in all regions we can guess at the max # of mines
        self.max_mines = sum([region.get_max_mines() for region in regions])
        self.max_mines = min(self.max_mines, 99)

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
            self.solver.add(self.cells[i] <= self.max_mines)

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

        # See comments in get_deduction
        # But basically, get_deduction was calling self.cells[i] == j so much that it took about 40% of the CPU time
        # so I had the idea of just making a lookup table for it!
        # table[i][j] is equivalent to self.cells[i] == j
        # Hopefully this isn't THAT unreadable.
        # Contemplating if either the compiler or Python is smart enough to convert this into straight C code...
        self.get_eq_lookup_table: List[Any] = [None]*self.num_cells
        for i in range(1, self.num_cells):
            self.get_eq_lookup_table[i] = [self.cells[i] == j for j in range(11)]

    def get_deduction(self):
        if self.solver.check() == unsat:
            # Save time when we already know.
            return None
        # It may be more efficient to do a binary search instead.
        # I'm not sure. I'm doing it one-at-a-time for now but I'll have to see.
        learned = Deduction(self.num_cells)
        for i in range(1, self.num_cells):
            for j in range(11):
                # from 0 to 10
                result = self.solver.check(self.get_eq_lookup_table[i][j])
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
        learned = self.get_deduction()
        self.solver.pop()
        return learned

    def recursive_test(self, start_index: int):
        """
        Recursively tests cell limits. Note that this only modifies indices start_index and beyond.
        So if you set start_index to 5, it won't touch indices 1-4.
        :param start_index: The cell to touch first.
        :return: A list containing deductions and more lists. For nonzero indices, the list will store either
        the recursive output of changing that digit with numbers or None. Since swapping index 0 is swapping cell 0
        (already known to be nonexistent), we save data by putting this recursive's test into index 0.
        TODO: Put optimizations in to remove recursive tests if they learn nothing vs their parent.
        """
        """
        Implementation note - gets its own block comment because this is LONG.
        This note is done assuming two regions, but should be expandable to any number of regions.
        For two regions, the cell array has four elements. Therefore, this should return a list of 4 elements.
        Ignore index 0 for a bit. Index 1 - aka out[1] - will contain the information as a result of changing out[1].
        Note that out[1] should be a list: out[1][0] will be what happens if the first cell is limited to 0
        And out[1][1] will be what happens if the first cell is limited to 1
        etc.
        Because of the recursion, out[1][1] should be a recursive result. 
        (All of this out[1] stuff is assuming start_index=1; if start_index=2 or more then we shouldn't even touch
        cell #1 and out[1] = None)
        But what if we want to set index 1 to a "?"? Then we manipulate out[1], treat it "as a ?", and set out[2]
        This continues on until the very end - what if we want EVERYTHING to be a ?. 
        Good thing we kept the out[0] available! The information in out[0] will be "What if it was ALL ?s."
        """
        # Read the implNote
        # Note that this time we actually use out[0]
        out: List[Any] = [None] * self.num_cells
        out[0] = self.get_deduction()
        if out[0] is None:
            # Save a TON of time on unnecessary checks.
            return out
        # Now manipulate each index
        for idx in range(start_index, self.num_cells):
            # Note that if, somehow, start_index >= self.num_cells, this does nothing and the function will return.
            learned: List[Any] = [None]*11  # 1 for each number from 0-10
            for max_mines_in_cell in range(11):
                # Since we'll want to be able to undo this, have push/pop ready
                self.solver.push()
                self.solver.add(self.cells[idx] <= max_mines_in_cell)
                # Get information and save it:
                # Note that the recursive starts at idx+1 because we dont want to be touching idx
                # (we just touched it already)
                learned[max_mines_in_cell] = self.recursive_test(idx+1)
                self.solver.pop()
            # Save the learned information into the output:
            out[idx] = learned
        return out



