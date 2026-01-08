#include <cstdlib>

// Forward declare the assembly entry point
extern "C" int program_startup();

int main() {
    // Call the assembly startup routine
    int result = program_startup();
    return result;
}