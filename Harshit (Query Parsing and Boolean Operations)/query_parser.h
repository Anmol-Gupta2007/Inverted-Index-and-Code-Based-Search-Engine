#ifndef QUERY_PARSER_H
#define QUERY_PARSER_H
#include "index.h"
#ifdef __cplusplus
extern "C" {
#endif
PostingNode* intersect_and(const PostingNode *p1, const PostingNode *p2);
PostingNode* union_or(const PostingNode *p1, const PostingNode *p2);
PostingNode* difference_not(const PostingNode *p1, const PostingNode *p2);
void free_result_postings(PostingNode *head);
#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
#include <string>
#include <vector>
std::vector<std::string> tokenize_query(const std::string &raw_query);
#endif
#endif
