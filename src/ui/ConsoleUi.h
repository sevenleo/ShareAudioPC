#pragma once

#include "app/AppController.h"
#include "app/SessionController.h"

#include <string>
#include <vector>

namespace shareaudio {

class ConsoleUi {
public:
    explicit ConsoleUi(AppController& controller);
    int run(const std::vector<std::string>& args);

private:
    void print_help() const;
    void print_local_ips() const;
    void print_audio_devices() const;

    AppController& controller_;
    mutable SessionController session_;
};

} // namespace shareaudio
