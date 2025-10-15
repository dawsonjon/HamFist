#ifndef __CW_DECODE_H__
#define __CW_DECODE_H__


struct s_observation
{
  bool mark;
  float duration;
};

void decode_cw(s_observation signal[], int num_observations, int beam_width);

#endif
