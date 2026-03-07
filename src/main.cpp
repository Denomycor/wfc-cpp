#include "abstract_wfc.hpp"
#include "wfc.hpp"
#include "utils.hpp"
#include <string>

int main(int argc, char** argv){
    srand(std::stoi(argv[1]));
    WFC gen({5,5,1}, {0.5,0.5});
    gen.init();
    std::cout << gen.get_constraints().get() << std::endl;

    gen.stepped.connect([](WFC* ptr){
        print_array2d(ptr->get_wave());
        std::cout << "========================================" << std::endl;
    });
    gen.run();
}

