#include "driver/audiodrv.h"
#include "pools/audiopool.h"

using namespace love::driver;

Audrv::Audrv() : audioInitialized(false)
{
    static const AudioRendererConfig config = {
        .output_rate     = AudioRendererOutputRate_48kHz,
        .num_voices      = 24,
        .num_effects     = 0,
        .num_sinks       = 1,
        .num_mix_objs    = 1,
        .num_mix_buffers = 2,
    };

    AudioPool::AUDIO_POOL_BASE = memalign(AUDREN_MEMPOOL_ALIGNMENT, AudioPool::AUDIO_POOL_SIZE);

    Result res = audrenInitialize(&config);

    this->audioInitialized = R_SUCCEEDED(res);

    if (!this->audioInitialized)
        return;

    res = audrvCreate(&this->driver, &config, 2);

    if (R_SUCCEEDED(res))
        this->initialized = true;

    int mempoolID =
        audrvMemPoolAdd(&this->driver, AudioPool::AUDIO_POOL_BASE, AudioPool::AUDIO_POOL_SIZE);

    audrvMemPoolAttach(&this->driver, mempoolID);

    static const u8 sink_channels[] = { 0, 1 };

    audrvDeviceSinkAdd(&this->driver, AUDREN_DEFAULT_DEVICE_NAME, 2, sink_channels);

    audrvUpdate(&this->driver);

    audrenStartAudioRenderer();
}

/*
** Sets the master volume out
*/
void Audrv::SetMixVolume(int mix, float volume)
{
    thread::Lock lock(this->mutex);

    audrvMixSetVolume(&this->driver, mix, volume);
}

/*
** Reset a channel to a given format and sample rate
** Also needs to Mix Factor reset.. for some reason
*/
void Audrv::ResetChannel(size_t channel, PcmFormat format, int sampleRate)
{
    thread::Lock lock(this->mutex);

    audrvVoiceInit(&this->driver, channel, 2, format, sampleRate);
    audrvVoiceSetDestinationMix(&this->driver, channel, AUDREN_FINAL_MIX_ID);

    audrvVoiceSetMixFactor(&this->driver, channel, 1.0f, 0, 0);
    audrvVoiceSetMixFactor(&this->driver, channel, 1.0f, 0, 1);
}

/*
** Set a channel's volume
*/
void Audrv::SetChannelVolume(size_t channel, float volume)
{
    thread::Lock lock(this->mutex);

    audrvVoiceSetVolume(&this->driver, channel, volume);
}

/*
** Check if a channel is playing
*/
bool Audrv::IsChannelPlaying(size_t channel)
{
    thread::Lock lock(this->mutex);

    return audrvVoiceIsPlaying(&this->driver, channel);
}

/*
** Adds an AudioDriverWaveBuf to the Audio Driver channel
** NOTE: Stop the Voice before Adding
** However, this is done when released from the audio pool
** or Source:PepareAtomic()
*/
void Audrv::AddWaveBuf(size_t channel, AudioDriverWaveBuf* waveBuf)
{
    thread::Lock lock(this->mutex);

    audrvVoiceAddWaveBuf(&this->driver, channel, waveBuf);
    audrvVoiceStart(&this->driver, channel);
}

/*
** Pause a channel
*/
void Audrv::PauseChannel(size_t channel, bool pause)
{
    thread::Lock lock(this->mutex);

    audrvVoiceSetPaused(&this->driver, channel, pause);
}

/*
** Stop a channel
** This is typically done on audio pool release
** or Source:PrepareAtomic()
*/
void Audrv::StopChannel(size_t channel)
{
    thread::Lock lock(this->mutex);

    audrvVoiceStop(&this->driver, channel);
}

/*
** Get the sample offset of the audio
** Unlike 3DS, this is continguous data
** So just return the offset and it's fine
*/
u32 Audrv::GetSampleOffset(size_t channel)
{
    thread::Lock lock(this->mutex);

    return audrvVoiceGetPlayedSampleCount(&this->driver, channel);
}

Audrv::~Audrv()
{
    if (this->initialized)
        audrvClose(&this->driver);

    if (this->audioInitialized)
        audrenExit();
}

void Audrv::Update()
{
    thread::Lock lock(this->mutex);
    audrvUpdate(&this->driver);
}
