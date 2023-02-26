#ifndef TIMESTAMP_HH
#define TIMESTAMP_HH

#include <cstdint>

/* nanoseconds since epoch */
uint64_t timestamp_ns();

/* microseconds since epoch */
uint64_t timestamp_us();

/* milliseconds since epoch */
uint64_t timestamp_ms();

#endif /* TIMESTAMP_HH */
