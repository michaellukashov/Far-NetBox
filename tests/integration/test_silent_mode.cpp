#define NOMINMAX
#include <iostream>
#include <cassert>
#include <FileOperationProgress.h>

int main()
{
  // --- TFileOperationErrorLog basic operations ---
  {
    TFileOperationErrorLog ErrorLog;
    assert(!ErrorLog.HasErrors());
    assert(ErrorLog.GetErrorCount() == 0);

    ErrorLog.AddError(L"test.txt", L"Permission denied", osLocal, TFileOperationErrorCategory::PermissionDenied);
    assert(ErrorLog.HasErrors());
    assert(ErrorLog.GetErrorCount() == 1);

    ErrorLog.AddError(L"remote.dat", L"Network timeout", osRemote, TFileOperationErrorCategory::NetworkError);
    assert(ErrorLog.GetErrorCount() == 2);

    std::wcout << L"Test: Error log collects errors correctly" << std::endl;
  }

  // --- TFileOperationErrorLog report generation ---
  {
    TFileOperationErrorLog ErrorLog;
    ErrorLog.AddError(L"file1.txt", L"Error 1", osLocal, TFileOperationErrorCategory::Other);
    ErrorLog.AddError(L"file2.txt", L"Error 2", osRemote, TFileOperationErrorCategory::Other);

    UnicodeString Report = ErrorLog.GenerateReport();
    assert(!Report.IsEmpty());
    assert(Report.Pos(L"file1.txt") > 0);
    assert(Report.Pos(L"file2.txt") > 0);
    assert(Report.Pos(L"Local") > 0);
    assert(Report.Pos(L"Remote") > 0);

    std::wcout << L"Test: Error report generation works" << std::endl;
  }

  // --- TFileOperationErrorLog empty report ---
  {
    TFileOperationErrorLog ErrorLog;
    UnicodeString Report = ErrorLog.GenerateReport();
    assert(Report == L"No errors occurred.");

    std::wcout << L"Test: Empty error report handled correctly" << std::endl;
  }

  // --- TFileOperationErrorLog clear ---
  {
    TFileOperationErrorLog ErrorLog;
    ErrorLog.AddError(L"file.txt", L"Error", osLocal, TFileOperationErrorCategory::Other);
    assert(ErrorLog.HasErrors());

    ErrorLog.Clear();
    assert(!ErrorLog.HasErrors());
    assert(ErrorLog.GetErrorCount() == 0);

    std::wcout << L"Test: Error log clear works correctly" << std::endl;
  }

  // --- TFileOperationProgressType integrates error log ---
  {
    TFileOperationProgressType Progress;
    assert(!Progress.GetErrorLog().HasErrors());

    Progress.AddOperationError(L"test.cpp", L"Syntax error", osLocal, TFileOperationErrorCategory::Other);
    assert(Progress.GetErrorLog().HasErrors());
    assert(Progress.GetErrorLog().GetErrorCount() == 1);

    Progress.Clear();
    assert(!Progress.GetErrorLog().HasErrors());

    std::wcout << L"Test: OperationProgress error log integration works" << std::endl;
  }

  std::wcout << L"All silent mode tests passed." << std::endl;
  return 0;
}
