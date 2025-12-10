#include "git_utils.hpp"
#include <git2.h>
#include <iostream>

// RAII Wrapper for LibGit2 initialization
struct GitLib {
    GitLib() { git_libgit2_init(); }
    ~GitLib() { git_libgit2_shutdown(); }
};

std::optional<std::string> get_git_branch(const std::string& cwd_path) {
    static GitLib git_init; // Initialize once

    git_repository* repo = nullptr;
    git_reference* head = nullptr;
    std::string branch_name;
    bool found = false;

    // 1. Try to open repository from the current working directory (searches up)
    int error = git_repository_open_ext(&repo, cwd_path.c_str(), 0, nullptr);
    
    if (error == 0) {
        // 2. Get HEAD
        error = git_repository_head(&head, repo);
        
        if (error == 0) {
            // 3. Get Shorthand name (e.g., "refs/heads/main" -> "main")
            const char* name = git_reference_shorthand(head);
            if (name) {
                branch_name = name;
                found = true;
            }
        } else {
            // HEAD might be detached or repo is empty
            // Optional: You could check for detached HEAD commit hash here
        }
    }

    // Cleanup
    if (head) git_reference_free(head);
    if (repo) git_repository_free(repo);

    if (found) return branch_name;
    return std::nullopt;
}