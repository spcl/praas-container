
#ifndef __RUNTIME_RING_BUFFER_HPP__
#define __RUNTIME_RING_BUFFER_HPP__

#include <cstdint>
#include <string>
#include <queue>
#include <mutex>

namespace praas::buffer {

  template<typename T>
  struct Buffer
  {
    T* val;
    size_t size;

    Buffer():
      val(nullptr),
      size(0)
    {}

    Buffer(T* val, size_t size):
      val(val),
      size(size)
    {}
  };

  template<typename T>
  struct BufferQueue {

    typedef Buffer<T> val_t;

    std::queue<val_t> _buffers;
    std::mutex _mutex;
    size_t _elements;

    BufferQueue(size_t elements, size_t elem_size):
      _elements(elements)
    {
      for(size_t i = 0; i < _elements; ++i)
        _buffers.push(val_t{new T[elem_size], elem_size});
    }

    ~BufferQueue()
    {
      while(!_buffers.empty()) {
        delete[] _buffers.front().val;
        _buffers.pop();
      }
    }

    Buffer<T> retrieve_buffer(size_t size)
    {
      const std::lock_guard<std::mutex> lock(_mutex);
      if(_buffers.empty())
        return {nullptr, 0};

      Buffer buf = _buffers.front();
      if(_buffers.size() < size) {
        delete[] buf.val;
        buf.val = new T[size];
        buf.size = size;
      }
      _buffers.pop();
      return buf;
    }

    void return_buffer(Buffer<T> && buf)
    {
      const std::lock_guard<std::mutex> lock(_mutex);
      _buffers.push(buf);
    }
  };

}

#endif
