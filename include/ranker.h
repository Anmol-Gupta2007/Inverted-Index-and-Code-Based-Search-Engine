#ifndef RANKER_H
#define RANKER_H

#include <string>
#include <vector>
#include "index.h"

struct SearchResult {
    int doc_id;
    double score;
};

std::vector<SearchResult> compute_tfidf_scores(PostingNode *postings, int total_docs, int total_corpus_docs);

std::vector<SearchResult> get_top_k_results(const std::vector<SearchResult> &results, int k);

void display_results(const std::vector<SearchResult> &ranked_results, double elapsed_ms);

#endif 
