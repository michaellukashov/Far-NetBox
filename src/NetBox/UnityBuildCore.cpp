#define PUTTY_DO_GLOBALS
#define _CRT_SECURE_NO_WARNINGS

#pragma warning(push)
#pragma warning(disable: 4512) // assignment operator could not be generated

#include "..\nbvcl\Classes.cpp"
#include "..\nbvcl\SysUtils.cpp"

#include "..\far\UnicodeString.cpp"
#include "..\far\local.cpp"
#include "..\far\farrtl.cpp"

#include "..\core\Common.cpp"
#include "..\core\RemoteFiles.cpp"
#include "..\core\Terminal.cpp"
#include "..\core\Exceptions.cpp"
#include "..\core\FileOperationProgress.cpp"
#include "..\core\Queue.cpp"
#include "..\core\SecureShell.cpp"
#include "..\core\SessionInfo.cpp"
#include "..\core\CoreMain.cpp"
#include "..\core\FileMasks.cpp"
#include "..\core\CopyParam.cpp"
#include "..\core\SessionData.cpp"
#include "..\core\Configuration.cpp"
#include "..\core\ScpFileSystem.cpp"
#include "..\core\FtpFileSystem.cpp"
#include "..\core\SftpFileSystem.cpp"
#include "..\core\WebDAVFileSystem.cpp"
#include "..\core\PuttyIntf.cpp"
#include "..\core\Cryptography.cpp"
#include "..\core\FileBuffer.cpp"
#include "..\core\NamedObjs.cpp"
#include "..\core\HierarchicalStorage.cpp"
#include "..\core\Option.cpp"
#include "..\core\FileInfo.cpp"
#include "..\core\FileSystems.cpp"
#include "..\core\Bookmarks.cpp"
#include "..\core\WinSCPSecurity.cpp"
#include "..\windows\SynchronizeController.cpp"
#include "..\windows\GUIConfiguration.cpp"
#include "..\windows\GUITools.cpp"

#pragma warning(pop)
