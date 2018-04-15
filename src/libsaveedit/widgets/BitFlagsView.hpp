/***************************************************************************
 * GameCube Memory Card Recovery Program [libsaveedit]                     *
 * BitFlagsView.hpp: Bit Flags editor.                                     *
 *                                                                         *
 * Copyright (c) 2015-2018 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#ifndef __LIBSAVEEDIT_WIDGETS_BITFLAGSVIEW_HPP__
#define __LIBSAVEEDIT_WIDGETS_BITFLAGSVIEW_HPP__

#include <QWidget>

class BitFlagsModel;

class BitFlagsViewPrivate;
class BitFlagsView : public QWidget
{
	Q_OBJECT

	Q_PROPERTY(BitFlagsModel* bitFlagsModel READ bitFlagsModel WRITE setBitFlagsModel)
	Q_PROPERTY(int pageSize READ pageSize)

	public:
		explicit BitFlagsView(QWidget *parent = 0);
		~BitFlagsView();

	private:
		typedef QWidget super;
		BitFlagsViewPrivate *const d_ptr;
		Q_DECLARE_PRIVATE(BitFlagsView)
		Q_DISABLE_COPY(BitFlagsView)

	protected:
		// State change event. (Used for switching the UI language at runtime.)
		void changeEvent(QEvent *event) final;

	public:
		/** Model access. **/

		/**
		 * Get the BitFlagsModel this widget is editing.
		 * @return BitFlagsModel.
		 */
		BitFlagsModel *bitFlagsModel(void) const;

		/**
		 * Set the BitFlagsModel to edit.
		 * @param bitFlagsModel BitFlagsModel.
		 */
		void setBitFlagsModel(BitFlagsModel *model);

		/** Data access. **/

		/**
		 * Get the page size.
		 * @return Page size.
		 */
		int pageSize(void) const;

		// TODO: Page count?
};

#endif /* __LIBSAVEEDIT_WIDGETS_BITFLAGSVIEW_HPP__ */