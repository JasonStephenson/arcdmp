arcdmp
======
A tool which prints the target architecture of an executable. It does by examining windows PE headers. It is capable of working on both native and managed executables.

Getting
======
>git clone https://github.com/JasonStephenson/arcdmp.git

Building
======
* Compile with relevant version of Visual Studio.

Using
======
By default arcdmp will output the architecture of all executables within the current working directory.
It supports these commands:

* --help : Displays some help.
* --directory : Specifies the target directory.
* --arc : Filter results by architecture.
* --filename : Filter results by file name.

Example Usage
======

All executables within the current directory
>arcdmp

All executables within C:\Temp
>arcdmp --directory C:\Temp

All executables within C:\Temp that are 64bit
>arcdmp --directory C:\Temp --arc x64

All executables that are 32bit in current directory
>arcdmp --arc x86

MyAssembly.dll
>arcdmp --filename MyAssembly.dll

MyAssembly.dll that is compiled for ANY CPU
>arcdmp --filename MyAssembly.dll --arc ANYCPU
