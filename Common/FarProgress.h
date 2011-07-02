/**************************************************************************
 *  Far plugin helper (for FAR 2.0) (http://code.google.com/p/farplugs)   *
 *  Copyright (C) 2000-2011 by Artem Senichev <artemsen@gmail.com>        *
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

#include "FarPlugin.h"
#include "FarDialog.h"

/**
 * Progress window
 */
class CFarProgress
{
public:
	/**
	 * Constructor
	 * \param minValue minimal value
	 * \param maxValue maximal value
	 */
	CFarProgress(const __int64 minValue, const __int64 maxValue)
		: _MinValue(minValue), _MaxValue(maxValue), _CurrValue(maxValue > minValue ? minValue : maxValue)
	{
		CFarPlugin::AdvControl(ACTL_SETPROGRESSSTATE, reinterpret_cast<void*>(PS_INDETERMINATE));
	}

	~CFarProgress()
	{
		Close();
	}

	/**
	 * Show progress window
	 * \param newValue new value
	 * \return true if Esc key was pressed
	 */
	bool Show(const __int64 /*newValue*/)
	{
		//assert(newValue >= (_MaxValue < _MinValue ? _MaxValue : _MinValue) && newValue <= (_MaxValue > _MinValue ? _MaxValue : _MinValue));

// 		if (_CurrValue != newValue) {
// 			_CurrValue = newValue;
// 
// 			const size_t percent = static_cast<size_t>(((_CurrValue - _MinValue) * 100) / (_MaxValue - _MinValue));
// 			wstring fillBuff(percent * 32 /*progress bar length*/ / 100, L'\x2588');
// 			CFarPlugin::GetPSI()->Text(_WndPosCol + 4 , _WndPosRow + 3, 0x70, fillBuff.c_str());
// 			CFarPlugin::GetPSI()->Text(0, 0, 0, NULL);	 //Reset screen buffer
// 
// 			PROGRESSVALUE pv;
// 			pv.Completed = percent;
// 			pv.Total = 100;
// 			CFarPlugin::AdvControl(ACTL_SETPROGRESSVALUE, &pv);
// 		}

		return IsEscPressed();
	}

	void Close()
	{
		CFarPlugin::AdvControl(ACTL_PROGRESSNOTIFY, 0);
		CFarPlugin::AdvControl(ACTL_SETPROGRESSSTATE, reinterpret_cast<void*>(PS_NOPROGRESS));
	}

	/**
	 * Check for Esc key event
	 * \return true if Esc key was pressed
	 */
	static bool IsEscPressed()
	{
		static HANDLE stdIn = GetStdHandle(STD_INPUT_HANDLE);
		INPUT_RECORD rec;
		DWORD readCount = 0;
		while (PeekConsoleInput(stdIn, &rec, 1, &readCount) && readCount != 0) {
			ReadConsoleInput(stdIn, &rec, 1, &readCount);
			if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE && rec.Event.KeyEvent.bKeyDown)
				return true;
		}
		return false;
	}

private:
	__int64	_MinValue;
	__int64	_MaxValue;
	__int64	_CurrValue;
	const wchar_t*	_Title;
	int	_WndPosCol;
	int	_WndPosRow;
};
