#include "webrtc/audio/utility/common_tool.h"

namespace webrtc {

  //-------------    RingBufferWrapper   ---------------//
const size_t RingBufferWrapper::kMaxBufferSize = 50 * 1024 * 1024;
RingBufferWrapper::RingBufferWrapper(size_t capacity, bool auto_adjust_capacity)
  : buff_handle_(WebRtc_CreateBuffer(capacity, 1)),
  auto_adjust_capacity_(auto_adjust_capacity) {
  }

  RingBufferWrapper::~RingBufferWrapper() {
    WebRtc_FreeBuffer(buff_handle_);
  }

const void* RingBufferWrapper:: BufferRead(void* data, size_t in_size, size_t* out_size) {
  if (nullptr == data || nullptr == out_size) {
    return nullptr;
  }
  void* data_ptr = nullptr;
  *out_size = WebRtc_ReadBuffer(buff_handle_, &data_ptr, data, in_size);
  return data_ptr;
}

size_t RingBufferWrapper::BufferRead(void* data, size_t size) {
  return WebRtc_ReadBuffer(buff_handle_, nullptr, data, size);
}

size_t RingBufferWrapper::BufferWrite(const void* data, size_t size) {
  size_t size_canbe_write = BufferCapacity() - BufferCurrentSize();
  if (size_canbe_write < size) {
    if (auto_adjust_capacity_) {
      AdjustCapacity(CalculatedExpansionCapacity(size));
      if (BufferCapacity() >= kMaxBufferSize) {
        auto_adjust_capacity_ = false;
        if (BufferCapacity() - BufferCurrentSize() < size) {
          WebRtc_MoveReadPtr(buff_handle_, size - BufferCapacity() + BufferCurrentSize());
        }
      }
    } else {
      WebRtc_MoveReadPtr(buff_handle_, size - size_canbe_write); // The extra data is thrown away
    }
  }
  return WebRtc_WriteBuffer(buff_handle_, data, size);
}

void RingBufferWrapper::Clear() {
  return WebRtc_InitBuffer(buff_handle_);
}

size_t RingBufferWrapper::BufferCurrentSize() {
  return WebRtc_available_read(buff_handle_);
}

size_t RingBufferWrapper::BufferCapacity() {
  return nullptr == buff_handle_ ? 0 : buff_handle_->element_count * buff_handle_->element_size;
}

int RingBufferWrapper::BufferSeek(int offset) {
  return nullptr == buff_handle_ ? 0 : WebRtc_MoveReadPtr(buff_handle_, offset);
}

void RingBufferWrapper::AdjustCapacity(size_t capacity) {
  if (capacity < BufferCurrentSize() || capacity == BufferCapacity()) {
    return;
  }
  RingBuffer* new_buff_handle = WebRtc_CreateBuffer(capacity, 1);
  size_t read_size = BufferCurrentSize();
  if (read_size > 0) {
    size_t read_size1 = buff_handle_->write_pos - buff_handle_->read_pos;
    read_size1 = read_size1 > 0? read_size1 : buff_handle_->element_count - buff_handle_->read_pos;
    WebRtc_WriteBuffer(new_buff_handle, buff_handle_->data + buff_handle_->read_pos *
        buff_handle_->element_size, read_size1);
    if (read_size > read_size1) {
      WebRtc_WriteBuffer(new_buff_handle, buff_handle_->data, read_size - read_size1);
    }
  }
  WebRtc_FreeBuffer(buff_handle_);
  buff_handle_ = new_buff_handle;
}

size_t RingBufferWrapper::CalculatedExpansionCapacity(size_t write_size) {
  size_t new_size = BufferCapacity();
  while (new_size - BufferCurrentSize() < write_size) {
    new_size *= 2;
  }
  RTC_DCHECK(write_size <= kMaxBufferSize);
  return std::min(new_size, kMaxBufferSize);
}
//-------------    RingBufferWrapper   ---------------//

//-------------    ThreadHandle   ---------------//
ThreadHandle::ThreadHandle(rtc::ThreadPriority thread_priority, const char* thread_name)
  : is_stop_(true),
    thread_handle_(std::make_unique<rtc::PlatformThread>(ThreadHandle::RunThread,
          this, thread_name, thread_priority)) {}

ThreadHandle::~ThreadHandle() {
  if (nullptr != thread_handle_) {
    thread_handle_->Stop();
  }
}

void ThreadHandle::FuctionIsOver() {}

bool ThreadHandle::Start() {
  if (is_stop_ && nullptr != thread_handle_) {
    thread_handle_->Start();
    is_stop_ = !thread_handle_->IsRunning();
  }
  return !is_stop_;
}

bool ThreadHandle::Stop() {
  if (!is_stop_ && nullptr != thread_handle_) {
    thread_handle_->Stop();
    is_stop_ = !thread_handle_->IsRunning();
  }
  return is_stop_;
}

bool  ThreadHandle::IsRunning() {
  return nullptr == thread_handle_ ? false : thread_handle_->IsRunning();
}

void ThreadHandle::RunThread(void* obj) {
  ThreadHandle* handle_ptr = reinterpret_cast<ThreadHandle*>(obj);
  bool ret = true;
  while(ret) {
    if (nullptr != handle_ptr) {
      ret = handle_ptr->Handle();
      if (!ret) {
        handle_ptr->FuctionIsOver();
      }
    }
  }
}

class ThreadHandleImpl final : public ThreadHandle {
public:
  ThreadHandleImpl(std::function<bool(void)> handle_func,
      const char* thread_name,rtc::ThreadPriority thread_priority);
private:
  bool Handle() override;

private:
  std::function<bool(void)> handle_func_;
};

ThreadHandleImpl::ThreadHandleImpl(std::function<bool(void)> handle_func,
    const char* thread_name,rtc::ThreadPriority thread_priority) :
  ThreadHandle(thread_priority, thread_name),
  handle_func_(handle_func) {}

bool ThreadHandleImpl::Handle() {
  return handle_func_();
}

std::unique_ptr<ThreadHandle> ThreadHandle::CreateThread(std::function<bool(void)> handle_func, const char* thread_name,
    rtc::ThreadPriority thread_priority) {
  return std::unique_ptr<ThreadHandle>(new ThreadHandleImpl(handle_func, thread_name, thread_priority));
}

//-------------    ThreadHandle   ---------------//

} // webrtc
