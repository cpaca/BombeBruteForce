from typing import List

from z3 import *

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
        # TODO: Get this information from RegionType instead of being hardcoded
        # (It's hardcoded for now to make sure this works.)
        self.solver.add(self.regions[0] == 1)
        self.solver.add(self.regions[1] == 1)
        self.solver.add(self.regions[2] == 2)

    def test_cells(self, cell_limits: List[int]):
        self.solver.push()
        if len(cell_limits) != self.num_cells:
            raise ArgumentError
        for i in range(1, self.num_cells):
            limit = cell_limits[i]
            if limit == 11:
                continue  # Let 11 fill in for "?"
            self.solver.add(self.cells[i] <= cell_limits[i])
        # It may be more efficient to do a binary search instead.
        # I'm not sure. I'm doing it one-at-a-time for now but I'll have to see.
        for i in range(1, len(cell_limits)):
            results = [None]*11
            for j in range(11):
                # from 0 to 10
                results[j] = self.solver.check(self.cells[i] == j)
                # If it is SAT, then it is possible to have J mines in cell I
                # If it is UNSAT, then it is not possible to have J mines in cell I.
            print(i, results)
        self.solver.pop()
