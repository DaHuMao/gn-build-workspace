/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef RTC_BASE_PLATFORM_THREAD_H_
#define RTC_BASE_PLATFORM_THREAD_H_

#ifndef WEBRTC_WIN
#include <pthread.h>
#endif
#include <string>
#include <atomic>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <vector>

#include "absl/strings/string_view.h"
#include "api/sequence_checker.h"
#include "rtc_base/constructor_magic.h"
#include "rtc_base/platform_thread_types.h"

namespace rtc {

// Callback function that the spawned thread will enter once spawned.
typedef void (*ThreadRunFunction)(void*);

enum ThreadPriority {
#ifdef WEBRTC_WIN
  kLowPriority = THREAD_PRIORITY_BELOW_NORMAL,
  kNormalPriority = THREAD_PRIORITY_NORMAL,
  kHighPriority = THREAD_PRIORITY_ABOVE_NORMAL,
  kHighestPriority = THREAD_PRIORITY_HIGHEST,
  kRealtimePriority = THREAD_PRIORITY_TIME_CRITICAL
#else
  kLowPriority = 1,
  kNormalPriority = 2,
  kHighPriority = 3,
  kHighestPriority = 4,
  kRealtimePriority = 5
#endif
};

// Represents a simple worker thread.  The implementation must be assumed
// to be single threaded, meaning that all methods of the class, must be
// called from the same thread, including instantiation.
class PlatformThread {
 public:
  PlatformThread(ThreadRunFunction func,
                 void* obj,
                 absl::string_view thread_name,
                 ThreadPriority priority = kNormalPriority);
  virtual ~PlatformThread();

  const std::string& name() const { return name_; }

  // Spawns a thread and tries to set thread priority according to the priority
  // from when CreateThread was called.
  void Start();

  bool IsRunning() const;

  // Returns an identifier for the worker thread that can be used to do
  // thread checks.
  PlatformThreadRef GetThreadRef() const;

  // Stops (joins) the spawned thread.
  void Stop();

 protected:
#if defined(WEBRTC_WIN)
  // Exposed to derived classes to allow for special cases specific to Windows.
  bool QueueAPC(PAPCFUNC apc_function, ULONG_PTR data);
#endif

 private:
  void Run();
  bool SetPriority(ThreadPriority priority);

  ThreadRunFunction const run_function_ = nullptr;
  const ThreadPriority priority_ = kNormalPriority;
  void* const obj_;
  // TODO(pbos): Make sure call sites use string literals and update to a const
  // char* instead of a std::string.
  const std::string name_;
  webrtc::SequenceChecker thread_checker_;
  webrtc::SequenceChecker spawned_thread_checker_;
#if defined(WEBRTC_WIN)
  static DWORD WINAPI StartThread(void* param);

  HANDLE thread_ = nullptr;
  DWORD thread_id_ = 0;
#else
  static void* StartThread(void* param);

  pthread_t thread_ = 0;
#endif  // defined(WEBRTC_WIN)
  RTC_DISALLOW_COPY_AND_ASSIGN(PlatformThread);
};

//----------------------  ThreadHandle ----------------------------------//
enum class ThreadStatus {
  kPrepareStart,
  kRunning,
  kWait,
  kWaitForCondition,
  kPrepareStop,
  kStop,
};
class ThreadHandle {
public:
  ThreadHandle(const char* thread_name = "ThreadHandle",
      rtc::ThreadPriority thread_priority = rtc::kNormalPriority);
  virtual ~ThreadHandle();
  ThreadStatus Start();
  ThreadStatus Stop();
  ThreadStatus GetStatus();
  // thread blocked until |WakeUp| is called
  ThreadStatus Suspend();
  // thread wait and Check if |ped| is true every |wait_time_ms| ms
  // if |pred| is true or WakeUp is called, The thread will end blocking
  ThreadStatus Suspend(size_t wait_time_ms, std::function<bool(void)> condition);
  ThreadStatus WakeUp();

  static std::unique_ptr<ThreadHandle> CreateThread(std::function<bool(void)> handle_func, const char* thread_name = "ThreadHandle",
     rtc::ThreadPriority thread_priority = rtc::kNormalPriority);

protected:
  virtual bool Handle() = 0;
  virtual void FuctionIsOver();

private:
  bool IsThisStatus(const std::vector<ThreadStatus>& thread_status);
  bool SetStatus(ThreadStatus thread_status_vector);
  void Run();

  const std::string name_;
  const ThreadPriority priority_ = kNormalPriority;
  std::atomic<ThreadStatus> status_;
  std::mutex mutex_;
  std::condition_variable cond_;
  std::function<bool(void)> end_wait_cond_;
  size_t wait_time_ms_ = 10;

#if defined(WEBRTC_WIN)
  static DWORD WINAPI RunThread(void* obj);
  HANDLE thread_ = nullptr;
  DWORD thread_id_ = 0;
#else
  static void* RunThread(void *obj);
  pthread_t thread_ = 0;
#endif  // defined(WEBRTC_WIN)
};
//----------------------  ThreadHandle ----------------------------------//
}  // namespace rtc

#endif  // RTC_BASE_PLATFORM_THREAD_H_
