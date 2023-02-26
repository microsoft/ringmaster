#include <chrono>
#include "timestamp.hh"

using namespace std;
using namespace chrono;

uint64_t timestamp_ns()
{
  return system_clock::now().time_since_epoch() / 1ns;
}

uint64_t timestamp_us()
{
  return system_clock::now().time_since_epoch() / 1us;
}

uint64_t timestamp_ms()
{
  return system_clock::now().time_since_epoch() / 1ms;
}
