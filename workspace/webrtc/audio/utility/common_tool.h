/**
* common_tool.h, December 2020.
*
* Copyright 2020 fenbi.com. All rights reserved.
* FENBI.COM PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
*/
#ifndef WEBRTC_AUDIO_UTILITY_COMMON_TOOL_H_
#define WEBRTC_AUDIO_UTILITY_COMMON_TOOL_H_

#include "webrtc/common_audio/ring_buffer.h"

namespace webrtc {

//----------------------  CacheBuffer ----------------------------------//
class RingBufferWrapper {
public:
  RingBufferWrapper(const RingBufferWrapper&) = delete;
  RingBufferWrapper(RingBufferWrapper&&) = delete;
  // If expandable_ is true, the buffer will automatically expand when it is full,
  // otherwise, the buffer will overwrite the past data when it is full
  RingBufferWrapper(size_t capacity, bool auto_expand_capacity = false);
  virtual ~RingBufferWrapper();
  const void* BufferRead(void* data, size_t in_size, size_t* out_size);
  size_t BufferRead(void* data, size_t size);
  size_t BufferWrite(const void* data, size_t size);
  void Clear();
  size_t BufferCurrentSize();
  size_t BufferCapacity();
  int BufferSeek(int offset);
  void AdjustCapacity(size_t capacity);

private:
  size_t CalculatedExpansionCapacity(size_t write_size);
private:
  // If buffer is in expandable mode, exceeding this capacity will automatically turn into non-expandable mode
  static const size_t kMaxBufferSize;

  RingBuffer* buff_handle_ = nullptr;
  bool auto_adjust_capacity_; // Automatically adjusts buffer capacity
};
//----------------------  CacheBuffer ----------------------------------//

} // namespace fenbi
#endif // WEBRTC_AUDIO_UTILITY_COMMON_TOOL_H_
