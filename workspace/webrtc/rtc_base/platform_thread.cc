/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "rtc_base/platform_thread.h"

#if !defined(WEBRTC_WIN)
#include <sched.h>
#endif
#include <stdint.h>
#include <time.h>

#include <algorithm>
#include <chrono>

#include "rtc_base/checks.h"

namespace rtc {
namespace {
#if !defined(WEBRTC_WIN)
struct ThreadAttributes {
  ThreadAttributes() { pthread_attr_init(&attr); }
  ~ThreadAttributes() { pthread_attr_destroy(&attr); }
  pthread_attr_t* operator&() { return &attr; }
  pthread_attr_t attr;
};
#endif  // defined(WEBRTC_WIN)
}  // namespace

#if !defined(WEBRTC_WIN)
typedef pthread_t THREAD_TYPE;
#else
typedef HANDLE THREAD_TYPE;
#endif  // defined(WEBRTC_WIN)
static bool StaticSetThreadPriority(THREAD_TYPE thread_handle, ThreadPriority priority) {

#if defined(WEBRTC_WIN)
  return SetThreadPriority(thread_handle, priority) != FALSE;
#elif defined(__native_client__) || defined(WEBRTC_FUCHSIA)
  // Setting thread priorities is not supported in NaCl or Fuchsia.
  return true;
#elif defined(WEBRTC_CHROMIUM_BUILD) && defined(WEBRTC_LINUX)
  // TODO(tommi): Switch to the same mechanism as Chromium uses for changing
  // thread priorities.
  return true;
#else
  const int policy = SCHED_FIFO;
  const int min_prio = sched_get_priority_min(policy);
  const int max_prio = sched_get_priority_max(policy);
  if (min_prio == -1 || max_prio == -1) {
    return false;
  }

  if (max_prio - min_prio <= 2)
    return false;

  // Convert webrtc priority to system priorities:
  sched_param param;
  const int top_prio = max_prio - 1;
  const int low_prio = min_prio + 1;
  switch (priority) {
    case kLowPriority:
      param.sched_priority = low_prio;
      break;
    case kNormalPriority:
      // The -1 ensures that the kHighPriority is always greater or equal to
      // kNormalPriority.
      param.sched_priority = (low_prio + top_prio - 1) / 2;
      break;
    case kHighPriority:
      param.sched_priority = std::max(top_prio - 2, low_prio);
      break;
    case kHighestPriority:
      param.sched_priority = std::max(top_prio - 1, low_prio);
      break;
    case kRealtimePriority:
      param.sched_priority = top_prio;
      break;
  }
  return pthread_setschedparam(thread_handle, policy, &param) == 0;
#endif  // defined(WEBRTC_WIN)

}

PlatformThread::PlatformThread(ThreadRunFunction func,
                               void* obj,
                               absl::string_view thread_name,
                               ThreadPriority priority /*= kNormalPriority*/)
    : run_function_(func), priority_(priority), obj_(obj), name_(thread_name) {
  RTC_DCHECK(func);
  RTC_DCHECK(!name_.empty());
  // TODO(tommi): Consider lowering the limit to 15 (limit on Linux).
  RTC_DCHECK(name_.length() < 64);
  spawned_thread_checker_.Detach();
}

PlatformThread::~PlatformThread() {
  RTC_DCHECK(thread_checker_.IsCurrent());
#if defined(WEBRTC_WIN)
  RTC_DCHECK(!thread_);
  RTC_DCHECK(!thread_id_);
#endif  // defined(WEBRTC_WIN)
}

#if defined(WEBRTC_WIN)
DWORD WINAPI PlatformThread::StartThread(void* param) {
  // The GetLastError() function only returns valid results when it is called
  // after a Win32 API function that returns a "failed" result. A crash dump
  // contains the result from GetLastError() and to make sure it does not
  // falsely report a Windows error we call SetLastError here.
  ::SetLastError(ERROR_SUCCESS);
  static_cast<PlatformThread*>(param)->Run();
  return 0;
}
#else
void* PlatformThread::StartThread(void* param) {
  static_cast<PlatformThread*>(param)->Run();
  return 0;
}
#endif  // defined(WEBRTC_WIN)

void PlatformThread::Start() {
  RTC_DCHECK(thread_checker_.IsCurrent());
  RTC_DCHECK(!thread_) << "Thread already started?";
#if defined(WEBRTC_WIN)
  // See bug 2902 for background on STACK_SIZE_PARAM_IS_A_RESERVATION.
  // Set the reserved stack stack size to 1M, which is the default on Windows
  // and Linux.
  thread_ = ::CreateThread(nullptr, 1024 * 1024, &StartThread, this,
                           STACK_SIZE_PARAM_IS_A_RESERVATION, &thread_id_);
  RTC_CHECK(thread_) << "CreateThread failed";
  RTC_DCHECK(thread_id_);
#else
  ThreadAttributes attr;
  // Set the stack stack size to 1M.
  pthread_attr_setstacksize(&attr, 1024 * 1024);
  RTC_CHECK_EQ(0, pthread_create(&thread_, &attr, &StartThread, this));
#endif  // defined(WEBRTC_WIN)
}

bool PlatformThread::IsRunning() const {
  RTC_DCHECK(thread_checker_.IsCurrent());
#if defined(WEBRTC_WIN)
  return thread_ != nullptr;
#else
  return thread_ != 0;
#endif  // defined(WEBRTC_WIN)
}

PlatformThreadRef PlatformThread::GetThreadRef() const {
#if defined(WEBRTC_WIN)
  return thread_id_;
#else
  return thread_;
#endif  // defined(WEBRTC_WIN)
}

void PlatformThread::Stop() {
  RTC_DCHECK(thread_checker_.IsCurrent());
  if (!IsRunning())
    return;

#if defined(WEBRTC_WIN)
  WaitForSingleObject(thread_, INFINITE);
  CloseHandle(thread_);
  thread_ = nullptr;
  thread_id_ = 0;
#else
  RTC_CHECK_EQ(0, pthread_join(thread_, nullptr));
  thread_ = 0;
#endif  // defined(WEBRTC_WIN)
  spawned_thread_checker_.Detach();
}

void PlatformThread::Run() {
  // Attach the worker thread checker to this thread.
  RTC_DCHECK(spawned_thread_checker_.IsCurrent());
  rtc::SetCurrentThreadName(name_.c_str());
  SetPriority(priority_);
  run_function_(obj_);
}

bool PlatformThread::SetPriority(ThreadPriority priority) {
  RTC_DCHECK(spawned_thread_checker_.IsCurrent());
  return StaticSetThreadPriority(thread_, priority);
}

#if defined(WEBRTC_WIN)
bool PlatformThread::QueueAPC(PAPCFUNC function, ULONG_PTR data) {
  RTC_DCHECK(thread_checker_.IsCurrent());
  RTC_DCHECK(IsRunning());

  return QueueUserAPC(function, thread_, data) != FALSE;
}
#endif

//-------------    ThreadHandle   ---------------//
ThreadHandle::ThreadHandle(const char* name, rtc::ThreadPriority thread_priority)
  : name_(name),
    priority_(thread_priority),
    status_(ThreadStatus::kStop) {
}

ThreadHandle::~ThreadHandle() {
  if (ThreadStatus::kRunning == status_) {
    Stop();
  }
}

void ThreadHandle::FuctionIsOver() {}

ThreadStatus ThreadHandle::Start() {
  if (!SetStatus(ThreadStatus::kPrepareStart)) {
    return status_;
  }
#if defined(WEBRTC_WIN)
  thread_ = ::CreateThread(nullptr, 1024 * 1024, &RunThread, this,
                           STACK_SIZE_PARAM_IS_A_RESERVATION, &thread_id_);
  RTC_CHECK(thread_) << "CreateThread failed";
  RTC_DCHECK(thread_id_);
#else
  ThreadAttributes attr;
  // Set the stack stack size to 1M.
  pthread_attr_setstacksize(&attr, 1024 * 1024);
  RTC_CHECK_EQ(0, pthread_create(&thread_, &attr, &RunThread, this));
#endif  // defined(WEBRTC_WIN)
  SetStatus(ThreadStatus::kRunning);
  return status_;
}

ThreadStatus ThreadHandle::Stop() {
  // if status_ is ThreadStatus::kWait or kWaitForCondition, Convert it to ThreadStatus::kRunning
  WakeUp();

  // stop
  if (SetStatus(ThreadStatus::kPrepareStop) && nullptr != thread_) {
#if defined(WEBRTC_WIN)
    WaitForSingleObject(thread_, INFINITE);
    CloseHandle(thread_);
    thread_ = nullptr;
    thread_id_ = 0;
#else
    RTC_CHECK_EQ(0, pthread_join(thread_, nullptr));
    thread_ = 0;
#endif  // defined(WEBRTC_WIN)
  }
  SetStatus(ThreadStatus::kStop);
  return status_;
}

ThreadStatus  ThreadHandle::GetStatus() {
  return status_;
}

ThreadStatus ThreadHandle::Suspend() {
  std::lock_guard<std::mutex> lk(mutex_);
  if (ThreadStatus::kRunning == status_) {
    status_ = ThreadStatus::kWait;
  }
  return status_;
}

ThreadStatus ThreadHandle::Suspend(size_t wait_time_ms, std::function<bool(void)> condition) {
  std::lock_guard<std::mutex> lk(mutex_);
  if (ThreadStatus::kRunning == status_ && nullptr != condition && wait_time_ms > 0) {
    status_ = ThreadStatus::kWaitForCondition;
    end_wait_cond_ = condition;
    wait_time_ms_ = wait_time_ms;
  }
  return status_;
}

ThreadStatus ThreadHandle::WakeUp() {
  std::unique_lock<std::mutex> lk(mutex_);
  if (ThreadStatus::kWait == status_ || ThreadStatus::kWaitForCondition == status_) {
    status_ = ThreadStatus::kRunning;
    cond_.notify_one();
  }
  return status_;
}

bool ThreadHandle::IsThisStatus(const std::vector<ThreadStatus>& thread_status_vector) {
  std::lock_guard<std::mutex> lk(mutex_);
  for (ThreadStatus thread_status : thread_status_vector) {
    if (status_ == thread_status) {
      return true;
    }
  }
  return false;
}
bool ThreadHandle::SetStatus(ThreadStatus thread_status) {
  std::lock_guard<std::mutex> lk(mutex_);
  switch(thread_status) {
    case ThreadStatus::kPrepareStart:
      if (ThreadStatus::kStop != status_) {
        return false;
      }
      break;
    case ThreadStatus::kRunning:
      if (ThreadStatus::kPrepareStart != status_ &&
          ThreadStatus::kWait != status_ &&
          ThreadStatus::kWaitForCondition !=  status_) {
        return false;
      }
      break;
    case ThreadStatus::kWait:
    case ThreadStatus::kWaitForCondition:
      if (ThreadStatus::kRunning != status_) {
        return false;
      }
      break;
    case ThreadStatus::kPrepareStop:
      if (ThreadStatus::kPrepareStop == status_ || ThreadStatus::kStop == status_) {
        return false;
      }
      break;
    case ThreadStatus::kStop:
      if (ThreadStatus::kPrepareStop != status_) {
        return false;
      }
      break;
    default:
      return false;
  }
  status_ = thread_status;
  return true;
}

void ThreadHandle::Run() {
  if (!IsThisStatus({ThreadStatus::kPrepareStart, ThreadStatus::kRunning})) {
    return;
  }
  rtc::SetCurrentThreadName(name_.c_str());
  StaticSetThreadPriority(thread_, priority_);
  do {
    if (ThreadStatus::kRunning != status_) {
      std::unique_lock<std::mutex> lk(mutex_);
      if (ThreadStatus::kWait == status_) {
        cond_.wait(lk);
      } else {
        while(ThreadStatus::kWaitForCondition == status_ && !end_wait_cond_()) {  
          cond_.wait_for(lk, std::chrono::milliseconds(wait_time_ms_));
        }
      }
    }
  } while(Handle());
}


#if defined(WEBRTC_WIN)
DWORD WINAPI PlatformThread::RunThread(void* obj) {
  ::SetLastError(ERROR_SUCCESS);
  static_cast<PlatformThread*>(ooj)->Run();
  return 0;
}
#else
void* ThreadHandle::RunThread(void* obj) {
  reinterpret_cast<ThreadHandle*>(obj)->Run();
  return 0;
}
#endif  // defined(WEBRTC_WIN)

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
  ThreadHandle(thread_name, thread_priority),
  handle_func_(handle_func) {}

bool ThreadHandleImpl::Handle() {
  return handle_func_();
}

std::unique_ptr<ThreadHandle> ThreadHandle::CreateThread(std::function<bool(void)> handle_func, const char* thread_name,
    rtc::ThreadPriority thread_priority) {
  return std::unique_ptr<ThreadHandle>(new ThreadHandleImpl(handle_func, thread_name, thread_priority));
}

//-------------    ThreadHandle   ---------------//
}  // namespace rtc
