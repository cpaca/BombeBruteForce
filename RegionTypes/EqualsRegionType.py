from typing import List

from z3 import *

from RegionTypes.RegionType import RegionType


class EqualsRegionType(RegionType):
    """
    The EqualsRegionType is for when the value is exactly equal to a certain number.
    So 2/4 is exactly 2 or exactly 4
    And 3/5/7 is exactly 3, 5, or 7
    Note that =2 and =4 also fits in here, just that it's only = to one number.
    """
    def __init__(self, vals: List):
        self.vals = [int(i) for i in vals]

    def get_expr(self, var: Int):
        return Or([var == val for val in self.vals])
