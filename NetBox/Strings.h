/**************************************************************************
 *  NetBox plugin for FAR 2.0 (http://code.google.com/p/farplugs)         *
 *  Copyright (C) 2011 by Artem Senichev <artemsen@gmail.com>             *
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


//! Plugin strings
enum PluginString {
	StringTitle = 0,

	StringOK,
	StringCancel,

	//Edit link dialog
	StringEdCrtTitle,
	StringEdEdtTitle,
	StringEdName,
	StringEdURL,
	StringEdCP,
	StringEdAuth,
	StringEdAuthUser,
	StringEdAuthPsw,
	StringEdAuthPromtpPsw,
	StringEdAuthShowPsw,
	StringEdAuthCert,
	StringEdErrURLEmpty,
	StringEdErrURLInvalid,
	StringEdErrNameEmpty,
	StringEdErrNameInvalid,

	//Configure dialog
	StringCfgAddToPM,
	StringCfgAddToDM,
	StringCfgUseOwnKey,
	StringCfgPrefix,
	StringCfgAltPrefixes,
	StringCfgTimeout,
	StringCfgSessionsPath,

	//Prompt to crypto key
	StringSessionPwd,

	//Create directory dialog
	StringMKDirTitle,
	StringMKDirName,

	//Delete items dialog
	StringDelTitle,
	StringDelQuestion,
	StringDelQuestSession,
	StringDelQuestFolder,
	StringDelQuestFile,
	StringDelSelected,
	StringDelBtnDelete,

	//Copy dialog
	StringCopyTitle,
	StringCopyPath,
	StringCopySelected,
	StringCopyBtnCopy,

	//Move dialog
	StringMoveTitle,
	StringMovePath,
	StringMoveSelected,
	StringMoveBtnCopy,

	//Progress titles
	StringPrgConnect,
	StringPrgChangeDir,
	StringPrgGetList,
	StringPrgRcvFile,
	StringPrgSendFile,
	StringPrgTo,
	StringPrgDelete,

	//Error messages
	StringErrKeyFile,
	StringErrEstablish,
	StringErrCreateDir,
	StringErrChangeDir,
	StringErrListDir,
	StringErrCopyFile,	
	StringErrRenameMove,	
	StringErrDeleteFile,
	StringErrDeleteDir,
};
