#include <filesystem>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

#include "Viewer.h"

#ifdef _WIN32
int main()
#else
int main(int argc, char **argv)
#endif
{
#ifdef _WIN32
    int  argc;
    auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    SetConsoleOutputCP(CP_UTF8);
    setvbuf(stdout, nullptr, _IOFBF, 1000);
    setlocale(LC_ALL, ".UTF8");
#endif

    try
    {
        std::filesystem::path executableDir =
            std::filesystem::absolute(argv[0]).parent_path();
        Viewer viewer(executableDir);
        viewer.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }

#ifdef _WIN32
    LocalFree(argv);
#endif
}
