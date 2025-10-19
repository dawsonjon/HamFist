#include <cstdio>

#include "../cw_decoder/fft.h"
#include "../cw_decoder/cw_dsp.h"
int main()
{

  FILE *inf = fopen("test_recording.pcm", "rb");
  c_cw_dsp cw_dsp;
  fft_initialise();

  while(1)
  {
    int16_t sample;
    int success = fread(&sample, sizeof(int16_t), 1, inf);
    if(!success) return 1;
    cw_dsp.process_sample(sample);
  }

}
