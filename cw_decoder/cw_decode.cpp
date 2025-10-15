#include <cmath>
#include <cstring>
#include <string>
#include <algorithm>
#include <vector>
#include <numeric>

#include "cw_data.h"
#include "cw_decode.h"

//#include <cstdio>


//return the log likelihood x in a normal distribution mu/sigma
float log_gaussian(float x, float mu, float sigma)
{
    return -0.5f * pow((x - mu) / sigma, 2.0f);
}

//Is this pattern part of a morse symbol?
bool is_start_of_code(std::string &pattern)
{
    for(int idx=0; idx<NUM_MORSE_LETTERS; ++idx)
    {
      if(strncmp(MORSE[idx].code, pattern.c_str(), strlen(pattern.c_str())) == 0) return true;
    }
    return false;
}

//Is this pattern an exact match to a morse symbol?
bool is_code(std::string &pattern)
{
    for(int idx=0; idx<NUM_MORSE_LETTERS; ++idx)
    {
      if(strlen(pattern.c_str()) != strlen(MORSE[idx].code)) continue;
      if(strncmp(MORSE[idx].code, pattern.c_str(), strlen(pattern.c_str())) == 0) return true;
    }
    return false;
}

//What letter does this code correspond to?
char get_letter_from_code(std::string &pattern)
{
    for(int idx=0; idx<NUM_MORSE_LETTERS; ++idx)
    {
      if(strlen(pattern.c_str()) != strlen(MORSE[idx].code)) continue;
      if(strncmp(MORSE[idx].code, pattern.c_str(), strlen(pattern.c_str())) == 0) return MORSE[idx].letter;
    }
    return '#';
}

//What is the log probability that this is a word?
float language_log_prob(std::string &word)
{
    
    for(int idx=0; idx<NUM_CW_ABBREVIATIONS; ++idx)
    {
      if(strncmp(CW_ABBREVIATIONS[idx], word.c_str(), strlen(word.c_str())) == 0) return 6.0f;
    }

    for(int idx=0; idx<NUM_CW_WORDS; ++idx)
    {
      if(strncmp(CW_WORDS[idx], word.c_str(), strlen(word.c_str())) == 0) return 4.0f;
    }

    return 0;
}

//Find last word in a string
std::string get_last_word(const std::string& text) 
{
    // Trim trailing spaces
    size_t end = text.find_last_not_of(" \t\n\r");
    if (end == std::string::npos) return ""; // no non-space characters

    // Find start of last word
    size_t start = text.find_last_of(" \t\n\r", end);
    if (start == std::string::npos)
        start = 0;
    else
        start += 1; // move past the space

    return text.substr(start, end - start + 1);
}

//cluster **************
//def cluster(x, k, iterations):
//    clusters = [[] for i in range(k)]
//    for item in range(len(x)): 
//        clusters[item%k].append(x[item])
//
//    for iteration in range(iterations):
//        means = [np.mean(cluster) for cluster in clusters]
//        clusters = [[] for i in range(k)]
//        for item in x:
//            distances = [abs(item-mean)*abs(item-mean) for mean in means]
//            cluster = np.argmin(distances)
//            clusters[cluster].append(item)
//
//    clusters = np.array(clusters)[np.argsort(means)]
//    return clusters

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

struct s_candidate
{
  std::string text;
  std::string pattern;
  float logp;
};


//find the n most likely decodes
void decode_cw(s_observation signal[], int num_observations, int beam_width)
{

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
    printf("mu %f dash_mu %f, sigma %f\n", mu, dah_mu, sigma);

    //use k-means to estimate space lengths
    float spaces[num_observations];
    int num_spaces = 0;
    for(int idx=0; idx<num_observations; idx++) { 
      if(!signal[idx].mark && signal[idx].duration < 10*mu) spaces[num_spaces++] = signal[idx].duration; 
    }
    kmeans(spaces, num_spaces, means, sigmas, 3, 5);
    float short_mu = means[0];
    float medium_mu = means[1];
    float long_mu = means[2];
    float short_sigma = sigmas[0];
    float medium_sigma = sigmas[1];
    float long_sigma = sigmas[2];
    printf("short mu %f medium_mu %f, long_mu %f\n", short_mu, medium_mu, long_mu);
    printf("short sigma %f medium sigma %f, long sigma %f\n", short_sigma, medium_sigma, long_sigma);


    //the beam contains hte most likely decodes and their probabilities
    s_candidate beam[beam_width];
    beam[0] = {"", "", 0.0f}; //decoded_text, current_pattern, log_probability
    int items_in_beam = 1;

    for(int i=0; i<num_observations; ++i)
    {


        float logp_dot  = log_gaussian(signal[i].duration, mu, sigma);
        float logp_dash = log_gaussian(signal[i].duration, dah_mu, sigma);
        float logp_gap1 = log_gaussian(signal[i].duration, short_mu, short_sigma);
        float logp_gap3 = log_gaussian(signal[i].duration, medium_mu, medium_sigma);
        float logp_gap7 = signal[i].duration<long_mu?log_gaussian(signal[i].duration, long_mu, long_sigma):0;

        s_candidate candidates[beam_width*3]; //max 3 new predictions for each item in beam
        int num_candidates = 0;


        for(int j=0; j<items_in_beam; j++)
        {

            if(signal[i].mark) {
              // Case 1: dot
              std::string dot_pattern = beam[j].pattern+'.';
              if (is_start_of_code(dot_pattern))
              {
                candidates[num_candidates].text    = beam[j].text;
                candidates[num_candidates].pattern = beam[j].pattern + '.';
                candidates[num_candidates].logp    = beam[j].logp + logp_dot;
                //printf("dot \"%s\" \"%s\"\n", candidates[num_candidates].text.c_str(), candidates[num_candidates].pattern.c_str());
                num_candidates++;
              }
              else if (is_code(beam[j].pattern))
              {
                candidates[num_candidates].text    = beam[j].text + get_letter_from_code(beam[j].pattern);
                candidates[num_candidates].pattern = ".";
                candidates[num_candidates].logp    = beam[j].logp + logp_dot;
                //printf("dot2 \"%s\" \"%s\"\n", candidates[num_candidates].text.c_str(), candidates[num_candidates].pattern.c_str());
                num_candidates++;
              }

              // Case 2: dash
              std::string dash_pattern = beam[j].pattern+'-';
              if (is_start_of_code(dash_pattern))
              {
                candidates[num_candidates].text    = beam[j].text;
                candidates[num_candidates].pattern = beam[j].pattern + '-';
                candidates[num_candidates].logp    = beam[j].logp + logp_dash;
                //printf("dash \"%s\" \"%s\"\n", candidates[num_candidates].text.c_str(), candidates[num_candidates].pattern.c_str());
                num_candidates++;
              }
              else if (is_code(beam[j].pattern))
              {
                candidates[num_candidates].text    = beam[j].text + get_letter_from_code(beam[j].pattern);
                candidates[num_candidates].pattern = "-";
                candidates[num_candidates].logp    = beam[j].logp + logp_dash;
                //printf("dash2 \"%s\" \"%s\"\n", candidates[num_candidates].text.c_str(), candidates[num_candidates].pattern.c_str());
                num_candidates++;
              }

            }
            else
            {
              //Case 3: symbol gap
              candidates[num_candidates].text    = beam[j].text;
              candidates[num_candidates].pattern = beam[j].pattern;
              candidates[num_candidates].logp    = beam[j].logp + logp_gap1;
              //printf("symbol_gap \"%s\" \"%s\"\n", candidates[num_candidates].text.c_str(), candidates[num_candidates].pattern.c_str());
              num_candidates++;

              if (is_code(beam[j].pattern))
              {
                  std::string letter = std::string("")+get_letter_from_code(beam[j].pattern);
                  std::string last_word = get_last_word(beam[j].text + letter);
                  float language_bonus = language_log_prob(last_word);

                  // Case 4: letter gap
                  candidates[num_candidates].text    = beam[j].text + letter;
                  candidates[num_candidates].pattern = "";
                  candidates[num_candidates].logp    = beam[j].logp + logp_gap3;
                  //printf("letter_gap \"%s\" \"%s\"\n", candidates[num_candidates].text.c_str(), candidates[num_candidates].pattern.c_str());
                  num_candidates++;

                  // Case 5: word gap
                  candidates[num_candidates].text    = beam[j].text + letter + ' ';
                  candidates[num_candidates].pattern = "";
                  candidates[num_candidates].logp    = beam[j].logp + logp_gap7 + language_bonus;
                  //printf("word_gap \"%s\" \"%s\"\n", candidates[num_candidates].text.c_str(), candidates[num_candidates].pattern.c_str());
                  num_candidates++;
              }
           }
        }

        //sort candidates in descending order of logp
        std::sort(candidates, candidates + num_candidates,
            [](const s_candidate& a, const s_candidate& b) { return a.logp > b.logp; }
        );

        //copy best candidates into beam
        items_in_beam = std::min(beam_width, num_candidates);
        for(int idx = 0; idx < items_in_beam; ++idx)
        {
          beam[idx] = candidates[idx];
        }


    }

    std::string letter = std::string("")+get_letter_from_code(beam[0].pattern);
    std::string text = beam[0].text + letter;
    printf("%s\n", text.c_str());

}
