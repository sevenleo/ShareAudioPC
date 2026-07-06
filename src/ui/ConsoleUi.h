#pragma once

#include "app/AppController.h"

#include <string>
#include <vector>

namespace shareaudio {

class ConsoleUi {
public:
    explicit ConsoleUi(AppController& controller);
    int run(const std::vector<std::string>& args);

private:
    void print_help() const;
    void print_status() const;
    void print_local_ips() const;

    AppController& controller_;
};

} // namespace shareaudio
