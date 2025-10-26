#ifndef _CW_DECODER_H__
#define _CW_DECODER_H__

#include <cstdint>
#include <list>
#include <vector>

#include "cw_dsp.h"
#include "cw_decode.h"

static const uint16_t FRAME_SIZE = 64;
static const uint16_t OBSERVATION_BUFFER_SIZE = 200;
static const uint16_t TIMEOUT = 1000;
static const uint16_t NUM_CLUSTERS = 6;
static const uint16_t CLUSTER_SIZE = 5;
static const float SAMPLE_FREQUENCY = 15000.0f;
static const float FRAME_MS = 1000.0f*64.0f/SAMPLE_FREQUENCY;

struct s_cluster
{
  uint16_t duration;
  bool value;
  s_observation observations[OBSERVATION_BUFFER_SIZE];
  uint16_t num_observations;
  c_cw_decoder decoder;
};

class c_cw_dsp
{

  uint16_t frequency_count=0;
  uint32_t sample_number_f16=0;
  int16_t i[FRAME_SIZE];
  int16_t q[FRAME_SIZE];
  int32_t window[FRAME_SIZE];
  uint32_t magnitude[FRAME_SIZE/2];
  uint32_t old_magnitude[FRAME_SIZE/2];
  uint32_t new_magnitude[FRAME_SIZE/2];
  uint32_t smoothed_threshold;
  uint32_t frame_count;

  s_cluster clusters[7];

  void print_frame(const char filename[], uint32_t frame[]);
  void print_element(const char filename[], uint32_t element);
  void print_bool(const char filename[], bool element);
  void process_frame();
  void process_clusters(uint32_t threshold);
  virtual void decode(uint16_t cluster, std::string text, std::string partial);

  public:

  c_cw_dsp();
  void process_sample(int16_t sample);
  void flush();
  uint32_t *get_magnitudes()
  {
    return &magnitude[0];
  }

};

#endif
