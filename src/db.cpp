#include "db.hpp"
#include <iostream>

// Constructor now opens the DB immediately
HistoryDB::HistoryDB(const std::string& db_path) : db_path_(db_path) {
    // Open DB in Read/Write mode with Create flag
    db_ = std::make_unique<SQLite::Database>(db_path_, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    
    // Performance Pragma
    db_->exec("PRAGMA journal_mode=WAL;");
    db_->exec("PRAGMA synchronous=NORMAL;");
}

void HistoryDB::initSchema() {
    try {
        // Use the persistent connection
        db_->exec("CREATE TABLE IF NOT EXISTS commands ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "cmd_text TEXT UNIQUE NOT NULL"
                ");");

        db_->exec("CREATE TABLE IF NOT EXISTS executions ("
                "id INTEGER PRIMARY KEY, "
                "command_id INTEGER, "
                "session_id TEXT, "
                "cwd TEXT, "
                "git_branch TEXT, "
                "exit_code INTEGER, "
                "duration_ms INTEGER, "
                "timestamp INTEGER, "
                "FOREIGN KEY (command_id) REFERENCES commands (id)"
                ");");

        db_->exec("CREATE INDEX IF NOT EXISTS idx_exec_cwd ON executions(cwd);");
        db_->exec("CREATE INDEX IF NOT EXISTS idx_exec_branch ON executions(git_branch);");
        db_->exec("CREATE INDEX IF NOT EXISTS idx_exec_exit ON executions(exit_code);");
        db_->exec("CREATE INDEX IF NOT EXISTS idx_exec_ts ON executions(timestamp);");

        // --- PREPARE STATEMENTS ONCE ---
        stmt_insert_cmd_ = std::make_unique<SQLite::Statement>(*db_, 
            "INSERT OR IGNORE INTO commands (cmd_text) VALUES (?)");
            
        stmt_get_id_ = std::make_unique<SQLite::Statement>(*db_, 
            "SELECT id FROM commands WHERE cmd_text = ?");
            
        stmt_insert_exec_ = std::make_unique<SQLite::Statement>(*db_, 
            "INSERT INTO executions (command_id, session_id, cwd, git_branch, exit_code, duration_ms, timestamp) VALUES (?, ?, ?, ?, ?, ?, ?)");

    } catch (std::exception& e) {
        std::cerr << "DB Init Error: " << e.what() << std::endl;
    }
}

void HistoryDB::logCommand(const std::string& cmd, const std::string& session, 
                           const std::string& cwd, const std::string& branch, 
                           int exit_code, int duration, long long timestamp) {
    try {
        // 1. Insert Command
        stmt_insert_cmd_->reset(); // Reset state from previous run
        stmt_insert_cmd_->bind(1, cmd);
        stmt_insert_cmd_->exec();

        // 2. Get ID
        stmt_get_id_->reset();
        stmt_get_id_->bind(1, cmd);
        if (stmt_get_id_->executeStep()) {
            int cmd_id = stmt_get_id_->getColumn(0);

            // 3. Log Execution
            stmt_insert_exec_->reset();
            stmt_insert_exec_->bind(1, cmd_id);
            stmt_insert_exec_->bind(2, session);
            stmt_insert_exec_->bind(3, cwd);
            if (branch.empty()) stmt_insert_exec_->bind(4, (char*)nullptr);
            else stmt_insert_exec_->bind(4, branch);
            stmt_insert_exec_->bind(5, exit_code);
            stmt_insert_exec_->bind(6, duration);
            stmt_insert_exec_->bind(7, (int64_t)timestamp);
            stmt_insert_exec_->exec();
        }
    } catch (std::exception& e) {
        std::cerr << "Log Error: " << e.what() << std::endl;
    }
}

std::vector<SearchResult> HistoryDB::search(const std::string& query, 
                                            SearchScope scope,
                                            const std::string& context_val,
                                            bool only_success) {
    std::vector<SearchResult> results;
    try {
        // Note: Dynamic queries are harder to pre-compile because the WHERE clause changes.
        // However, since the DB connection (db_) is already open, this is still FAST.
        
        std::string sql = "SELECT c.id, c.cmd_text "
                          "FROM executions e "
                          "JOIN commands c ON e.command_id = c.id "
                          "WHERE c.cmd_text LIKE ? "
                          "AND c.cmd_text NOT LIKE 'bsh%' "
                          "AND c.cmd_text NOT LIKE './bsh%' "
                          "AND TRIM(c.cmd_text) NOT LIKE '#%' ";

        if (scope == SearchScope::DIRECTORY) sql += " AND e.cwd = ?";
        else if (scope == SearchScope::BRANCH) sql += " AND e.git_branch = ?";
        
        if (only_success) sql += " AND e.exit_code = 0";

        sql += " GROUP BY c.cmd_text ORDER BY MAX(e.timestamp) DESC LIMIT 5";

        SQLite::Statement query_stmt(*db_, sql); // Use existing DB connection

        query_stmt.bind(1, "%" + query + "%");
        if (scope != SearchScope::GLOBAL) query_stmt.bind(2, context_val);

        while (query_stmt.executeStep()) {
            results.push_back({
                query_stmt.getColumn(0),
                query_stmt.getColumn(1)
            });
        }
    } catch (std::exception& e) {
        std::cerr << "DB Error: " << e.what() << std::endl;
    }
    return results;
}