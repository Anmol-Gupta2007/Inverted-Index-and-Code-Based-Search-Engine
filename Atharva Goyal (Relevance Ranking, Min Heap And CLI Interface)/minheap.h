#ifndef MINHEAP_H
#define MINHEAP_H

// User-defined structure (C++ Unit 1 / DS Unit 1)
typedef struct 
{
    int docID;
    char filename[64];
    double score;
} Result;

// Heap structure using dynamic memory and pointers (DS Unit 1 & 2)
typedef struct 
{
    Result* data;
    int size;
    int capacity;
} MinHeap;

// Min-Heap function prototypes (DS Unit 2 & 4)
MinHeap* createMinHeap(int capacity);
void insertMinHeap(MinHeap* heap, Result item);
Result extractMin(MinHeap* heap);
void freeMinHeap(MinHeap* heap);

#endif // MINHEAP_H
