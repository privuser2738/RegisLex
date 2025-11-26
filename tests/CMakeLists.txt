# RegisLex - Test Suite CMake Configuration

# Test executable
add_executable(regislex_tests
    test_main.c
)

# Link against the core library if needed
# For now, tests are self-contained
# target_link_libraries(regislex_tests regislex_core)

# Platform-specific test dependencies
if(WIN32)
    target_link_libraries(regislex_tests ws2_32)
endif()

# Register tests with CTest
add_test(NAME RegisLexUnitTests COMMAND regislex_tests)

# Set test properties
set_tests_properties(RegisLexUnitTests PROPERTIES
    TIMEOUT 60
    LABELS "unit"
)
