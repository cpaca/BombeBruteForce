#include <iostream>
#include <sstream>
#include<vector>
#include <fstream>
#include"z3++.h"
#include "RegionTypes/RegionType.h"
#include "RegionTypes/EqualsRegionType.h"
#include "RegionManager.h"

using namespace z3;

/**
 * This demo was taken from the Z3 C++ Example project.
 * The Z3 C++ Example Project is available online at this URL:
 * https://github.com/Z3Prover/z3/blob/master/examples/c%2B%2B/example.cpp
   Demonstration of how Z3 can be used to prove validity of
   De Morgan's Duality Law: {e not(x and y) <-> (not x) or ( not y) }
*/
void demorgan() {
    std::cout << "de-Morgan example\n";

    context c;

    expr x = c.bool_const("x");
    expr y = c.bool_const("y");
    expr conjecture = (!(x && y)) == (!x || !y);

    solver s(c);
    // adding the negation of the conjecture as a constraint.
    s.add(!conjecture);
    std::cout << s << "\n";
    std::cout << s.to_smt2() << "\n";
    switch (s.check()) {
        case unsat:   std::cout << "de-Morgan is valid\n"; break;
        case sat:     std::cout << "de-Morgan is not valid\n"; break;
        case unknown: std::cout << "unknown\n"; break;
    }
}

int main() {
    std::cout << "Hello, World!" << std::endl;

    // demorgan();

    auto types = new RegionType*[3];
    types[0] = new EqualsRegionType(1);
    types[1] = new EqualsRegionType(1);
    types[2] = new EqualsRegionType(2);

    RegionManager manager(types, 3);

    int* limits = new int[8] {0, 11, 11, 11, 11, 11, 11, 11};
    // manager.restrict(limits);

    std::cout << "Starting recursion." << std::endl;
    uint64_t startTime = clock();
    auto results = manager.recursive_test(3);
    uint64_t stopTime = clock();

    // output detailed information to a file
    std::ofstream out("../details.txt");
    out << results->toLongStr();

    std::cout << "Total time taken: " << stopTime - startTime << std::endl;

    manager.getModels(std::cout);

    delete[] limits;

    for(size_t i = 0; i < 3; i++){
        delete types[i];
    }
    delete[] types;

    return 0;
}
