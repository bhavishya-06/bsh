#pragma once
#include <string>

// The "Address" where they meet
const std::string SOCKET_PATH = "/tmp/bsh.sock";

// Special delimiter that won't appear in normal shell commands
const char DELIMITER = '\x1F'; 
const int BUFFER_SIZE = 8192;