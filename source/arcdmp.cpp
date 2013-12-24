#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

//stl
#include <iostream>
#include <fstream>
#include <vector>
using namespace std;

//boost
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost;
using namespace boost::filesystem;
using namespace boost::program_options;

string GetMachineType(string file_in);

int main(int argc, wchar_t* argv[])
{
  options_description desc("Allowed options");
  desc.add_options()
      ("help", "Displays target platform of executables in a directory")
      ("filter", value<string>(), "Filter to apply. (arg = x86, x64 or ANYCPU)")
      ("directory", value<string>(), "The target directory. (arg defaults to current working directory)")
	    ("file", value<string>(), "Display only results which match this filename")
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
    if(varMap.count("filter"))
      arcFilter = varMap["filter"].as<string>();

	  string fileFilter;
	  if(varMap.count("file"))
		  fileFilter = varMap["file"].as<string>();

    //extract target directory if supplied
    string target_dir = current_path().string();
    if(varMap.count("directory"))
      target_dir = varMap["directory"].as<string>();

    directory_iterator end;
    for(directory_iterator iter(target_dir); iter != end; ++iter)
    {
      string location = iter->path().string();
      string filename = iter->path().filename().string();
      string extension = to_upper_copy(iter->path().extension().string());

      if(extension == ".DLL" || extension == ".EXE")
      {
        string architecture = GetMachineType(location);
        if((arcFilter.empty() || architecture == arcFilter) && (fileFilter.empty() || fileFilter == filename) )
          cout << filename << " (" << architecture << ")" << endl;
      }
    } 
  }
  catch(const std::exception& e)
  {
    cerr << e.what() << endl;
    cout << desc << endl;
  }

	return 0;
}

string GetMachineType(string file_in)
{
	string toReturn = "???";
  fstream file_stream = fstream(file_in, ios::in | ios::binary);

  file_stream.seekg(0, ios::beg);
  if(file_stream.is_open())
  {
    //read the dos header
		IMAGE_DOS_HEADER dosHeader ={};
    file_stream.read((char*)&dosHeader, sizeof(IMAGE_DOS_HEADER));

    //seek to the start of IMAGE_NT_HEADERS (skipping the Signature)
    file_stream.seekg(dosHeader.e_lfanew + sizeof(DWORD));

    //read the NTHeaders signature
		IMAGE_FILE_HEADER fileHeader ={};
    file_stream.read((char*)&fileHeader, sizeof(fileHeader));   
    
    //read into the correct structure
		IMAGE_OPTIONAL_HEADER64 nt64OptHeader ={};
		IMAGE_OPTIONAL_HEADER32 nt32OptHeader ={};
		IMAGE_DATA_DIRECTORY clrHeaderLocation ={};
    if(sizeof(IMAGE_OPTIONAL_HEADER64) == fileHeader.SizeOfOptionalHeader)
    {
      file_stream.read((char*)&nt64OptHeader, fileHeader.SizeOfOptionalHeader);
      clrHeaderLocation = nt64OptHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR];
    }
    else if(sizeof(IMAGE_OPTIONAL_HEADER32) == fileHeader.SizeOfOptionalHeader)
    {
      file_stream.read((char*)&nt32OptHeader, fileHeader.SizeOfOptionalHeader);
      clrHeaderLocation = nt32OptHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR];
    }

    bool isManaged = (clrHeaderLocation.VirtualAddress != 0);

    //For native executables we can tell its x64 by looking at fileHeader.Machine
    //For managed executables:
		//									If its compiled for x64 we also can tell by fileHeader.Machine
		//									To determine whether its x86 | ANY CPU we need the 32BITREQ flag, which lives in the CLR header

		IMAGE_COR20_HEADER clrHeader ={};
		IMAGE_SECTION_HEADER sectClrHeaderIsIn ={};
    if(isManaged)
    { 
      //find out which section header the clr header lives in;
      //And when we do, continue to iterate the sections so that
      //the read pointer in the file_stream is at the end of all the section headers
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

    switch(fileHeader.Machine)
    {
    case IMAGE_FILE_MACHINE_UNKNOWN:
      toReturn = "Unknown";
      break;

    case IMAGE_FILE_MACHINE_AMD64:
      toReturn = "x64";
      break;

    case IMAGE_FILE_MACHINE_I386:
      if(isManaged)
				toReturn = ((clrHeader.Flags & COMIMAGE_FLAGS_32BITREQUIRED) > 0) ? "x86" : "ANYCPU";
			else
				toReturn = "x86";
      break;

    case IMAGE_FILE_MACHINE_IA64:
      toReturn = "Itanium";
      break;

    default:
      break;
    }
  }

  return toReturn;
}