// FileZilla - a Windows ftp client

// Copyright (C) 2002-2004 - Tim Kosse <tim.kosse@gmx.de>

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#if !defined(FZAPISTRUCTURES__INCLUDED)
#define FZAPISTRUCTURES__INCLUDED

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class t_server
{
public:
	t_server();
	~t_server();
	CString host;
	int port;
	CString user, pass, account;
	BOOL fwbypass;
	CString path;
	BOOL bDontRememberPass;
	int nServerType;
	CString name; //Name of server, used in SiteManager
	int nPasv;
	int nTimeZoneOffset;
	int nUTF8;
#ifdef MPEXT
	BOOL bForcePasvIp;
#endif
	bool operator<(const t_server &op) const; //Needed by STL map
};

const bool operator == (const t_server &a,const t_server &b);
const bool operator != (const t_server &a,const t_server &b);

#include "ServerPath.h"

typedef struct
{
    CString localfile;
    CString remotefile;
    CServerPath remotepath;
    BOOL get;
    __int64 size;
    t_server server;
    int nType;
    void *UserData;
} t_transferfile;

/* information stored about one single SSL session */
typedef struct
{
	char *name;       /* host name for which this ID was used */
	void *sessionid;  /* as returned from the SSL layer */
	long age;         /* just a number, the higher the more recent */
	unsigned short remote_port; /* remote port to connect to */
} ssl_session_info_t;

#ifdef MPEXT

//---------------------------------------------------------------------------
enum ftp_capabilities_t
{
    unknown,
    yes,
    no
};
//---------------------------------------------------------------------------
enum ftp_capability_names_t
{
    syst_command = 1, // reply of SYST command as option
    feat_command,
    clnt_command, // set to 'yes' if CLNT should be sent
    utf8_command, // set to 'yes' if OPTS UTF8 ON should be sent
    mlsd_command,
    opts_mlst_command, // Arguments for OPTS MLST command
    mfmt_command,
    pret_command,
    mdtm_command,
    size_command,
    mode_z_support,
    tvfs_support, // Trivial virtual file store (RFC 3659)
    list_hidden_support, // LIST -a command
    rest_stream, // supports REST+STOR in addition to APPE
};
#endif

#endif
