#ifndef __dictionary_h__
#define __dictionary_h__


//#include <algorithm>
//#include <cassert>
//#include <climits>
//#include <cmath>
//#include <cstring>
//#include <numeric>
//#include <string>
//#include <vector>

// #define LOGGING

#ifdef LOGGING
#ifdef ARDUINO
#include <Arduino.h>
#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#include <cstdio>
#define DEBUG_PRINTF(...) std::printf(__VA_ARGS__)
#endif
#else
#define DEBUG_PRINTF(...)
#endif

bool binary_search_word(const char* words[], int num_words, const std::string& target);
bool binary_search_prefix(const char* words[], int num_words, const std::string& target);
int binary_search_insertion_point(const char* words[], int num_words, const std::string& key);
int binary_search_ranking(const char* words[], int num_words, const std::string& target);
int levenshtein_distance_1(const char* a, const char* b);
std::string suggestion(std::string& word, std::string &second, std::string &third);
void autocorrect(std::string& word);

#endif
