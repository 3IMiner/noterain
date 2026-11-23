#include "audio/engine.h"

#include <objbase.h>
#include <windows.h>

#include <audioclient.h>
#include <mmdeviceapi.h>

#include <algorithm>

namespace nr {
namespace {

// GUIDs are declared locally so no extra import library is required.
const CLSID kClsidMMDeviceEnumerator = {
    0xBCDE0395, 0xE52F, 0x467C, {0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E}};
const IID kIidMMDeviceEnumerator = {
    0xA95664D2, 0x9614, 0x4F35, {0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6}};
const IID kIidAudioClient = {
    0x1CB9AD4C, 0xDBFA, 0x4C32, {0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2}};
const IID kIidAudioRenderClient = {
    0xF294ACFC, 0x3146, 0x4483, {0xA7, 0xBF, 0xAD, 0xDC, 0xA7, 0xC2, 0x60, 0xE2}};
const PROPERTYKEY kPkeyFriendlyName = {
    {0xa45c254e, 0xdf1c, 0x4efd, {0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0}}, 14};
const GUID kSubtypeIeeeFloat = {
    0x00000003, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71}};

constexpr DWORD kFlagEventCallback = 0x00040000;
constexpr DWORD kFlagAutoConvert = 0x80000000;
constexpr DWORD kFlagSrcDefaultQuality = 0x08000000;
constexpr REFERENCE_TIME kBufferDuration = 100000; // 10 ms
constexpr double kSampleRate = 48000.0;

struct ComScope {
  bool ok = false;
  ComScope() { ok = SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED)); }
  ~ComScope() {
    if (ok) CoUninitialize();
  }
};

std::string toUtf8(const wchar_t* text) {
  if (!text) return {};
  const int len = (int)wcslen(text);
  if (len == 0) return {};
  const int size = WideCharToMultiByte(CP_UTF8, 0, text, len, nullptr, 0, nullptr, nullptr);
  std::string out((size_t)std::max(0, size), '\0');
  if (size > 0) WideCharToMultiByte(CP_UTF8, 0, text, len, out.data(), size, nullptr, nullptr);
  return out;
}

} // namespace

AudioEngine::~AudioEngine() { stop(); }

std::vector<std::string> AudioEngine::outputDevices() {
  std::vector<std::string> names;
  ComScope com;
  if (!com.ok) return names;

  IMMDeviceEnumerator* enumerator = nullptr;
  if (FAILED(CoCreateInstance(kClsidMMDeviceEnumerator, nullptr, CLSCTX_ALL,
                              kIidMMDeviceEnumerator, (void**)&enumerator)))
    return names;

  IMMDeviceCollection* collection = nullptr;
  if (SUCCEEDED(enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &collection)) &&
      collection) {
    UINT count = 0;
    if (SUCCEEDED(collection->GetCount(&count))) {
      for (UINT i = 0; i < count; ++i) {
        IMMDevice* device = nullptr;
        if (FAILED(collection->Item(i, &device)) || !device) continue;
        IPropertyStore* store = nullptr;
        std::string name = "Audio Device " + std::to_string(i + 1);
        if (SUCCEEDED(device->OpenPropertyStore(STGM_READ, &store)) && store) {
          PROPVARIANT value;
          PropVariantInit(&value);
          if (SUCCEEDED(store->GetValue(kPkeyFriendlyName, &value)) &&
              value.vt == VT_LPWSTR) {
            name = toUtf8(value.pwszVal);
          }
          PropVariantClear(&value);
          store->Release();
        }
        device->Release();
        names.push_back(name);
      }
    }
    collection->Release();
  }
  enumerator->Release();
  return names;
}

void AudioEngine::stop() {
  stop_.store(true);
  if (thread_.joinable()) thread_.join();
  running_ = false;
  initDone_.store(false);
  initOk_.store(false);
}

bool AudioEngine::start(PianoSynth* synth) {
  if (!synth) return false;
  stop();
  synth_ = synth;
  stop_.store(false);
  initDone_.store(false);
  initOk_.store(false);
  error_.clear();

  thread_ = std::thread([this] { this->runLoop(); });

  for (int i = 0; i < 160 && !initDone_.load(); ++i) Sleep(20); // up to ~3.2 s
  running_ = initOk_.load();
  return running_;
}

void AudioEngine::runLoop() {
  ComScope com;
  if (!com.ok) {
    error_ = "COM initialisation failed";
    initDone_.store(true);
    return;
  }

  IMMDeviceEnumerator* enumerator = nullptr;
  HRESULT hr = CoCreateInstance(kClsidMMDeviceEnumerator, nullptr, CLSCTX_ALL,
                                kIidMMDeviceEnumerator, (void**)&enumerator);
  if (FAILED(hr) || !enumerator) {
    error_ = "audio endpoint enumerator unavailable";
    initDone_.store(true);
    return;
  }

  IMMDevice* device = nullptr;
  hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
  if (FAILED(hr) || !device) {
    enumerator->Release();
    error_ = "no audio output device";
    initDone_.store(true);
    return;
  }

  IAudioClient* client = nullptr;
  hr = device->Activate(kIidAudioClient, CLSCTX_ALL, nullptr, (void**)&client);
  if (FAILED(hr) || !client) {
    device->Release();
    enumerator->Release();
    error_ = "cannot activate audio client";
    initDone_.store(true);
    return;
  }

  WAVEFORMATEXTENSIBLE format{};
  format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
  format.Format.nChannels = 2;
  format.Format.nSamplesPerSec = (DWORD)kSampleRate;
  format.Format.wBitsPerSample = 32;
  format.Format.nBlockAlign = (WORD)(format.Format.nChannels * format.Format.wBitsPerSample / 8);
  format.Format.nAvgBytesPerSec = format.Format.nSamplesPerSec * format.Format.nBlockAlign;
  format.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
  format.Samples.wValidBitsPerSample = 32;
  format.dwChannelMask = 0x3; // FL | FR
  format.SubFormat = kSubtypeIeeeFloat;

  hr = client->Initialize((AUDCLNT_SHAREMODE)0,
                          kFlagEventCallback | kFlagAutoConvert | kFlagSrcDefaultQuality,
                          kBufferDuration, 0, (WAVEFORMATEX*)&format, nullptr);
  if (FAILED(hr)) {
    client->Release();
    device->Release();
    enumerator->Release();
    error_ = "audio client initialise failed (0x" + std::to_string((unsigned)hr) + ")";
    initDone_.store(true);
    return;
  }

  HANDLE event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
  if (!event || FAILED(client->SetEventHandle(event))) {
    if (event) CloseHandle(event);
    client->Release();
    device->Release();
    enumerator->Release();
    error_ = "audio event handle failed";
    initDone_.store(true);
    return;
  }

  UINT32 bufferFrames = 0;
  client->GetBufferSize(&bufferFrames);

  IAudioRenderClient* render = nullptr;
  hr = client->GetService(kIidAudioRenderClient, (void**)&render);
  if (FAILED(hr) || !render) {
    CloseHandle(event);
    client->Release();
    device->Release();
    enumerator->Release();
    error_ = "render service unavailable";
    initDone_.store(true);
    return;
  }

  synth_->prepare(kSampleRate);
  sampleRate_ = kSampleRate;
  latencyMs_ = (float)(bufferFrames * 1000.0 / kSampleRate);

  client->Start();
  initOk_.store(true);
  initDone_.store(true);

  while (!stop_.load()) {
    const DWORD wait = WaitForSingleObject(event, 200);
    if (stop_.load()) break;
    if (wait != WAIT_OBJECT_0) continue;

    UINT32 padding = 0;
    if (FAILED(client->GetCurrentPadding(&padding))) break;
    const UINT32 available = bufferFrames - padding;
    if (available == 0) continue;

    BYTE* buffer = nullptr;
    if (FAILED(render->GetBuffer(available, &buffer)) || !buffer) break;
    synth_->render(reinterpret_cast<float*>(buffer), (int)available);
    render->ReleaseBuffer(available, 0);
  }

  client->Stop();
  render->Release();
  CloseHandle(event);
  client->Release();
  device->Release();
  enumerator->Release();
  initDone_.store(true);
}

} // namespace nr
