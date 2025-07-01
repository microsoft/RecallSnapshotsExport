// Copyright (C) Microsoft Corporation. All rights reserved.
// RecallSnapshotsExport.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <string>

#include <rometadataresolution.h>
#include <wil/resource.h>
#include <winrt/base.h>
#include <winrt/Windows.ApplicationModel.Activation.h>
#include <winrt/Windows.Data.Json.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Globalization.h>
#include <winrt/Windows.Globalization.DateTimeFormatting.h>
#include <winrt/windows.graphics.imaging.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.FileProperties.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Graphics.h>

#include "JsonHelper.h"

inline constexpr std::wstring_view ImageMetadataStorageTag = L"/app1/ifd/exif/{ushort=37500}";

constexpr auto c_keySizeInBytes = 32;
constexpr auto c_tagSizeInBytes = 16;
constexpr auto c_nonceSizeInBytes = 12;
constexpr auto c_childKeySizeInBytes = 32;
const auto c_totalSizeInBytes = c_nonceSizeInBytes + c_childKeySizeInBytes + c_tagSizeInBytes;

namespace winrt
{
    using namespace winrt::Windows::Data::Json;
    using namespace winrt::Windows::Foundation;
    using namespace winrt::Windows::Foundation::Collections;
    using namespace winrt::Windows::Graphics::Imaging;
    using namespace winrt::Windows::Storage;
    using namespace winrt::Windows::Storage::FileProperties;
    using namespace winrt::Windows::Storage::Streams;
    using namespace winrt::Windows::ApplicationModel::Activation;
}

inline HRESULT HResultFromBCryptStatus(NTSTATUS status)
{
    #define STATUS_AUTH_TAG_MISMATCH ((NTSTATUS)0xC000A002L)

    RETURN_HR_IF_EXPECTED((HRESULT)STATUS_AUTH_TAG_MISMATCH, status == STATUS_AUTH_TAG_MISMATCH);
    RETURN_IF_NTSTATUS_FAILED_EXPECTED(status);
    return S_OK;
}

void WriteJSONToFile(winrt::JsonObject const& jsonObject, winrt::hstring const& fileName, winrt::StorageFolder const& outputFolder)
{
    auto storageFile = outputFolder.CreateFileAsync(fileName + L".json", winrt::CreationCollisionOption::ReplaceExisting).get();
    
    auto jsonString = jsonObject.Stringify();
    winrt::FileIO::WriteTextAsync(storageFile, jsonString).get();
    
    std::wcout << L"Decrypted metadata: " << fileName.c_str() << L".json" << std::endl;
}

winrt::ValueSet TryGetSnapshotMetadataAsync(winrt::IRandomAccessStream const& decryptedStream)
{
    auto decoder = winrt::BitmapDecoder::CreateAsync(decryptedStream).get();
    auto properties = decoder.BitmapProperties().GetPropertiesAsync({ winrt::hstring{ImageMetadataStorageTag} }).get();
    auto prop = properties.TryLookup(ImageMetadataStorageTag);

    winrt::ValueSet valueSet;
    if (prop && (prop.Type() == winrt::PropertyType::UInt8Array))
    {
        auto propValue = prop.Value().as<winrt::IPropertyValue>();
        winrt::com_array<uint8_t> propArray;
        propValue.GetUInt8Array(propArray);

        winrt::Buffer buffer(propArray.size());
        buffer.Length(propArray.size());
        memcpy_s(buffer.data(), buffer.Length(), propArray.data(), propArray.size());

        winrt::IPropertySetSerializer serializer;
        winrt::check_hresult(RoCreatePropertySetSerializer(
            reinterpret_cast<ABI::Windows::Storage::Streams::IPropertySetSerializer**>(winrt::put_abi(serializer))));

        serializer.Deserialize(valueSet, buffer);
    }

    return valueSet;
}

std::vector<uint8_t> DecryptPackedData(BCRYPT_KEY_HANDLE key, std::span<uint8_t const> payload)
{
    THROW_HR_IF(E_INVALIDARG, payload.size() < c_tagSizeInBytes);
    const auto dataSize = payload.size() - c_tagSizeInBytes;
    const auto data = payload.data();

    uint8_t zeroNonce[c_nonceSizeInBytes] = { 0 };
    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo{};
    BCRYPT_INIT_AUTH_MODE_INFO(authInfo);
    authInfo.pbNonce = zeroNonce;
    authInfo.cbNonce = c_nonceSizeInBytes;
    authInfo.pbTag = const_cast<uint8_t*>(payload.data() + dataSize);
    authInfo.cbTag = c_tagSizeInBytes;

    std::vector<uint8_t> decryptedContent(dataSize);
    ULONG decryptedSize = 0;
    const auto result = BCryptDecrypt(
        key, const_cast<uint8_t*>(data), static_cast<ULONG>(dataSize), &authInfo, nullptr, 0, decryptedContent.data(), static_cast<ULONG>(dataSize), &decryptedSize, 0);
    decryptedContent.resize(decryptedSize);

    THROW_IF_FAILED(HResultFromBCryptStatus(result));

    return decryptedContent;
}

wil::unique_bcrypt_key DecryptExportKey(BCRYPT_KEY_HANDLE key, std::span<uint8_t const> encryptedKey)
{
    THROW_HR_IF(E_INVALIDARG, encryptedKey.size() != c_totalSizeInBytes);

    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO AuthInfo{};
    BCRYPT_INIT_AUTH_MODE_INFO(AuthInfo);
    AuthInfo.pbNonce = const_cast<uint8_t*>(encryptedKey.data()); 
    AuthInfo.cbNonce = c_nonceSizeInBytes;
    AuthInfo.pbTag = const_cast<uint8_t*>(encryptedKey.data() + c_nonceSizeInBytes + c_childKeySizeInBytes);
    AuthInfo.cbTag = c_tagSizeInBytes;

    uint8_t decryptedKey[c_childKeySizeInBytes] = { 0 };

    ULONG decryptedByteCount{};
    THROW_IF_FAILED(HResultFromBCryptStatus(BCryptDecrypt(
        key,
        const_cast<uint8_t*>(encryptedKey.data() + c_nonceSizeInBytes),
        c_childKeySizeInBytes,
        &AuthInfo,
        nullptr,
        0,
        decryptedKey,
        sizeof(decryptedKey),
        &decryptedByteCount,
        0)));

    wil::unique_bcrypt_key childKey;
    THROW_IF_NTSTATUS_FAILED(
        BCryptGenerateSymmetricKey(BCRYPT_AES_GCM_ALG_HANDLE, &childKey, nullptr, 0, decryptedKey, c_childKeySizeInBytes, 0));

    return childKey;
}

std::vector<uint8_t> HexStringToBytes(const std::wstring& hexString)
{
    std::vector<uint8_t> bytes;
    if (hexString.length() % 2 != 0)
    {
        throw std::invalid_argument("Hex string must have an even length");
    }

    for (size_t i = 0; i < hexString.length(); i += 2)
    {
        std::wstring byteString = hexString.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoi(byteString, nullptr, 16));
        bytes.push_back(byte);
    }

    return bytes;
}

struct EncryptedSnapshotHeader
{
    UINT32 Version;
    UINT32 KeySize;
    UINT32 ContentSize;
    UINT32 ContentType;
};

winrt::IRandomAccessStream DecryptSnapshot(winrt::IRandomAccessStream const& inputStream, std::wstring const& exportCode)
{
    std::vector<uint8_t> exportCodeBytes = HexStringToBytes(exportCode);
    std::vector<uint8_t> exportKeyBytes(c_keySizeInBytes);
    THROW_IF_NTSTATUS_FAILED(BCryptHash(
        BCRYPT_SHA256_ALG_HANDLE,
        nullptr,
        0,
        exportCodeBytes.data(),
        static_cast<ULONG>(exportCodeBytes.size()),
        exportKeyBytes.data(),
        c_keySizeInBytes));

    wil::unique_bcrypt_key exportKey;
    THROW_IF_NTSTATUS_FAILED(BCryptGenerateSymmetricKey(
       BCRYPT_AES_GCM_ALG_HANDLE, &exportKey, nullptr, 0, exportKeyBytes.data(), static_cast<ULONG>(exportKeyBytes.size()), 0));

    winrt::DataReader reader(inputStream.GetInputStreamAt(0));
    reader.LoadAsync(static_cast<uint32_t>(inputStream.Size())).get();

    if (reader.UnconsumedBufferLength() < sizeof(EncryptedSnapshotHeader))
    {
        throw std::runtime_error("Insufficient data in the buffer.");
    }

    EncryptedSnapshotHeader header{};
    reader.ByteOrder(winrt::ByteOrder::LittleEndian);

    header.Version = reader.ReadUInt32();
    header.KeySize = reader.ReadUInt32();
    header.ContentSize = reader.ReadUInt32();
    header.ContentType = reader.ReadUInt32();

    if (header.Version != 2)
    {
        throw std::runtime_error("Insufficient data header version.");
    }
 
    std::vector<uint8_t> keybytes(header.KeySize);
    reader.ReadBytes(keybytes);
    wil::unique_bcrypt_key contentKey = DecryptExportKey(exportKey.get(), keybytes);

    std::vector<uint8_t> contentbytes(header.ContentSize);
    reader.ReadBytes(contentbytes);
    std::vector<uint8_t> decryptedContent = DecryptPackedData(contentKey.get(), contentbytes);

    winrt::InMemoryRandomAccessStream decryptedStream;
    winrt::DataWriter dataWriter(decryptedStream);
    dataWriter.WriteBytes(winrt::array_view<const uint8_t>(decryptedContent));
    dataWriter.StoreAsync().get();
    dataWriter.DetachStream();
    return decryptedStream;
}

void WriteSnapshotToOutputFolder(winrt::StorageFolder const& outputFolder, winrt::hstring const& fileName, winrt::IRandomAccessStream const& decryptedStream)
{
    auto outputFile = outputFolder.CreateFileAsync(fileName + L".jpg", winrt::CreationCollisionOption::ReplaceExisting).get();
    auto outputStream = outputFile.OpenAsync(winrt::FileAccessMode::ReadWrite).get();
    winrt::DataReader reader(decryptedStream.GetInputStreamAt(0));
    winrt::DataWriter writer(outputStream);

    reader.LoadAsync(static_cast<uint32_t>(decryptedStream.Size())).get();
    writer.WriteBuffer(reader.ReadBuffer(static_cast<uint32_t>(decryptedStream.Size())));
    writer.StoreAsync().get();
    writer.FlushAsync().get();

    std::wcout << L"Decrypted screenshot: " << fileName.c_str() << L".jpg" << std::endl;
}

void WriteImageAndMetadataToOutputFolder(winrt::StorageFile const& file, winrt::StorageFolder const& outputFolder, std::wstring const& exportCode)
{
    auto inputStream = file.OpenAsync(winrt::FileAccessMode::Read).get();
    auto decryptedStream = DecryptSnapshot(inputStream, exportCode);

    WriteSnapshotToOutputFolder(outputFolder, file.Name(), decryptedStream);

    auto valueSet = TryGetSnapshotMetadataAsync(decryptedStream);
    if (valueSet)
    {
        auto jsonObject = SerializeValueSet(valueSet);
        WriteJSONToFile(jsonObject, file.Name(), outputFolder);
    }
}

void ExportSnapshotsToFolder(std::wstring const& exportFolderPath, std::wstring const& outputFolderPath, std::wstring const& exportCode)
{
    if (!std::filesystem::is_directory(outputFolderPath))
    {
        std::wcout << L"The output folder path doesn't exist." << std::endl;
        std::wcout << L"Creating directory: " << outputFolderPath << std::endl << std::endl;
        std::filesystem::create_directory(outputFolderPath);
    }

    winrt::StorageFolder exportFolder = winrt::StorageFolder::GetFolderFromPathAsync(exportFolderPath).get();
    winrt::StorageFolder outputFolder = winrt::StorageFolder::GetFolderFromPathAsync(outputFolderPath).get();

    auto files = exportFolder.GetFilesAsync().get();
    for (auto const& file : files)
    {
        try
        {
            WriteImageAndMetadataToOutputFolder(file, outputFolder, exportCode);
        }
        catch (...)
        {
            std::wcout << L"Decryption of the file has failed. FileName: " << file.Name().c_str() << std::endl;
        }
    }
}

std::wstring UnexpandExportCode(std::wstring code)
{
    if (code.size() > 32)
    {
        code.erase(std::remove(code.begin(), code.end(), ' '), code.end()); // Remove spaces
        code.erase(std::remove(code.begin(), code.end(), '-'), code.end()); // Remove hyphens
    }

    if (code.size() != 32)
    {
        std::wcout << L"The export code has incorrect number of characters."<< std::endl;
    }

    return code;
}

int wmain(int argc, wchar_t *argv[])
{
    if (argc != 4)
    {
        std::wcout << L"Usage: RecallSnapshotsExport.exe <exportFolderPath> <outputFolderPath> <recoveryKey>" << std::endl;
        return 0;
    }

    std::wcout << L"Reading snapshots from: " << argv[1] << std::endl;
    std::wcout << L"Writing content to: " << argv[2] << std::endl;
    std::wcout << L"Recall export code: " << argv[3] << std::endl << std::endl;

    try
    {
        ExportSnapshotsToFolder( argv[1] /*exportFolderPath*/, argv[2] /*outputFolderPath*/, UnexpandExportCode(argv[3]) /*exportCode*/ );
    }
    catch (...)
    {
        std::wcout << L"Decryption of snapshot and metadata has failed." << std::endl;
    }

    return 0;
}