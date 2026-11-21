#include "index.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long hash_djb2(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = (unsigned char)*str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash % TABLE_SIZE;
}

static PostingNode* create_posting_node(int doc_id) {
    PostingNode *node = (PostingNode *)malloc(sizeof(PostingNode));
    if (!node) {
        perror("Failed to allocate PostingNode");
        return NULL;
    }
    node->doc_id = doc_id;
    node->term_frequency = 1;
    node->next = NULL;
    return node;
}

InvertedIndex* create_inverted_index(void) {
    InvertedIndex *index = (InvertedIndex *)malloc(sizeof(InvertedIndex));
    if (!index) {
        perror("Failed to allocate InvertedIndex");
        return NULL;
    }
    for (int i = 0; i < TABLE_SIZE; i++) {
        index->buckets[i] = NULL;
    }
    index->total_documents = 0;
    return index;
}

void insert_token(InvertedIndex *index, const char *token, int doc_id) {
    if (!index || !token || strlen(token) == 0) return;

    unsigned long bucket_idx = hash_djb2(token);
    HashEntry *entry = index->buckets[bucket_idx];

    while (entry != NULL) {
        if (strcmp(entry->word, token) == 0) {
            
            PostingNode *p = entry->head;
            PostingNode *prev = NULL;

            while (p != NULL) {
                if (p->doc_id == doc_id) {
                    p->term_frequency++; 
                    return;
                }
                prev = p;
                p = p->next;
            }
            PostingNode *new_p = create_posting_node(doc_id);
            if (prev) {
                prev->next = new_p;
            } else {
                entry->head = new_p;
            }
            return;
        }
        entry = entry->next;
    }

    
    HashEntry *new_entry = (HashEntry *)malloc(sizeof(HashEntry));
    if (!new_entry) {
        perror("Failed to allocate HashEntry");
        return;
    }

    new_entry->word = (char *)malloc(strlen(token) + 1);
    strcpy(new_entry->word, token);
    new_entry->head = create_posting_node(doc_id);

    new_entry->next = index->buckets[bucket_idx];
    index->buckets[bucket_idx] = new_entry;
}


PostingNode* get_postings(InvertedIndex *index, const char *token) {
    if (!index || !token) return NULL;

    unsigned long bucket_idx = hash_djb2(token);
    HashEntry *entry = index->buckets[bucket_idx];

    while (entry != NULL) {
        if (strcmp(entry->word, token) == 0) {
            return entry->head;
        }
        entry = entry->next;
    }

    return NULL; 
}


void free_inverted_index(InvertedIndex *index) {
    if (!index) return;

    for (int i = 0; i < TABLE_SIZE; i++) {
        HashEntry *entry = index->buckets[i];
        while (entry != NULL) {
            HashEntry *temp_entry = entry;
            entry = entry->next;

            PostingNode *p = temp_entry->head;
            while (p != NULL) {
                PostingNode *temp_p = p;
                p = p->next;
                free(temp_p);
            }


            free(temp_entry->word);
            free(temp_entry);
        }
    }

    free(index);
}

void print_index_stats(const InvertedIndex *index) {
    if (!index) return;
    int unique_words = 0;
    int total_nodes = 0;

    for (int i = 0; i < TABLE_SIZE; i++) {
        HashEntry *entry = index->buckets[i];
        while (entry != NULL) {
            unique_words++;
            PostingNode *p = entry->head;
            while (p != NULL) {
                total_nodes++;
                p = p->next;
            }
            entry = entry->next;
        }
    }

    printf("=== Inverted Index Statistics ===\n");
    printf("Unique Vocabulary Words : %d\n", unique_words);
    printf("Total Postings Recorded : %d\n", total_nodes);
    printf("=================================\n");
}
