/**************************************************************************
 *  NetBox plugin for FAR 2.0 (http://code.google.com/p/farplugs)         *
 *  Copyright (C) 2011 by Artem Senichev <artemsen@gmail.com>             *
 *  Copyright (C) 2011 by Michael Lukashov <michael.lukashov@gmail.com>   *
 *                                                                        *
 *  This program is free software: you can redistribute it and/or modify  *
 *  it under the terms of the GNU General Public License as published by  *
 *  the Free Software Foundation, either version 3 of the License, or     *
 *  (at your option) any later version.                                   *
 *                                                                        *
 *  This program is distributed in the hope that it will be useful,       *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *  GNU General Public License for more details.                          *
 *                                                                        *
 *  You should have received a copy of the GNU General Public License     *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 **************************************************************************/

#pragma once

#include "FTP.h"


/**
 * FTPS saved session
 */
class CSessionFTPS : public CSessionFTP
{
protected:
    //From CSession
    virtual PSessionEditor CreateEditorInstance();
    virtual PProtocol CreateClientInstance() const;
};


/**
 * FTPS session editor dialog
 */
class CSessionEditorFTPS : public CSessionEditorFTP
{
public:
    explicit CSessionEditorFTPS(CSession *session) :
        CSessionEditorFTP(session)
    {}
};


/**
 * FTPS client implementation
 */
class CFTPS : public CFTP
{
public:
    explicit CFTPS(const CSession *session);
    ~CFTPS();

protected:
    virtual CURLcode CURLPrepare(const char *ftpPath, const bool handleTimeout = true);

private:
};
