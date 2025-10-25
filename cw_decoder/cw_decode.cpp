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

//find the n most likely decodes
std::string c_cw_decoder :: get_text()
{

  //std::string letter = std::string("")+get_letter_from_code(beam[0].pattern);
  //std::string word = beam[0].word + letter;
  std::string text = beam[0].text;// + word + letter;

  //prune candidates that don't share the same prefix
  int filtered_candidate_index=1;
  for(int candidate_index = 1; candidate_index<items_in_beam; ++candidate_index) {
    if(beam[candidate_index].text == beam[0].text) {
      beam[filtered_candidate_index] = beam[candidate_index];
      beam[filtered_candidate_index].text = "";
      filtered_candidate_index++;
    } 
  }
  beam[0].text = "";
  items_in_beam = filtered_candidate_index;
  DEBUG_PRINTF("get_text %s\n", text.c_str());

  return text;
}

static void pre_filter_observations(s_observation signal[], int &num_observations)
{

  //set some hard limits on minimum and maximum sizes
  float dot_min_ms = 15.0f;
  float dash_max_ms = 720.0f;
  float min_hard = std::max(dot_min_ms*0.5f, 8.0f);
  float max_hard = dash_max_ms*2.0f;

  //remove low signals that are too small
  int filtered_index=0;
  for(int index=0; index<num_observations; ++index) {
    signal[filtered_index] = signal[index];
    while((index+2<num_observations) && (signal[index+1].mark == false) && (signal[index+1].duration<min_hard)) {
      signal[filtered_index].duration += signal[index+1].duration + signal[index+2].duration;
      index+=2;
    }
    filtered_index++;
  }
  num_observations = filtered_index;

  //remove high signals that are too small
  filtered_index=0;
  for(int index=0; index<num_observations; ++index) {
    signal[filtered_index] = signal[index];
    while((index+2<num_observations) && (signal[index+1].mark == true) && (signal[index+1].duration<min_hard)) {
      signal[filtered_index].duration += signal[index+1].duration + signal[index+2].duration;
      index+=2;
    }
    filtered_index++;
  }
  num_observations = filtered_index;

  //remove high signals that are too big
  filtered_index=0;
  for(int index=0; index<num_observations; ++index) {
    signal[filtered_index] = signal[index];
    while((index+2<num_observations) && (signal[index+1].mark == true) && (signal[index+1].duration>max_hard)) {
      signal[filtered_index].duration += signal[index+1].duration + signal[index+2].duration;
      index+=2;
    }
    filtered_index++;
  }
  num_observations = filtered_index;

  //remove high signals are in the middle of two long spaces
  filtered_index=0;
  for(int index=0; index<num_observations; ++index) {
    signal[filtered_index] = signal[index];
    while((index+2<num_observations) && (signal[index+1].mark == true) && (signal[index].duration > max_hard) && (signal[index+2].duration > max_hard)) {
      signal[filtered_index].duration += signal[index+1].duration + signal[index+2].duration;
      index+=2;
    }
    filtered_index++;
  }
  num_observations = filtered_index;

} 

void print_durations(s_observation signal[], int num_observations)
{
    DEBUG_PRINTF("Duration: \n");
    for(int idx=0; idx<num_observations; idx++) {
      DEBUG_PRINTF("%u %u, %f\n", idx, signal[idx].mark, signal[idx].duration);
    }
    DEBUG_PRINTF("\n");
}

//find the n most likely decodes
void c_cw_decoder :: decode(s_observation signal[], int num_observations)
{

    //prefilter signal to remove obvious errors
    print_durations(signal, num_observations);
    pre_filter_observations(signal, num_observations);
    print_durations(signal, num_observations);

    //update adaptive clissifier with latest on and off durations
    float on_durations[num_observations];
    float off_durations[num_observations];
    int on_count = 0;
    int off_count = 0;
    for(int idx=0; idx<num_observations; ++idx) {
      if(signal[idx].mark) on_durations[on_count++] = signal[idx].duration;
      if(!signal[idx].mark) off_durations[off_count++] = signal[idx].duration;
    }

    // ON (key down) times:
    classifier.updateOnModel(on_durations, on_count);

    // OFF (silence) times:
    classifier.updateOffModel(off_durations, off_count);


    DEBUG_PRINTF("num observations %u\n", num_observations);
    for(int i=0; i<num_observations; ++i)
    {
      
        float duration = signal[i].duration;
        float logp_dot, logp_dash;
        classifier.classifyOn(duration, logp_dot, logp_dash);
        if(signal[i].mark) DEBUG_PRINTF("duration: %f dot: %f dash: %f\n", duration, logp_dot, logp_dash);

        float logp[3];
        classifier.classifyOff(duration, logp);
        float logp_gap1 = logp[0];
        float logp_gap3 = logp[1];
        float logp_gap7 = logp[2];
        if(!signal[i].mark) DEBUG_PRINTF("duration: %f symbol: %f letter: %f space: %f\n", duration, logp_gap1, logp_gap3, logp_gap7);


        //attempt to fix glitches 
        //if(i+2 < num_observations && signal[i+1].duration < mu/4) { //next symbol is a glitch
          //assume that this symbol and the next one (gitch) and the one after should have been one symbol
        //  const float fixed_duration = signal[i].duration + signal[i+1].duration + signal[i+2].duration; 
        //  signal[i+2].duration = fixed_duration;
        //  i++; //skip this symbol ....
        //  continue; //... and the next one
        //}

        s_candidate candidates[BEAM_WIDTH*4]; //max 3 new predictions for each item in beam
        int num_candidates = 0;

        for(int j=0; j<items_in_beam; j++)
        {
            std::string &text = beam[j].text;
            std::string &word = beam[j].word;
            std::string &pattern = beam[j].pattern;
            float &logp = beam[j].logp;

            if(signal[i].mark) {
              
              char letter = get_letter_from_code(pattern);
              bool pattern_is_code = letter != '#' && letter != '~';
              
              // Case 1: dot
              std::string dot_pattern = pattern+'.';
              if (is_start_of_code(dot_pattern))
              {
                assert(num_candidates < BEAM_WIDTH*4);
                candidates[num_candidates].text    = text;
                candidates[num_candidates].word    = word;
                candidates[num_candidates].pattern = pattern + '.';
                candidates[num_candidates].logp    = logp + logp_dot;
                DEBUG_PRINTF("%u dot \"%s\" \"%s\" %s %f\n", i, candidates[num_candidates].text.c_str(), candidates[num_candidates].word.c_str(), candidates[num_candidates].pattern.c_str(), candidates[num_candidates].logp);
                num_candidates++;
              }
              else if (pattern_is_code)
              {
                assert(num_candidates < BEAM_WIDTH*4);
                candidates[num_candidates].text    = text;
                candidates[num_candidates].word    = word + letter;
                candidates[num_candidates].pattern = ".";
                candidates[num_candidates].logp    = logp + logp_dot;
                DEBUG_PRINTF("%u dot2 \"%s\" \"%s\" %s %f\n", i, candidates[num_candidates].text.c_str(), candidates[num_candidates].word.c_str(), candidates[num_candidates].pattern.c_str(), candidates[num_candidates].logp);
                num_candidates++;
              }


              // Case 2: dash
              std::string dash_pattern = pattern+'-';
              if (is_start_of_code(dash_pattern))
              {
                assert(num_candidates < BEAM_WIDTH*4);
                candidates[num_candidates].text    = text;
                candidates[num_candidates].word    = word;
                candidates[num_candidates].pattern = pattern + '-';
                candidates[num_candidates].logp    = logp + logp_dash;
                DEBUG_PRINTF("%u dash \"%s\" \"%s\" %s %f\n", i, candidates[num_candidates].text.c_str(), candidates[num_candidates].word.c_str(), candidates[num_candidates].pattern.c_str(), candidates[num_candidates].logp);
                num_candidates++;
              }
              else if (pattern_is_code)
              {
                assert(num_candidates < BEAM_WIDTH*4);
                candidates[num_candidates].text    = text;
                candidates[num_candidates].word    = word + letter;
                candidates[num_candidates].pattern = "-";
                candidates[num_candidates].logp    = logp + logp_dash;
                DEBUG_PRINTF("%u dash2 \"%s\" \"%s\" %s %f\n", i, candidates[num_candidates].text.c_str(), candidates[num_candidates].word.c_str(), candidates[num_candidates].pattern.c_str(), candidates[num_candidates].logp);
                num_candidates++;
              }

            }
            else
            {
              //Case 3: symbol gap
              assert(num_candidates < BEAM_WIDTH*4);
              candidates[num_candidates].text    = text;
              candidates[num_candidates].word    = word;
              candidates[num_candidates].pattern = pattern;
              candidates[num_candidates].logp    = logp + logp_gap1;
              DEBUG_PRINTF("%u symbol_gap \"%s\" \"%s\" %s %f\n", i, candidates[num_candidates].text.c_str(), candidates[num_candidates].word.c_str(), candidates[num_candidates].pattern.c_str(), candidates[num_candidates].logp);
              num_candidates++;

              char letter = get_letter_from_code(pattern);
              bool pattern_is_code = letter != '#' && letter != '~';
              if (pattern_is_code)
              {
                  std::string last_word = word + letter;
                  float language_bonus = language_log_prob(last_word);

                  // Case 4: letter gap
                  assert(num_candidates < BEAM_WIDTH*4);
                  candidates[num_candidates].word    = word + letter;
                  candidates[num_candidates].text    = text;
                  candidates[num_candidates].pattern = "";
                  candidates[num_candidates].logp    = logp + logp_gap3;
                  DEBUG_PRINTF("%u letter_gap \"%s\" \"%s\" %s %f\n", i, candidates[num_candidates].text.c_str(), candidates[num_candidates].word.c_str(), candidates[num_candidates].pattern.c_str(), candidates[num_candidates].logp);
                  num_candidates++;

                  // Case 5: word gap
                  assert(num_candidates < BEAM_WIDTH*4);
                  candidates[num_candidates].text    = text + word + letter + ' ';
                  candidates[num_candidates].word    = "";
                  candidates[num_candidates].pattern = "";
                  candidates[num_candidates].logp    = logp + logp_gap7 + language_bonus;
                  DEBUG_PRINTF("language bonus%f\n", language_bonus);
                  DEBUG_PRINTF("%u word_gap \"%s\" \"%s\" %s %f\n", i, candidates[num_candidates].text.c_str(), candidates[num_candidates].word.c_str(), candidates[num_candidates].pattern.c_str(), candidates[num_candidates].logp);
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
