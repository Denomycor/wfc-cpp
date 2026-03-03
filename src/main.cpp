#include <cfloat>
#include <cstdlib>
#include <iostream>
#include <string>
#include "wfc.hpp"

int main(int argc, char** argv){
    srand(std::stoi(argv[1]));
    WFC2D gen(5, 5, {{1,50}, {2,50}});
    gen.add_constraint(1, {1,2}, {1,2}, {1,2}, {1,2});
    gen.add_constraint(2, {1,2}, {1,2}, {1,2}, {1,2});
    gen.stepped.connect([](WFC3D* ptr){
        dynamic_cast<WFC2D*>(ptr)->print2D();
        std::cout << "===============================" << std::endl;
    });
    gen.init();
    gen.run();
}

