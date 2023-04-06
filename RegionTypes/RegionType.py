from z3 import Int


class RegionType:
    """
    RegionType handles the number of bombs in each region.
    For example, an Exactly-3 region must have exactly 3 bombs in it.
    A 3/5/7 can have either exactly 3, exactly 5, or exactly 7 bombs in it.
    Etc. I suggest playing Bombe itself for more info.
    """

    def get_expr(self, var: Int):
        """
        Convert this RegionType into a Z3 expression (using the z3 variable)
        So an =2 region, when fed variable "A", will return A==2
        But with variable "B", will return B==2
        Not implemented in RegionType, meant to be implemented by subclasses.
        :return:
        """
        raise NotImplementedError
