#include <math.h>
#include "tfidf.h"

double calculateTFIDF(int termFrequency, int documentFrequency, int totalDocuments) 
{
    if (termFrequency <= 0 || documentFrequency <= 0 || totalDocuments <= 0 || documentFrequency > totalDocuments) {
        return 0.0;
    }
    // IDF calculation using natural log
    double idf = log((double)totalDocuments / (double)documentFrequency);
    return (double)termFrequency * idf;
}
