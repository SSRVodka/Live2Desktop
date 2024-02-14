#include <cmath>
#include <cstdint>

#include "tools.h"
#include "wavFileHandler.h"


WavFileHandler::WavFileHandler()
    : _pcmData(NULL)
    , _userTimeSeconds(0.0f)
    , _lastRms(0.0f)
    , _sampleOffset(0) {
}

WavFileHandler::~WavFileHandler() {
    if (_pcmData != NULL)
        ReleasePcmData();
}

Csm::csmBool WavFileHandler::Update(Csm::csmFloat32 deltaTimeSeconds) {
    Csm::csmUint32 goalOffset;
    Csm::csmFloat32 rms;

    /* Do not update before data load/when end of file is reached. */
    if ((_pcmData == NULL)
        || (_sampleOffset >= _wavFileInfo._samplesPerChannel)) {

        _lastRms = 0.0f;
        return false;
    }

    /* Maintains state after elapsed time. */
    _userTimeSeconds += deltaTimeSeconds;
    goalOffset = static_cast<Csm::csmUint32>(_userTimeSeconds * _wavFileInfo._samplingRate);
    if (goalOffset > _wavFileInfo._samplesPerChannel) {
        goalOffset = _wavFileInfo._samplesPerChannel;
    }

    /* RMS Measurement. */
    rms = 0.0f;
    for (Csm::csmUint32 channelCount = 0; channelCount < _wavFileInfo._numberOfChannels; channelCount++) {
        for (Csm::csmUint32 sampleCount = _sampleOffset; sampleCount < goalOffset; sampleCount++) {
            Csm::csmFloat32 pcm = _pcmData[channelCount][sampleCount];
            rms += pcm * pcm;
        }
    }
    rms = sqrt(rms / (_wavFileInfo._numberOfChannels * (goalOffset - _sampleOffset)));

    _lastRms = rms;
    _sampleOffset = goalOffset;
    return true;
}

void WavFileHandler::Start(const Csm::csmString& filePath) {
    /* Loading WAV files. */
    if (!LoadWavFile(filePath))
        return;

    /* Initialize sample reference position. */
    _sampleOffset = 0;
    _userTimeSeconds = 0.0f;

    /* Reset RMS value. */
    _lastRms = 0.0f;
}

Csm::csmFloat32 WavFileHandler::GetRms() const {
    return _lastRms;
}

Csm::csmBool WavFileHandler::LoadWavFile(const Csm::csmString& filePath) {
    Csm::csmBool ret;

    /* Free up space if already loaded wav file. */
    if (_pcmData != NULL)
        ReleasePcmData();

    /* File load. */
    _byteReader._fileByte = ToolFunctions::LoadFileAsBytes(filePath.GetRawString(), &(_byteReader._fileSize));
    _byteReader._readOffset = 0;

    /* Failure if the file load fails
     * or there is no size to put the first signature "RIFF". */
    if ((_byteReader._fileByte == NULL) || (_byteReader._fileSize < 4))
        return false;

    _wavFileInfo._fileName = filePath;

    do {
        /* Signature "RIFF". */
        if (!_byteReader.GetCheckSignature("RIFF")) {
            ret = false;
            break;
        }
        /* File size - 8 (skip) */
        _byteReader.Get32LittleEndian();
        /* Signature "WAVE". */
        if (!_byteReader.GetCheckSignature("WAVE")) {
            ret = false;
            break;
        }
        /* Signature "fmt ". */
        if (!_byteReader.GetCheckSignature("fmt ")) {
            ret = false;
            break;
        }
        /* fmt chunk size. */
        const Csm::csmUint32 fmtChunkSize = _byteReader.Get32LittleEndian();
        /* Format IDs other than 1 (linear PCM) are not accepted. */
        if (_byteReader.Get16LittleEndian() != 1) {
            ret = false;
            break;
        }
        /* Number of channels. */
        _wavFileInfo._numberOfChannels = _byteReader.Get16LittleEndian();
        /* Sampling rate. */
        _wavFileInfo._samplingRate = _byteReader.Get32LittleEndian();
        /* Data rate [byte/sec] (skip reading). */
        _byteReader.Get32LittleEndian();
        /* Block size (skip). */
        _byteReader.Get16LittleEndian();
        /* Quantization bit rate. */
        _wavFileInfo._bitsPerSample = _byteReader.Get16LittleEndian();
        /* Skipping the extended part of the fmt chunk. */
        if (fmtChunkSize > 16) {
            _byteReader._readOffset += (fmtChunkSize - 16);
        }
        /* Skip over "data" chunks until they appear. */
        while (!(_byteReader.GetCheckSignature("data"))
            && (_byteReader._readOffset < _byteReader._fileSize)) {
            _byteReader._readOffset += _byteReader.Get32LittleEndian();
        }
        /* No "data" chunks appeared in the file. */
        if (_byteReader._readOffset >= _byteReader._fileSize) {
            ret = false;
            break;
        }
        /* Number of samples. */
        {
            const Csm::csmUint32 dataChunkSize = _byteReader.Get32LittleEndian();
            _wavFileInfo._samplesPerChannel = (dataChunkSize * 8) / (_wavFileInfo._bitsPerSample * _wavFileInfo._numberOfChannels);
        }
        /* partitioning. */
        _pcmData = static_cast<Csm::csmFloat32**>(CSM_MALLOC(sizeof(Csm::csmFloat32*) * _wavFileInfo._numberOfChannels));
        for (Csm::csmUint32 channelCount = 0; channelCount < _wavFileInfo._numberOfChannels; channelCount++) {
            _pcmData[channelCount] = static_cast<Csm::csmFloat32*>(CSM_MALLOC(sizeof(Csm::csmFloat32) * _wavFileInfo._samplesPerChannel));
        }
        /* Waveform data acquisition. */
        for (Csm::csmUint32 sampleCount = 0; sampleCount < _wavFileInfo._samplesPerChannel; sampleCount++) {
            for (Csm::csmUint32 channelCount = 0; channelCount < _wavFileInfo._numberOfChannels; channelCount++) {
                _pcmData[channelCount][sampleCount] = GetPcmSample();
            }
        }

        ret = true;

    }  while (false);

    /* File resource release. */
    ToolFunctions::ReleaseBytes(_byteReader._fileByte);
    _byteReader._fileByte = NULL;
    _byteReader._fileSize = 0;

    return ret;
}

Csm::csmFloat32 WavFileHandler::GetPcmSample() {
    Csm::csmInt32 pcm32;

    /* Expand to 32 bits wide and then round to the range of -1 to 1. */
    switch (_wavFileInfo._bitsPerSample) {
    case 8:
        pcm32 = static_cast<Csm::csmInt32>(_byteReader.Get8()) - 128;
        pcm32 <<= 24;
        break;
    case 16:
        pcm32 = _byteReader.Get16LittleEndian() << 16;
        break;
    case 24:
        pcm32 = _byteReader.Get24LittleEndian() << 8;
        break;
    default:
        /* Bit widths not supported. */
        pcm32 = 0;
        break;
    }

    return static_cast<Csm::csmFloat32>(pcm32) / INT32_MAX;
}

void WavFileHandler::ReleasePcmData() {
    for (Csm::csmUint32 channelCount = 0; channelCount < _wavFileInfo._numberOfChannels; channelCount++) {
        CSM_FREE(_pcmData[channelCount]);
    }
    CSM_FREE(_pcmData);
    _pcmData = NULL;
}
