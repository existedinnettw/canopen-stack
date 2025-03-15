#include "canopen-stack.h"
#include <vector>
#include <string>

int main() {
    canopen_stack();

    std::vector<std::string> vec;
    vec.push_back("test_package");

    canopen_stack_print_vector(vec);
}
