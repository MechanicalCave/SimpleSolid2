#include <simplesolid2/kernel/evidence.hpp>

#include <cstdlib>
#include <iostream>

int main() {
    using simplesolid2::kernel::ReferenceStatus;

    if (ReferenceStatus::resolved ==
        ReferenceStatus::ambiguous) {
        return EXIT_FAILURE;
    }

    std::cout
        << "PM00A_E02_REGISTRATION_PASS\n";
    return EXIT_SUCCESS;
}
