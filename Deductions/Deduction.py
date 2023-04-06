class Deduction:
    """
    Stores what you've deduced about the current Region set.
    Tip: Use "None" to store the UNSAT Deduction
    """
    def __init__(self, size: int):
        self.size = size
        self.known = []
        for i in range(size):
            self.known.append([])

    def add(self, cell_num: int, mines: int):
        """
        When you call this function, you state that you are learning the following:
        Cell [cell_num] COULD have [mines] mines in it.
        Not necessarily that it DOES. (Although if it's the only case, then yes, it does have that many mines in it)
        But that it COULD.
        """
        self.known[cell_num].append(mines)

    def __str__(self):
        out = ""
        for i in range(1, self.size):
            out += "Cell " + str(i) + " could have " + str(self.known[i]) + " bombs in it.\n"
        return out
