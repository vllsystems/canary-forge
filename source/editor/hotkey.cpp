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

#include "app/main.h"

#include "editor/hotkey.h"
#include "brushes/brush.h"

Hotkey::Hotkey() :
	type(NONE) {
	////
}

Hotkey::Hotkey(Position _pos) :
	type(POSITION), pos(_pos) {
	////
}

Hotkey::Hotkey(Brush* brush) :
	type(BRUSH), brushname(brush->getName()) {
	////
}

Hotkey::Hotkey(std::string _name) :
	type(BRUSH), brushname(_name) {
	////
}

Hotkey::~Hotkey() {
	////
}

Position Hotkey::GetPosition() const {
	ASSERT(IsPosition());
	return pos;
}

std::string Hotkey::GetBrushname() const {
	ASSERT(IsBrush());
	return brushname;
}

std::ostream &operator<<(std::ostream &os, const Hotkey &hotkey) {
	switch (hotkey.type) {
		case Hotkey::POSITION: {
			os << "pos:{" << hotkey.pos << "}";
		} break;
		case Hotkey::BRUSH: {
			if (hotkey.brushname.find('{') != std::string::npos || hotkey.brushname.find('}') != std::string::npos) {
				break;
			}
			os << "brush:{" << hotkey.brushname << "}";
		} break;
		default: {
			os << "none:{}";
		} break;
	}
	return os;
}

std::istream &operator>>(std::istream &is, Hotkey &hotkey) {
	std::string type;
	getline(is, type, ':');
	if (type == "none") {
		is.ignore(2); // ignore "{}"
	} else if (type == "pos") {
		is.ignore(1); // ignore "{"
		Position pos;
		is >> pos;
		hotkey = Hotkey(pos);
		is.ignore(1); // ignore "}"
	} else if (type == "brush") {
		is.ignore(1); // ignore "{"
		std::string brushname;
		getline(is, brushname, '}');
		hotkey = Hotkey(brushname);
	} else {
		// Do nothing...
	}

	return is;
}
