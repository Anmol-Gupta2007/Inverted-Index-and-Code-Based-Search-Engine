#ifndef TFIDF_H
#define TFIDF_H

#ifdef __cplusplus
extern "C" {
#endif

// Calculates TF-IDF weight: TF * ln(N / DF)
double calculateTFIDF(int termFrequency, int documentFrequency, int totalDocuments);

#ifdef __cplusplus
}
#endif

#endif
