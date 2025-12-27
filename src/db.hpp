#pragma once
#include <SQLiteCpp/SQLiteCpp.h>
#include <string>
#include <vector>
#include <memory> // Required for unique_ptr

enum class SearchScope { GLOBAL, DIRECTORY, BRANCH };

struct SearchResult {
    int id;
    std::string cmd;
};

class HistoryDB {
public:
    explicit HistoryDB(const std::string& db_path);
    void initSchema();
    
    void logCommand(const std::string& cmd, const std::string& session, 
                    const std::string& cwd, const std::string& branch, 
                    int exit_code, int duration, long long timestamp);

    std::vector<SearchResult> search(const std::string& query, 
                                     SearchScope scope,
                                     const std::string& context_val,
                                     bool only_success = false); 

private:
    std::string db_path_;
    
    // --- THE FIX: Persistent Objects ---
    // We use unique_ptr because we need to initialize them in the body of the constructor
    // after the schema might be created.
    std::unique_ptr<SQLite::Database> db_;
    
    // Prepared Statements (Compiled SQL)
    std::unique_ptr<SQLite::Statement> stmt_insert_cmd_;
    std::unique_ptr<SQLite::Statement> stmt_get_id_;
    std::unique_ptr<SQLite::Statement> stmt_insert_exec_;
    // Note: Search is dynamic (building WHERE clauses), so we might keep that one fresh 
    // or use a base prepared statement if the query structure is static.
};