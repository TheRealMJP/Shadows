//=================================================================================================
//
//  MJP's DX11 Sample Framework
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#pragma once

#include "PCH.h"

#include "Exceptions.h"

namespace SampleFramework11
{

// Serialization helpers
template<typename T> void SerializeWrite(HANDLE fileHandle, const T& val)
{
    DWORD bytesWritten = 0;
    Win32Call(WriteFile(fileHandle, &val, sizeof(T), &bytesWritten, NULL));
}

template<typename T> void SerializeWriteVector(HANDLE fileHandle, const std::vector<T>& vec)
{
    DWORD bytesWritten = 0;
    uint32 numElements = static_cast<uint32>(vec.size());
    uint32 vectorSize = numElements * sizeof(T);
    Win32Call(WriteFile(fileHandle, &numElements, sizeof(uint32), &bytesWritten, NULL));
    if(numElements > 0)
        Win32Call(WriteFile(fileHandle, &vec[0], vectorSize, &bytesWritten, NULL));
}

template<typename T> void SerializeWriteString(HANDLE fileHandle, const std::basic_string<T>& str)
{
    DWORD bytesWritten = 0;
    uint32 numChars = static_cast<uint32>(str.length());
    uint32 strSize = numChars * sizeof(T);
    Win32Call(WriteFile(fileHandle, &numChars, sizeof(uint32), &bytesWritten, NULL));
    if(numChars > 0)
        Win32Call(WriteFile(fileHandle, str.c_str(), strSize, &bytesWritten, NULL));

}

template<typename T> void SerializeRead(HANDLE fileHandle, T& val)
{
    DWORD bytesRead = 0;
    Win32Call(ReadFile(fileHandle, &val, sizeof(T), &bytesRead, NULL));
}

template<typename T> void SerializeReadVector(HANDLE fileHandle, std::vector<T>& vec)
{
    DWORD bytesRead = 0;
    uint32 numElements = 0;
    Win32Call(ReadFile(fileHandle, &numElements, sizeof(uint32), &bytesRead, NULL));

    vec.clear();
    if(numElements > 0)
    {
        vec.resize(numElements);
        Win32Call(ReadFile(fileHandle, &vec[0], sizeof(T) * numElements, &bytesRead, NULL));
    }
}

template<typename T> void SerializeReadString(HANDLE fileHandle, std::basic_string<T>& str)
{
    DWORD bytesRead = 0;
    uint32 numChars = 0;
    Win32Call(ReadFile(fileHandle, &numChars, sizeof(uint32), &bytesRead, NULL));

    str.clear();
    if(numChars > 0)
    {
        str.resize(numChars + 1, 0);
        Win32Call(ReadFile(fileHandle, &str[0], sizeof(T) * numChars, &bytesRead, NULL));
    }
}

}