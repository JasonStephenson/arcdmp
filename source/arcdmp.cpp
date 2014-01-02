#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <iostream>
#include <fstream>
#include <vector>
using namespace std;

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
using namespace boost;
using namespace boost::filesystem;
using namespace boost::program_options;

void OutputMachineTypes(string directory, string arcFilter, string fileFilter);

int main(int argc, char* argv[])
{
  options_description desc("Allowed options");
  desc.add_options()
    ("help", "Displays target platform of executables in a directory")
    ("directory", value<string>(), "The target directory. (defaults to current working directory)")
    ("arc", value<string>(), "Filter by architecture. (x86, x64, ANYCPU or Itanium)")
    ("filename", value<string>(), "Filter by filename.")
    ;

  try
  {
    variables_map varMap;
    store(parse_command_line(argc, argv, desc), varMap);
    notify(varMap);

    if(varMap.count("help"))
    {
      cout << desc << endl;
      return 1;
    }

    string arcFilter;
    if(varMap.count("arc"))
      arcFilter = to_upper_copy(varMap["arc"].as<string>());

    string fileFilter;
    if(varMap.count("filename"))
      fileFilter = to_upper_copy(varMap["filename"].as<string>());

    string target_dir = current_path().string();
    if(varMap.count("directory"))
      target_dir = varMap["directory"].as<string>();

    OutputMachineTypes(target_dir, arcFilter, fileFilter);
  }
  catch(const std::exception& e)
  {
    cerr << e.what() << endl;
    cout << desc << endl;
  }

  return 0;
}

void OutputMachineTypes(string directory, string arcFilter, string fileFilter)
{
  directory_iterator end;
  for(directory_iterator iter(directory); iter != end; ++iter)
  {
    fstream file_stream = fstream(iter->path().string(), ios::in | ios::binary);
    file_stream.seekg(0, ios::beg);

    if(file_stream.is_open())
    {
      //read the dos header
      IMAGE_DOS_HEADER dosHeader ={};
      file_stream.read((char*)&dosHeader, sizeof(IMAGE_DOS_HEADER));

      const bool isExecutable = (dosHeader.e_magic == IMAGE_DOS_SIGNATURE);
      if(isExecutable)
      {
        //seek to the start of IMAGE_NT_HEADERS (skipping the Signature)
        file_stream.seekg(dosHeader.e_lfanew + sizeof(DWORD), ios::beg);

        //read the NTHeaders signature
        IMAGE_FILE_HEADER fileHeader ={};
        file_stream.read((char*)&fileHeader, sizeof(fileHeader));   

        //read into the correct structure
        IMAGE_DATA_DIRECTORY clrHeaderLocation ={};
        if(sizeof(IMAGE_OPTIONAL_HEADER64) == fileHeader.SizeOfOptionalHeader)
        {
          IMAGE_OPTIONAL_HEADER64 nt64OptHeader ={};
          file_stream.read((char*)&nt64OptHeader, fileHeader.SizeOfOptionalHeader);
          clrHeaderLocation = nt64OptHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR];
        }
        else if(sizeof(IMAGE_OPTIONAL_HEADER32) == fileHeader.SizeOfOptionalHeader)
        {
          IMAGE_OPTIONAL_HEADER32 nt32OptHeader ={};
          file_stream.read((char*)&nt32OptHeader, fileHeader.SizeOfOptionalHeader);
          clrHeaderLocation = nt32OptHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR];
        }

        //For native executables we can tell its x64 by looking at fileHeader.Machine
        //For managed executables:
        //									If its compiled for x64 we also can tell by fileHeader.Machine
        //									To determine whether its x86 | ANY CPU we need the 32BITREQ flag, which lives in the CLR header

        IMAGE_COR20_HEADER clrHeader ={};
        const bool isManaged = (clrHeaderLocation.VirtualAddress != 0);
        if(isManaged)
        { 
          //find out which section header the clr header lives in;
          //And when we do, continue to iterate the sections so that
          //the read pointer in the file_stream is at the end of all the section headers
          IMAGE_SECTION_HEADER sectClrHeaderIsIn ={};
          for(int i = 0; i < fileHeader.NumberOfSections; ++i)
          { 
            IMAGE_SECTION_HEADER currentSection ={};
            file_stream.read((char*)&currentSection, sizeof(currentSection));
            if(currentSection.VirtualAddress <= clrHeaderLocation.VirtualAddress && currentSection.VirtualAddress + currentSection.SizeOfRawData > clrHeaderLocation.VirtualAddress)
              sectClrHeaderIsIn = currentSection;
          }

          //get the clr header
          file_stream.seekg(sectClrHeaderIsIn.PointerToRawData + (clrHeaderLocation.VirtualAddress - sectClrHeaderIsIn.VirtualAddress), ios::beg);
          file_stream.read((char*)&clrHeader, sizeof(IMAGE_COR20_HEADER));
        }

        file_stream.close();

        string architecture;
        switch(fileHeader.Machine)
        {
        case IMAGE_FILE_MACHINE_AMD64:
          architecture = "X64";
          break;

        case IMAGE_FILE_MACHINE_I386:
          if(isManaged)
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

        const string filename = to_upper_copy(iter->path().filename().string());
        if((arcFilter.empty() || architecture == arcFilter) && (fileFilter.empty() || fileFilter == filename) )
          cout << iter->path().filename().string() << " (" << architecture << ")" << endl;
      }
    }
  }
}