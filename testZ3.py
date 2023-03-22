from z3 import *

"""
This file is full of functions that I used to learn how Z3 works.
The examples exist, yes, but actually screwing around and making observations will be good for upscaling.
"""


def de_morgans():
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


def situation_test():
    """
    Situation testing.
    This is testing the following situation:
    1   1
    7 5 7
    8 6 8 0 2
    (The nonzero cell numbers are arbitrary, for explanation purposes. This works even if they're all ?s)
    (?s will be accounted for by analyzing the outputs)
    Observations:
    If the 6 is a bomb, then the 8s cannot be bombs (1= regions are filled)
    But then 2 is unfilled. Therefore, the 6 is clear.
    Additionally, if the 5 is a bomb, the 8s cannot be bombs for the same reason
    The same is true if any of the 7s are bombs
    Therefore, the 8s are the only bombs

    For generality reasons, instead of naming the regions 1 and 2 in the code, we name them A, B, and C
    And we have the chart as:
     A  ...  B
    a   ab   b
    a c abc  bc  c - C
    a is how many bombs are in cell a
    A is how many bombs are in region A
    Observe:
    A = a+ab+ac+abc
    B = ab+b+abc+bc
    C = ac+abc+bc+c
    :return:
    """
    ctx = Context()
    solver = Solver(ctx=ctx)

    # I'm observing that I don't feel like typing A = Int("A") for... 3 regions+7 cells
    # I've also already learned a lot from just examining the problem before getting Z3 working
    # So there's a lot of "generalization" going on here.
    regions = [None]*3
    for i in range(len(regions)):
        # 65 is A, 66 is B, etc
        regions[i] = Int(chr(65+i), ctx)

    # It's easier to have 0b001 be a, 0b010 be b, 0b011 be c, etc.
    # byproduct is that 0b000 is a NULL cell that isn't in any regions.
    cells = [None]*(2**3)
    for i in range(len(cells)):
        if i == 0:
            continue
        name = ""
        # the //1 is unnecessary but cleans up for the later stuff well
        if i // 1 % 2:
            # 0bXX1
            name += chr(96+1)
        if i // 2 % 2:
            # 0bX1X
            name += chr(96+2)
        if i // 4 % 2:
            # 0b1XX
            name += chr(96+3)
        cells[i] = Int(name, ctx)

    # Next: Region addition
    # A =   a  + ab  +  ac
    # A = 0b001+0b011+0b101, etc.
    regionSums = [None]*3
    # Thinking through, when I implement this it might be better to merge it with the above for-loop
    # but keeping it separate for testing purposes is better, since I don't need it to go fast for now.
    for i in range(len(cells)):
        if i // 1 % 2:
            regionSums[0] = sum_regions(cells[i], regionSums[0])
        if i // 2 % 2:
            regionSums[1] = sum_regions(cells[i], regionSums[1])
        if i // 4 % 2:
            regionSums[2] = sum_regions(cells[i], regionSums[2])

    # Tell the solver about this restriction:
    for i in range(len(regions)):
        solver.add(regions[i] == regionSums[i])

    # Also tell the solver about the cell restriction
    # Upper-bounded at 10 because the final product won't solve the ?s
    # Lower-bounded at 0 because obviously can't have negative mines in one spot
    # This could probably be merged into the cells for-loop
    for cell in cells:
        if cell is None:
            continue
        solver.add(cell >= 0)
        solver.add(cell <= 10)

    # Finally - and this is just for this particular situation -
    # have c = 0, A=1, B=1, C=2
    solver.add(cells[0b100] == 0)
    solver.add(regions[0] == 1)
    solver.add(regions[1] == 1)
    solver.add(regions[2] == 2)
    # Have this so we can go back to it.
    solver.push()

    # Check if this is possible.
    print("Base region situation: " + str(solver.check()))
    # Now prove that a, ab, b, and abc are clear
    # If they are clear, then them being != 0 is UNSAT
    print("If a is not-clear: " + str(solver.check(cells[0b001] != 0)))

    # Proving that assumptions only stay true for one exec
    print("Return to push(): " + str(solver.check()))

    # Won't bother with b, but I'll still do ab and abc
    print("If ab is not-clear: " + str(solver.check(cells[0b011] != 0)))
    print("If abc is not-clear: " + str(solver.check(cells[0b111] != 0)))

    # Similarly, I guess we could prove ac and bc have exactly one bomb in them
    print("If ac has != 1 bomb in it: " + str(solver.check(cells[0b101] != 1)))


def sum_regions(a, b):
    if a is None:
        return b
    if b is None:
        return a
    return a+b

