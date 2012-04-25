//---------------------------------------------------------------------------
#include "stdafx.h"

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>
#include <boost/bind.hpp>

#include "PuttyIntf.h"
#include "Exceptions.h"
#include "Interface.h"
#include "SecureShell.h"
#include "TextsCore.h"
#include "HelpCore.h"
#include "Common.h"
#include "CoreMain.h"
#include "FileSystems.h"

#ifndef AUTO_WINSOCK
#include <winsock2.h>
#endif
//---------------------------------------------------------------------------
#define MAX_BUFSIZE 32768
//---------------------------------------------------------------------------
struct TPuttyTranslation
{
    const char *Original;
    int Translation;
};
//---------------------------------------------------------------------------
TSecureShell::TSecureShell(TSessionUI *UI,
                           TSessionData *SessionData, TSessionLog *Log, TConfiguration *Configuration) :
    PendLen(0)
{
    FUI = UI;
    FSessionData = SessionData;
    FLog = Log;
    FConfiguration = Configuration;
    FActive = false;
    FWaiting = 0;
    FOpened = false;
    OutPtr = NULL;
    Pending = NULL;
    FBackendHandle = NULL;
    ResetConnection();
    FConfig = new Config();
    memset(FConfig, 0, sizeof(*FConfig));
    FSocket = INVALID_SOCKET;
    FSocketEvent = CreateEvent(NULL, false, false, NULL);
    FFrozen = false;
    FSimple = false;
    Self = this;
}
//---------------------------------------------------------------------------
TSecureShell::~TSecureShell()
{
    assert(FWaiting == 0);
    SetActive(false);
    ResetConnection();
    ::CloseHandle(FSocketEvent);
    ClearConfig(FConfig);
    delete FConfig;
    FConfig = NULL;
}
//---------------------------------------------------------------------------
void TSecureShell::ResetConnection()
{
    FreeBackend();
    ClearStdError();
    PendLen = 0;
    PendSize = 0;
    sfree(Pending);
    Pending = NULL;
    FCWriteTemp = L"";
    ResetSessionInfo();
    FAuthenticating = false;
    FAuthenticated = false;
    FStoredPasswordTried = false;
    FStoredPasswordTriedForKI = false;
}
//---------------------------------------------------------------------------
void TSecureShell::ResetSessionInfo()
{
    FSessionInfoValid = false;
    FMinPacketSize = NULL;
    FMaxPacketSize = NULL;
}
//---------------------------------------------------------------------------
inline void TSecureShell::UpdateSessionInfo()
{
    if (!FSessionInfoValid)
    {
        FSshVersion = get_ssh_version(FBackendHandle);
        FSessionInfo.ProtocolBaseName = L"SSH";
        FSessionInfo.ProtocolName =
            FORMAT(L"%s-%d", FSessionInfo.ProtocolBaseName.c_str(), get_ssh_version(FBackendHandle));
        FSessionInfo.SecurityProtocolName = FSessionInfo.ProtocolName;

        FSessionInfo.CSCompression =
            FuncToCompression(FSshVersion, get_cscomp(FBackendHandle));
        FSessionInfo.SCCompression =
            FuncToCompression(FSshVersion, get_sccomp(FBackendHandle));

        if (FSshVersion == 1)
        {
            FSessionInfo.CSCipher = CipherNames[FuncToSsh1Cipher(get_cipher(FBackendHandle))];
            FSessionInfo.SCCipher = CipherNames[FuncToSsh1Cipher(get_cipher(FBackendHandle))];
        }
        else
        {
            FSessionInfo.CSCipher = CipherNames[FuncToSsh2Cipher(get_cscipher(FBackendHandle))];
            FSessionInfo.SCCipher = CipherNames[FuncToSsh2Cipher(get_sccipher(FBackendHandle))];
        }

        FSessionInfoValid = true;
    }
}
//---------------------------------------------------------------------------
const TSessionInfo &TSecureShell::GetSessionInfo()
{
    if (!FSessionInfoValid)
    {
        UpdateSessionInfo();
    }
    return FSessionInfo;
}
//---------------------------------------------------------------------
void TSecureShell::ClearConfig(Config *cfg)
{
    delete[] cfg->remote_cmd_ptr;
    delete[] cfg->remote_cmd_ptr2;
    // clear all
    memset(cfg, 0, sizeof(*cfg));
}
//---------------------------------------------------------------------
void TSecureShell::StoreToConfig(TSessionData *Data, Config *cfg, bool Simple)
{
    ClearConfig(cfg);

    // user-configurable settings
    ASCOPY(cfg->host, System::W2MB(Data->GetHostName().c_str(), Data->GetCodePageAsNumber()));
    // cfg->host = ::StrNew(System::W2MB(Data->GetHostName().c_str(), Data->GetCodePageAsNumber()).c_str());
    ASCOPY(cfg->username, System::W2MB(Data->GetUserName().c_str(), Data->GetCodePageAsNumber()));
    // cfg->username = ::StrNew(System::W2MB(Data->GetUserName().c_str(), Data->GetCodePageAsNumber())).c_str();
    cfg->port = Data->GetPortNumber();
    cfg->protocol = PROT_SSH;
    // always set 0, as we will handle keepalives ourselves to avoid
    // multi-threaded issues in putty timer list
    cfg->ping_interval = 0;
    cfg->compression = Data->GetCompression();
    cfg->tryagent = Data->GetTryAgent();
    cfg->agentfwd = Data->GetAgentFwd();
    cfg->addressfamily = Data->GetAddressFamily();
    ASCOPY(cfg->ssh_rekey_data, System::W2MB(Data->GetRekeyData().c_str(), Data->GetCodePageAsNumber()));
    // cfg->ssh_rekey_data = ::StrNew(System::W2MB(Data->GetRekeyData().c_str(), Data->GetCodePageAsNumber())).c_str();
    cfg->ssh_rekey_time = Data->GetRekeyTime();

    for (int c = 0; c < CIPHER_COUNT; c++)
    {
        int pcipher = 0;
        switch (Data->GetCipher(c))
        {
        case cipWarn: pcipher = CIPHER_WARN; break;
        case cip3DES: pcipher = CIPHER_3DES; break;
        case cipBlowfish: pcipher = CIPHER_BLOWFISH; break;
        case cipAES: pcipher = CIPHER_AES; break;
        case cipDES: pcipher = CIPHER_DES; break;
        case cipArcfour: pcipher = CIPHER_ARCFOUR; break;
        default: assert(false);
        }
        cfg->ssh_cipherlist[c] = pcipher;
    }

    for (int k = 0; k < KEX_COUNT; k++)
    {
        int pkex = 0;
        switch (Data->GetKex(k))
        {
        case kexWarn: pkex = KEX_WARN; break;
        case kexDHGroup1: pkex = KEX_DHGROUP1; break;
        case kexDHGroup14: pkex = KEX_DHGROUP14; break;
        case kexDHGEx: pkex = KEX_DHGEX; break;
        case kexRSA: pkex = KEX_RSA; break;
        default: assert(false);
        }
        cfg->ssh_kexlist[k] = pkex;
    }

    std::wstring SPublicKeyFile = Data->GetPublicKeyFile();
    if (SPublicKeyFile.empty()) { SPublicKeyFile = Configuration->GetDefaultKeyFile(); }
    SPublicKeyFile = StripPathQuotes(ExpandEnvironmentVariables(SPublicKeyFile));
    ASCOPY(cfg->keyfile.path, System::W2MB(SPublicKeyFile.c_str(), Data->GetCodePageAsNumber()));
    // cfg->keyfile.path = ::StrNew(System::W2MB(SPublicKeyFile.c_str(), Data->GetCodePageAsNumber())).c_str();
    cfg->sshprot = Data->GetSshProt();
    cfg->ssh2_des_cbc = Data->GetSsh2DES();
    cfg->ssh_no_userauth = Data->GetSshNoUserAuth();
    cfg->try_tis_auth = Data->GetAuthTIS();
    cfg->try_ki_auth = Data->GetAuthKI();
    cfg->try_gssapi_auth = Data->GetAuthGSSAPI();
    cfg->gssapifwd = Data->GetGSSAPIFwdTGT();
    cfg->change_username = Data->GetChangeUsername();

    cfg->proxy_type = Data->GetProxyMethod();
    ASCOPY(cfg->proxy_host, System::W2MB(Data->GetProxyHost().c_str(), Data->GetCodePageAsNumber()));
    // cfg->proxy_host = ::StrNew(System::W2MB(Data->GetProxyHost().c_str(), Data->GetCodePageAsNumber())).c_str();
    cfg->proxy_port = Data->GetProxyPort();
    ASCOPY(cfg->proxy_username, System::W2MB(Data->GetProxyUsername().c_str(), Data->GetCodePageAsNumber()));
    // cfg->proxy_username = ::StrNew(System::W2MB(Data->GetProxyUsername().c_str(), Data->GetCodePageAsNumber())).c_str();
    ASCOPY(cfg->proxy_password, System::W2MB(Data->GetProxyPassword().c_str(), Data->GetCodePageAsNumber()));
    // cfg->proxy_password = ::StrNew(System::W2MB(Data->GetProxyPassword().c_str(), Data->GetCodePageAsNumber())).c_str();
    if (Data->GetProxyMethod() == pmCmd)
    {
        ASCOPY(cfg->proxy_telnet_command, System::W2MB(Data->GetProxyLocalCommand().c_str(), Data->GetCodePageAsNumber()));
        // cfg->proxy_telnet_command = ::StrNew(System::W2MB(Data->GetProxyLocalCommand().c_str(), Data->GetCodePageAsNumber())).c_str();
    }
    else
    {
        ASCOPY(cfg->proxy_telnet_command, System::W2MB(Data->GetProxyTelnetCommand().c_str(), Data->GetCodePageAsNumber()));
        // cfg->proxy_telnet_command = ::StrNew(System::W2MB(Data->GetProxyTelnetCommand().c_str(), Data->GetCodePageAsNumber())).c_str();
    }
    cfg->proxy_dns = Data->GetProxyDNS();
    cfg->even_proxy_localhost = Data->GetProxyLocalhost();

    // #pragma option push -w-eas
    // after 0.53b values were reversed, however putty still stores
    // settings to registry in same way as before
    cfg->sshbug_ignore1 = Data->GetBug(sbIgnore1);
    cfg->sshbug_plainpw1 = Data->GetBug(sbPlainPW1);
    cfg->sshbug_rsa1 = Data->GetBug(sbRSA1);
    cfg->sshbug_hmac2 = Data->GetBug(sbHMAC2);
    cfg->sshbug_derivekey2 = Data->GetBug(sbDeriveKey2);
    cfg->sshbug_rsapad2 = Data->GetBug(sbRSAPad2);
    cfg->sshbug_rekey2 = Data->GetBug(sbRekey2);
    // new after 0.53b
    cfg->sshbug_pksessid2 = Data->GetBug(sbPKSessID2);
    cfg->sshbug_maxpkt2 = Data->GetBug(sbMaxPkt2);
    cfg->sshbug_ignore2 = asAuto;
    // #pragma option pop

    if (!Data->GetTunnelPortFwd().empty())
    {
        assert(!Simple);
        // DEBUG_PRINTF(L"Data->GetTunnelPortFwd = %s", Data->GetTunnelPortFwd().c_str());
        ASCOPY(cfg->portfwd, System::W2MB(Data->GetTunnelPortFwd().c_str(), Data->GetCodePageAsNumber()));
        // cfg->portfwd = ::StrNew(System::W2MB(Data->GetTunnelPortFwd().c_str(), Data->GetCodePageAsNumber()).c_str());
        // when setting up a tunnel, do not open shell/sftp
        cfg->ssh_no_shell = TRUE;
    }
    else
    {
        assert(Simple);
        cfg->ssh_simple = Simple;

        if (Data->GetFSProtocol() == fsSCPonly)
        {
            cfg->ssh_subsys = FALSE;
            if (Data->GetShell().empty())
            {
                // Following forces Putty to open default shell
                // see ssh.c: do_ssh2_authconn() and ssh1_protocol()
                cfg->remote_cmd[0] = '\0';
            }
            else
            {
                // ASCOPY(cfg->remote_cmd_ptr, System::W2MB(Data->GetShell().c_str(), Data->GetCodePageAsNumber()));
                cfg->remote_cmd_ptr = ::StrNew(System::W2MB(Data->GetShell().c_str(), Data->GetCodePageAsNumber()).c_str());
            }
        }
        else
        {
            if (Data->GetSftpServer().empty())
            {
                cfg->ssh_subsys = TRUE;
                strcpy_s(cfg->remote_cmd, sizeof(cfg->remote_cmd), "sftp");
            }
            else
            {
                cfg->ssh_subsys = FALSE;
                // cfg->remote_cmd_ptr = (char *)System::W2MB(Data->GetSftpServer().c_str(), Data->GetCodePageAsNumber()).c_str();
                cfg->remote_cmd_ptr = ::StrNew(System::W2MB(Data->GetSftpServer().c_str(), Data->GetCodePageAsNumber()).c_str());
            }

            if (Data->GetFSProtocol() != fsSFTPonly)
            {
                cfg->ssh_subsys2 = FALSE;
                if (Data->GetShell().empty())
                {
                    // Following forces Putty to open default shell
                    // see ssh.c: do_ssh2_authconn() and ssh1_protocol()
                    cfg->remote_cmd_ptr2 = ::StrNew("\0");
                }
                else
                {
                    cfg->remote_cmd_ptr2 = ::StrNew(System::W2MB(Data->GetShell().c_str(), Data->GetCodePageAsNumber()).c_str());
                }
            }

            if ((Data->GetFSProtocol() == fsSFTPonly) && Data->GetSftpServer().empty())
            {
                // see psftp_connect() from psftp.c
                cfg->ssh_subsys2 = FALSE;
                cfg->remote_cmd_ptr2 = ::StrNew(
                                           "test -x /usr/lib/sftp-server && exec /usr/lib/sftp-server\n"
                                           "test -x /usr/local/lib/sftp-server && exec /usr/local/lib/sftp-server\n"
                                           "exec sftp-server");
            }
        }
    }

    cfg->connect_timeout = Data->GetTimeout() * 1000;

    // permanent settings
    cfg->nopty = TRUE;
    cfg->tcp_keepalives = 0;
    cfg->ssh_show_banner = TRUE;
    for (int Index = 0; Index < ngsslibs; Index++)
    {
        cfg->ssh_gsslist[Index] = gsslibkeywords[Index].v;
    }
}
//---------------------------------------------------------------------------
void TSecureShell::Open()
{
    FBackend = &ssh_backend;
    ResetConnection();

    FAuthenticating = false;
    FAuthenticated = false;

    SetActive(false);

    FAuthenticationLog = L"";
    FUI->Information(LoadStr(STATUS_LOOKUPHOST), true);
    StoreToConfig(FSessionData, FConfig, GetSimple());

    char *RealHost = NULL;
    FreeBackend(); // in case we are reconnecting
    const char *InitError = FBackend->init(this, &FBackendHandle, FConfig,
                                           const_cast<char *>(System::W2MB(FSessionData->GetHostName().c_str(), FSessionData->GetCodePageAsNumber()).c_str()), FSessionData->GetPortNumber(), &RealHost, 0,
                                           FConfig->tcp_keepalives);
    sfree(RealHost);
    if (InitError)
    {
        PuttyFatalError(System::MB2W(InitError, FSessionData->GetCodePageAsNumber()));
    }
    FUI->Information(LoadStr(STATUS_CONNECT), true);
    Init();

    CheckConnection(CONNECTION_FAILED);
    FLastDataSent = System::Now();

    FSessionInfo.LoginTime = System::Now();

    FAuthenticating = false;
    FAuthenticated = true;
    FUI->Information(LoadStr(STATUS_AUTHENTICATED), true);

    ResetSessionInfo();

    assert(!FSessionInfo.SshImplementation.empty());
    FOpened = true;
}
//---------------------------------------------------------------------------
void TSecureShell::Init()
{
    try
    {
        try
        {
            // Recent pscp checks FBackend->exitcode(FBackendHandle) in the loop
            // (see comment in putty revision 8110)
            // It seems that we do not need to do it.

            while (!get_ssh_state_session(FBackendHandle))
            {
                if (Configuration->GetActualLogProtocol() >= 1)
                {
                    LogEvent(L"Waiting for the server to continue with the initialisation");
                }
                WaitForData();
            }

            // unless this is tunnel session, it must be safe to send now
            assert(FBackend->sendok(FBackendHandle) || !FSessionData->GetTunnelPortFwd().empty());
        }
        catch (const std::exception &E)
        {
            if (FAuthenticating && !FAuthenticationLog.empty())
            {
                FUI->FatalError(&E, FMTLOAD(AUTHENTICATION_LOG, FAuthenticationLog.c_str()));
            }
            else
            {
                throw;
            }
        }
    }
    catch (const std::exception &E)
    {
        if (FAuthenticating)
        {
            FUI->FatalError(&E, LoadStr(AUTHENTICATION_FAILED));
        }
        else
        {
            throw;
        }
    }
}
//---------------------------------------------------------------------------
void TSecureShell::PuttyLogEvent(const std::wstring Str)
{
    // DEBUG_PRINTF(L"Str = %s", Str.c_str());
#define SERVER_VERSION_MSG L"Server version: "
    // Gross hack
    if (Str.find(std::wstring(SERVER_VERSION_MSG)) == 0)
    {
        FSessionInfo.SshVersionString = Str.substr(std::wstring(SERVER_VERSION_MSG).size() + 1,
                                        Str.size() - std::wstring(SERVER_VERSION_MSG).size());

        const wchar_t *Ptr = wcschr(FSessionInfo.SshVersionString.c_str(), '-');
        // const wchar_t * Ptr = NULL;
        // int pos = FSessionInfo.SshVersionString.find('-');
        // if (pos >= 0)
        // Ptr = &FSessionInfo.SshVersionString[pos];
        if (Ptr != NULL)
        {
            Ptr = wcschr(Ptr + 1, '-');
        }
        FSessionInfo.SshImplementation = (Ptr != NULL) ? Ptr + 1 : L"";
        // DEBUG_PRINTF(L"FSessionInfo.SshImplementation = %s", FSessionInfo.SshImplementation.c_str());
    }
#define FORWARDING_FAILURE_MSG L"Forwarded connection refused by server: "
    else if (Str.find(std::wstring(FORWARDING_FAILURE_MSG)) == 0)
    {
        FLastTunnelError = Str.substr(std::wstring(FORWARDING_FAILURE_MSG).size(),
                                      Str.size() - std::wstring(FORWARDING_FAILURE_MSG).size());
        // DEBUG_PRINTF(L"FLastTunnelError = %s", FLastTunnelError.c_str());
        static const TPuttyTranslation Translation[] =
        {
            { "Administratively prohibited [%]", PFWD_TRANSL_ADMIN },
            { "Connect failed [%]", PFWD_TRANSL_CONNECT },
        };
        TranslatePuttyMessage(Translation, LENOF(Translation), FLastTunnelError);
        // DEBUG_PRINTF(L"FLastTunnelError = %s", FLastTunnelError.c_str());
    }
    LogEvent(Str);
}
//---------------------------------------------------------------------------
bool TSecureShell::PromptUser(bool /*ToServer*/,
                              const std::wstring AName, bool /*NameRequired*/,
                              const std::wstring Instructions, bool InstructionsRequired,
                              System::TStrings *Prompts, System::TStrings *Results)
{
    // there can be zero prompts!
    std::wstring instructions = Instructions;
    assert(Results->GetCount() == Prompts->GetCount());

    TPromptKind PromptKind;
    // beware of changing order
    static const TPuttyTranslation NameTranslation[] =
    {
        { "SSH login name", USERNAME_TITLE },
        { "SSH key passphrase", PASSPHRASE_TITLE },
        { "SSH TIS authentication", SERVER_PROMPT_TITLE },
        { "SSH CryptoCard authentication", SERVER_PROMPT_TITLE },
        { "SSH server: %", SERVER_PROMPT_TITLE2 },
        { "SSH server authentication", SERVER_PROMPT_TITLE },
        { "SSH password", PASSWORD_TITLE },
        { "New SSH password", NEW_PASSWORD_TITLE },
    };

    std::wstring Name = AName;
    int Index = TranslatePuttyMessage(NameTranslation, LENOF(NameTranslation), Name);

    const TPuttyTranslation *InstructionTranslation = NULL;
    const TPuttyTranslation *PromptTranslation = NULL;
    size_t PromptTranslationCount = 1;

    if (Index == 0) // username
    {
        static const TPuttyTranslation UsernamePromptTranslation[] =
        {
            { "login as: ", USERNAME_PROMPT2 },
        };

        PromptTranslation = UsernamePromptTranslation;
        PromptKind = pkUserName;
    }
    else if (Index == 1) // passhrase
    {
        static const TPuttyTranslation PassphrasePromptTranslation[] =
        {
            { "Passphrase for key \"%\": ", PROMPT_KEY_PASSPHRASE },
        };

        PromptTranslation = PassphrasePromptTranslation;
        PromptKind = pkPassphrase;
    }
    else if (Index == 2) // TIS
    {
        static const TPuttyTranslation TISInstructionTranslation[] =
        {
            { "Using TIS authentication.%", TIS_INSTRUCTION },
        };
        static const TPuttyTranslation TISPromptTranslation[] =
        {
            { "Response: ", PROMPT_PROMPT },
        };

        InstructionTranslation = TISInstructionTranslation;
        PromptTranslation = TISPromptTranslation;
        PromptKind = pkTIS;
    }
    else if (Index == 3) // CryptoCard
    {
        static const TPuttyTranslation CryptoCardInstructionTranslation[] =
        {
            { "Using CryptoCard authentication.%", CRYPTOCARD_INSTRUCTION },
        };
        static const TPuttyTranslation CryptoCardPromptTranslation[] =
        {
            { "Response: ", PROMPT_PROMPT },
        };

        InstructionTranslation = CryptoCardInstructionTranslation;
        PromptTranslation = CryptoCardPromptTranslation;
        PromptKind = pkCryptoCard;
    }
    else if ((Index == 4) || (Index == 5))
    {
        static const TPuttyTranslation KeybInteractiveInstructionTranslation[] =
        {
            { "Using keyboard-interactive authentication.%", KEYBINTER_INSTRUCTION },
        };

        InstructionTranslation = KeybInteractiveInstructionTranslation;
        PromptKind = pkKeybInteractive;
    }
    else if (Index == 6)
    {
        assert(Prompts->GetCount() == 1);
        Prompts->PutString(0, LoadStr(PASSWORD_PROMPT));
        PromptKind = pkPassword;
    }
    else if (Index == 7)
    {
        static const TPuttyTranslation NewPasswordPromptTranslation[] =
        {
            { "Current password (blank for previously entered password): ", NEW_PASSWORD_CURRENT_PROMPT },
            { "Enter new password: ", NEW_PASSWORD_NEW_PROMPT },
            { "Confirm new password: ", NEW_PASSWORD_CONFIRM_PROMPT },
        };
        PromptTranslation = NewPasswordPromptTranslation;
        PromptTranslationCount = LENOF(NewPasswordPromptTranslation);
        PromptKind = pkNewPassword;
    }
    else
    {
        PromptKind = pkPrompt;
        assert(false);
    }

    LogEvent(FORMAT(L"Prompt (%d, %s, %s, %s)", PromptKind, AName.c_str(),
                    instructions.c_str(), (Prompts->GetCount() > 0 ? Prompts->GetString(0).c_str() : std::wstring(L"<no prompt>").c_str())).c_str());

    Name = ::Trim(Name);
    if (0)
    {
        DEBUG_PRINTF(L"InstructionTranslation = %x", InstructionTranslation);
        static const TPuttyTranslation KeybInteractiveInstructionTranslation[] =
        {
            { "Using keyboard-interactive authentication.%", KEYBINTER_INSTRUCTION },
        };
        InstructionTranslation = KeybInteractiveInstructionTranslation;
        instructions = L"Using keyboard-interactive authentication.";
    }
    if (InstructionTranslation != NULL)
    {
        TranslatePuttyMessage(InstructionTranslation, 1, instructions);
    }

    // some servers add leading blank line to make the prompt look prettier
    // on terminal console
    instructions = ::Trim(instructions);

    for (size_t Index = 0; Index < Prompts->GetCount(); Index++)
    {
        std::wstring Prompt = Prompts->GetString(Index);
        // DEBUG_PRINTF(L"Prompt = %s", Prompt.c_str());
        if (PromptTranslation != NULL)
        {
            TranslatePuttyMessage(PromptTranslation, PromptTranslationCount, Prompt);
        }
        // some servers add leading blank line to make the prompt look prettier
        // on terminal console
        Prompts->PutString(Index, ::Trim(Prompt));
    }

    bool Result = false;
    if (PromptKind == pkUserName)
    {
        if (FSessionData->GetAuthGSSAPI())
        {
            // use empty username if no username was filled on login dialog
            // and GSSAPI auth is enabled, hence there's chance that the server can
            // deduce the username otherwise
            Results->PutString(0, L"");
            Result = true;
        }
    }
    else if ((PromptKind == pkTIS) || (PromptKind == pkCryptoCard) ||
             (PromptKind == pkKeybInteractive))
    {
        if (FSessionData->GetAuthKIPassword() && !FSessionData->GetPassword().empty() &&
                !FStoredPasswordTriedForKI && (Prompts->GetCount() == 1) &&
                !(Prompts->GetObject(0) != NULL))
        {
            LogEvent(L"Using stored password.");
            FUI->Information(LoadStr(AUTH_PASSWORD), false);
            Result = true;
            Results->PutString(0, FSessionData->GetPassword());
            FStoredPasswordTriedForKI = true;
        }
        else if (instructions.empty() && !InstructionsRequired && (Prompts->GetCount() == 0))
        {
            LogEvent(L"Ignoring empty SSH server authentication request");
            Result = true;
        }
    }
    else if (PromptKind == pkPassword)
    {
        if (!FSessionData->GetPassword().empty() && !FStoredPasswordTried)
        {
            LogEvent(L"Using stored password.");
            FUI->Information(LoadStr(AUTH_PASSWORD), false);
            Result = true;
            Results->PutString(0, FSessionData->GetPassword());
            FStoredPasswordTried = true;
            // DEBUG_PRINTF(L"Results = %s", Results->GetText().c_str());
            // DEBUG_PRINTF(L"FSessionData->GetPassword = %s", FSessionData->GetPassword().c_str());
        }
    }

    if (!Result)
    {
        Result = FUI->PromptUser(FSessionData,
                                 PromptKind, Name, instructions, Prompts, Results);

        if (Result)
        {
            if ((PromptKind == pkUserName) && (Prompts->GetCount() == 1))
            {
                FUserName = Results->GetString(0);
            }
        }
    }

    return Result;
}
//---------------------------------------------------------------------------
void TSecureShell::GotHostKey()
{
    // due to re-key GotHostKey() may be called again later during session
    if (!FAuthenticating && !FAuthenticated)
    {
        FAuthenticating = true;
        FUI->Information(LoadStr(STATUS_AUTHENTICATE), true);
    }
}
//---------------------------------------------------------------------------
void TSecureShell::CWrite(const char *Data, size_t Length)
{
    // some messages to stderr may indicate that something has changed with the
    // session, so reset the session info
    ResetSessionInfo();

    // We send only whole line at once, so we have to cache incoming data
    FCWriteTemp += DeleteChar(std::wstring(System::MB2W(std::string(Data, Length).c_str(), FSessionData->GetCodePageAsNumber())), '\r');

    std::wstring Line;
    // Do we have at least one complete line in std error cache?
    while (FCWriteTemp.find_first_of(L"\n") != std::wstring::npos)
    {
        std::wstring Line = CutToChar(FCWriteTemp, '\n', false);

        FLog->Add(llStdError, Line);

        if (FAuthenticating)
        {
            TranslateAuthenticationMessage(Line);
            FAuthenticationLog += (FAuthenticationLog.empty() ? L"" : L"\n") + Line;
        }

        FUI->Information(Line, false);
    }
}
//---------------------------------------------------------------------------
void TSecureShell::RegisterReceiveHandler(const TNotifyEvent &Handler)
{
    assert(FOnReceive.empty());
    FOnReceive.connect(Handler);
}
//---------------------------------------------------------------------------
void TSecureShell::UnregisterReceiveHandler(const TNotifyEvent &Handler)
{
    assert(!FOnReceive.empty());
    USEDPARAM(Handler);
    FOnReceive.disconnect_all_slots();
}
//---------------------------------------------------------------------------
void TSecureShell::FromBackend(bool IsStdErr, const char *Data, size_t Length)
{
    CheckConnection();

    if (Configuration->GetActualLogProtocol() >= 1)
    {
        LogEvent(FORMAT(L"Received %u bytes (%d)", Length, static_cast<int>(IsStdErr)));
    }
    // DEBUG_PRINTF(L"IsStdErr = %d, Length = %d, Data = '%s'", IsStdErr, Length, System::MB2W(Data, FSessionData->GetCodePageAsNumber()).c_str());

    // Following is taken from scp.c from_backend() and modified

    if (IsStdErr)
    {
        std::string Str(Data, Length);
        AddStdError(System::MB2W(Str.c_str(), FSessionData->GetCodePageAsNumber()));
    }
    else
    {
        unsigned char *p = reinterpret_cast<unsigned char *>(const_cast<char *>(Data));
        size_t Len = Length;

        // with event-select mechanism we can now receive data even before we
        // actually expect them (OutPtr can be NULL)

        if ((OutPtr != NULL) && (OutLen > 0) && (Len > 0))
        {
            size_t Used = OutLen;
            if (Used > Len) { Used = Len; }
            memmove(OutPtr, p, Used);
            OutPtr += Used; OutLen -= Used;
            p += Used; Len -= Used;
        }

        if (static_cast<int>(Len) > 0)
        {
            if (PendSize < PendLen + Len)
            {
                PendSize = PendLen + Len + 4096;
                Pending = static_cast<char *>(Pending ? srealloc(Pending, PendSize) : smalloc(PendSize));
                if (!Pending) { FatalError(L"Out of memory"); }
            }
            memmove(Pending + PendLen, p, Len);
            PendLen += Len;
            // DEBUG_PRINTF(L"PendLen = %d", PendLen);
        }

        if (!FOnReceive.empty())
        {
            if (!FFrozen)
            {
                FFrozen = true;
                {
                    BOOST_SCOPE_EXIT ( (&Self) )
                    {
                        Self->FFrozen = false;
                    } BOOST_SCOPE_EXIT_END
                    do
                    {
                        FDataWhileFrozen = false;
                        // DEBUG_PRINTF(L"before FOnReceive");
                        FOnReceive(NULL);
                    }
                    while (FDataWhileFrozen);
                }
            }
            else
            {
                FDataWhileFrozen = true;
            }
        }
    }
}
//---------------------------------------------------------------------------
bool TSecureShell::Peek(char *& Buf, size_t Len)
{
    bool Result = (PendLen >= Len);

    if (Result)
    {
        Buf = Pending;
    }

    return Result;
}
//---------------------------------------------------------------------------
size_t TSecureShell::Receive(char *Buf, size_t Len)
{
    CheckConnection();

    if (Len > 0)
    {
        // Following is taken from scp.c ssh_scp_recv() and modified

        OutPtr = Buf;
        OutLen = Len;

        {
            BOOST_SCOPE_EXIT ( (&OutPtr) )
            {
                OutPtr = NULL;
            } BOOST_SCOPE_EXIT_END
            /*
             * See if the pending-input block contains some of what we
             * need.
             */
            if (PendLen > 0)
            {
                size_t PendUsed = PendLen;
                if (PendUsed > OutLen)
                {
                    PendUsed = OutLen;
                }
                memmove(OutPtr, Pending, PendUsed);
                memmove(Pending, Pending + PendUsed, PendLen - PendUsed);
                OutPtr += PendUsed;
                OutLen -= PendUsed;
                PendLen -= PendUsed;
                if (PendLen == 0)
                {
                    PendSize = 0;
                    sfree(Pending);
                    Pending = NULL;
                }
            }

            while (OutLen > 0)
            {
                if (Configuration->GetActualLogProtocol() >= 1)
                {
                    LogEvent(FORMAT(L"Waiting for another %u bytes", static_cast<int>(OutLen)));
                }
                WaitForData();
            }

            // This seems ambiguous
            if (Len <= 0) { FatalError(LoadStr(LOST_CONNECTION)); }
        }
    };
    if (Configuration->GetActualLogProtocol() >= 1)
    {
        LogEvent(FORMAT(L"Read %u bytes (%d pending)",
                        static_cast<int>(Len), static_cast<int>(PendLen)));
    }
    return Len;
}
//---------------------------------------------------------------------------
std::wstring TSecureShell::ReceiveLine()
{
    size_t Index = 0;
    char Ch;
    std::string Line;
    bool EOL = false;

    do
    {
        // If there is any buffer of received chars
        // DEBUG_PRINTF(L"PendLen = %d", PendLen);
        if (PendLen > 0)
        {
            Index = 0;
            // Repeat until we walk thru whole buffer or reach end-of-line
            while ((Index < PendLen) && (!Index || (Pending[Index-1] != '\n')))
            {
                Index++;
            }
            EOL = static_cast<bool>(Index && (Pending[Index-1] == '\n'));
            // DEBUG_PRINTF(L"PendLen = %d, Index = %d, EOL = %d, Pending = %s", PendLen, Index, EOL, System::MB2W(Pending, FSessionData->GetCodePageAsNumber()).c_str());
            size_t PrevLen = Line.size();
            Line.resize(PrevLen + Index);
            Receive(const_cast<char *>(Line.c_str()) + PrevLen, Index);
        }

        // If buffer don't contain end-of-line character
        // we read one more which causes receiving new buffer of chars
        // DEBUG_PRINTF(L"EOL = %d", EOL);
        if (!EOL)
        {
            Receive(&Ch, 1);
            Line += Ch;
            EOL = (Ch == '\n');
        }
    }
    while (!EOL);

    // We don't want end-of-line character
    std::wstring LineW = ::TrimRight(System::MB2W(Line.c_str(), FSessionData->GetCodePageAsNumber()));
    CaptureOutput(llOutput, LineW);
    return LineW;
}
//---------------------------------------------------------------------------
void TSecureShell::SendSpecial(int Code)
{
    LogEvent(FORMAT(L"Sending special code: %d", (Code)));
    CheckConnection();
    FBackend->special(FBackendHandle, static_cast<Telnet_Special>(Code));
    CheckConnection();
    FLastDataSent = System::Now();
}
//---------------------------------------------------------------------------
void TSecureShell::SendEOF()
{
    SendSpecial(TS_EOF);
}
//---------------------------------------------------------------------------
int TSecureShell::TimeoutPrompt(queryparamstimer_slot_type *PoolEvent)
{
    FWaiting++;

    int Answer;
    {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
            Self->FWaiting--;
        } BOOST_SCOPE_EXIT_END
        TQueryParams Params(qpFatalAbort | qpAllowContinueOnError | qpIgnoreAbort);
        Params.Timer = 500;
        Params.TimerEvent = PoolEvent;
        Params.TimerMessage = FMTLOAD(TIMEOUT_STILL_WAITING3, FSessionData->GetTimeout());
        Params.TimerAnswers = qaAbort;
        Answer = FUI->QueryUser(FMTLOAD(CONFIRM_PROLONG_TIMEOUT3, FSessionData->GetTimeout(), FSessionData->GetTimeout()),
                                NULL, qaRetry | qaAbort, &Params);
    }
    return Answer;
}
//---------------------------------------------------------------------------
void TSecureShell::SendBuffer(size_t &Result)
{
    // for comments see PoolForData
    if (!GetActive())
    {
        Result = qaRetry;
    }
    else
    {
        try
        {
            if (FBackend->sendbuffer(FBackendHandle) <= MAX_BUFSIZE)
            {
                Result = qaOK;
            }
        }
        catch(...)
        {
            Result = qaRetry;
        }
    }
}
//---------------------------------------------------------------------------
void TSecureShell::DispatchSendBuffer(size_t BufSize)
{
    System::TDateTime Start = System::Now();
    do
    {
        CheckConnection();
        if (Configuration->GetActualLogProtocol() >= 1)
        {
            LogEvent(FORMAT(L"There are %u bytes remaining in the send buffer, "
                            L"need to send at least another %u bytes",
                            BufSize, BufSize - MAX_BUFSIZE));
        }
        EventSelectLoop(100, false, NULL);
        BufSize = FBackend->sendbuffer(FBackendHandle);
        if (Configuration->GetActualLogProtocol() >= 1)
        {
            LogEvent(FORMAT(L"There are %u bytes remaining in the send buffer", BufSize));
        }

        if (System::Now() - Start > FSessionData->GetTimeoutDT())
        {
            LogEvent(L"Waiting for dispatching send buffer timed out, asking user what to do.");
            queryparamstimer_slot_type slot = boost::bind(&TSecureShell::SendBuffer, this, _1);
            int Answer = TimeoutPrompt(&slot);
            switch (Answer)
            {
            case qaRetry:
                Start = System::Now();
                break;

            case qaOK:
                BufSize = 0;
                break;

            default:
                assert(false);
                // fallthru

            case qaAbort:
                FatalError(LoadStr(USER_TERMINATED));
                break;
            }
        }
    }
    while (BufSize > MAX_BUFSIZE);
}
//---------------------------------------------------------------------------
void TSecureShell::Send(const char *Buf, size_t Len)
{
    CheckConnection();
    size_t BufSize = FBackend->send(FBackendHandle, const_cast<char *>(Buf), static_cast<int>(Len));
    if (Configuration->GetActualLogProtocol() >= 1)
    {
        LogEvent(FORMAT(L"Sent %u bytes", Len));
        LogEvent(FORMAT(L"There are %u bytes remaining in the send buffer", BufSize));
    }
    FLastDataSent = System::Now();
    // among other forces receive of pending data to free the servers's send buffer
    EventSelectLoop(0, false, NULL);

    if (BufSize > MAX_BUFSIZE)
    {
        DispatchSendBuffer(BufSize);
    }
    CheckConnection();
}
//---------------------------------------------------------------------------
void TSecureShell::SendNull()
{
    LogEvent(L"Sending NULL.");
    Send("", 1);
}
//---------------------------------------------------------------------------
void TSecureShell::SendStr(const std::wstring Str)
{
    CheckConnection();
    std::string str = System::W2MB(Str.c_str(), FSessionData->GetCodePageAsNumber());
    Send(str.c_str(), str.size());
}
//---------------------------------------------------------------------------
void TSecureShell::SendLine(const std::wstring Line)
{
    SendStr(Line);
    Send("\n", 1);
    FLog->Add(llInput, Line);
}
//---------------------------------------------------------------------------
int TSecureShell::TranslatePuttyMessage(
    const TPuttyTranslation *Translation, size_t Count, std::wstring &Message)
{
    int Result = -1;
    for (size_t Index = 0; Index < Count; Index++)
    {
        const char *Original = Translation[Index].Original;
        // DEBUG_PRINTF(L"Original = %s", System::MB2W(Original, FSessionData->GetCodePageAsNumber()).c_str());
        const char *Div = strchr(Original, '%');
        if (Div == NULL)
        {
            if (strcmp(System::W2MB(Message.c_str(), FSessionData->GetCodePageAsNumber()).c_str(), Original) == 0)
            {
                Message = LoadStr(Translation[Index].Translation);
                // DEBUG_PRINTF(L"Message = %s", Message.c_str());
                Result = static_cast<int>(Index);
                break;
            }
        }
        else
        {
            size_t OriginalLen = strlen(Original);
            size_t PrefixLen = Div - Original;
            size_t SuffixLen = OriginalLen - PrefixLen - 1;
            if ((static_cast<size_t>(Message.size()) >= OriginalLen - 1) &&
                    (strncmp(System::W2MB(Message.c_str(), FSessionData->GetCodePageAsNumber()).c_str(), Original, PrefixLen) == 0) &&
                    (strncmp(System::W2MB(Message.c_str(), FSessionData->GetCodePageAsNumber()).c_str() + Message.size() - SuffixLen, Div + 1, SuffixLen) == 0))
            {
                Message = FMTLOAD(Translation[Index].Translation,
                                  ::TrimRight(Message.substr(PrefixLen, Message.size() - PrefixLen - SuffixLen)).c_str());
                // DEBUG_PRINTF(L"Message = %s", Message.c_str());
                Result = static_cast<int>(Index);
                break;
            }
        }
    }
    return Result;
}
//---------------------------------------------------------------------------
int TSecureShell::TranslateAuthenticationMessage(std::wstring &Message)
{
    static const TPuttyTranslation Translation[] =
    {
        { "Using username \"%\".", AUTH_TRANSL_USERNAME },
        { "Using keyboard-interactive authentication.", AUTH_TRANSL_KEYB_INTER }, // not used anymore
        { "Authenticating with public key \"%\" from agent", AUTH_TRANSL_PUBLIC_KEY_AGENT },
        { "Authenticating with public key \"%\"", AUTH_TRANSL_PUBLIC_KEY },
        { "Authenticated using RSA key \"%\" from agent", AUTH_TRANSL_PUBLIC_KEY_AGENT },
        { "Wrong passphrase", AUTH_TRANSL_WRONG_PASSPHRASE },
        { "Wrong passphrase.", AUTH_TRANSL_WRONG_PASSPHRASE },
        { "Access denied", AUTH_TRANSL_ACCESS_DENIED },
        { "Trying public key authentication.", AUTH_TRANSL_TRY_PUBLIC_KEY },
        { "Server refused our public key.", AUTH_TRANSL_KEY_REFUSED },
        { "Server refused our key", AUTH_TRANSL_KEY_REFUSED }
    };

    return TranslatePuttyMessage(Translation, LENOF(Translation), Message);
}
//---------------------------------------------------------------------------
void TSecureShell::AddStdError(const std::wstring Str)
{
    FStdError += Str;

    size_t P;
    std::wstring str = DeleteChar(Str, '\r');
    // We send only whole line at once to log, so we have to cache
    // incoming std error data
    FStdErrorTemp += str;
    std::wstring Line;
    // Do we have at least one complete line in std error cache?
    while ((P = FStdErrorTemp.find_first_of(L"\n")) != std::wstring::npos)
    {
        Line = ::TrimRight(FStdErrorTemp.substr(0, P));
        FStdErrorTemp.erase(0, P + 1);
        // DEBUG_PRINTF(L"P = %d, Line = '%s', FStdErrorTemp = '%s'", P, Line.c_str(), FStdErrorTemp.c_str());
        AddStdErrorLine(Line);
    }
}
//---------------------------------------------------------------------------
void TSecureShell::AddStdErrorLine(const std::wstring Str)
{
    if (FAuthenticating)
    {
        FAuthenticationLog += (FAuthenticationLog.empty() ? L"" : L"\n") + Str;
    }
    if (!::Trim(Str).empty())
    {
        // DEBUG_PRINTF(L"Str = %s", ::Trim(Str).c_str());
        CaptureOutput(llStdError, Str);
    }
}
//---------------------------------------------------------------------------
const std::wstring TSecureShell::GetStdError()
{
    return FStdError;
}
//---------------------------------------------------------------------------
void TSecureShell::ClearStdError()
{
    // Flush std error cache
    if (!FStdErrorTemp.empty())
    {
        if (FAuthenticating)
        {
            FAuthenticationLog +=
                (FAuthenticationLog.empty() ? L"" : L"\n") + FStdErrorTemp;
        }
        CaptureOutput(llStdError, FStdErrorTemp);
        FStdErrorTemp = L"";
    }
    FStdError = L"";
}
//---------------------------------------------------------------------------
void TSecureShell::CaptureOutput(TLogLineType Type,
                                 const std::wstring Line)
{
    if (!FOnCaptureOutput.empty())
    {
        FOnCaptureOutput(Line, (Type == llStdError));
    }
    FLog->Add(Type, Line);
}
//---------------------------------------------------------------------------
int TSecureShell::TranslateErrorMessage(std::wstring &Message)
{
    static const TPuttyTranslation Translation[] =
    {
        { "Server unexpectedly closed network connection", UNEXPECTED_CLOSE_ERROR },
        { "Network error: Connection refused", NET_TRANSL_REFUSED },
        { "Network error: Connection reset by peer", NET_TRANSL_RESET },
        { "Network error: Connection timed out", NET_TRANSL_TIMEOUT },
    };

    return TranslatePuttyMessage(Translation, LENOF(Translation), Message);
}
//---------------------------------------------------------------------------
void TSecureShell::PuttyFatalError(const std::wstring Error)
{
    std::wstring error = Error;
    TranslateErrorMessage(error);

    FatalError(error);
}
//---------------------------------------------------------------------------
void TSecureShell::FatalError(const std::wstring Error)
{
    FUI->FatalError(NULL, Error);
}
//---------------------------------------------------------------------------
void inline TSecureShell::LogEvent(const std::wstring Str)
{
    if (FLog->GetLogging())
    {
        FLog->Add(llMessage, Str);
    }
}
//---------------------------------------------------------------------------
void TSecureShell::SocketEventSelect(SOCKET Socket, HANDLE Event, bool Startup)
{
    // DEBUG_PRINTF(L"Socket = %d, Event = %d, Startup = %d", Socket, Event, Startup);
    int Events;

    if (Startup)
    {
        Events = (FD_CONNECT | FD_READ | FD_WRITE | FD_OOB | FD_CLOSE | FD_ACCEPT);
    }
    else
    {
        Events = 0;
    }

    // DEBUG_PRINTF(L"Events = %d, Configuration->GetActualLogProtocol = %d", Events, Configuration->GetActualLogProtocol());
    if (Configuration->GetActualLogProtocol() >= 2)
    {
        LogEvent(FORMAT(L"Selecting events %d for socket %d", static_cast<int>(Events), static_cast<int>(Socket)));
    }

    if (WSAEventSelect(Socket, (WSAEVENT)Event, Events) == SOCKET_ERROR)
    {
        if (Configuration->GetActualLogProtocol() >= 2)
        {
            LogEvent(FORMAT(L"Error selecting events %d for socket %d", static_cast<int>(Events), static_cast<int>(Socket)));
        }

        if (Startup)
        {
            FatalError(FMTLOAD(EVENT_SELECT_ERROR, WSAGetLastError()));
        }
    }
}
//---------------------------------------------------------------------------
void TSecureShell::UpdateSocket(SOCKET value, bool Startup)
{
    // DEBUG_PRINTF(L"FActive = %d, Startup = %d", FActive, Startup);
    // DEBUG_PRINTF(L"value = %d, INVALID_SOCKET = %d", value, INVALID_SOCKET);
    if (!FActive && !Startup)
    {
        // no-op
        // Remove the branch eventualy:
        // When TCP connection fails, PuTTY does not release the memory allocate for
        // socket. As a simple hack we call sk_tcp_close() in ssh.c to release the memory,
        // until they fix it better. Unfortunately sk_tcp_close calls do_select,
        // so we must filter that out.
    }
    else
    {
        assert(value);
        assert((FActive && (FSocket == value)) || (!FActive && Startup));

        // filter our "local proxy" connection, which has no socket
        if (value != INVALID_SOCKET)
        {
            SocketEventSelect(value, FSocketEvent, Startup);
        }
        else
        {
            assert(FSessionData->GetProxyMethod() == pmCmd);
        }

        if (Startup)
        {
            FSocket = value;
            FActive = true;
        }
        else
        {
            FSocket = INVALID_SOCKET;
            Discard();
        }
    }
}
//---------------------------------------------------------------------------
void TSecureShell::UpdatePortFwdSocket(SOCKET value, bool Startup)
{
    // DEBUG_PRINTF(L"Configuration->GetActualLogProtocol = %d", Configuration->GetActualLogProtocol());
    if (Configuration->GetActualLogProtocol() >= 2)
    {
        LogEvent(FORMAT(L"Updating forwarding socket %d (%d)", static_cast<int>(value), static_cast<int>(Startup)));
    }

    SocketEventSelect(value, FSocketEvent, Startup);
    // DEBUG_PRINTF(L"Startup = %d, FPortFwdSockets.size = %d", Startup, FPortFwdSockets.size());
    if (Startup)
    {
        FPortFwdSockets.insert(value);
    }
    else
    {
        FPortFwdSockets.erase(value);
    }
}
//---------------------------------------------------------------------------
void TSecureShell::SetActive(bool value)
{
    if (FActive != value)
    {
        if (value)
        {
            Open();
        }
        else
        {
            Close();
        }
    }
}
//---------------------------------------------------------------------------
void TSecureShell::FreeBackend()
{
    if (FBackendHandle != NULL)
    {
        FBackend->bfree(FBackendHandle);
        FBackendHandle = NULL;
    }
}
//---------------------------------------------------------------------------
void TSecureShell::Discard()
{
    bool WasActive = FActive;
    FActive = false;
    FOpened = false;

    if (WasActive)
    {
        FUI->Closed();
    }
}
//---------------------------------------------------------------------------
void TSecureShell::Close()
{
    LogEvent(L"Closing connection.");
    assert(FActive);

    // this is particularly necessary when using local proxy command
    // (e.g. plink), otherwise it hangs in sk_localproxy_close
    SendEOF();

    FreeBackend();

    Discard();
}
//---------------------------------------------------------------------------
void inline TSecureShell::CheckConnection(int Message)
{
    if (!FActive || get_ssh_state_closed(FBackendHandle))
    {
        std::wstring Str = LoadStr(Message >= 0 ? Message : NOT_CONNECTED);
        int ExitCode = get_ssh_exitcode(FBackendHandle);
        if (ExitCode >= 0)
        {
            Str += L" " + std::wstring(FMTLOAD(SSH_EXITCODE, ExitCode));
        }
        FatalError(Str);
    }
}
//---------------------------------------------------------------------------
void TSecureShell::PoolForData(WSANETWORKEVENTS &Events, size_t &Result)
{
    if (!GetActive())
    {
        // see comment below
        Result = qaRetry;
    }
    else
    {
        try
        {
            if (Configuration->GetActualLogProtocol() >= 2)
            {
                LogEvent(L"Pooling for data in case they finally arrives");
            }

            // in extreme condition it may happen that send buffer is full, but there
            // will be no data comming and we may not empty the send buffer because we
            // do not process FD_WRITE until we receive any FD_READ
            if (EventSelectLoop(0, false, &Events))
            {
                LogEvent(L"Data has arrived, closing query to user.");
                Result = qaOK;
            }
        }
        catch(...)
        {
            // if we let the exception out, it may popup another message dialog
            // in whole event loop, another call to PoolForData from original dialog
            // would be invoked, leading to an infinite loop.
            // by retrying we hope (that probably fatal) error would repeat in WaitForData.
            // anyway now once no actual work is done in EventSelectLoop,
            // hardly any exception can occur actually
            Result = qaRetry;
        }
    }
}
//---------------------------------------------------------------------------
class TPoolForDataEvent
{
public:
    TPoolForDataEvent(TSecureShell *SecureShell, WSANETWORKEVENTS &Events) :
        FSecureShell(SecureShell),
        FEvents(Events)
    {
    }

    void PoolForData(size_t &Result)
    {
        FSecureShell->PoolForData(FEvents, Result);
    }

private:
    TSecureShell *FSecureShell;
    WSANETWORKEVENTS &FEvents;
};
//---------------------------------------------------------------------------
void TSecureShell::WaitForData()
{
    // see winsftp.c
    bool IncomingData;

    do
    {
        if (Configuration->GetActualLogProtocol() >= 2)
        {
            LogEvent(L"Looking for incoming data");
        }

        IncomingData = EventSelectLoop(FSessionData->GetTimeout() * 1000, true, NULL);
        if (!IncomingData)
        {
            WSANETWORKEVENTS Events;
            memset(&Events, 0, sizeof(Events));
            TPoolForDataEvent Event(this, Events);

            LogEvent(L"Waiting for data timed out, asking user what to do.");
            queryparamstimer_slot_type slot = boost::bind(&TPoolForDataEvent::PoolForData, &Event, _1);
            int Answer = TimeoutPrompt(&slot);
            switch (Answer)
            {
            case qaRetry:
                // noop
                break;

            case qaOK:
                // read event was already captured in PoolForData(),
                // make sure we do not try to select it again as it would timeout
                // unless another read event occurs
                IncomingData = true;
                HandleNetworkEvents(FSocket, Events);
                break;

            default:
                assert(false);
                // fallthru

            case qaAbort:
                FatalError(LoadStr(USER_TERMINATED));
                break;
            }
        }
    }
    while (!IncomingData);
}
//---------------------------------------------------------------------------
bool TSecureShell::SshFallbackCmd() const
{
    return ssh_fallback_cmd(FBackendHandle) != 0;
}
//---------------------------------------------------------------------------
bool TSecureShell::EnumNetworkEvents(SOCKET Socket, WSANETWORKEVENTS &Events)
{
    if (Configuration->GetActualLogProtocol() >= 2)
    {
        LogEvent(FORMAT(L"Enumerating network events for socket %d", static_cast<int>(Socket)));
    }

    // see winplink.c
    WSANETWORKEVENTS AEvents;
    if (WSAEnumNetworkEvents(Socket, NULL, &AEvents) == 0)
    {
        noise_ultralight(static_cast<unsigned long>(Socket));
        noise_ultralight(AEvents.lNetworkEvents);

        Events.lNetworkEvents |= AEvents.lNetworkEvents;
        for (size_t Index = 0; Index < FD_MAX_EVENTS; Index++)
        {
            if (AEvents.iErrorCode[Index] != 0)
            {
                Events.iErrorCode[Index] = AEvents.iErrorCode[Index];
            }
        }

        if (Configuration->GetActualLogProtocol() >= 2)
        {
            LogEvent(FORMAT(L"Enumerated %d network events making %d cumulative events for socket %d",
                            int(AEvents.lNetworkEvents), int(Events.lNetworkEvents), int(Socket)));
        }
    }
    else
    {
        if (Configuration->GetActualLogProtocol() >= 2)
        {
            LogEvent(FORMAT(L"Error enumerating network events for socket %d", static_cast<int>(Socket)));
        }
    }

    return
        FLAGSET(Events.lNetworkEvents, FD_READ) ||
        FLAGSET(Events.lNetworkEvents, FD_CLOSE);
}
//---------------------------------------------------------------------------
void TSecureShell::HandleNetworkEvents(SOCKET Socket, WSANETWORKEVENTS &Events)
{
    static const struct { int Bit, Mask; const wchar_t *Desc; } EventTypes[] =
    {
        { FD_READ_BIT, FD_READ, L"read" },
        { FD_WRITE_BIT, FD_WRITE, L"write" },
        { FD_OOB_BIT, FD_OOB, L"oob" },
        { FD_ACCEPT_BIT, FD_ACCEPT, L"accept" },
        { FD_CONNECT_BIT, FD_CONNECT, L"connect" },
        { FD_CLOSE_BIT, FD_CLOSE, L"close" },
    };

    for (int Event = 0; Event < LENOF(EventTypes); Event++)
    {
        if (FLAGSET(Events.lNetworkEvents, EventTypes[Event].Mask))
        {
            int Err = Events.iErrorCode[EventTypes[Event].Bit];
            if (Configuration->GetActualLogProtocol() >= 2)
            {
                LogEvent(FORMAT(L"Handling network %s event on socket %d with error %d",
                                EventTypes[Event].Desc, int(Socket), Err));
            }
            // #pragma option push -w-prc
            LPARAM SelectEvent = WSAMAKESELECTREPLY(EventTypes[Event].Mask, Err);
            // #pragma option pop
            if (!select_result(static_cast<WPARAM>(Socket), SelectEvent))
            {
                // note that connection was closed definitely,
                // so "check" is actually not required
                CheckConnection();
            }
        }
    }
}
//---------------------------------------------------------------------------
bool TSecureShell::ProcessNetworkEvents(SOCKET Socket)
{
    WSANETWORKEVENTS Events;
    memset(&Events, 0, sizeof(Events));
    bool Result = EnumNetworkEvents(Socket, Events);
    HandleNetworkEvents(Socket, Events);
    return Result;
}
//---------------------------------------------------------------------------
bool TSecureShell::EventSelectLoop(unsigned int MSec, bool ReadEventRequired,
                                   WSANETWORKEVENTS *Events)
{
    // DEBUG_PRINTF(L"begin, MSec = %u", MSec);
    CheckConnection();

    bool Result = false;

    do
    {
        if (Configuration->GetActualLogProtocol() >= 2)
        {
            // LogEvent(L"Looking for network events");
        }
        unsigned int TicksBefore = GetTickCount();
        int HandleCount;
        // note that this returns all handles, not only the session-related handles
        HANDLE *Handles = handle_get_events(&HandleCount);
        {
            BOOST_SCOPE_EXIT ( (&Handles) )
            {
                sfree(Handles);
            } BOOST_SCOPE_EXIT_END
            Handles = sresize(Handles, static_cast<size_t>(HandleCount + 1), HANDLE);
            Handles[HandleCount] = FSocketEvent;
            unsigned int WaitResult = WaitForMultipleObjects(HandleCount + 1, Handles, FALSE, MSec);
            if (WaitResult < WAIT_OBJECT_0 + HandleCount)
            {
#ifdef MPEXT
                if (handle_got_event(Handles[WaitResult - WAIT_OBJECT_0]))
#else
                handle_got_event(Handles[WaitResult - WAIT_OBJECT_0]);
#endif
                {
                    Result = true;
                }
            }
            else if (WaitResult == WAIT_OBJECT_0 + HandleCount)
            {
                if (Configuration->GetActualLogProtocol() >= 1)
                {
                    LogEvent(L"Detected network event");
                }

                if (Events == NULL)
                {
                    if (ProcessNetworkEvents(FSocket))
                    {
                        Result = true;
                    }
                }
                else
                {
                    if (EnumNetworkEvents(FSocket, *Events))
                    {
                        Result = true;
                    }
                }

                {
                    TSockets::iterator i = FPortFwdSockets.begin();
                    while (i != FPortFwdSockets.end())
                    {
                        ProcessNetworkEvents(*i);
                        i++;
                    }
                }
            }
            else if (WaitResult == WAIT_TIMEOUT)
            {
                if (Configuration->GetActualLogProtocol() >= 2)
                {
                    // LogEvent(L"Timeout waiting for network events");
                }

                MSec = 0;
            }
            else
            {
                if (Configuration->GetActualLogProtocol() >= 2)
                {
                    LogEvent(FORMAT(L"Unknown waiting result %d", static_cast<int>(WaitResult)));
                }

                MSec = 0;
            }
        }

        unsigned int TicksAfter = GetTickCount();
        // ticks wraps once in 49.7 days
        if (TicksBefore < TicksAfter)
        {
            unsigned int Ticks = TicksAfter - TicksBefore;
            if (Ticks > MSec)
            {
                MSec = 0;
            }
            else
            {
                MSec -= Ticks;
            }
        }
    }
    while (ReadEventRequired && (MSec > 0) && !Result);
    // DEBUG_PRINTF(L"end");
    return Result;
}
//---------------------------------------------------------------------------
void TSecureShell::Idle(unsigned int MSec)
{
    noise_regular();

    call_ssh_timer(FBackendHandle);

    EventSelectLoop(MSec, false, NULL);
}
//---------------------------------------------------------------------------
void TSecureShell::KeepAlive()
{
    if (FActive && (FWaiting == 0))
    {
        LogEvent(L"Sending null packet to keep session alive.");
        SendSpecial(TS_PING);
    }
    else
    {
        // defer next keepalive attempt
        FLastDataSent = System::Now();
    }
}
//---------------------------------------------------------------------------
static unsigned int minPacketSize = 0;

unsigned long TSecureShell::MinPacketSize()
{
    if (!FSessionInfoValid)
    {
        UpdateSessionInfo();
    }

    if (FSshVersion == 1)
    {
        return 0;
    }
    else
    {
        if (FMinPacketSize == NULL)
        {
            FMinPacketSize = &minPacketSize;
        }
        return *FMinPacketSize;
    }
}
//---------------------------------------------------------------------------
unsigned long TSecureShell::MaxPacketSize()
{
    if (!FSessionInfoValid)
    {
        UpdateSessionInfo();
    }

    if (FSshVersion == 1)
    {
        return 0;
    }
    else
    {
        if (FMaxPacketSize == NULL)
        {
            FMaxPacketSize = ssh2_remmaxpkt(FBackendHandle);
        }
        return *FMaxPacketSize;
    }
}
//---------------------------------------------------------------------------
std::wstring TSecureShell::FuncToCompression(
    int SshVersion, const void *Compress) const
{
    enum TCompressionType { ctNone, ctZLib };
    if (SshVersion == 1)
    {
        return get_ssh1_compressing(FBackendHandle) ? L"ZLib" : L"";
    }
    else
    {
        return reinterpret_cast<ssh_compress *>(const_cast<void *>(Compress)) == &ssh_zlib ? L"ZLib" : L"";
    }
}
//---------------------------------------------------------------------------
TCipher TSecureShell::FuncToSsh1Cipher(const void *Cipher)
{
    const ssh_cipher *CipherFuncs[] =
    {&ssh_3des, &ssh_des, &ssh_blowfish_ssh1};
    const TCipher TCiphers[] = {cip3DES, cipDES, cipBlowfish};
    assert(LENOF(CipherFuncs) == LENOF(TCiphers));
    TCipher Result = cipWarn;

    for (int Index = 0; Index < LENOF(TCiphers); Index++)
    {
        if (static_cast<ssh_cipher *>(const_cast<void *>(Cipher)) == CipherFuncs[Index])
        {
            Result = TCiphers[Index];
        }
    }

    assert(Result != cipWarn);
    return Result;
}
//---------------------------------------------------------------------------
TCipher TSecureShell::FuncToSsh2Cipher(const void *Cipher)
{
    const ssh2_ciphers *CipherFuncs[] =
    {&ssh2_3des, &ssh2_des, &ssh2_aes, &ssh2_blowfish, &ssh2_arcfour};
    const TCipher TCiphers[] = {cip3DES, cipDES, cipAES, cipBlowfish, cipArcfour};
    assert(LENOF(CipherFuncs) == LENOF(TCiphers));
    TCipher Result = cipWarn;

    for (int C = 0; C < LENOF(TCiphers); C++)
    {
        for (int F = 0; F < CipherFuncs[C]->nciphers; F++)
        {
            if (reinterpret_cast<ssh2_cipher *>(const_cast<void *>(Cipher)) == CipherFuncs[C]->list[F])
            {
                Result = TCiphers[C];
            }
        }
    }

    assert(Result != cipWarn);
    return Result;
}
//---------------------------------------------------------------------------
void TSecureShell::VerifyHostKey(const std::wstring Host, int Port,
                                 const std::wstring KeyType, const std::wstring KeyStr, const std::wstring Fingerprint)
{
    GotHostKey();

    wchar_t Delimiter = L';';
    assert(KeyStr.find_first_of(Delimiter) == std::wstring::npos);

    std::wstring host = Host;
    std::wstring keyStr = KeyStr;
    if (FSessionData->GetTunnel())
    {
        host = FSessionData->GetOrigHostName();
        Port = FSessionData->GetOrigPortNumber();
    }

    FSessionInfo.HostKeyFingerprint = Fingerprint;

    bool Result = false;

    std::wstring Buf = FSessionData->GetHostKey();
    while (!Result && !Buf.empty())
    {
        std::wstring ExpectedKey = CutToChar(Buf, Delimiter, false);
        if (ExpectedKey == Fingerprint)
        {
            Result = true;
        }
    }

    std::wstring StoredKeys;
    if (!Result)
    {
        std::string StoredKeys2(10240, 0);
#ifdef MPEXT
        if (retrieve_host_key(System::W2MB(host.c_str(), FSessionData->GetCodePageAsNumber()).c_str(), Port, System::W2MB(KeyType.c_str(), FSessionData->GetCodePageAsNumber()).c_str(),
                              const_cast<char *>(StoredKeys2.c_str()), static_cast<int>(StoredKeys2.size())) == 0)
#else
        if (verify_host_key(System::W2MB(host.c_str(), FSessionData->GetCodePageAsNumber()).c_str(), Port, System::W2MB(KeyType.c_str(), FSessionData->GetCodePageAsNumber()).c_str(),
                            (char *)StoredKeys2.c_str()))
#endif
        // if (0)
        {
            StoredKeys = System::MB2W(StoredKeys2.c_str(), FSessionData->GetCodePageAsNumber()); // PackStr(StoredKeys);
            std::wstring buf = StoredKeys;
            while (!Result && !buf.empty())
            {
                std::wstring StoredKey = ::CutToChar(buf, Delimiter, false);
                if (StoredKey == keyStr)
                {
                    Result = true;
                }
            }
        }
        else
        {
            StoredKeys = L"";
        }
    }

    if (!Result)
    {
        if (Configuration->GetDisableAcceptingHostKeys())
        {
            FatalError(LoadStr(KEY_NOT_VERIFIED));
        }
        else
        {
            TClipboardHandler ClipboardHandler;
            ClipboardHandler.Text = Fingerprint;

            bool Unknown = StoredKeys.empty();

            int Answers;
            int AliasesCount;
            TQueryButtonAlias Aliases[3];
            Aliases[0].Button = qaRetry;
            Aliases[0].Alias = LoadStr(COPY_KEY_BUTTON);
            Aliases[0].OnClick.connect(boost::bind(&TClipboardHandler::Copy, ClipboardHandler, _1));
            Answers = qaYes | qaCancel | qaRetry;
            AliasesCount = 1;
            if (!Unknown)
            {
                Aliases[1].Button = qaYes;
                Aliases[1].Alias = LoadStr(UPDATE_KEY_BUTTON);
                Aliases[2].Button = qaOK;
                Aliases[2].Alias = LoadStr(ADD_KEY_BUTTON);
                AliasesCount += 2;
                Answers |= qaSkip | qaOK;
            }
            else
            {
                Answers |= qaNo;
            }

            TQueryParams Params;
            Params.NoBatchAnswers = qaYes | qaRetry | qaSkip | qaOK;
            Params.HelpKeyword = (Unknown ? HELP_UNKNOWN_KEY : HELP_DIFFERENT_KEY);
            Params.Aliases = Aliases;
            Params.AliasesCount = AliasesCount;
            int R = FUI->QueryUser(
                        FMTLOAD((Unknown ? UNKNOWN_KEY2 : DIFFERENT_KEY3), KeyType.c_str(), Fingerprint.c_str()),
                        NULL, Answers, &Params, qtWarning);

            switch (R)
            {
            case qaOK:
                assert(!Unknown);
                keyStr = StoredKeys + Delimiter + keyStr;
                // fall thru
            case qaYes:
                store_host_key(System::W2MB(host.c_str(), FSessionData->GetCodePageAsNumber()).c_str(), Port, System::W2MB(KeyType.c_str(), FSessionData->GetCodePageAsNumber()).c_str(),
                               const_cast<char *>(System::W2MB(keyStr.c_str(), FSessionData->GetCodePageAsNumber()).c_str()));
                break;

            case qaCancel:
                FatalError(LoadStr(KEY_NOT_VERIFIED));
            }
        }
    }
}
//---------------------------------------------------------------------------
void TSecureShell::AskAlg(const std::wstring AlgType,
                          const std::wstring AlgName)
{
    std::wstring Msg;
    if (AlgType == L"key-exchange algorithm")
    {
        Msg = FMTLOAD(KEX_BELOW_TRESHOLD, AlgName.c_str());
    }
    else
    {
        int CipherType = 0;
        if (AlgType == L"cipher")
        {
            CipherType = CIPHER_TYPE_BOTH;
        }
        else if (AlgType == L"client-to-server cipher")
        {
            CipherType = CIPHER_TYPE_CS;
        }
        else if (AlgType == L"server-to-client cipher")
        {
            CipherType = CIPHER_TYPE_SC;
        }
        else
        {
            assert(false);
        }

        Msg = FMTLOAD(CIPHER_BELOW_TRESHOLD, LoadStr(CipherType).c_str(), AlgName.c_str());
    }

    if (FUI->QueryUser(Msg, NULL, qaYes | qaNo, NULL, qtWarning) == qaNo)
    {
        System::Abort();
    }
}
//---------------------------------------------------------------------------
void TSecureShell::DisplayBanner(const std::wstring Banner)
{
    FUI->DisplayBanner(Banner);
}
//---------------------------------------------------------------------------
void TSecureShell::OldKeyfileWarning()
{
    // actually never called, see Net.cpp
    FUI->QueryUser(LoadStr(OLD_KEY), NULL, qaOK, NULL, qtWarning);
}
//---------------------------------------------------------------------------
bool TSecureShell::GetStoredCredentialsTried()
{
    return FStoredPasswordTried || FStoredPasswordTriedForKI;
}
//---------------------------------------------------------------------------
bool TSecureShell::GetReady()
{
    return FOpened && (FWaiting == 0);
}
