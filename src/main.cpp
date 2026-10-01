#include "VulkanEngine.hpp"
#include <iostream>

int main() {
    VulkanEngine engine;
    
    try {
        engine.init();
        engine.run();
        engine.cleanup();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
