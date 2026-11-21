#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <unordered_map>
#include <cctype>
#include <cstdlib>
#include <filesystem>

extern "C" {
    #include "index.h"          
}
#include "tokenizer.h"          
#include "query_parser.h"        
#include "ranker.h"              

using namespace std;
using namespace std::chrono;
namespace fs = std::filesystem;

static unordered_map<int, string> doc_id_to_path;
static InvertedIndex *global_index = nullptr;

string to_lower_str(string s) {
    for (auto &c : s) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    return s;
}

string to_upper_str(string s) {
    for (auto &c : s) c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
    return s;
}

void open_file_in_editor(const string &filepath) {
    string abs_path = fs::absolute(filepath).string();
    cout << "[Opening] " << abs_path << " ...\n";
    string vscode_cmd = "code \"" + abs_path + "\" 2>nul";
    int status = system(vscode_cmd.c_str());

    if (status != 0) {
        #ifdef _WIN32
            string win_cmd = "start \"\" \"" + abs_path + "\"";
            system(win_cmd.c_str());
        #else
            string linux_cmd = "xdg-open \"" + abs_path + "\" &";
            system(linux_cmd.c_str());
        #endif
    }
}

void print_clickable_link(const string &filepath) {
    string abs_path = fs::absolute(filepath).string();
    // Forward slashes for file:// URI
    string uri_path = abs_path;
    for (char &c : uri_path) {
        if (c == '\\') c = '/';
    }
    cout << "\033]8;;file:///" << uri_path << "\033\\" 
         << abs_path 
         << "\033]8;;\033\\";
}

void index_codebase(const string &folder_path) {
    if (!fs::exists(folder_path) || !fs::is_directory(folder_path)) {
        cerr << "[Error] Directory not exist: " << folder_path << "\n";
        exit(1);
    }

    cout << "\n[1/2] Scanning & Indexing Codebase: " << fs::absolute(folder_path).string() << "\n";
    cout << "--------------------------------------------------------------------\n";

    global_index = create_inverted_index(); 
    auto start = high_resolution_clock::now();

    vector<DocumentInfo> docs = scan_directory(folder_path);

    long long total_tokens = 0;
    for (const auto &doc : docs) {
        doc_id_to_path[doc.doc_id] = doc.filepath;
        vector<string> tokens = tokenize_source_file(doc.filepath);
        total_tokens += tokens.size();

        for (const auto &token : tokens) {
            string clean_token = to_lower_str(token);
            insert_token(global_index, clean_token.c_str(), doc.doc_id);
        }
    }

    auto end = high_resolution_clock::now();
    double time_ms = duration<double, milli>(end - start).count();

    cout << "Total Files Indexed  : " << docs.size() << "\n";
    cout << "Total Tokens Stored  : " << total_tokens << "\n";
    cout << "Indexing Latency     : " << time_ms << " ms\n";
    cout << "--------------------------------------------------------------------\n";
    cout << "Indexing Completed! Ready to search.\n\n";
}
void execute_search(const string &raw_query) {
    auto start = high_resolution_clock::now();

    vector<string> query_tokens = tokenize_query(raw_query);
    if (query_tokens.empty()) return;

    PostingNode *final_postings = nullptr;
    bool is_allocated_result = false;

    if (query_tokens.size() == 1) {
        string term = to_lower_str(query_tokens[0]);
        final_postings = get_postings(global_index, term.c_str());
    } 
    else if (query_tokens.size() >= 3) {
        string term1 = to_lower_str(query_tokens[0]);
        string op    = to_upper_str(query_tokens[1]);
        string term2 = to_lower_str(query_tokens[2]);

        PostingNode *p1 = get_postings(global_index, term1.c_str());
        PostingNode *p2 = get_postings(global_index, term2.c_str());

        if (op == "AND") {
            final_postings = intersect_and(p1, p2);
            is_allocated_result = true;
        } else if (op == "OR") {
            final_postings = union_or(p1, p2);
            is_allocated_result = true;
        } else if (op == "NOT") {
            final_postings = difference_not(p1, p2);
            is_allocated_result = true;
        } else {
            cout << "[Error] Unknown operator: " << op << " (Use AND, OR, or NOT)\n";
            return;
        }
    } 
    else {
        cout << "[Error] Invalid query. Format: <word> OR <word1 AND|OR|NOT word2>\n";
        return;
    }

    if (!final_postings) {
        cout << "No matching files found.\n\n";
        return;
    }

    int total_corpus_docs = static_cast<int>(doc_id_to_path.size());
    int match_count = 0;
    PostingNode *temp = final_postings;
    while (temp) {
        match_count++;
        temp = temp->next;
    }

    vector<SearchResult> scored_results = compute_tfidf_scores(final_postings, match_count, total_corpus_docs);
    int K = 5;
    vector<SearchResult> top_k = get_top_k_results(scored_results, K);

    auto end = high_resolution_clock::now();
    double latency_ms = duration<double, milli>(end - start).count();

    cout << "\n====================================================================\n";
    cout << "Query: \"" << raw_query << "\" | Matches: " << match_count 
         << " | Latency: " << latency_ms << " ms\n";
    cout << "--------------------------------------------------------------------\n";
    
    vector<string> ranked_paths;
    for (size_t i = 0; i < top_k.size(); ++i) {
        int id = top_k[i].doc_id;
        string path = doc_id_to_path.count(id) ? doc_id_to_path[id] : "Unknown";
        ranked_paths.push_back(path);

        cout << "[" << (i + 1) << "] Score: " << top_k[i].score << " | File: ";
        print_clickable_link(path); // Ctrl+Click support in VS Code Terminal
        cout << "\n";
    }
    cout << "====================================================================\n";

    while (!ranked_paths.empty()) {
        cout << "File kholne ke liye number (1-" << ranked_paths.size() 
             << ") dabayein (ya next search ke liye '0' / Enter): ";
        string choice;
        getline(cin, choice);

        if (choice.empty() || choice == "0") break;

        try {
            int idx = stoi(choice);
            if (idx >= 1 && idx <= static_cast<int>(ranked_paths.size())) {
                open_file_in_editor(ranked_paths[idx - 1]);
            } else {
                cout << "Invalid number.\n";
            }
        } catch (...) {
            cout << "Invalid input.\n";
        }
    }
    cout << "\n";

    if (is_allocated_result && final_postings) {
        free_result_postings(final_postings);
    }
}

int main(int argc, char *argv[]) {
    cout << "====================================================================\n";
    cout << "          CODEBASE INVERTED INDEX SEARCH ENGINE                     \n";
    cout << "====================================================================\n";

    string target_dir;
    if (argc > 1) {
        target_dir = argv[1];
    } else {
        cout << "Enter directory path to scan (e.g., C:\\MyProject ya ./src): ";
        getline(cin, target_dir);
        if (target_dir.empty()) target_dir = "./src";
    }

    index_codebase(target_dir);

    string query;
    while (true) {
        cout << "Search Query (ya 'exit'): ";
        if (!getline(cin, query)) break;

        if (query.empty()) continue;
        if (to_lower_str(query) == "exit" || to_lower_str(query) == "quit") break;

        execute_search(query);
    }

    if (global_index) {
        free_inverted_index(global_index);
    }

    cout << "Shutdown complete.\n";
    return 0;
}
