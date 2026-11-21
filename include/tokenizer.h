#ifndef TOKENIZER_H
#define TOKENIZER_H
#include <string>
#include <vector>
struct DocumentInfo {
    int doc_id;
    std::string filepath;
};
std::vector<DocumentInfo> scan_directory(const std::string &dir_path);
std::string strip_comments_and_strings(const std::string &source_code);
std::vector<std::string> tokenize_source_file(const std::string &filepath);
#endif // TOKENIZER_H
