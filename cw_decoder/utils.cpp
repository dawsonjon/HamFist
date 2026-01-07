#include "utils.h"
#include <cstdint>
#include <math.h>

// from: http://dspguru.com/dsp/tricks/magnitude-estimator/
uint16_t rectangular_2_magnitude(int16_t i, int16_t q)
{
  // Measure magnitude
  const int16_t absi = i > 0 ? i : -i;
  const int16_t absq = q > 0 ? q : -q;
  return absi > absq ? absi + absq / 4 : absq + absi / 4;
}

// Returns true if insertion succeeded
bool str_insert(char* buffer, size_t buffer_size, size_t index, const char* insert)
{
  if (!buffer || !insert)
    return false;

  size_t buf_len = strnlen(buffer, buffer_size);
  size_t insert_len = strlen(insert);

  // Buffer not properly terminated or index out of range
  if (buf_len >= buffer_size || index > buf_len)
    return false;

  // New length including insertion (excluding null)
  size_t new_len = buf_len + insert_len;

  // Check capacity (+1 for null terminator)
  if (new_len + 1 > buffer_size)
    return false;

  // Move tail right to make room (include '\0')
  memmove(buffer + index + insert_len, buffer + index, buf_len - index + 1);

  // Copy inserted text
  memcpy(buffer + index, insert, insert_len);

  return true;
}

bool str_replace(char* buffer, size_t buffer_size, const char* search, const char* replace)
{
  if (!buffer || !search || !replace)
    return false;

  size_t buf_len = strnlen(buffer, buffer_size);
  size_t search_len = strlen(search);
  size_t replace_len = strlen(replace);

  if (search_len == 0 || buf_len >= buffer_size)
    return false;

  char* pos = strstr(buffer, search);
  if (!pos)
    return false;

  size_t index = pos - buffer;

  // Calculate new length (excluding null terminator)
  size_t new_len = buf_len - search_len + replace_len;

  // Check buffer capacity
  if (new_len + 1 > buffer_size)
    return false;

  // Move the tail if needed
  if (replace_len != search_len) {
    memmove(buffer + index + replace_len, buffer + index + search_len,
            buf_len - index - search_len + 1); // include '\0'
  }

  // Copy replacement
  memcpy(buffer + index, replace, replace_len);

  return true;
}

bool str_replace_all(char* buffer, size_t buffer_size, const char* search, const char* replace)
{
  bool replaced = false;

  while (str_replace(buffer, buffer_size, search, replace)) {
    replaced = true;
  }

  return replaced;
}