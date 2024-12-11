#include <windows.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <cstring>

using namespace std;

class PeFileException : public runtime_error {
public:
    explicit PeFileException(const string& message) : runtime_error(message) {}
};

class PeFileReader {
private:
    PUCHAR imageBase;
    struct PeDetails {
        PIMAGE_DOS_HEADER dosHeader;
        PIMAGE_NT_HEADERS ntHeaders;
        PIMAGE_SECTION_HEADER sectionHeader;
        WORD sectionCount;
    } peDetails;

public:
    explicit PeFileReader(LPCWSTR filePath) {
        HANDLE fileHandle = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (fileHandle == INVALID_HANDLE_VALUE) {
            throw PeFileException("Failed to open file: " + to_string(GetLastError()));
        }

        HANDLE mappingHandle = CreateFileMapping(fileHandle, NULL, PAGE_READONLY, 0, 0, NULL);
        if (!mappingHandle) {
            CloseHandle(fileHandle);
            throw PeFileException("Failed to create file mapping: " + to_string(GetLastError()));
        }

        LPVOID mappedView = MapViewOfFile(mappingHandle, FILE_MAP_READ, 0, 0, 0);
        if (!mappedView) {
            CloseHandle(mappingHandle);
            CloseHandle(fileHandle);
            throw PeFileException("Failed to map view of file: " + to_string(GetLastError()));
        }

        imageBase = static_cast<PUCHAR>(mappedView);
        peDetails.dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(imageBase);

        if (peDetails.dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
            UnmapViewOfFile(mappedView);
            CloseHandle(mappingHandle);
            CloseHandle(fileHandle);
            throw PeFileException("Invalid DOS signature");
        }

        peDetails.ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(imageBase + peDetails.dosHeader->e_lfanew);
        if (peDetails.ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
            UnmapViewOfFile(mappedView);
            CloseHandle(mappingHandle);
            CloseHandle(fileHandle);
            throw PeFileException("Invalid NT signature");
        }

        peDetails.sectionCount = peDetails.ntHeaders->FileHeader.NumberOfSections;
        peDetails.sectionHeader = IMAGE_FIRST_SECTION(peDetails.ntHeaders);

        CloseHandle(mappingHandle);
        CloseHandle(fileHandle);
    }

    void printDosSignature() const {
        printf("DOS Signature: %c%c\n", imageBase[0], imageBase[1]);
    }

    void printPeArchitecture() const {
        switch (peDetails.ntHeaders->FileHeader.Machine) {
        case IMAGE_FILE_MACHINE_I386:
            printf("PE Architecture: x86\n");
            break;
        case IMAGE_FILE_MACHINE_AMD64:
            printf("PE Architecture: x64\n");
            break;
        default:
            printf("PE Architecture: Unknown\n");
        }
    }

    void printSections() const {
        for (int i = 0; i < peDetails.sectionCount; ++i) {
            char sectionName[9] = { 0 };
            memcpy(sectionName, peDetails.sectionHeader[i].Name, 8);
            printf("Section %d: %s\n", i + 1, sectionName);
        }
    }

    ~PeFileReader() {
        UnmapViewOfFile(imageBase);
    }
};

int wmain(int argc, wchar_t* argv[]) {
    if (argc != 2) {
        wcerr << L"Usage: PeAnalyzer <PE File Path>" << endl;
        return 1;
    }

    try {
        PeFileReader reader(argv[1]);
        reader.printDosSignature();
        reader.printPeArchitecture();
        reader.printSections();
    } catch (const PeFileException& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
