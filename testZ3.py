from z3 import *

"""
This file is full of functions that I used to learn how Z3 works.
The examples exist, yes, but actually screwing around and making observations will be good for upscaling.
"""


def deMorgans():
    # From some searching, it seems a default global Context is used in Z3Py
    # That isn't going to be good when I multithread this in the future so I'll use Context to normalize things.
    ctx = Context()
    solver = Solver(ctx=ctx)

    # De Morgan's Law states that NOT(A AND B) = NOT(A) OR NOT(B)
    # If this is always true, then it is never (at least once false)
    # Therefore, NOT(that statement) is UNSAT if De Morgan's Law is true
    A = Bool("A", ctx)
    B = Bool("B", ctx)

    left = Not(And(A, B))
    right = Or(Not(A), Not(B))

    demorgans = (left == right)
    negate = Not(demorgans)

    solver.add(negate)

    result = solver.check()
    if result == unsat:
        print("deMorgans is proven")
        print("More accurately, the negation of deMorgans is always false")
        print("therefore deMorgans must be true")
