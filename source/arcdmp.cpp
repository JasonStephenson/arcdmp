#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>

#ifdef __cpp_lib_filesystem
#include <filesystem>
namespace stdef = std::filesystem;
#elif __cpp_lib_experimental_filesystem
#include <experimental/filesystem>
namespace stdef = std::experimental::filesystem;
#else
#error "no filesystem support ='("
#endif

void print_usage();
void output_machine_types(const std::string& directory, const std::string& arcFilter, const std::string& fileFilter);

int main(int argc, char* argv[])
{
    /* Ignore environment param */
    std::vector<std::string> paramData;
    for (int i = 1; i < argc; ++i)
        paramData.emplace_back(argv[i]);

    if (!paramData.empty() && paramData[0] == "--help")
    {
        print_usage();
        return 0;
    }

    std::string target_dir = stdef::current_path().string();
    std::string fileFilter;
    std::string arcFilter;

    for (size_t i = 0; i < paramData.size(); ++i)
    {
        if (paramData[i] == "--directory") { i++;  target_dir = paramData[i]; }
        else if (paramData[i] == "--filename") { i++; fileFilter = paramData[i]; }
        else if (paramData[i] == "--arc") { i++; arcFilter = paramData[i]; }
    }

    output_machine_types(target_dir, arcFilter, fileFilter);

    return 0;
}

void output_machine_types(const std::string& directory, const std::string& arcFilter, const std::string& fileFilter)
{
    for (const stdef::directory_entry& entry : stdef::directory_iterator(directory))
    {
        std::fstream file_stream = std::fstream(entry.path().string(), std::ios::in | std::ios::binary);
        file_stream.seekg(0, std::ios::beg);

        if (file_stream.is_open())
        {
            /* Read the dos header */
            IMAGE_DOS_HEADER dosHeader = {};
            file_stream.read((char*)&dosHeader, sizeof(IMAGE_DOS_HEADER));

            const bool isExecutable = (dosHeader.e_magic == IMAGE_DOS_SIGNATURE);
            if (isExecutable)
            {
                /* Seek to the start of IMAGE_NT_HEADERS (skipping the Signature) */
                file_stream.seekg(dosHeader.e_lfanew + sizeof(DWORD), std::ios::beg);

                /* Read the NTHeaders signature */
                IMAGE_FILE_HEADER fileHeader = {};
                file_stream.read((char*)&fileHeader, sizeof(fileHeader));

                /* Read into the correct structure */
                IMAGE_DATA_DIRECTORY clrHeaderLocation = {};
                if (sizeof(IMAGE_OPTIONAL_HEADER64) == fileHeader.SizeOfOptionalHeader)
                {
                    IMAGE_OPTIONAL_HEADER64 nt64OptHeader = {};
                    file_stream.read((char*)&nt64OptHeader, fileHeader.SizeOfOptionalHeader);
                    clrHeaderLocation = nt64OptHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR];
                }
                else if (sizeof(IMAGE_OPTIONAL_HEADER32) == fileHeader.SizeOfOptionalHeader)
                {
                    IMAGE_OPTIONAL_HEADER32 nt32OptHeader = {};
                    file_stream.read((char*)&nt32OptHeader, fileHeader.SizeOfOptionalHeader);
                    clrHeaderLocation = nt32OptHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR];
                }

                /*
                For native executables we can tell its x64 by looking at fileHeader.Machine
                For managed executables:
                    If its compiled for x64 we also can tell by fileHeader.Machine
                    To determine whether its x86 | ANY CPU we need the 32BITREQ flag, which lives in the CLR header
                */

                IMAGE_COR20_HEADER clrHeader = {};
                const bool isManaged = (clrHeaderLocation.VirtualAddress != 0);
                if (isManaged)
                {
                    /*
                    Find out which section header the clr header lives in;
                    And when we do, continue to iterate the sections so that
                    the read pointer in the file_stream is at the end of all the section headers
                    */
                    IMAGE_SECTION_HEADER sectClrHeaderIsIn = {};
                    for (int i = 0; i < fileHeader.NumberOfSections; ++i)
                    {
                        IMAGE_SECTION_HEADER currentSection = {};
                        file_stream.read((char*)&currentSection, sizeof(currentSection));
                        if (currentSection.VirtualAddress <= clrHeaderLocation.VirtualAddress && currentSection.VirtualAddress + currentSection.SizeOfRawData > clrHeaderLocation.VirtualAddress)
                            sectClrHeaderIsIn = currentSection;
                    }

                    /* Get the clr header */
                    file_stream.seekg(sectClrHeaderIsIn.PointerToRawData + (clrHeaderLocation.VirtualAddress - sectClrHeaderIsIn.VirtualAddress), std::ios::beg);
                    file_stream.read((char*)&clrHeader, sizeof(IMAGE_COR20_HEADER));
                }

                file_stream.close();

                std::string architecture;
                switch (fileHeader.Machine)
                {
                case IMAGE_FILE_MACHINE_AMD64:
                    architecture = "X64";
                    break;

                case IMAGE_FILE_MACHINE_I386:
                    if (isManaged)
                        architecture = ((clrHeader.Flags & COMIMAGE_FLAGS_32BITREQUIRED) > 0) ? "X86" : "ANYCPU";
                    else
                        architecture = "X86";
                    break;

                case IMAGE_FILE_MACHINE_IA64:
                    architecture = "ITANIUM";
                    break;

                case IMAGE_FILE_MACHINE_UNKNOWN:
                default:
                    architecture = "???";
                    break;
                }

                std::string fnUpper = entry.path().filename().string();
                std::transform(fnUpper.begin(), fnUpper.end(), fnUpper.begin(), toupper);
                if ((arcFilter.empty() || architecture == arcFilter) && (fileFilter.empty() || fileFilter == fnUpper))
                    std::cout << entry.path().filename().string() << " (" << architecture << ")" << std::endl;
            }
        }
    }
}

void print_usage()
{
    std::cout << std::endl;
    std::cout << "Arcdmp.exe parses the PE headers of executables and" << std::endl;
    std::cout << "outputs their target architecture. Usage: arcdmp.exe" << std::endl;
    std::cout << std::endl;
    std::cout << "Parameters:" << std::endl;
    std::cout << "    --help: Displays command line usage options" << std::endl;
    std::cout << "    --directory: The directory containing executables. (Default is cwd)" << std::endl;
    std::cout << "    --filename: The filename of an executable (Optional)" << std::endl;
    std::cout << "    --arc: Filter output by target architecture (x86, x64, ANYCPU or Itanium)" << std::endl;
}
