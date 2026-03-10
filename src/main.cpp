#include "abstract_wfc.hpp"
#include "wfc.hpp"
#include "utils.hpp"
#include <string>


int main(int argc, char** argv){
    srand(std::stoi(argv[1]));

    WFC gen({5,5,1}, {0.5,0.5, 100});
    gen.init();
    gen.init(2, false);

    WFC gen2(gen, {0,0,0}, {3,3,1});
    gen2.init(2, true);

    gen2.run();
    // gen.get_constraints().change_rule(0, UP, 1, false);

    gen.stepped.connect([](WFC* ptr){
        print_array2d(ptr->get_wave());
        std::cout << "========================================" << std::endl;
    });
    gen.run();

}

