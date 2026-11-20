#include "aether/core/module_registry.hpp"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace aether::core {

Status ModuleRegistry::add(std::unique_ptr<IModule> module) {
    if (!module) {
        return Status::failure(ErrorCode::InvalidArgument, "Cannot register a null module.");
    }
    const auto module_name = module->name();
    if (module_name.empty()) {
        return Status::failure(ErrorCode::InvalidArgument, "Module name cannot be empty.");
    }
    const auto duplicate = std::find_if(modules_.begin(), modules_.end(),
        [module_name](const auto& existing) { return existing->name() == module_name; });
    if (duplicate != modules_.end()) {
        return Status::failure(ErrorCode::DuplicateModule,
            "A module named '" + std::string(module_name) + "' is already registered.");
    }

    modules_.push_back(std::move(module));
    ordered_modules_.clear();
    validated_ = false;
    return Status::success();
}

Status ModuleRegistry::validate() {
    ordered_modules_.clear();
    std::vector<bool> scheduled(modules_.size(), false);
    ordered_modules_.reserve(modules_.size());

    for (std::size_t i = 0; i < modules_.size(); ++i) {
        for (const auto& dependency : modules_[i]->dependencies()) {
            const auto exists = std::any_of(modules_.begin(), modules_.end(),
                [&dependency](const auto& candidate) {
                    return candidate->name() == dependency;
                });
            if (!exists) {
                validated_ = false;
                return Status::failure(ErrorCode::MissingDependency,
                    "Module '" + std::string(modules_[i]->name()) +
                    "' requires missing dependency '" + dependency + "'.");
            }
        }
    }

    while (ordered_modules_.size() < modules_.size()) {
        bool progress = false;
        for (std::size_t i = 0; i < modules_.size(); ++i) {
            if (scheduled[i]) continue;
            const auto dependencies_ready = std::all_of(
                modules_[i]->dependencies().begin(), modules_[i]->dependencies().end(),
                [this, &scheduled](const ModuleId& dependency) {
                    for (std::size_t j = 0; j < modules_.size(); ++j) {
                        if (modules_[j]->name() == dependency) return scheduled[j];
                    }
                    return false;
                });
            if (!dependencies_ready) continue;
            scheduled[i] = true;
            ordered_modules_.push_back(modules_[i].get());
            progress = true;
            break;
        }
        if (!progress) {
            ordered_modules_.clear();
            validated_ = false;
            return Status::failure(ErrorCode::DependencyCycle,
                "Module dependencies contain a cycle.");
        }
    }

    validated_ = true;
    return Status::success();
}

} // namespace aether::core
