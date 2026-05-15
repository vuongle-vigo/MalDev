#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <wmcodecdsp.h>
#include <wrl/client.h>

#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstdint>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "wmcodecdspuuid.lib")

using Microsoft::WRL::ComPtr;

static uint8_t ClampByte(int v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return static_cast<uint8_t>(v);
}

bool BGRAtoNV12(
    const uint8_t* bgra,
    int width,
    int height,
    std::vector<uint8_t>& nv12
) {
    if (!bgra || width <= 0 || height <= 0) {
        return false;
    }

    if (width % 2 != 0 || height % 2 != 0) {
        std::cout << "Width/height must be even for NV12\n";
        return false;
    }

    const int ySize = width * height;
    const int uvSize = width * height / 2;

    nv12.resize(ySize + uvSize);

    uint8_t* yPlane = nv12.data();
    uint8_t* uvPlane = nv12.data() + ySize;

    // Y plane
    for (int y = 0; y < height; y++) {
        const uint8_t* srcRow = bgra + y * width * 4;
        uint8_t* dstY = yPlane + y * width;

        for (int x = 0; x < width; x++) {
            uint8_t B = srcRow[x * 4 + 0];
            uint8_t G = srcRow[x * 4 + 1];
            uint8_t R = srcRow[x * 4 + 2];

            // BT.601 limited range approximate
            int Y = ((66 * R + 129 * G + 25 * B + 128) >> 8) + 16;

            dstY[x] = ClampByte(Y);
        }
    }

    // UV plane, 2x2 average
    for (int y = 0; y < height; y += 2) {
        for (int x = 0; x < width; x += 2) {
            int sumU = 0;
            int sumV = 0;

            for (int dy = 0; dy < 2; dy++) {
                const uint8_t* srcRow = bgra + (y + dy) * width * 4;

                for (int dx = 0; dx < 2; dx++) {
                    const uint8_t* p = srcRow + (x + dx) * 4;

                    uint8_t B = p[0];
                    uint8_t G = p[1];
                    uint8_t R = p[2];

                    int U = ((-38 * R - 74 * G + 112 * B + 128) >> 8) + 128;
                    int V = ((112 * R - 94 * G - 18 * B + 128) >> 8) + 128;

                    sumU += U;
                    sumV += V;
                }
            }

            int uvIndex = (y / 2) * width + x;

            uvPlane[uvIndex + 0] = ClampByte(sumU / 4); // U
            uvPlane[uvIndex + 1] = ClampByte(sumV / 4); // V
        }
    }

    return true;
}

class H264EncoderMF {
public:
    bool Init(int w, int h, int fpsValue, int bitrateValue) {
        width = w;
        height = h;
        fps = fpsValue;
        bitrate = bitrateValue;

        frameDuration = 10'000'000LL / fps; // 100ns unit
        frameIndex = 0;

        HRESULT hr = MFStartup(MF_VERSION);
        if (FAILED(hr)) {
            std::cout << "MFStartup failed: 0x" << std::hex << hr << "\n";
            return false;
        }

        hr = CoCreateInstance(
            CLSID_CMSH264EncoderMFT,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&encoder)
        );

        if (FAILED(hr)) {
            std::cout << "Create H264 Encoder MFT failed: 0x" << std::hex << hr << "\n";
            return false;
        }

        if (!SetOutputType()) {
            return false;
        }

        if (!SetInputType()) {
            return false;
        }

        encoder->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, 0);
        encoder->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0);
        encoder->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);

        return true;
    }

    bool EncodeNV12(
        const std::vector<uint8_t>& nv12,
        std::vector<std::vector<uint8_t>>& outputPackets
    ) {
        outputPackets.clear();

        const DWORD inputSize = static_cast<DWORD>(nv12.size());

        ComPtr<IMFSample> inputSample;
        ComPtr<IMFMediaBuffer> inputBuffer;

        HRESULT hr = MFCreateSample(&inputSample);
        if (FAILED(hr)) return false;

        hr = MFCreateMemoryBuffer(inputSize, &inputBuffer);
        if (FAILED(hr)) return false;

        BYTE* dst = nullptr;
        DWORD maxLen = 0;
        DWORD curLen = 0;

        hr = inputBuffer->Lock(&dst, &maxLen, &curLen);
        if (FAILED(hr)) return false;

        memcpy(dst, nv12.data(), inputSize);

        inputBuffer->Unlock();
        inputBuffer->SetCurrentLength(inputSize);

        inputSample->AddBuffer(inputBuffer.Get());

        LONGLONG sampleTime = frameIndex * frameDuration;

        inputSample->SetSampleTime(sampleTime);
        inputSample->SetSampleDuration(frameDuration);

        hr = encoder->ProcessInput(0, inputSample.Get(), 0);

        if (FAILED(hr)) {
            std::cout << "ProcessInput failed: 0x" << std::hex << hr << "\n";
            return false;
        }

        frameIndex++;

        return DrainOutput(outputPackets);
    }

    bool EncodeBGRA(
        const std::vector<uint8_t>& bgra,
        std::vector<std::vector<uint8_t>>& outputPackets
    ) {
        std::vector<uint8_t> nv12;

        if (!BGRAtoNV12(bgra.data(), width, height, nv12)) {
            return false;
        }

        return EncodeNV12(nv12, outputPackets);
    }

    void Shutdown() {
        if (encoder) {
            encoder->ProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, 0);
            encoder->ProcessMessage(MFT_MESSAGE_COMMAND_DRAIN, 0);
            encoder.Reset();
        }

        MFShutdown();
    }

private:
    bool SetInputType() {
        ComPtr<IMFMediaType> inputType;

        HRESULT hr = MFCreateMediaType(&inputType);
        if (FAILED(hr)) return false;

        inputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        inputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
        inputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);

        MFSetAttributeSize(inputType.Get(), MF_MT_FRAME_SIZE, width, height);
        MFSetAttributeRatio(inputType.Get(), MF_MT_FRAME_RATE, fps, 1);
        MFSetAttributeRatio(inputType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);

        hr = encoder->SetInputType(0, inputType.Get(), 0);

        if (FAILED(hr)) {
            std::cout << "SetInputType failed: 0x" << std::hex << hr << "\n";
            return false;
        }

        return true;
    }

    bool SetOutputType() {
        ComPtr<IMFMediaType> outputType;

        HRESULT hr = MFCreateMediaType(&outputType);
        if (FAILED(hr)) return false;

        outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        outputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
        outputType->SetUINT32(MF_MT_AVG_BITRATE, bitrate);
        outputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);

        MFSetAttributeSize(outputType.Get(), MF_MT_FRAME_SIZE, width, height);
        MFSetAttributeRatio(outputType.Get(), MF_MT_FRAME_RATE, fps, 1);
        MFSetAttributeRatio(outputType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);

        hr = encoder->SetOutputType(0, outputType.Get(), 0);

        if (FAILED(hr)) {
            std::cout << "SetOutputType failed: 0x" << std::hex << hr << "\n";
            return false;
        }

        return true;
    }

    bool DrainOutput(std::vector<std::vector<uint8_t>>& outputPackets) {
        while (true) {
            MFT_OUTPUT_STREAM_INFO streamInfo{};
            HRESULT hr = encoder->GetOutputStreamInfo(0, &streamInfo);

            if (FAILED(hr)) {
                std::cout << "GetOutputStreamInfo failed: 0x" << std::hex << hr << "\n";
                return false;
            }

            ComPtr<IMFSample> outputSample;
            ComPtr<IMFMediaBuffer> outputBuffer;

            hr = MFCreateSample(&outputSample);
            if (FAILED(hr)) return false;

            DWORD bufferSize = streamInfo.cbSize;
            if (bufferSize == 0) {
                bufferSize = width * height;
            }

            hr = MFCreateMemoryBuffer(bufferSize, &outputBuffer);
            if (FAILED(hr)) return false;

            outputSample->AddBuffer(outputBuffer.Get());

            MFT_OUTPUT_DATA_BUFFER outputData{};
            outputData.dwStreamID = 0;
            outputData.pSample = outputSample.Get();

            DWORD status = 0;

            hr = encoder->ProcessOutput(
                0,
                1,
                &outputData,
                &status
            );

            if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
                return true;
            }

            if (hr == MF_E_TRANSFORM_STREAM_CHANGE) {
                std::cout << "Stream changed\n";
                continue;
            }

            if (FAILED(hr)) {
                std::cout << "ProcessOutput failed: 0x" << std::hex << hr << "\n";
                return false;
            }

            ComPtr<IMFMediaBuffer> encodedBuffer;
            hr = outputSample->ConvertToContiguousBuffer(&encodedBuffer);
            if (FAILED(hr)) return false;

            BYTE* data = nullptr;
            DWORD maxLen = 0;
            DWORD curLen = 0;

            hr = encodedBuffer->Lock(&data, &maxLen, &curLen);
            if (FAILED(hr)) return false;

            if (curLen > 0) {
                std::vector<uint8_t> packet(data, data + curLen);
                outputPackets.push_back(std::move(packet));
            }

            encodedBuffer->Unlock();

            if (outputData.pEvents) {
                outputData.pEvents->Release();
            }
        }
    }

private:
    ComPtr<IMFTransform> encoder;

    int width = 0;
    int height = 0;
    int fps = 30;
    int bitrate = 4'000'000;

    LONGLONG frameDuration = 0;
    LONGLONG frameIndex = 0;
};