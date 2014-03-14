arcdmp
======
A tool which outputs the architecture for which an executable was compiled. It does so by examining its windows headers and pulling out relevant information. It is capable of working on both native and managed executables.

Getting
======
>git clone https://github.com/JasonStephenson/arcdmp.git

Building
======
* Download and extract Boost 1.53 (http://www.boost.org/users/history/version_1_53_0.html).
* Copy the contents of the boost_1_53_0 directory into \3rdparty\boost_1_53_0. (This folder already exists with the required boost static libs in a libsx64 folder for convenience).
* Compile with relevant version of Visual Studio.

Using
======
By default arcdmp will output the architecture of all executables within the current working directory.
It also supports the following commands:

* --help : Displays some help.
* --directory : Specifies the target directory.
* --arc : Filter results by architecture.
* --filename : Filter results by file name.

Example Usage
======
Use the following to get results for : 

All executables within the current directory
>arcdmp

All executables within C:\Temp
>arcdmp --directory=C:\Temp

All executables within C:\Temp that are 64bit
>arcdmp --directory=C:\Temp --arc=x64

All executables that are 32bit
>arcdmp --arc=x86

MyAssembly.dll
>arcdmp --filename=MyAssembly.dll

MyAssembly.dll that is compiled for ANY CPU
>arcdmp --filename=MyAssembly.dll --arc=ANYCPU
