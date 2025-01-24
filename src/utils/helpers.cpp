#include "helpers.h"
#include <tlhelp32.h>
#include <iostream>

std::string WStringToUtf8(const std::wstring &wstr)
{
    if (wstr.empty()) {
        return {};
    }
    int neededSize = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    if (neededSize <= 0) {
        return {};
    }
    std::string result(neededSize, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), &result[0], neededSize, nullptr, nullptr);
    // remove trailing null
    while(!result.empty() && result.back()=='\0') {
        result.pop_back();
    }
    return result;
}

std::wstring Utf8ToWstring(const std::string& utf8Str)
{
    if (utf8Str.empty()) {
        return std::wstring();
    }
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8Str.data(), (int)utf8Str.size(), NULL, 0);
    if (size_needed == 0) {
        return std::wstring();
    }
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.data(), (int)utf8Str.size(), &wstr[0], size_needed);
    return wstr;
}

bool FileExists(const std::wstring& path)
{
    DWORD attributes = GetFileAttributesW(path.c_str());
    return (attributes != INVALID_FILE_ATTRIBUTES &&
            !(attributes & FILE_ATTRIBUTE_DIRECTORY));
}

bool DirectoryExists(const std::wstring& dirPath)
{
    DWORD attributes = GetFileAttributesW(dirPath.c_str());
    return (attributes != INVALID_FILE_ATTRIBUTES &&
            (attributes & FILE_ATTRIBUTE_DIRECTORY));
}

bool IsDuplicateProcessRunning(const std::vector<std::wstring>& targetProcesses)
{
    DWORD currentPid = GetCurrentProcessId();
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return false;
    }
    PROCESSENTRY32W processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32W);
    if (!Process32FirstW(hSnapshot, &processEntry)) {
        CloseHandle(hSnapshot);
        return false;
    }
    do {
        if (processEntry.th32ProcessID == currentPid) {
            continue;
        }
        std::wstring exeName(processEntry.szExeFile);
        for (const auto& target : targetProcesses) {
            if (_wcsicmp(exeName.c_str(), target.c_str()) == 0) {
                CloseHandle(hSnapshot);
                return true;
            }
        }
    } while (Process32NextW(hSnapshot, &processEntry));

    CloseHandle(hSnapshot);
    return false;
}

// For local files
std::string decodeURIComponent(const std::string& encoded) {
    std::string result;
    result.reserve(encoded.size());

    for (size_t i = 0; i < encoded.size(); ++i) {
        char c = encoded[i];
        if (c == '%' && i + 2 < encoded.size() &&
            std::isxdigit(static_cast<unsigned char>(encoded[i + 1])) &&
            std::isxdigit(static_cast<unsigned char>(encoded[i + 2]))) {
            // Convert the two hex digits to a character
            std::string hex = encoded.substr(i + 1, 2);
            char decodedChar = static_cast<char>(std::strtol(hex.c_str(), nullptr, 16));
            result.push_back(decodedChar);
            i += 2;
            } else {
                result.push_back(c);
            }
    }
    return result;
}

std::wstring GetExeDirectory()
{
    wchar_t buf[MAX_PATH];
    GetModuleFileNameW(nullptr,buf,MAX_PATH);
    std::wstring path(buf);
    size_t pos=path.find_last_of(L"\\/");
    if(pos!=std::wstring::npos)
        path.erase(pos);
    return path;
}