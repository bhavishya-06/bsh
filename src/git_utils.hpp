#pragma once
#include <string>
#include <optional>

// Returns the branch name (e.g., "main", "feature/login")
// Returns std::nullopt if not in a git repo.
std::optional<std::string> get_git_branch(const std::string& cwd_path);