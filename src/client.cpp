#include "ipc.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <filesystem>

// Simple manual arg parsing to avoid heavy CLI libraries
int main(int argc, char* argv[]) {
    if (argc < 2) return 0;
    std::string mode = argv[1];
    
    // Construct Message
    std::string msg = "";

    // FORMAT: SUGGEST|query|scope|context|success
    if (mode == "suggest") {
        msg = "SUGGEST";
        msg += DELIMITER + std::string(argv[2]); // query
        
        // Default values
        std::string scope = "global";
        std::string context = "";
        std::string success = "0";

        for (int i=3; i<argc; i++) {
            std::string arg = argv[i];
            if (arg == "--scope" && i+1 < argc) scope = argv[++i];
            // Send CWD as context; Daemon will resolve branch if needed
            if ((arg == "--cwd" || arg == "--branch") && i+1 < argc) context = argv[++i];
            if (arg == "--success") success = "1";
        }
        msg += DELIMITER + scope + DELIMITER + context + DELIMITER + success;
    }
    // FORMAT: RECORD|cmd|session|cwd|exit|duration
    else if (mode == "record") {
        msg = "RECORD";
        // ... (Add naive parsing similar to above to extract flags) ...
        // For brevity: assuming args come in a predictable order from Zsh script
        // In reality, you'd replicate the parsing logic to fill these slots:
        // msg += DELIMITER + cmd + DELIMITER + session + DELIMITER + cwd ...
        
        // Quick/Dirty parser for the specific Zsh call:
        std::string cmd, session, cwd, exit_code, duration;
        for(int i=2; i<argc; i++) {
            std::string k = argv[i];
            if(k == "--cmd") cmd = argv[++i];
            if(k == "--session") session = argv[++i];
            if(k == "--cwd") cwd = argv[++i];
            if(k == "--exit") exit_code = argv[++i];
            if(k == "--duration") duration = argv[++i];
        }
        msg += DELIMITER + cmd + DELIMITER + session + DELIMITER + cwd + 
               DELIMITER + exit_code + DELIMITER + duration;
    }
    else {
        return 0; 
    }

    // --- Networking ---
    int sock = 0;
    struct sockaddr_un serv_addr;
    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        // Fallback or fail silently (Server might be down)
        return 1;
    }

    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, SOCKET_PATH.c_str(), sizeof(serv_addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        // ERROR: Daemon is not running. 
        // Option A: Print nothing and exit (Fail safe)
        // Option B: Try to launch daemon (Advanced)
        return 1;
    }

    send(sock, msg.c_str(), msg.length(), 0);

    // Read Response
    char buffer[BUFFER_SIZE] = {0};
    read(sock, buffer, BUFFER_SIZE);
    
    // Print to stdout (Zsh captures this)
    std::cout << buffer;
    
    close(sock);
    return 0;
}