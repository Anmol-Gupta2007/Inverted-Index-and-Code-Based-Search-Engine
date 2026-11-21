#include "ranker.h"
#include <iostream>
#include <cmath>
#include <queue>
#include <iomanip>

struct CompareScore {
    bool operator()(const SearchResult &a, const SearchResult &b) {
        return a.score > b.score; 
    }
};

std::vector<SearchResult> compute_tfidf_scores(PostingNode *postings, int doc_count_for_term, int total_corpus_docs) {
    std::vector<SearchResult> scored_list;
    if (!postings || total_corpus_docs <= 0) return scored_list;

    double idf = std::log(1.0 + ((double)total_corpus_docs / (double)(doc_count_for_term > 0 ? doc_count_for_term : 1)));

    PostingNode *curr = postings;
    while (curr != nullptr) {
        double tf = (curr->term_frequency > 0) ? (1.0 + std::log((double)curr->term_frequency)) : 0.0;
        double final_score = tf * idf;

        scored_list.push_back({curr->doc_id, final_score});
        curr = curr->next;
    }

    return scored_list;
}

std::vector<SearchResult> get_top_k_results(const std::vector<SearchResult> &results, int k) {
    if (results.empty() || k <= 0) return {};
    std::priority_queue<SearchResult, std::vector<SearchResult>, CompareScore> min_heap;

    for (const auto &item : results) {
        if ((int)min_heap.size() < k) {
            min_heap.push(item);
        } else if (item.score > min_heap.top().score) {
            min_heap.pop();
            min_heap.push(item);
        }
    }
    std::vector<SearchResult> top_k(min_heap.size());
    for (int i = (int)top_k.size() - 1; i >= 0; --i) {
        top_k[i] = min_heap.top();
        min_heap.pop();
    }

    return top_k;
}

void display_results(const std::vector<SearchResult> &ranked_results, double elapsed_ms) {
    std::cout << "\n============================================\n";
    std::cout << "             SEARCH RESULTS                 \n";
    std::cout << "============================================\n";

    if (ranked_results.empty()) {
        std::cout << "  [!] No matching documents found.\n";
    } else {
        std::cout << std::left << std::setw(10) << "Rank"
                  << std::setw(15) << "Document ID"
                  << std::setw(15) << "Relevance Score" << "\n";
        std::cout << "--------------------------------------------\n";

        int rank = 1;
        for (const auto &res : ranked_results) {
            std::cout << std::left << std::setw(10) << rank++
                      << "Doc " << std::setw(11) << res.doc_id
                      << std::fixed << std::setprecision(4) << res.score << "\n";
        }
    }

    std::cout << "--------------------------------------------\n";
    std::cout << "Execution Time: " << elapsed_ms << " ms\n";
    std::cout << "============================================\n\n";
}
