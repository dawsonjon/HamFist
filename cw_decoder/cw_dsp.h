#ifndef _CW_DECODER_H__
#define _CW_DECODER_H__

#include <cstdint>
#include <list>
#include <vector>

#include "cw_dsp.h"
#include "cw_decode.h"

static const uint16_t FRAME_SIZE = 64;
static const uint16_t MAX_DECODERS = 10;
static const uint16_t OBSERVATION_BUFFER_SIZE = 400;
static const uint16_t TIMEOUT = 1000;
static const uint16_t CLUSTER_WIDTH = 2;

struct s_cluster
{
  uint16_t bin;
  bool value;
  bool last_value;
  uint16_t duration;
  uint32_t timeout;
  s_observation observations[OBSERVATION_BUFFER_SIZE];
  uint16_t num_observations;
  uint16_t move_up_count;
  uint16_t move_down_count;
  uint32_t frame_count;
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
  uint32_t smoothed_threshold;
  uint32_t frame_count;

  std::list<s_cluster> clusters;

  void print_frame(const char filename[], uint32_t frame[]);
  void print_element(const char filename[], uint32_t element);
  void print_bool(const char filename[], bool element);
  void process_frame();
  void cluster_detections(uint32_t threshold);
  void process_clusters(uint32_t threshold);
  void decode(uint8_t bin, bool state, uint16_t duration);

  public:

  c_cw_dsp();
  void process_sample(int16_t sample);

};

#endif
