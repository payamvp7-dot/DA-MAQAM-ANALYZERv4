#include <iostream>
extern bool run_passthrough_tests();
extern bool run_yin_tests();
extern bool run_database_tests();

int main()
{
    int failures=0;
    auto run = [&](const char* name, bool (*fn)()){
        std::cout << "[TEST] " << name << " ... " << std::flush;
        bool ok = fn();
        std::cout << (ok ? "PASS" : "FAIL") << "\n";
        if(!ok) ++failures;
    };
    run("passthrough", run_passthrough_tests);
    run("yin", run_yin_tests);
    run("database", run_database_tests);
    std::cout << "\n" << (failures ? "FAILURES: " : "ALL PASS: ") << failures << "\n";
    return failures ? 1 : 0;
}
