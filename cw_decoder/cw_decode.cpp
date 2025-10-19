#include <cmath>
#include <cstring>
#include <cassert>
#include <string>
#include <algorithm>
#include <vector>
#include <numeric>

#include "cw_data.h"
#include "cw_decode.h"

//#define LOGGING

#ifdef LOGGING
#ifdef ARDUINO
  #include <Arduino.h>
  #define DEBUG_PRINTF(...)  Serial.printf(__VA_ARGS__)
#else
  #include <cstdio>
  #define DEBUG_PRINTF(...)  std::printf(__VA_ARGS__)
#endif
#else
  #define DEBUG_PRINTF(...)
#endif


//return the log likelihood x in a normal distribution mu/sigma
float log_gaussian(float x, float mu, float sigma)
{
    return -0.5f * pow((x - mu) / sigma, 2.0f);
}

//Is this pattern part of a morse symbol?
bool is_start_of_code(std::string &pattern)
{
    int span = 128;
    int morse_index = 0;

    for(int i=0; i<7; i++) {
      span >>= 1;
      assert(i < strlen(pattern.c_str())+1);
      if(pattern.c_str()[i] == 0) {
        return MORSE[morse_index] != '~';
      } else if(pattern.c_str()[i] == '.'){
        morse_index++;
      } else if(pattern.c_str()[i] == '-'){
        morse_index+=span;
      }
    }
    return false;
}

//Is this pattern an exact match to a morse symbol?
bool is_code(std::string &pattern)
{
    int span = 128;
    int morse_index = 0;

    for(int i=0; i<7; ++i) {
      span >>= 1;
      assert(i < strlen(pattern.c_str())+1);
      if(pattern.c_str()[i] == 0) {
        return MORSE[morse_index] != '#' && MORSE[morse_index] != '~';
      } else if(pattern.c_str()[i] == '.'){
        morse_index++;
      } else if(pattern.c_str()[i] == '-'){
        morse_index+=span;
      }
    }
    return false;
}

//What letter does this code correspond to?
char get_letter_from_code(std::string &pattern)
{
    int span = 128;
    int morse_index = 0;

    for(int i=0; i<7; i++) {
      span >>= 1;
      assert(i < strlen(pattern.c_str())+1);
      if(pattern.c_str()[i] == 0) {
        return MORSE[morse_index];
      } else if(pattern.c_str()[i] == '.'){
        morse_index++;
      } else if(pattern.c_str()[i] == '-'){
        morse_index+=span;
      }
    }
    return '#';
}

bool binary_search_word(const char * words[], int num_words, const std::string &target)
{
    int left = 0;
    int right = num_words - 1;

    while (left <= right) {
        int mid = left + (right - left) / 2;
        assert(mid < num_words);
        int cmp = std::strcmp(words[mid], target.c_str());

        if (cmp == 0) {
            return true;  // found
        } else if (cmp < 0) {
            left = mid + 1;  // search right half
        } else {
            right = mid - 1; // search left half
        }
    }

    return false;
}

//What is the log probability that this is a word?
float language_log_prob(std::string &word)
{
    if(binary_search_word(CW_ABBREVIATIONS, NUM_CW_ABBREVIATIONS, word)) return 6.0f;
    if(binary_search_word(CW_WORDS, NUM_CW_WORDS, word)) return 4.0f;
    return 0;
}

//Find last word in a string
std::string get_last_word(const std::string& text) 
{
    // Trim trailing spaces
    size_t end = text.find_last_not_of(" \t\n\r");
    if (end == std::string::npos) return std::string(""); // no non-space characters

    // Find start of last word
    size_t start = text.find_last_of(" \t\n\r", end);
    if (start == std::string::npos)
        start = 0;
    else
        start += 1; // move past the space

    return text.substr(start, end - start + 1);
}

//calculate the mean of each cluster - output in means[]
void calculate_means(std::vector<float> clusters[], const int num_clusters, float means[])
{
  for(int i=0; i<num_clusters; ++i)
  {
    if (clusters[i].empty()){
      means[i] = 0.0f;
    } else {
      float sum = std::accumulate(clusters[i].begin(), clusters[i].end(), 0.0);
      means[i] =  sum / clusters[i].size();
    }
  }
}

//calculate the standard deviation of each cluster - input means[], output sigmas[]
void calculate_sigmas(std::vector<float> clusters[], const int num_clusters, const float means[], float sigmas[])
{
  for(int i=0; i<num_clusters; ++i)
  {
    float sq_sum = std::accumulate(clusters[i].begin(), clusters[i].end(), 0.0,
        [means, i](float acc, float x) {
            float diff = x - means[i];
            return acc + diff * diff;
        });

    sigmas[i] = std::sqrt(sq_sum/clusters[i].size());

  }
}

//find the closest cluster to a particular data item
int calculate_closest_cluster(float means[], int num_clusters, float x)
{
  
  float closest_distance = std::abs(means[0] - x);
  int closest_cluster = 0;

  for(int i=1; i<num_clusters; ++i) {
    float distance = std::abs(means[i] - x);
    if(distance < closest_distance) {
      closest_cluster = i;
      closest_distance = distance;
    }
  }

  return closest_cluster;
}

void kmeans(float x[], const int x_n, float means[], float sigmas[], const int num_clusters, const int iterations)
{
  std::vector<float> clusters[num_clusters];
  for(int item=0; item<x_n; item++)
  {
   int cluster_idx = item % num_clusters;
   clusters[cluster_idx].push_back(x[item]);
  }

  for(int iteration = 0; iteration < iterations; ++iteration)
  {
    calculate_means(clusters, num_clusters, means); 
    for(int idx=0; idx<num_clusters; idx++) clusters[idx].clear(); //empty clusters
    for(int item=0; item<x_n; item++) {
      int closest_cluster = calculate_closest_cluster(means, num_clusters, x[item]);
      clusters[closest_cluster].push_back(x[item]);
    }
  }

  calculate_means(clusters, num_clusters, means); 
  calculate_sigmas(clusters, num_clusters, means, sigmas);

  // Combine
  std::vector<std::pair<float, float>> pairs;
  for (size_t i = 0; i < num_clusters; ++i)
      pairs.emplace_back(means[i], sigmas[i]);

  // Sort by first element
  std::sort(pairs.begin(), pairs.end(),
      [](const auto& a, const auto& b) { return a.first < b.first; });

  // Unzip back
  for (size_t i = 0; i < num_clusters; ++i) {
      means[i] = pairs[i].first;
      sigmas[i] = pairs[i].second;
  }
}


//find the n most likely decodes
std::string c_cw_decoder :: get_text()
{
    std::string letter = std::string("")+get_letter_from_code(beam[0].pattern);
    std::string text = beam[0].text + letter;
    DEBUG_PRINTF("%s\n", text.c_str());

    return text;
}

//find the n most likely decodes
void c_cw_decoder :: decode(s_observation signal[], int num_observations)
{

    DEBUG_PRINTF("Duration: \n");
    for(uint16_t idx=0; idx<num_observations; idx++) {

      if(signal[idx].mark) DEBUG_PRINTF("%f ", signal[idx].duration);

    }
    DEBUG_PRINTF("\n");

    //global clustering
    float means[3];
    float sigmas[3];

    //use k-means to estimate dit and dah length
    float marks[num_observations];
    int num_marks = 0;
    for(int idx=0; idx<num_observations; idx++) { 
      if(signal[idx].mark) marks[num_marks++] = signal[idx].duration; 
    }
    kmeans(marks, num_marks, means, sigmas, 2, 5);
    float mu = means[0];
    float dah_mu = means[1];
    float sigma = sigmas[0];
    DEBUG_PRINTF("mu %f dash_mu %f, sigma %f\n", mu, dah_mu, sigma);
    //Serial.println(mu);
    //Serial.println(dah_mu);

    //use k-means to estimate space lengths
    float spaces[num_observations];
    int num_spaces = 0;
    for(int idx=0; idx<num_observations; idx++) { 
      if(!signal[idx].mark && signal[idx].duration < 10*mu) spaces[num_spaces++] = signal[idx].duration; 
    }
    kmeans(spaces, num_spaces, means, sigmas, 3, 5);
    //float short_mu = means[0];
    //float medium_mu = means[1];
    //float long_mu = means[2];
    //float short_sigma = sigmas[0];
    //float medium_sigma = sigmas[1];
    //float long_sigma = sigmas[2];

    float short_mu = mu;
    float medium_mu = 3*mu;
    float long_mu = 7*mu;
    float short_sigma = sigma;
    float medium_sigma = sigma;
    float long_sigma = sigma;

    DEBUG_PRINTF("short mu %f medium_mu %f, long_mu %f\n", short_mu, medium_mu, long_mu);
    DEBUG_PRINTF("short sigma %f medium sigma %f, long sigma %f\n", short_sigma, medium_sigma, long_sigma);
    //Serial.println(short_mu);
    //Serial.println(medium_mu);
    //Serial.println(long_mu);

    //the beam contains hte most likely decodes and their probabilities

    for(int i=0; i<num_observations; ++i)
    {
      float logp_dot  = log_gaussian(signal[i].duration, mu, sigma);
      float logp_dash = log_gaussian(signal[i].duration, dah_mu, sigma);
      if(signal[i].mark) {
        if(logp_dot > logp_dash)
          DEBUG_PRINTF("dot ");
        else
          DEBUG_PRINTF("dash ");
      }
    }
    DEBUG_PRINTF("\n");

    DEBUG_PRINTF("num observations %u\n", num_observations);
    for(int i=0; i<num_observations; ++i)
    {
      
        float duration = signal[i].duration;
        float logp_dot  = log_gaussian(duration, mu, sigma);
        float logp_dash = log_gaussian(duration, dah_mu, sigma);
        float logp_gap1 = log_gaussian(duration, short_mu, short_sigma);
        float logp_gap3 = log_gaussian(duration, medium_mu, medium_sigma);
        float logp_gap7 = duration<long_mu?log_gaussian(duration, long_mu, long_sigma):0;


        s_candidate candidates[BEAM_WIDTH*3]; //max 3 new predictions for each item in beam
        int num_candidates = 0;

        for(int j=0; j<items_in_beam; j++)
        {
            std::string &text = beam[j].text;
            std::string &pattern = beam[j].pattern;
            float &logp = beam[j].logp;

            if(signal[i].mark) {
              
              char letter = get_letter_from_code(pattern);
              bool pattern_is_code = letter != '#' && letter != '~';
              
              // Case 1: dot
              std::string dot_pattern = pattern+'.';
              if (is_start_of_code(dot_pattern))
              {
                assert(num_candidates < BEAM_WIDTH*3);
                candidates[num_candidates].text    = text;
                candidates[num_candidates].pattern = pattern + '.';
                candidates[num_candidates].logp    = logp + logp_dot;
                DEBUG_PRINTF("%u dot \"%s\" \"%s\"\n", i, candidates[num_candidates].text.c_str(), candidates[num_candidates].pattern.c_str());
                num_candidates++;
              }
              else if (pattern_is_code)
              {
                assert(num_candidates < BEAM_WIDTH*3);
                candidates[num_candidates].text    = text + letter;
                candidates[num_candidates].pattern = ".";
                candidates[num_candidates].logp    = logp + logp_dot;
                DEBUG_PRINTF("%u dot2 \"%s\" \"%s\"\n", i, candidates[num_candidates].text.c_str(), candidates[num_candidates].pattern.c_str());
                num_candidates++;
              }

              // Case 2: dash
              std::string dash_pattern = pattern+'-';
              if (is_start_of_code(dash_pattern))
              {
                assert(num_candidates < BEAM_WIDTH*3);
                candidates[num_candidates].text    = text;
                candidates[num_candidates].pattern = pattern + '-';
                candidates[num_candidates].logp    = logp + logp_dash;
                DEBUG_PRINTF("%u dash \"%s\" \"%s\"\n", i, candidates[num_candidates].text.c_str(), candidates[num_candidates].pattern.c_str());
                num_candidates++;
              }
              else if (pattern_is_code)
              {
                assert(num_candidates < BEAM_WIDTH*3);
                candidates[num_candidates].text    = text + letter;
                candidates[num_candidates].pattern = "-";
                candidates[num_candidates].logp    = logp + logp_dash;
                DEBUG_PRINTF("%u dash2 \"%s\" \"%s\"\n", i, candidates[num_candidates].text.c_str(), candidates[num_candidates].pattern.c_str());
                num_candidates++;
              }

            }
            else
            {
              //Case 3: symbol gap
              assert(num_candidates < BEAM_WIDTH*3);
              candidates[num_candidates].text    = text;
              candidates[num_candidates].pattern = pattern;
              candidates[num_candidates].logp    = logp + logp_gap1;
              DEBUG_PRINTF("%u symbol_gap \"%s\" \"%s\"\n", i, candidates[num_candidates].text.c_str(), candidates[num_candidates].pattern.c_str());
              num_candidates++;

              char letter = get_letter_from_code(pattern);
              bool pattern_is_code = letter != '#' && letter != '~';
              if (pattern_is_code)
              {
                  std::string last_word = get_last_word(text + letter);
                  float language_bonus = language_log_prob(last_word);

                  // Case 4: letter gap
                  assert(num_candidates < BEAM_WIDTH*3);
                  candidates[num_candidates].text    = text + letter;
                  candidates[num_candidates].pattern = "";
                  candidates[num_candidates].logp    = logp + logp_gap3;
                  DEBUG_PRINTF("%u letter_gap \"%s\" \"%s\"\n", i, candidates[num_candidates].text.c_str(), candidates[num_candidates].pattern.c_str());
                  num_candidates++;

                  // Case 5: word gap
                  assert(num_candidates < BEAM_WIDTH*3);
                  candidates[num_candidates].text    = text + letter + ' ';
                  candidates[num_candidates].pattern = "";
                  candidates[num_candidates].logp    = logp + logp_gap7 + language_bonus;
                  DEBUG_PRINTF("%u word_gap \"%s\" \"%s\"\n", i, candidates[num_candidates].text.c_str(), candidates[num_candidates].pattern.c_str());
                  num_candidates++;
              }
            }
        }

        //find best candidates to propagate forward
        items_in_beam = std::min(BEAM_WIDTH, num_candidates);
        std::vector<int> best_indices(num_candidates);
        std::iota(best_indices.begin(), best_indices.end(), 0);
        std::partial_sort(best_indices.begin(), best_indices.begin()+items_in_beam, best_indices.end(),
            [&](const int a, const int b) { return candidates[a].logp > candidates[b].logp; }
        );

        //copy best candidates into beam
        for(int idx = 0; idx < items_in_beam; ++idx)
        {
          assert(idx < BEAM_WIDTH);
          assert(idx < num_candidates);
          beam[idx] = candidates[best_indices[idx]];
        }

        assert(items_in_beam);

    }
}
