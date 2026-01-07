#ifndef _utils_
#define _utils_

#include <cstdint>
#include <cstring>

extern int16_t sin_table[2048];

uint16_t rectangular_2_magnitude(int16_t i, int16_t q);
void rectangular_2_polar(int16_t i, int16_t q, uint16_t* mag, int16_t* phase);
void initialise_luts();

bool str_insert(char* buffer, size_t buffer_size, size_t index, const char* insert);

bool str_replace(char* buffer, size_t buffer_size, const char* search, const char* replace);

bool str_replace_all(char* buffer, size_t buffer_size, const char* search, const char* replace);

#endif
