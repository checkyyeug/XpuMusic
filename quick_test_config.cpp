#include "foobar_plugin_manager.h"
#include <iostream>

int main() {
    FoobarPluginManager manager;
    manager.initialize("quick_test_config.json");

    // Test setting various parameter types
    manager.set_plugin_parameter("test_plugin", "string_value", std::string("hello world"));
    manager.set_plugin_parameter("test_plugin", "int_value", 42);
    manager.set_plugin_parameter("test_plugin", "double_value", 3.14159);
    manager.set_plugin_parameter("test_plugin", "bool_value", true);
    manager.set_plugin_enabled("test_plugin", false);

    manager.save_configuration();

    std::cout << "Configuration saved to quick_test_config.json" << std::endl;
    return 0;
}