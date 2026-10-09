#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdlib>
#include "../include/query_parser.h"
static PostingNode* create_node(int doc_id, int term_frequency) {
    PostingNode *new_node = (PostingNode *)malloc(sizeof(PostingNode));
    if (!new_node) return nullptr;
    new_node->doc_id = doc_id;
    new_node->term_frequency = term_frequency;
    new_node->next = nullptr;
    return new_node;
}
