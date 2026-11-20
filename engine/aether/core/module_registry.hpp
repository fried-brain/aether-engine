#pragma once

#include "aether/core/module.hpp"

#include <memory>
#include <vector>

namespace aether::core {

class Application;

class ModuleRegistry final {
public:
    [[nodiscard]] Status add(std::unique_ptr<IModule> module);
    [[nodiscard]] Status validate();

private:
    friend class Application;

    std::vector<std::unique_ptr<IModule>> modules_;
    std::vector<IModule*> ordered_modules_;
    bool validated_{false};
};

} // namespace aether::core
