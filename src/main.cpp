#include <iostream>
#include "wfc.hpp"

int main(int argc, char** argv){
    srand(std::stoi(argv[1]));

    WFC3D gen(8, 8, 1, {{1,1}, {2,99}});
    gen.add_constraint_allow_all(1);
    gen.add_constraint_allow_all(2);
    gen.add_constraint_allow_all(3);
    gen.init();

    WFC3D gen2(gen, {1,1,0}, {3,3,1}, {{1,99}, {2,1}, {3,100}});
    gen2.add_constraint_allow_all(1);
    gen2.add_constraint_allow_all(2);
    gen2.add_constraint_allow_all(3);

    gen2.run();

    gen.stepped.connect([](WFC3D* ptr){
        ptr->print2D();
        std::cout << "===============================" << std::endl;
    });

    gen.run();
}

