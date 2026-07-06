#include "app/Application.h"

#include "app/AppController.h"
#include "ui/ConsoleUi.h"

#include <string>
#include <vector>

namespace shareaudio {

int Application::run(int argc, char** argv)
{
    std::vector<std::string> args;
    args.reserve(argc > 0 ? static_cast<std::size_t>(argc - 1) : 0);
    for (int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    AppController controller;
    ConsoleUi ui(controller);
    return ui.run(args);
}

} // namespace shareaudio
