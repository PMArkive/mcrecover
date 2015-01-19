/***************************************************************************
 * GameCube Memory Card Recovery Program.                                  *
 * BitFlags.hpp: Generic bit flags base class.                             *
 *                                                                         *
 * Copyright (c) 2015 by David Korth.                                      *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "BitFlags.hpp"
#include "BitFlags_p.hpp"

// C includes. (C++ namespace)
#include <cassert>

// TODO: Put this in a common header file somewhere.
#define NUM_ELEMENTS(x) ((int)(sizeof(x) / sizeof(x[0])))

/** BitFlagsPrivate **/

BitFlagsPrivate::BitFlagsPrivate(int total_flags, const bit_flag_t *bit_flags, int count)
{
	// This is initialized by a derived private class.
	assert(total_flags > 0);
	assert(total_flags >= count);
	assert(count >= 0);

	// Initialize flags.
	// QVector automatically initializes the new elements to false.
	flags.resize(total_flags);

	// Initialize flags_desc.
	// TODO: Once per derived class, rather than once per instance?
	flags_desc.clear();
	flags_desc.reserve(count);
	for (int i = 0; i < count; i++, bit_flags++) {
		if (bit_flags->event < 0 || !bit_flags->description) {
			// End of list.
			// NOTE: count should have been set correctly...
			break;
		}

		flags_desc.insert(bit_flags->event, QLatin1String(bit_flags->description));
	}
}

BitFlagsPrivate::~BitFlagsPrivate()
{ }

/** BitFlags **/

BitFlags::BitFlags(BitFlagsPrivate *d, QObject *parent)
	: QObject(parent)
	, d_ptr(d)
{ }

BitFlags::~BitFlags()
{
	Q_D(BitFlags);
	delete d;
}

/**
 * Get the total number of flags.
 * @return Total number of flags.
 */
int BitFlags::count(void) const
{
	Q_D(const BitFlags);
	return d->flags.size();
}

/**
 * Get a flag's description.
 * @param flag Flag ID.
 * @return Description.
 */
QString BitFlags::description(int flag) const
{
	// TODO: Translate using the subclass?
	if (flag < 0 || flag >= count())
		return tr("Invalid flag ID");

	Q_D(const BitFlags);
	return d->flags_desc.value(flag, tr("Unknown"));
}

/**
 * Is a flag set?
 * @param flag Flag ID.
 * @return True if set; false if not.
 */
bool BitFlags::flag(int flag) const
{
	if (flag < 0 || flag >= count())
		return false;

	Q_D(const BitFlags);
	return d->flags.at(flag);
}

/**
 * Set a flag.
 * @param flag Flag ID.
 * @param value New flag value.
 */
void BitFlags::setFlag(int flag, bool value)
{
	if (flag < 0 || flag >= count())
		return;

	Q_D(BitFlags);
	d->flags[flag] = value;
	// TODO: Emit a signal?
}