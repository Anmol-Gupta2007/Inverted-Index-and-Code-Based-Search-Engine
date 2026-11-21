#include "tokenizer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <cctype>

using namespace std;
using namespace std::filesystem;
vector<DocumentInfo> scan_directory(const string &dir_path) {
    vector<DocumentInfo> documents;
    int current_id = 1;
    try {
        if (!exists(dir_path) || !is_directory(dir_path)) {
            cerr << "[Error] Invalid directory path: " << dir_path << "\n";
            return documents;
        }

        for (const auto &entry : recursive_directory_iterator(dir_path)) {
            if (entry.is_regular_file()) {
                string ext = entry.path().extension().string();
                // Filter only supported source code and header extensions
                if (ext == ".c" || ext == ".cpp" || ext == ".h" || ext == ".hpp" || ext == ".java" || ext == ".py") {
                    documents.push_back({current_id++, entry.path().string()});
                }
            }
        }
    } 
    catch (const exception &e) {
        cerr << "[Filesystem Error] " << e.what() << "\n";
    }

    return documents;
}

string strip_comments_and_strings(const string &source) {
    string clean;
    clean.reserve(source.size());

    bool in_single_comment = false;
    bool in_multi_comment = false;
    bool in_string = false;

    for (size_t i = 0; i < source.size(); ++i) {
        // Handle single-line comment (//)
        if (in_single_comment) {
            if (source[i] == '\n') {
                in_single_comment = false;
                clean += '\n'; // Preserve newline for line counting/previews
            }
            continue;
        }
        if (in_multi_comment) {
            if (source[i] == '*' && i + 1 < source.size() && source[i + 1] == '/') {
                in_multi_comment = false;
                i++; // Skip closing slash
            }
            continue;
        }

        if (in_string) {
            if (source[i] == '\\' && i + 1 < source.size()) {
                i++; // Skip escaped character like \"
            } else if (source[i] == '"') {
                in_string = false;
            }
            continue;
        }

        if (source[i] == '/' && i + 1 < source.size()) {
            if (source[i + 1] == '/') {
                in_single_comment = true;
                i++;
                continue;
            } else if (source[i + 1] == '*') {
                in_multi_comment = true;
                i++;
                continue;
            }
        }

        if (source[i] == '"') {
            in_string = true;
            continue;
        }

        clean += source[i];
    }

    return clean;
}

vector<string> tokenize_source_file(const string &filepath) {
    vector<string> tokens;
    ifstream file(filepath);
    if (!file.is_open()) {
        cerr << "[Error] Unable to open file: " << filepath << "\n";
        return tokens;
    }

    stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    string sanitized_code = strip_comments_and_strings(buffer.str());

    string current_token = "";
    for (char ch : sanitized_code) {
        if (isalnum(static_cast<unsigned char>(ch)) || ch == '_') {
            current_token += tolower(static_cast<unsigned char>(ch));
        } else {
            if (!current_token.empty()) {
                tokens.push_back(current_token);
                current_token.clear();
            }
        }
    }

    if (!current_token.empty()) {
        tokens.push_back(current_token);
    }

    return tokens;
}
