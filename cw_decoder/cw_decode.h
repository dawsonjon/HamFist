#ifndef __CW_DECODE_H__
#define __CW_DECODE_H__

#include <string>


struct s_observation
{
  bool mark;
  float duration;
};

struct s_candidate
{
  std::string text;
  std::string word;
  std::string pattern;
  float logp;
};

static const int BEAM_WIDTH = 3;
class c_cw_decoder
{

  private:
  s_candidate beam[BEAM_WIDTH];
  int items_in_beam;
    
  //timing variables
  float mu;
  float dah_mu;
  float sigma;
  float short_mu;
  float medium_mu;
  float long_mu;
  float short_sigma;
  float medium_sigma;
  float long_sigma;
  bool have_fallback_mark;
  bool have_fallback_space;

  public:
  void decode(s_observation signal[], int num_observations);
  std::string get_text();

  c_cw_decoder()
  {
    beam[0] = {"", "", "", 0.0f};
    items_in_beam = 1;
  }

};

#endif
