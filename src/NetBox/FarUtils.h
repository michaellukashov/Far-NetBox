#pragma once

#include <vcl.h>

void FarWrapText(const UnicodeString & Text, TStrings * Result, int32_t MaxWidth);

/**
 * File read/write wrapper
 */
class CNBFile : public TObject
{
  NB_DISABLE_COPY(CNBFile)
public:
  CNBFile() : m_File(INVALID_HANDLE_VALUE), m_LastError(0) {}
  ~CNBFile()
  {
    Close();
  }

  /**
     * Open file for writing
     * \param fileName file name
     * \return false if error
     */
  bool OpenWrite(const wchar_t * fileName);
  /**
     * Open file for reading
     * \param fileName file name
     * \return false if error
     */
  bool OpenRead(const wchar_t * fileName);
  /**
     * Read file
     * \param buff read buffer
     * \param buffSize on input - buffer size, on output - read size in bytes
     * \return false if error
     */
  bool Read(void * buff, size_t & buffSize);
  /**
     * Write file
     * \param buff write buffer
     * \param buffSize buffer size
     * \return false if error
     */
  bool Write(const void * buff, const size_t buffSize);
  /**
     * Get file size
     * \return file size or -1 if error
     */
  int64_t GetFileSize() const;
  /**
     * Close file
     */
  void Close();
  /**
     * Get last errno
     * \return last errno
     */
  DWORD LastError() const;
  /**
     * Save file
     * \param fileName file name
     * \param fileContent file content
     * \return error code
     */
  static DWORD SaveFile(const wchar_t * fileName, const nb::vector_t<char> & fileContent);
  /**
     * Save file
     * \param fileName file name
     * \param fileContent file content
     * \return error code
     */
  static DWORD SaveFile(const wchar_t * fileName, const char * fileContent);
  /**
     * Load file
     * \param fileName file name
     * \param fileContent file content
     * \return error code
     */
  static DWORD LoadFile(const wchar_t * fileName, nb::vector_t<char> & fileContent);

private:
  HANDLE  m_File{INVALID_HANDLE_VALUE};          ///< File handle
  mutable DWORD m_LastError{0};     ///< Last errno
};

