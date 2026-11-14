#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "minheap.h"

MinHeap* createMinHeap(int capacity) 
{
    MinHeap* heap = (MinHeap*)malloc(sizeof(MinHeap));
    heap->capacity = capacity;
    heap->size = 0;
    heap->data = (Result*)malloc(capacity * sizeof(Result));
    return heap;
}

static void swap(Result* a, Result* b) 
{
    Result temp = *a;
    *a = *b;
    *b = temp;
}

static void heapifyDown(MinHeap* heap, int idx) 
{
    int smallest = idx;
    int left = 2 * idx + 1;
    int right = 2 * idx + 2;

    if (left < heap->size && heap->data[left].score < heap->data[smallest].score)
        smallest = left;
    if (right < heap->size && heap->data[right].score < heap->data[smallest].score)
        smallest = right;

    if (smallest != idx) 
   {
        swap(&heap->data[idx], &heap->data[smallest]);
        heapifyDown(heap, smallest);
    }
}

static void heapifyUp(MinHeap* heap, int idx) 
{
    int parent = (idx - 1) / 2;
    while (idx > 0 && heap->data[idx].score < heap->data[parent].score) 
   {
        swap(&heap->data[idx], &heap->data[parent]);
        idx = parent;
        parent = (idx - 1) / 2;
    }
}

// Top-K Logic: maintains only the highest K scores
void insertMinHeap(MinHeap* heap, Result item) 
{
    if (heap->size < heap->capacity) 
    {
        heap->data[heap->size] = item;
        heap->size++;
        heapifyUp(heap, heap->size - 1);
    } else if (item.score > heap->data[0].score) 
    {
        heap->data[0] = item;
        heapifyDown(heap, 0);
    }
}

Result extractMin(MinHeap* heap) 
{
    Result root = heap->data[0];
    heap->data[0] = heap->data[heap->size - 1];
    heap->size--;
    heapifyDown(heap, 0);
    return root;
}

void freeMinHeap(MinHeap* heap) 
{
    if (heap) 
    {
        free(heap->data);
        free(heap);
    }
}
