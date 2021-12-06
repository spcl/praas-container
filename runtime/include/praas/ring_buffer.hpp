
#ifndef __RUNTIME_RING_BUFFER_HPP__
#define __RUNTIME_RING_BUFFER_HPP__

#include <cstdint>
#include <string>

namespace praas::ring_buffer {

  struct RingBuffer {
    typedef size_t index_t;

    index_t head;
    index_t tail;

  };

};

#endif
