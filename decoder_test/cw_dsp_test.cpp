#include <cstdio>
#include <cstdlib>

#include "../cw_decoder/fft.h"
#include "../cw_decoder/cw_dsp.h"
int main(int argc, char ** argv)
{
  if(argc < 2)
  {
    printf("usage: test input.wav\n");
    return -1;
  }
  printf("%s\n", argv[1]);

  FILE *inf = fopen(argv[1], "rb");
  c_cw_dsp cw_dsp;
  fft_initialise();

  while(1)
  {
    int16_t sample;
    int success = fread(&sample, sizeof(int16_t), 1, inf);
    if(!success){ 
      cw_dsp.flush();
      return 1;
    }
    cw_dsp.process_sample(sample);
  }

}
