#include "kianv/platform.h"

__weak void runtime_init(void)
{
    extern void (*__preinit_array_start[])(void),
        (*__preinit_array_end[])(void);
    extern void (*__init_array_start[])(void), (*__init_array_end[])(void);
    for (void (**f)(void) = __preinit_array_start; f < __preinit_array_end; f++)
        (*f)();
    for (void (**f)(void) = __init_array_start; f < __init_array_end; f++)
        (*f)();
}
