//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#ifndef RME_HOTKEY_H
#define RME_HOTKEY_H

#include "util/position.h"
#include <string>
#include <iostream>

class Brush;

class Hotkey {
public:
	Hotkey();
	Hotkey(Position pos);
	Hotkey(Brush* brush);
	Hotkey(std::string _brushname);
	~Hotkey();

	bool IsPosition() const {
		return type == POSITION;
	}
	bool IsBrush() const {
		return type == BRUSH;
	}
	Position GetPosition() const;
	std::string GetBrushname() const;

private:
	enum {
		NONE,
		POSITION,
		BRUSH,
	} type;

	Position pos;
	std::string brushname;

	friend std::ostream &operator<<(std::ostream &os, const Hotkey &hotkey);
	friend std::istream &operator>>(std::istream &os, Hotkey &hotkey);
};

std::ostream &operator<<(std::ostream &os, const Hotkey &hotkey);
std::istream &operator>>(std::istream &os, Hotkey &hotkey);

#endif
