#pragma once

#include "FarUtil.h"

#include <curl/curl.h>
#include "SessionInfo.h"

#define CHECK_CURL_CALL(code, fc) { if (code == CURLE_OK) { code = fc; } }

enum ProxyTypes
{
    PROXY_NONE = 0,
    PROXY_SOCKS4,
    PROXY_SOCKS5,
    PROXY_HTTP,
};

class TSessionData;
class TTerminal;

/** @brief CURL slist wrapper
 *
 */
class CSlistURL
{
public:
    explicit CSlistURL() : m_SList(NULL) {}
    ~CSlistURL()
    {
        if (m_SList)
        {
            curl_slist_free_all(m_SList);
        }
    }
    inline void Append(const char *val)
    {
        assert(val);
        m_SList = curl_slist_append(m_SList, val);
    }
    operator curl_slist *()
    {
        return m_SList;
    }
private:
    curl_slist *m_SList;
};

class TCURLIntf
{
public:
    virtual ~TCURLIntf()
    {}
    virtual void Init() = 0;

    enum TLogLevel
    {
        LOG_STATUS = 0,
        LOG_ERROR = 1,
        LOG_COMMAND = 2,
        LOG_REPLY = 3,
        LOG_LIST = 4,
        LOG_APIERROR = 5,
        LOG_WARNING = 6,
        LOG_INFO = 7,
        LOG_DEBUG = 8
    };
    /** @brief Initialize easy curl
     *  @param $url URL to connect
     *  @param $userName user name
     *  @param $password password
     *  @return false if error
     */
    virtual bool Initialize(const wchar_t *url, const wchar_t *userName,
        const wchar_t *password) = 0;

    /**
     * Close curl
     */
    virtual bool Close() = 0;

    /** @brief Prepare easy curl state
     *  @param $path requested path
     *  @param $handleTimeout true to handle timeout
     *  @return curl status
     */
    virtual CURLcode Prepare(const char *path,
        const TSessionData *Data,
        int LogLevel, const bool handleTimeout = true) = 0;

    /** @brief Set slist
     *  @param $slist slist object
     *  @return curl status
     */
    virtual CURLcode SetSlist(CSlistURL &slist) = 0;

    /** @brief Set output as std::string buffer
     *  @param $out output std::string buffer
     *  @param $progress pointer to variable to save progress percent of the current operation
     *  @return curl status
     */
    virtual CURLcode SetOutput(std::string &out, size_t *progress) = 0;

    /** @brief Set output as file
     *  @param $out output file
     *  @param $progress pointer to variable to save progress percent of the current operation
     *  @return curl status
     */
    virtual CURLcode SetOutput(CNBFile *out, size_t *progress) = 0;

    /** @brief Set input as file (upload operations)
     *  @param $in input file
     *  @param $progress pointer to variable to save progress percent of the current operation
     *  @return curl status
     */
    virtual CURLcode SetInput(CNBFile *in, size_t *progress) = 0;

    /** @brief Set abort event handle
     *  @param $event abort event handle
     */
    virtual void SetAbortEvent(HANDLE event) = 0;

    /** @brief Perform request
     *  @return curl status
     */
    virtual CURLcode Perform() = 0;

    /** @brief Get top URL
     *  @return top URL
     */
    virtual const char *GetTopURL() const = 0;
    // virtual operator CURL *() = 0;
    virtual bool Aborted() const = 0;

    virtual void SetDebugLevel(TLogLevel Level) = 0;
    virtual CURL *GetCURL() = 0;
};

/** @brief CURL easy wrapper
 */
class CEasyURL : public TCURLIntf
{
public:
    explicit CEasyURL(TTerminal *Terminal);
    virtual void Init();
    virtual ~CEasyURL();

    virtual bool Initialize(const wchar_t *url, const wchar_t *userName,
        const wchar_t *password);
    virtual bool Close();
    virtual CURLcode Prepare(const char *path,
        const TSessionData *Data,
        int LogLevel, const bool handleTimeout = true);
    virtual CURLcode SetSlist(CSlistURL &slist);
    virtual CURLcode SetOutput(std::string &out, size_t *progress);
    virtual CURLcode SetOutput(CNBFile *out, size_t *progress);
    virtual CURLcode SetInput(CNBFile *in, size_t *progress);
    virtual void SetAbortEvent(HANDLE event);
    virtual CURLcode Perform();
    virtual const char *GetTopURL() const
    {
        return m_TopURL.c_str();
    }

    virtual CURL *GetCURL()
    {
        return m_CURL;
    }

    operator CURL *()
    {
        return m_CURL;
    }

    virtual bool Aborted() const
    {
        return m_ProgressInfo.Aborted;
    }

    virtual void SetDebugLevel(TLogLevel Level) { FDebugLevel = Level; }

private:
    int DebugOutput(TLogLineType type, const char *data, size_t size);

private:
    /// Internal reader callback (see libcurl docs)
    static size_t InternalReader(void *buffer, size_t size, size_t nmemb, void *userData);

    /// Internal writer callback (see libcurl docs)
    static size_t InternalWriter(void *buffer, size_t size, size_t nmemb, void *userData);

    /// Internal progress counter callback (see libcurl docs)
    static int InternalProgress(void *userData, double dltotal, double dlnow, double ultotal, double ulnow);

    static int InternalDebug(CURL *handle, curl_infotype type,
        char *data, size_t size,
        void *userp);

private:
    TTerminal *FTerminal;
    CURL *m_CURL; ///< CURL
    bool m_Prepared; ///< Prepare statement flag

    std::string m_TopURL; ///< Top URL (ftp://host:21)
    std::string m_UserName; ///< User name
    std::string m_Password; ///< Password

    /// Output writer description
    struct OutputWriter
    {
        enum OutputWriterType
        {
            None,
            TypeString,
            TypeFile
        } Type;
        union
        {
            std::string *String;
            CNBFile  *File;
        };
        HANDLE AbortEvent;
    };
    OutputWriter m_Output;

    /// Input reader description
    struct InputReader
    {
        unsigned __int64 Current;
        unsigned __int64 Total;
        CNBFile *File;
        size_t *ProgressPtr;
        HANDLE AbortEvent;
        enum InputReaderType
        {
            None,
            TypeFile
        } Type;
    };
    InputReader m_Input;

    /// Progress description
    struct TCURLProgressInfo
    {
        size_t *ProgressPtr;
        HANDLE AbortEvent;
        bool Aborted;
    };
    TCURLProgressInfo m_ProgressInfo;
    HANDLE m_regex;
    RegExpMatch *m_match;
    int m_brackets;
    TLogLevel FDebugLevel;
};

