# Microsoft Developer Studio Project File - Name="util" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=util - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "util.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "util.mak" CFG="util - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "util - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "util - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "util - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "UTIL_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W4 /GX /O2 /I "../../inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "UTIL_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40d /d "NDEBUG"
# ADD RSC /l 0x40d /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"../../../win32/Release/util.dll" /implib:"../../win32/lib/util.lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "util - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "UTIL_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W4 /Gm /GX /ZI /Od /I "../../inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "UTIL_EXPORTS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40d /d "_DEBUG"
# ADD RSC /l 0x40d /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"../../../win32/Debug/util.dll" /implib:"../../win32/lib/util.lib" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "util - Win32 Release"
# Name "util - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\varr.c
# End Source File
# Begin Source File

SOURCE=..\vbitvector.c
# End Source File
# Begin Source File

SOURCE=..\vbuckethash.c
# End Source File
# Begin Source File

SOURCE=..\vbuffer.c
# End Source File
# Begin Source File

SOURCE=..\vcircbuf.c
# End Source File
# Begin Source File

SOURCE=..\vclosedhash.c
# End Source File
# Begin Source File

SOURCE=..\vcontext.c
# End Source File
# Begin Source File

SOURCE=..\vdlist.c
# End Source File
# Begin Source File

SOURCE=..\vdlistunrolled.c
# End Source File
# Begin Source File

SOURCE=..\vdring.c
# End Source File
# Begin Source File

SOURCE=..\verror.c
# End Source File
# Begin Source File

SOURCE=..\vhashfunction.c
# End Source File
# Begin Source File

SOURCE=..\vheap.c
# End Source File
# Begin Source File

SOURCE=..\vio.c
# End Source File
# Begin Source File

SOURCE=..\vslist.c
# End Source File
# Begin Source File

SOURCE=..\vsparsearray.c
# End Source File
# Begin Source File

SOURCE=..\vsring.c
# End Source File
# Begin Source File

SOURCE=..\vstra.c
# End Source File
# Begin Source File

SOURCE=..\vtree.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\inc\util\varr.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\util\vbasedefs.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\util\vbitvector.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\util\vbuckethash.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\util\vbuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\util\vcircbuf.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\util\vclosedhash.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\util\vcontext.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\util\vdlist.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\util\vdlistunrolled.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\util\vdring.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\util\verror.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\util\vhashfunction.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\util\vheap.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\util\vio.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\util\vplatform.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\util\vslist.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\util\vsparsearray.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\util\vsring.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\util\vstra.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\util\vtree.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\util\vutil.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
