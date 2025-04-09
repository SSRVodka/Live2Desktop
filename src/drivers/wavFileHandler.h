/**
 * @file wavFileHandler.h
 * @brief A source file defining the `*.wav` file handler.
 * 
 * @author Copyright(c) Live2D Inc. && SSRVodka
 * @date   Feb 12, 2024
 */

#pragma once

#include <CubismFramework.hpp>

#include <Utils/CubismString.hpp>

 /**
  * @class WavFileHandler
  * @brief `*.wav` file handler.
  * 
  * @attention Only 16bit wav file reading is implemented.
  */
class WavFileHandler {
public:
    WavFileHandler();
    ~WavFileHandler();

    /**
     * @brief Internal state update of wav file handler.
     *
     * @param[in] deltaTimeSeconds  Delta time [sec].
     * @retval true     updated
     * @retval false    not updated
     */
    Csm::csmBool Update(Csm::csmFloat32 deltaTimeSeconds);

    /**
     * @brief Start reading the wav file specified in the argument.
     *
     * @param[in] filePath wav file path
     * 
     * @retval  true    Load Success
     * @retval  false   Load Failure
     */
    Csm::csmBool Start(const Csm::csmString& filePath);

    /**
     * @brief Get current RMS value.
     *
     * @retval  csmFloat32 RMS value
     */
    Csm::csmFloat32 GetRms() const;

private:
    /**
     * @brief Loading wav files.
     *
     * @param[in] filePath wav file path
     * @retval  true    Read Success
     * @retval  false   Read Failure
     */
    Csm::csmBool LoadWavFile(const Csm::csmString& filePath);

    /**
     * @brief PCM data release.
     */
    void ReleasePcmData();

    /**
     * @brief Obtain 1 sample in the range -1 to 1.
     * @retval    csmFloat32    Normalized sample
     */
    Csm::csmFloat32 GetPcmSample();

    /**
     * @struct WavFileInfo
     * @brief Information on loaded wavfile
     */
    struct WavFileInfo {
        WavFileInfo() : _fileName(""), _numberOfChannels(0),
            _bitsPerSample(0), _samplingRate(0), _samplesPerChannel(0)
        { }

        Csm::csmString _fileName;
        Csm::csmUint32 _numberOfChannels;   /**< Number of Channels */
        Csm::csmUint32 _bitsPerSample;      /**< Bits per sample */
        Csm::csmUint32 _samplingRate;       /**< Sampling rate */
        Csm::csmUint32 _samplesPerChannel;  /**< Total samples per channel */
    } _wavFileInfo;

    struct ByteReader {
        ByteReader() : _fileByte(NULL), _fileSize(0), _readOffset(0)
        { }

        /**
         * @brief 8-bit read.
         * @return Csm::csmUint8 8-bit value read
         */
        Csm::csmUint8 Get8() {
            const Csm::csmUint8 ret = _fileByte[_readOffset];
            _readOffset++;
            return ret;
        }

        /**
         * @brief 16-bit read (little-endian).
         * @return Csm::csmUint16 16-bit value read
         */
        Csm::csmUint16 Get16LittleEndian() {
            const Csm::csmUint16 ret = (_fileByte[_readOffset + 1] << 8) | _fileByte[_readOffset];
            _readOffset += 2;
            return ret;
        }

        /**
         * @brief 24-bit read (little-endian).
         * @return Csm::csmUint32 24-bit value read (set to lower 24 bits)
         */
        Csm::csmUint32 Get24LittleEndian() {
            const Csm::csmUint32 ret =
                (_fileByte[_readOffset + 2] << 16) | (_fileByte[_readOffset + 1] << 8)
                | _fileByte[_readOffset];
            _readOffset += 3;
            return ret;
        }

        /**
         * @brief 32-bit read (little-endian).
         * @return Csm::csmUint32 32-bit value read
         */
        Csm::csmUint32 Get32LittleEndian() {
            const Csm::csmUint32 ret =
                (_fileByte[_readOffset + 3] << 24) | (_fileByte[_readOffset + 2] << 16)
                | (_fileByte[_readOffset + 1] << 8) | _fileByte[_readOffset];
            _readOffset += 4;
            return ret;
        }

        /**
         * @brief Obtaining signatures and checking for matches with reference strings.
         * @param[in] reference Signature string to be inspected
         * @retval  true    Match
         * @retval  false   Not match
         */
        Csm::csmBool GetCheckSignature(const Csm::csmString& reference) {
            Csm::csmChar getSignature[4] = { 0, 0, 0, 0 };
            const Csm::csmChar* referenceString = reference.GetRawString();
            if (reference.GetLength() != 4)
                return false;
            
            for (Csm::csmUint32 signatureOffset = 0; signatureOffset < 4; signatureOffset++) {
                getSignature[signatureOffset] = static_cast<Csm::csmChar>(Get8());
            }
            return (getSignature[0] == referenceString[0]) && (getSignature[1] == referenceString[1])
                && (getSignature[2] == referenceString[2]) && (getSignature[3] == referenceString[3]);
        }

        Csm::csmByte* _fileByte;    /**< Byte sequence of the loaded file */
        Csm::csmSizeInt _fileSize;  /**< File size */
        Csm::csmUint32 _readOffset; /**< File reference position */
    } _byteReader;

    Csm::csmFloat32** _pcmData;         /**< Audio data array expressed in the range -1 to 1 */
    Csm::csmUint32 _sampleOffset;       /**< Sample reference position */
    Csm::csmFloat32 _lastRms;           /**< Last measured RMS value */
    Csm::csmFloat32 _userTimeSeconds;   /**< Totalized delta time [s] */
 };
