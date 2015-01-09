/***************************************************************************
 * GameCube Memory Card Recovery Program.                                  *
 * MemCardModel.cpp: QAbstractListModel for GcnCard.                       *
 *                                                                         *
 * Copyright (c) 2012-2015 by David Korth.                                 *
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

#include "MemCardModel.hpp"
#include "McRecoverQApplication.hpp"

// GcnCard classes.
#include "GcnCard.hpp"
#include "GcnFile.hpp"
#include "McRecoverQApplication.hpp"

// Icon animation helper.
#include "IconAnimHelper.hpp"

// C includes.
#include <limits.h>

// C++ includes.
#include <bitset>

// Qt includes.
#include <QtCore/QHash>
#include <QtCore/QTimer>
#include <QtGui/QApplication>
#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtGui/QPalette>
#include <QtGui/QStyle>


/** MemCardModelPrivate **/

class MemCardModelPrivate
{
	public:
		MemCardModelPrivate(MemCardModel *q);
		~MemCardModelPrivate();

	protected:
		MemCardModel *const q_ptr;
		Q_DECLARE_PUBLIC(MemCardModel)
	private:
		Q_DISABLE_COPY(MemCardModelPrivate)

	public:
		GcnCard *card;

		QHash<const GcnFile*, IconAnimHelper*> animState;

		/**
		 * Initialize the animation state for all files.
		 */
		void initAnimState(void);

		/**
		 * Initialize the animation state for a given file.
		 * @param file GcnFile.
		 */
		void initAnimState(const GcnFile *file);

		/**
		 * Update the animation timer state.
		 * Starts the timer if animated icons are present; stops the timer if not.
		 */
		void updateAnimTimerState(void);

		// Animation timer.
		QTimer animTimer;
		// Pause count. If >0, animation is paused.
		int pauseCounter;
		// Animation timer "slot".
		void animTimerSlot(void);

		// Style variables.
		struct style_t {
			/**
			 * Initialize the style variables.
			 */
			void init(void);

			// Background colors for "lost" files.
			QBrush brush_lostFile;
			QBrush brush_lostFile_alt;

			// Pixmaps for COL_ISVALID.
			static const int pxmIsValid_width = 16;
			static const int pxmIsValid_height = 16;
			QPixmap pxmIsValid_unknown;
			QPixmap pxmIsValid_invalid;
			QPixmap pxmIsValid_good;
		};
		style_t style;

		/**
		 * Cached copy of card->numFiles().
		 * This value is needed after the card is destroyed,
		 * so we need to cache it here, since the destroyed()
		 * slot might be run *after* the GcnCard is deleted.
		 */
		int numFiles;

		// Row insert start/end indexes.
		int insertStart;
		int insertEnd;
};

MemCardModelPrivate::MemCardModelPrivate(MemCardModel *q)
	: q_ptr(q)
	, card(nullptr)
	, animTimer(new QTimer(q))
	, pauseCounter(0)
	, numFiles(0)
	, insertStart(-1)
	, insertEnd(-1)
{
	// Connect animTimer's timeout() signal.
	QObject::connect(&animTimer, SIGNAL(timeout()),
			 q, SLOT(animTimerSlot()));

	// Initialize the style variables.
	style.init();
}

/**
 * Initialize the style variables.
 */
void MemCardModelPrivate::style_t::init(void)
{
	// TODO: Call this function if the UI style changes.

	// Initialize the background colors for "lost" files.
	QPalette pal = QApplication::palette("QTreeView");
	QColor bgColor_lostFile = pal.base().color();
	QColor bgColor_lostFile_alt = pal.alternateBase().color();

	// Adjust the colors to have a yellow hue.
	int h, s, v;

	// "Lost" file. (Main)
	bgColor_lostFile.getHsv(&h, &s, &v, nullptr);
	h = 60;
	s = (255 - s);
	bgColor_lostFile.setHsv(h, s, v);

	// "Lost" file. (Alternate)
	bgColor_lostFile_alt.getHsv(&h, &s, &v, nullptr);
	h = 60;
	s = (255 - s);
	bgColor_lostFile_alt.setHsv(h, s, v);

	// Save the background colors in QBrush objects.
	brush_lostFile = QBrush(bgColor_lostFile);
	brush_lostFile_alt = QBrush(bgColor_lostFile_alt);

	// Initialize the COL_ISVALID pixmaps.
	pxmIsValid_unknown = McRecoverQApplication::StandardIcon(QStyle::SP_MessageBoxQuestion)
				.pixmap(pxmIsValid_width, pxmIsValid_height);
	pxmIsValid_invalid = McRecoverQApplication::StandardIcon(QStyle::SP_MessageBoxCritical)
				.pixmap(pxmIsValid_width, pxmIsValid_height);
	pxmIsValid_good    = McRecoverQApplication::StandardIcon(QStyle::SP_DialogApplyButton)
				.pixmap(pxmIsValid_width, pxmIsValid_height);
}

MemCardModelPrivate::~MemCardModelPrivate()
{
	animTimer.stop();

	// TODO: Check for race conditions.
	qDeleteAll(animState);
	animState.clear();
}

/**
 * Initialize the animation state.
 */
void MemCardModelPrivate::initAnimState(void)
{
	animTimer.stop();

	// TODO: Check for race conditions.
	qDeleteAll(animState);
	animState.clear();

	if (!card)
		return;

	// Initialize the animation state.
	for (int i = 0; i < numFiles; i++) {
		const GcnFile *file = card->getFile(i);
		initAnimState(file);
	}

	// Start the timer if animated icons are present.
	updateAnimTimerState();
}

/**
 * Initialize the animation state for a given file.
 * @param file GcnFile.
 */
void MemCardModelPrivate::initAnimState(const GcnFile *file)
{
	int numIcons = file->numIcons();
	if (numIcons <= 1) {
		// Not an animated icon.
		animState.remove(file);
		return;
	}

	IconAnimHelper *helper = new IconAnimHelper(file);
	animState.insert(file, helper);
}

/**
 * Update the animation timer state.
 * Starts the timer if animated icons are present; stops the timer if not.
 */
void MemCardModelPrivate::updateAnimTimerState(void)
{
	if (pauseCounter <= 0 && !animState.isEmpty()) {
		// Animation is not paused, and we have animated icons.
		// Start the timer.
		animTimer.start(IconAnimHelper::FAST_ANIM_TIMER);
	} else {
		// Either animation is paused, or we don't have animated icons.
		// Stop the timer.
		animTimer.stop();
	}
}

/**
 * Animation timer "slot".
 */
void MemCardModelPrivate::animTimerSlot(void)
{
	if (!card) {
		animTimer.stop();
		return;
	}

	// Check for icon animations.
	Q_Q(MemCardModel);
	for (int i = 0; i < numFiles; i++) {
		const GcnFile *file = card->getFile(i);
		IconAnimHelper *helper = animState.value(file);
		if (!helper)
			continue;

		// Tell the IconAnimHelper that a timer tick has occurred.
		// TODO: Connect the timer to the IconAnimHelper directly?
		bool iconUpdated = helper->tick();
		if (iconUpdated) {
			// Icon has been updated.
			// Notify the UI that the icon has changed.
			QModelIndex iconIndex = q->createIndex(i, MemCardModel::COL_ICON, 0);
			emit q->dataChanged(iconIndex, iconIndex);
		}
	}
}

/** MemCardModel **/

MemCardModel::MemCardModel(QObject *parent)
	: QAbstractListModel(parent)
	, d_ptr(new MemCardModelPrivate(this))
{
	// Connect the "themeChanged" signal.
	McRecoverQApplication *mcrqa = qobject_cast<McRecoverQApplication*>(McRecoverQApplication::instance());
	if (mcrqa) {
		connect(mcrqa, SIGNAL(themeChanged()),
			this, SLOT(themeChanged_slot()));
	}
}

MemCardModel::~MemCardModel()
{
	Q_D(MemCardModel);
	delete d;
}


int MemCardModel::rowCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent);
	Q_D(const MemCardModel);
	return (d->numFiles);
}

int MemCardModel::columnCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent);
	return COL_MAX;
}

QVariant MemCardModel::data(const QModelIndex& index, int role) const
{
	Q_D(const MemCardModel);
	if (!d->card || !index.isValid())
		return QVariant();
	if (index.row() >= rowCount())
		return QVariant();

	// Get the memory card file.
	const GcnFile *file = d->card->getFile(index.row());

	// TODO: Move some of this to MemCardItemDelegate?
	switch (role) {
		case Qt::DisplayRole:
			switch (index.column()) {
				case COL_DESCRIPTION:
					// Concatenate the two descriptions together.
					// Result: "GameDesc\0FileDesc"
					return QString(file->gameDesc() + QChar(L'\0') + file->fileDesc());
				case COL_SIZE:
					return file->size();
				case COL_MTIME:
					return file->lastModified().qDateTime();
				case COL_PERMISSION:
					return file->permissionAsString();
				case COL_GAMEID:
					return file->id6();
				case COL_FILENAME:
					return file->filename();
				default:
					break;
			}
			break;

		case Qt::DecorationRole:
			// Images must use Qt::DecorationRole.
			switch (index.column()) {
				case COL_ICON:
					// Check if this is an animated icon.
					if (d->animState.contains(file)) {
						// Animated icon.
						IconAnimHelper *helper = d->animState.value(file);
						return helper->icon();
					} else {
						// Not an animated icon.
						// Return the first icon.
						return file->icon(0);
					}

				case COL_BANNER:
					return file->banner();

				case COL_ISVALID:
					if (!file->isLostFile()) {
						// Regular files aren't checked right now.
						break;
					}

					switch (file->checksumStatus()) {
						default:
						case Checksum::CHKST_UNKNOWN:
							return d->style.pxmIsValid_unknown;
						case Checksum::CHKST_INVALID:
							return d->style.pxmIsValid_invalid;
						case Checksum::CHKST_GOOD:
							return d->style.pxmIsValid_good;
					}

				default:
					break;
			}
			break;

		case Qt::TextAlignmentRole:
			switch (index.column()) {
				case COL_SIZE:
				case COL_PERMISSION:
				case COL_GAMEID:
				case COL_ISVALID:
					// These columns should be center-aligned horizontally.
					return (int)(Qt::AlignHCenter | Qt::AlignVCenter);

				default:
					// Everything should be center-aligned vertically.
					return Qt::AlignVCenter;
			}
			break;

		case Qt::FontRole:
			switch (index.column()) {
				case COL_SIZE:
				case COL_PERMISSION:
				case COL_GAMEID: {
					// These columns should be monospaced.
					QFont fntMonospace(QLatin1String("Monospace"));
					fntMonospace.setStyleHint(QFont::TypeWriter);
					return fntMonospace;
				}

				default:
					break;
			}
			break;

		case Qt::BackgroundRole:
			// "Lost" files should be displayed using a different color.
			if (file->isLostFile()) {
				// TODO: Check if the item view is using alternating row colors before using them.
				if (index.row() & 1)
					return d->style.brush_lostFile_alt;
				else
					return d->style.brush_lostFile;
			}
			break;

		case Qt::SizeHintRole: {
			// Increase row height by 4px.
			// HACK: Increase icon/banner width on Windows.
			// Figure out a better method later.
		#ifdef Q_OS_WIN
			static const int iconWadj = 8;
		#else
			static const int iconWadj = 0;
		#endif
			switch (index.column()) {
				case COL_ICON:
					return QSize(CARD_ICON_W + iconWadj, (CARD_ICON_H + 4));
				case COL_BANNER:
					return QSize(CARD_BANNER_W + iconWadj, (CARD_BANNER_H + 4));
				case COL_ISVALID:
					return QSize(d->style.pxmIsValid_width,
						     (d->style.pxmIsValid_height + 4));
				default:
					break;
			}
			break;
		}

		default:
			break;
	}

	// Default value.
	return QVariant();
}

QVariant MemCardModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	Q_UNUSED(orientation);

	switch (role) {
		case Qt::DisplayRole:
			switch (section) {
				case COL_ICON:		return tr("Icon");
				case COL_BANNER:	return tr("Banner");
				case COL_DESCRIPTION:	return tr("Description");
				case COL_SIZE:		return tr("Size");
				case COL_MTIME:		return tr("Last Modified");
				//: File permissions. (Known as "mode" on Unix systems.)
				case COL_PERMISSION:	return tr("Mode");
				//: 6-digit game ID, e.g. GALE01.
				case COL_GAMEID:	return tr("Game ID");
				case COL_FILENAME:	return tr("Filename");

				// NOTE: Don't use a column header for COL_ISVALID.
				// Otherwise, the column will be too wide,
				// and the icon won't be aligned correctly.
				//case COL_ISVALID:	return tr("Valid?");

				default:
					break;
			}
			break;

		case Qt::TextAlignmentRole:
			switch (section) {
				case COL_ICON:
				case COL_SIZE:
				case COL_PERMISSION:
				case COL_GAMEID:
				case COL_ISVALID:
					// Center-align the text.
					return Qt::AlignHCenter;

				default:
					break;
			}
			break;
	}

	// Default value.
	return QVariant();
}

/**
 * Set the memory card to use in this model.
 * @param card Memory card.
 */
void MemCardModel::setMemCard(GcnCard *card)
{
	Q_D(MemCardModel);

	// Disconnect the GcnCard's changed() signal if a GcnCard is already set.
	if (d->card) {
		// Notify the view that we're about to remove all rows.
		d->numFiles = d->card->numFiles();
		if (d->numFiles > 0)
			beginRemoveRows(QModelIndex(), 0, (d->numFiles - 1));

		// Disconnect the GcnCard's signals.
		disconnect(d->card, SIGNAL(destroyed(QObject*)),
			   this, SLOT(memCard_destroyed_slot(QObject*)));
		disconnect(d->card, SIGNAL(filesAboutToBeInserted(int,int)),
			   this, SLOT(memCard_filesAboutToBeInserted_slot(int,int)));
		disconnect(d->card, SIGNAL(filesInserted()),
			   this, SLOT(memCard_filesInserted_slot()));
		disconnect(d->card, SIGNAL(filesAboutToBeRemoved(int,int)),
			   this, SLOT(memCard_filesAboutToBeRemoved_slot(int,int)));
		disconnect(d->card, SIGNAL(filesRemoved()),
			   this, SLOT(memCard_filesRemoved_slot()));

		d->card = nullptr;

		// Done removing rows.
		if (d->numFiles > 0)
			endRemoveRows();
	}

	if (card) {
		// Notify the view that we're about to add rows.
		d->numFiles = card->numFiles();
		if (d->numFiles > 0)
			beginInsertRows(QModelIndex(), 0, (d->numFiles - 1));

		// Set the card.
		d->card = card;

		// Initialize the animation state.
		d->initAnimState();

		// Connect the GcnCard's signals.
		connect(d->card, SIGNAL(destroyed(QObject*)),
			this, SLOT(memCard_destroyed_slot(QObject*)));
		connect(d->card, SIGNAL(filesAboutToBeInserted(int,int)),
			this, SLOT(memCard_filesAboutToBeInserted_slot(int,int)));
		connect(d->card, SIGNAL(filesInserted()),
			this, SLOT(memCard_filesInserted_slot()));
		connect(d->card, SIGNAL(filesAboutToBeRemoved(int,int)),
			this, SLOT(memCard_filesAboutToBeRemoved_slot(int,int)));
		connect(d->card, SIGNAL(filesRemoved()),
			this, SLOT(memCard_filesRemoved_slot()));

		// Done adding rows.
		if (d->numFiles > 0)
			endInsertRows();
	}
}

/** Public slots. **/

/**
 * Pause animation.
 * Should be used if e.g. the window is minimized.
 * NOTE: This uses an internal counter; the number of resumes
 * must match the number of pauses to resume animation.
 */
void MemCardModel::pauseAnimation(void)
{
	Q_D(MemCardModel);
	d->pauseCounter++;
	d->updateAnimTimerState();
}

/**
 * Resume animation.
 * Should be used if e.g. the window is un-minimized.
 * NOTE: This uses an internal counter; the number of resumes
 * must match the number of pauses to resume animation.
 */
void MemCardModel::resumeAnimation(void)
{
	Q_D(MemCardModel);
	if (d->pauseCounter > 0) {
		d->pauseCounter--;
	} else {
		// Not paused...
		d->pauseCounter = 0; // TODO: Probably not needed.
	}

	d->updateAnimTimerState();
}

/** Private slots. **/

/**
 * Animation timer slot.
 * Wrapper for MemCardModelPrivate::animTimerSlot().
 */
void MemCardModel::animTimerSlot(void)
{
	Q_D(MemCardModel);
	d->animTimerSlot();
}

/**
 * GcnCard object was destroyed.
 * @param obj QObject that was destroyed.
 */
void MemCardModel::memCard_destroyed_slot(QObject *obj)
{
	Q_D(MemCardModel);

	if (obj == d->card) {
		// Our GcnCard was destroyed.
		int old_numFiles = d->numFiles;
		if (old_numFiles > 0)
			beginRemoveRows(QModelIndex(), 0, (old_numFiles - 1));
		d->numFiles = 0;
		d->card = nullptr;
		if (old_numFiles > 0)
			endRemoveRows();
	}
}

/**
 * Files are about to be added to the GcnCard.
 * @param start First file index.
 * @param end Last file index.
 */
void MemCardModel::memCard_filesAboutToBeInserted_slot(int start, int end)
{
	// Start adding rows.
	beginInsertRows(QModelIndex(), start, end);

	// Save the start/end indexes.
	Q_D(MemCardModel);
	d->insertStart = start;
	d->insertEnd = end;
}

/**
 * Files have been added to the GcnCard.
 */
void MemCardModel::memCard_filesInserted_slot(void)
{
	Q_D(MemCardModel);

	// If these files have animated icons, add them.
	if (d->insertStart >= 0 && d->insertEnd >= 0) {
		for (int i = d->insertStart; i <= d->insertEnd; i++) {
			const GcnFile *file = d->card->getFile(i);
			d->initAnimState(file);
		}

		// Reset the row insert start/end indexes.
		d->insertStart = -1;
		d->insertEnd = -1;
	}

	// Update the animation timer state.
	d->updateAnimTimerState();

	// Update the file count.
	if (d->card)
		d->numFiles = d->card->numFiles();

	// Done adding rows.
	endInsertRows();
}

/**
 * Files are about to be removed from the GcnCard.
 * @param start First file index.
 * @param end Last file index.
 */
void MemCardModel::memCard_filesAboutToBeRemoved_slot(int start, int end)
{
	// Start removing rows.
	beginRemoveRows(QModelIndex(), start, end);

	// Remove animation states for these files.
	Q_D(MemCardModel);
	for (int i = start; i <= end; i++) {
		const GcnFile *file = d->card->getFile(i);
		d->animState.remove(file);
	}
}

/**
 * Files have been removed from the GcnCard.
 */
void MemCardModel::memCard_filesRemoved_slot(void)
{
	// Update the file count.
	Q_D(MemCardModel);
	if (d->card)
		d->numFiles = d->card->numFiles();

	// Done removing rows.
	endRemoveRows();
}

/** Slots. **/

/**
 * The system theme has changed.
 */
void MemCardModel::themeChanged_slot(void)
{
	// Reinitialize the style.
	Q_D(MemCardModel);
	d->style.init();

	// TODO: Force an update?
}