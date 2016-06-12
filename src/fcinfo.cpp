/*
    fcinfo.cpp


    FLEXplorer, An explorer for any FLEX file or disk container
    Copyright (C) 1998-2004  W. Schwotzer

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdio.h>
#include <string.h>

#include "fcinfo.h"
#include "filecntb.h"	// needed for containertypes


FlexContainerInfo::FlexContainerInfo(void) :
	sectors(0),
	tracks(0),
	type(0),
	free(0),
	totalSize(0),
	attributes(0)
{
	name[0] = '\0';
} // FlexContainerInfo

FlexContainerInfo::~FlexContainerInfo(void)
{
}

void FlexContainerInfo::SetName(const char *n)
{
	strncpy(name, n, FLEX_DISKNAME_LENGTH-1);
	name[FLEX_DISKNAME_LENGTH-1] = '\0';
}

const BString FlexContainerInfo::GetTypeString(void) const
{
	BString str;

	if (type & TYPE_CONTAINER) {
		if (type & TYPE_DSK_CONTAINER)
			str = "file container, DSK format";
		else if (type & TYPE_FLX_CONTAINER)
			str = "file container, FLX format";
		else
			str = "file container";
	} else if (type & TYPE_DIRECTORY) {
		if (type & TYPE_NAFS_DIRECTORY)
			str = "directory, without text file conversion";
		else
			str = "directory, with text file conversion";
	} else
		str = "Unknown type";
	return str;
}
