#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <vector>
#include <iostream>
#include "save.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

using Microsoft::WRL::ComPtr;

class DDACapturer {
public:
    bool Init() {
        HRESULT hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            0,
            nullptr,
            0,
            D3D11_SDK_VERSION,
            &device,
            nullptr,
            &context
        );

        if (FAILED(hr)) {
            std::cout << "D3D11CreateDevice failed\n";
            return false;
        }

        ComPtr<IDXGIDevice> dxgiDevice;
        hr = device.As(&dxgiDevice);
        if (FAILED(hr)) return false;

        ComPtr<IDXGIAdapter> adapter;
        hr = dxgiDevice->GetAdapter(&adapter);
        if (FAILED(hr)) return false;

        ComPtr<IDXGIOutput> output;
        hr = adapter->EnumOutputs(0, &output); // monitor 0
        if (FAILED(hr)) {
            std::cout << "EnumOutputs failed\n";
            return false;
        }

        DXGI_OUTPUT_DESC outputDesc{};
        output->GetDesc(&outputDesc);

        width = outputDesc.DesktopCoordinates.right - outputDesc.DesktopCoordinates.left;
        height = outputDesc.DesktopCoordinates.bottom - outputDesc.DesktopCoordinates.top;

        ComPtr<IDXGIOutput1> output1;
        hr = output.As(&output1);
        if (FAILED(hr)) return false;

        hr = output1->DuplicateOutput(device.Get(), &duplication);
        if (FAILED(hr)) {
            std::cout << "DuplicateOutput failed: 0x" << std::hex << hr << "\n";
            return false;
        }

        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

        hr = device->CreateTexture2D(&desc, nullptr, &stagingTexture);
        if (FAILED(hr)) {
            std::cout << "CreateTexture2D staging failed\n";
            return false;
        }

        return true;
    }

    bool CaptureFrame(std::vector<unsigned char>& bgra, int& outWidth, int& outHeight) {
        DXGI_OUTDUPL_FRAME_INFO frameInfo{};
        ComPtr<IDXGIResource> desktopResource;

        HRESULT hr = duplication->AcquireNextFrame(
            100, // timeout ms
            &frameInfo,
            &desktopResource
        );

        if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
            return false; // chưa có frame mới
        }

        if (hr == DXGI_ERROR_ACCESS_LOST) {
            std::cout << "DDA access lost, need reinit\n";
            return false;
        }

        if (FAILED(hr)) {
            std::cout << "AcquireNextFrame failed: 0x" << std::hex << hr << "\n";
            return false;
        }

        ComPtr<ID3D11Texture2D> desktopTexture;
        hr = desktopResource.As(&desktopTexture);

        if (FAILED(hr)) {
            duplication->ReleaseFrame();
            return false;
        }

        context->CopyResource(stagingTexture.Get(), desktopTexture.Get());

        D3D11_MAPPED_SUBRESOURCE mapped{};
        hr = context->Map(
            stagingTexture.Get(),
            0,
            D3D11_MAP_READ,
            0,
            &mapped
        );

        if (FAILED(hr)) {
            duplication->ReleaseFrame();
            return false;
        }

        outWidth = width;
        outHeight = height;

        bgra.resize(width * height * 4);

        unsigned char* src = static_cast<unsigned char*>(mapped.pData);
        unsigned char* dst = bgra.data();

        for (int y = 0; y < height; y++) {
            memcpy(
                dst + y * width * 4,
                src + y * mapped.RowPitch,
                width * 4
            );
        }

        context->Unmap(stagingTexture.Get(), 0);
        duplication->ReleaseFrame();

        return true;
    }

    int GetWidth() const {
        return width;
    }

    int GetHeight() const {
        return height;
    }

private:
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<IDXGIOutputDuplication> duplication;
    ComPtr<ID3D11Texture2D> stagingTexture;

    int width = 0;
    int height = 0;
};

int main() {
    DDACapturer capturer;

    if (!capturer.Init()) {
        std::cout << "Init DDA failed\n";
        return 1;
    }

    int index = 0;

    while (true) {
        std::vector<unsigned char> frame;
        int width = 0;
        int height = 0;

        if (capturer.CaptureFrame(frame, width, height)) {
            std::wstring filename = L"frame_" + std::to_wstring(index) + L".bmp";

            SaveBGRAtoBMP(filename, frame, width, height);

            std::wcout << L"Saved " << filename << L"\n";

            index++;

            if (index >= 10) {
                break;
            }
        }

        Sleep(33);
    }

    return 0;
}