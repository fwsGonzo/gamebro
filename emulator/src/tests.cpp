#include <libgbc/machine.hpp>
using namespace gbc;

inline void execute_n(gbc::Machine& m, int n)
{
    while (n--) m.cpu.simulate();
}

static void test_alu()
{
    Machine machine({0x3E, 0xFF, // LD A,  0xFF
                     0xD6, 0x1,  // SUB A, 0x1
                     0x0, 0x0},
                    false);
    execute_n(machine, 2);
    assert(machine.cpu.registers().accum == 0xfe);
}

void do_test_machine()
{
    test_alu();

    printf("Tests SUCCESS!\n");
    exit(0);
}
