#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdlib>
#include "../include/query_parser.h"
using namespace std;
static PostingNode* create_node(int doc_id, int term_frequency) {
    PostingNode *new_node = (PostingNode *)malloc(sizeof(PostingNode));
    if (!new_node) return nullptr;
    new_node->doc_id = doc_id;
    new_node->term_frequency = term_frequency;
    new_node->next = nullptr;
    return new_node;
}
vector<string> tokenize_query(const string &raw_query) {
    vector<string> tokens;
    stringstream ss(raw_query);
    string word;
    while (ss >> word) {
        transform(word.begin(), word.end(), word.begin(), ::tolower);
        tokens.push_back(word);
    }
    return tokens;
}
extern "C" {
PostingNode* intersect_and(const PostingNode *p1, const PostingNode *p2) {
    PostingNode *head = nullptr;
    PostingNode *tail = nullptr;
    while (p1 != nullptr && p2 != nullptr) {
        if (p1->doc_id == p2->doc_id) {
            PostingNode *new_node = create_node(p1->doc_id, p1->term_frequency + p2->term_frequency);
            if (!head) head = new_node;
            else tail->next = new_node;
            tail = new_node;
            p1 = p1->next;
            p2 = p2->next;
        } else if (p1->doc_id < p2->doc_id) {
            p1 = p1->next;
        } else {
            p2 = p2->next;
        }
    }
    return head;
}
}
