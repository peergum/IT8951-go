/*
   it8951,
   Copyright (C) 2024  Phil Hilger

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

package it8951

type Color uint16

func Bpp(bpp int) PixelMode {
	switch bpp {
	case 2:
		return BPP2
	case 3:
		return BPP3
	case 4:
		return BPP4
	case 1:
	case 8:
		return BPP8
	}
	return BPP8
}
