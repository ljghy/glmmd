#include <iostream>

#ifdef _WIN32
#include <Windows.h>
#endif

#include "Context.h"

int main()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    setvbuf(stdout, nullptr, _IOFBF, 1000);
#endif

    std::unique_ptr<Context> context;
    try
    {
        context = std::make_unique<Context>("init.json");
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
    context->run();
}
