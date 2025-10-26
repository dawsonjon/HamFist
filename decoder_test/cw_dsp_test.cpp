#include <cstdio>
#include <cstdlib>

#include "../cw_decoder/fft.h"
#include "../cw_decoder/cw_dsp.h"

class my_cw_dsp : public c_cw_dsp
{
  std::string decoded_text[7]; 
  virtual void decode(uint16_t cluster, std::string text, std::string partial)
  {
    decoded_text[cluster]+=text; 
  }

  public:
  void print_decodes()
  {
    for(int i=0; i<7; ++i)
    {
      printf("decoder %u: %s\n", i, decoded_text[i].c_str());
    }
  }
};

int main(int argc, char ** argv)
{
  if(argc < 2)
  {
    printf("usage: test input.wav\n");
    return -1;
  }
  printf("%s\n", argv[1]);

  FILE *inf = fopen(argv[1], "rb");
  my_cw_dsp cw_dsp;
  fft_initialise();

  while(1)
  {
    int16_t sample;
    int success = fread(&sample, sizeof(int16_t), 1, inf);
    if(!success){ 
      cw_dsp.flush();
      cw_dsp.print_decodes();
      return 1;
    }
    cw_dsp.process_sample(sample);
  }

}
