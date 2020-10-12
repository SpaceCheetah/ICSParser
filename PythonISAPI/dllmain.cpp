#include "framework.h"

DWORD WINAPI HttpExtensionProc (EXTENSION_CONTROL_BLOCK* pECB);
BOOL WINAPI GetExtensionVersion (HSE_VERSION_INFO* pVer);
BOOL WINAPI TerminateExtension (DWORD dwFlags);

BOOL APIENTRY DllMain (HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    return TRUE;
}

BOOL WINAPI GetExtensionVersion (HSE_VERSION_INFO* pVer) {
    pVer->dwExtensionVersion = HSE_VERSION;
    strncpy(pVer->lpszExtensionDesc,
        "Python ISAPI extension", HSE_MAX_EXT_DLL_NAME_LEN);
    return TRUE;
}

void threadFunc(EXTENSION_CONTROL_BLOCK* pECB) {
    std::string header = "HTTP/1.1 200 OK\r\n";
    LPSTR queryString = pECB->lpszQueryString;
    pECB->ServerSupportFunction(pECB->ConnID, HSE_REQ_NORMALIZE_URL, queryString, 0, 0);
    if (std::string{ queryString } == "download=true") {
        header += "Content-Type: text/x-python\r\n";
        std::filesystem::path path{ pECB->lpszPathTranslated };
        header += "Content-Length: ";
        header += std::filesystem::file_size(path);
        header += "\r\n\r\n";
        std::ifstream stream(pECB->lpszPathTranslated);
        std::stringstream buffer{};
        buffer << stream.rdbuf();
        header += buffer.str();
    }
    else {
        std::map<std::string, std::string> headers{};
        headers["Content-Type"] = "text/html";
        std::string response = "";
        std::string command = "python ";
        command += pECB->lpszPathTranslated;
        command += " ";
        command += queryString;
        FILE* pipe = _popen(command.c_str(), "r");
        if (pipe) {
            char buffer[128];
            while (!feof(pipe)) {
                if (fgets(buffer, 128, pipe) != NULL) {
                    response += buffer;
                }
            }
            _pclose(pipe);
            size_t configStart = response.find("<PythonISAPIConfig>");
            if (configStart != std::string::npos) {
                size_t configEnd = response.find("</PythonISAPIConfig>");
                if (configEnd != std::string::npos && configEnd > configStart) {
                    std::string token;
                    std::istringstream tokenStream(response.substr(configStart + 19, configEnd - configStart - 19));
                    while (std::getline(tokenStream, token, '\n')) {
                        size_t colonIndex = token.find(":");
                        if (colonIndex != std::string::npos) {
                            headers[token.substr(0, colonIndex)] = token.substr(colonIndex + 1);
                        }
                    }
                    response.erase(configStart, configEnd - configStart + 20);
                }
            }
        }
        else {
            response = "PythonISAPI ERROR: Could not open file";
        }
        std::map<std::string, std::string>::iterator iterator = headers.begin();
        while (iterator != headers.end()) {
            header += iterator->first;
            header += ": ";
            header += iterator->second;
            header += "\r\n";
            iterator++;
        }
        header += "Content-Length: ";
        header += std::to_string(response.length());
        header += "\r\n\r\n";
        header += response;
    }
    DWORD length = header.length();
    pECB->WriteClient(pECB->ConnID, (void*)header.c_str(), &length, HSE_IO_SYNC);
    DWORD status = HSE_STATUS_SUCCESS_AND_KEEP_CONN;
    pECB->ServerSupportFunction(pECB->ConnID, HSE_REQ_DONE_WITH_SESSION, &status, 0, 0);
}

DWORD WINAPI HttpExtensionProc (EXTENSION_CONTROL_BLOCK* pECB) {
    std::thread(threadFunc, pECB).detach();
    return HSE_STATUS_PENDING;
}

BOOL WINAPI TerminateExtension (DWORD dwFlags) {
    return TRUE;
}