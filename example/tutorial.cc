#include <stdio.h>

#include "modules/audio_device/include/audio_device.h"
#include "system_wrappers/include/sleep.h"
#include "api/task_queue/default_task_queue_factory.h"

class AudioDeviceCallback : public webrtc::AudioTransport {
  public:
    AudioDeviceCallback();
    ~AudioDeviceCallback() override;
    int32_t RecordedDataIsAvailable(const void* audioSamples,
        const size_t nSamples,
        const size_t nBytesPerSample,
        const size_t nChannels,
        const uint32_t samplesPerSec,
        const uint32_t totalDelayMS,
        const int32_t clockDrift,
        const uint32_t currentMicLevel,
        const bool keyPressed,
        uint32_t& newMicLevel) override;  // NOLINT

    // Implementation has to setup safe values for all specified out parameters.
    int32_t NeedMorePlayData(const size_t nSamples,
        const size_t nBytesPerSample,
        const size_t nChannels,
        const uint32_t samplesPerSec,
        void* audioSamples,
        size_t& nSamplesOut,  // NOLINT
        int64_t* elapsed_time_ms,
        int64_t* ntp_time_ms) override { return 0; }  // NOLINT

    // Method to pull mixed render audio data from all active VoE channels.
    // The data will not be passed as reference for audio processing internally.
    void PullRenderData(int bits_per_sample,
        int sample_rate,
        size_t number_of_channels,
        size_t number_of_frames,
        void* audio_data,
        int64_t* elapsed_time_ms,
        int64_t* ntp_time_ms) override {}
private:
    FILE* f = nullptr;
};

AudioDeviceCallback::AudioDeviceCallback() {
  f = fopen("./data/record.pcm", "wb");
}

AudioDeviceCallback::~AudioDeviceCallback() {
  fclose(f);
}

int32_t AudioDeviceCallback::RecordedDataIsAvailable(const void* audioSamples,
    const size_t nSamples,
    const size_t nBytesPerSample,
    const size_t nChannels,
    const uint32_t samplesPerSec,
    const uint32_t totalDelayMS,
    const int32_t clockDrift,
    const uint32_t currentMicLevel,
    const bool keyPressed,
    uint32_t& newMicLevel) {
  return fwrite(audioSamples, nBytesPerSample, nSamples, f);
}
int main() {
  printf("hello word\n");
  std::unique_ptr<webrtc::TaskQueueFactory> task_queue_factory =
    webrtc::CreateDefaultTaskQueueFactory();
  auto device_ = webrtc::AudioDeviceModule::Create(webrtc::AudioDeviceModule::kPlatformDefaultAudio, task_queue_factory.get());
  printf("%d\n", device_->Init());
  std::unique_ptr<AudioDeviceCallback> callback =std::make_unique<AudioDeviceCallback>();
  device_->RegisterAudioCallback(callback.get());
  device_->SetRecordingDevice(0);
  printf("InitRecording %d\n", device_->InitRecording());
  printf("StartRecording: %d\n", device_->StartRecording());
  webrtc::SleepMs(1000 * 10);
  device_->StopRecording();
}
