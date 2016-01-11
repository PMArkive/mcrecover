/***************************************************************************
 * GameCube Memory Card Recovery Program.                                  *
 * SAAdventure.hpp: Sonic Adventure - Adventure Mode status editor.        *
 *                                                                         *
 * Copyright (c) 2015-2016 by David Korth.                                 *
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

#include "SAAdventure.hpp"

// Qt includes.
#include <QtCore/QSignalMapper>

// Qt widgets.
#include <QtGui/QLabel>
#include <QtGui/QSpinBox>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>

// C includes. (C++ namespace)
#include <cstring>

// Sonic Adventure save file definitions.
#include "sa_defs.h"

// Common data.
#include "SAData.h"

// TODO: Put this in a common header file somewhere.
#define NUM_ELEMENTS(x) ((int)(sizeof(x) / sizeof(x[0])))

// Hide "unknown" values to save space.
// FIXME: Unhide these later - they're related to story progression.
#define DONT_SHOW_UNKNOWN 1

/** SAAdventurePrivate **/

#include "ui_SAAdventure.h"
class SAAdventurePrivate
{
	public:
		SAAdventurePrivate(SAAdventure *q);
		~SAAdventurePrivate();

	protected:
		SAAdventure *const q_ptr;
		Q_DECLARE_PUBLIC(SAAdventure)
	private:
		Q_DISABLE_COPY(SAAdventurePrivate)

	public:
		Ui_SAAdventure ui;

		// Total of 8 characters.
		#define TOTAL_CHARACTERS 8
		// Level widgets.
		struct {
			QLabel *lblCharacter;
			QSpinBox *spnLives;		// [0, 127]
			QCheckBox *chkCompleted;
			QComboBox *cboTimeOfDay;
			QSpinBox *spnEntrance;		// [-32,768, 32,767]
			QComboBox *cboLevelName;
			QSpinBox *spnLevelAct;		// [0, 7]

#ifndef DONT_SHOW_UNKNOWN
			QSpinBox *spnUnknown[3];	// [-32,768, 32,767]
#endif
		} characters[TOTAL_CHARACTERS];

		/**
		 * Initialize level widgets.
		 */
		void initCharacters(void);
};

SAAdventurePrivate::SAAdventurePrivate(SAAdventure *q)
	: q_ptr(q)
{ }

SAAdventurePrivate::~SAAdventurePrivate()
{
	// TODO: Is this needed?
	// The level widgets are owned by this widget, so they
	// should be automatically deleted...
	for (int i = 0; i < TOTAL_CHARACTERS; i++) {
		delete characters[i].lblCharacter;
		delete characters[i].spnLives;
		delete characters[i].chkCompleted;
		delete characters[i].cboTimeOfDay;
		delete characters[i].spnEntrance;
		delete characters[i].cboLevelName;
		delete characters[i].spnLevelAct;

#ifndef DONT_SHOW_UNKNOWN
		for (int j = 0; j < NUM_ELEMENTS(characters[i].spnUnknown); j++) {
			delete characters[i].spnUnknown[j];
		}
#endif
	}
}

/**
 * Initialize character widgets.
 */
void SAAdventurePrivate::initCharacters(void)
{
	// Create widgets for all levels.
	// TODO: Top or VCenter?
	// TODO: Scroll area is screwing with minimum width...
	Q_Q(SAAdventure);

#ifdef DONT_SHOW_UNKNOWN
	// Don't show the "Unknown" items to save space.
	ui.lblUnknown1->hide();
	ui.lblUnknown2->hide();
	ui.lblUnknown3->hide();

	// Adjust grid positions of the other labels.
	ui.gridLevels->addWidget(ui.lblEntrance, 0, 4, Qt::AlignVCenter);
	ui.gridLevels->addWidget(ui.lblLevel, 0, 5, Qt::AlignVCenter);

	// Allow the "Level" column to stretch, but nothing else.
	ui.gridLevels->setColumnStretch(5, 1);
#else
	// Allow the "Level" column to stretch, but nothing else.
	ui.gridLevels->setColumnStretch(7, 1);
#endif

	// Convert the level names to QString here
	// to reduce the total number of conversions
	// and memory usage.
	QString levelNames[SA_LEVEL_NAMES_ALL_COUNT];
	for (int i = 0; i < SA_LEVEL_NAMES_ALL_COUNT; i++) {
		if (i == 34 /* The Past */ ||
		    strchr(sa_level_names_all[i], '('))
		{
			// Level name is unused here.
			levelNames[i] = SAAdventure::tr("Unused (%1)").arg(i+1);
		} else {
			levelNames[i] = QLatin1String(sa_level_names_all[i]);
		}
	}

	QString qsCssCheckBox = QLatin1String(sa_ui_css_emblem_checkbox);
	for (int chr = 0; chr < TOTAL_CHARACTERS; chr++) {
		// Character icon.
		characters[chr].lblCharacter = new QLabel(q);
		if (sa_ui_char_icons_super[chr] && sa_ui_char_icons_super[chr][0] != 0) {
			// Icon is available.
			characters[chr].lblCharacter->setPixmap(QPixmap(QLatin1String(sa_ui_char_icons_super[chr])));
			characters[chr].lblCharacter->setToolTip(QLatin1String(sa_ui_char_names_super[chr]));
		} else {
			// No icon. Use text instead.
			characters[chr].lblCharacter->setTextFormat(Qt::PlainText);
			characters[chr].lblCharacter->setText(QLatin1String(sa_ui_char_names_super[chr]));
			// TODO: Make it the same height as [0] if this isn't [0]?
			// Or, add a dummy "unused" icon.
		}
		characters[chr].lblCharacter->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
		ui.gridLevels->addWidget(characters[chr].lblCharacter, chr+1, 0, Qt::AlignCenter | Qt::AlignVCenter);

		// NOTE: The character icon is taller than the various widgets.
		// We should use Qt::AlignVCenter.
		// TODO: Use a 16x16 character icon instead?

		// Number of lives.
		characters[chr].spnLives = new QSpinBox(q);
		characters[chr].spnLives->setRange(0, 127);
		characters[chr].spnLives->setSingleStep(1);
		characters[chr].spnLives->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
		ui.gridLevels->addWidget(characters[chr].spnLives, chr+1, 1, Qt::AlignVCenter);

		// Completed?
		characters[chr].chkCompleted = new QCheckBox(q);
		characters[chr].chkCompleted->setStyleSheet(qsCssCheckBox);
		characters[chr].chkCompleted->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
		ui.gridLevels->addWidget(characters[chr].chkCompleted, chr+1, 2, Qt::AlignCenter | Qt::AlignVCenter);

		// Time of day.
		characters[chr].cboTimeOfDay = new QComboBox(q);
		characters[chr].cboTimeOfDay->addItem(QLatin1String("Day"));
		characters[chr].cboTimeOfDay->addItem(QLatin1String("Evening"));
		characters[chr].cboTimeOfDay->addItem(QLatin1String("Night"));
		characters[chr].cboTimeOfDay->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
		ui.gridLevels->addWidget(characters[chr].cboTimeOfDay, chr+1, 3, Qt::AlignVCenter);

#ifdef DONT_SHOW_UNKNOWN
		const int idx = 4;
#else
		const int idx = 6;
#endif

		// Entrance.
		characters[chr].spnEntrance = new QSpinBox(q);
		characters[chr].spnEntrance->setRange(-32768, 32767);
		characters[chr].spnEntrance->setSingleStep(1);
		characters[chr].spnEntrance->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
		ui.gridLevels->addWidget(characters[chr].spnEntrance, chr+1, idx, Qt::AlignVCenter);

		// NOTE: I originally had spnLevelAct in an HBox with cboLevelName,
		// but the stretch options didn't work right.
		// It's now in its own column with no header.

		// Level name.
		characters[chr].cboLevelName = new QComboBox(q);
		for (int i = 0; i < SA_LEVEL_NAMES_ALL_COUNT; i++) {
			characters[chr].cboLevelName->addItem(levelNames[i]);
		}
		characters[chr].cboLevelName->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
		ui.gridLevels->addWidget(characters[chr].cboLevelName, chr+1, idx+1, Qt::AlignVCenter);

		// Level act.
		characters[chr].spnLevelAct = new QSpinBox(q);
		characters[chr].spnLevelAct->setRange(0, 7);
		characters[chr].spnLevelAct->setSingleStep(1);
		characters[chr].spnLevelAct->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
		ui.gridLevels->addWidget(characters[chr].spnLevelAct, chr+1, idx+2, Qt::AlignVCenter);

#ifndef DONT_SHOW_UNKNOWN
		// Unknown items.
		for (int i = 0; i < 3; i++) {
			characters[chr].spnUnknown[i] = new QSpinBox(q);
			characters[chr].spnUnknown[i]->setRange(-32768, 32767);
			characters[chr].spnUnknown[i]->setSingleStep(1);
			characters[chr].spnUnknown[i]->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
			ui.gridLevels->addWidget(characters[chr].spnUnknown[i], chr+1, (i >= 2 ? i+7 : i+4), Qt::AlignVCenter);
		}
#endif

		// Spacers to reduce 
#if 0		
		levels[level].lblLevel = new QLabel(q);
		levels[level].lblLevel->setText(QLatin1String(levelNames[level]));
		levels[level].lblLevel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
		ui.gridLevels->addWidget(levels[level].lblLevel, level+1, 0, Qt::AlignTop);

		// Spinbox for each character.
		for (int chr = 0; chr < NUM_ELEMENTS(levels[level].spnCount); chr++) {
			levels[level].spnCount[chr] = new QSpinBox(q);
			levels[level].spnCount[chr]->setRange(0, 255);
			levels[level].spnCount[chr]->setSingleStep(1);
			levels[level].spnCount[chr]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
			ui.gridLevels->addWidget(levels[level].spnCount[chr], level+1, chr+1, Qt::AlignTop);

			// Connect the valueChanged() signal.
			QObject::connect(levels[level].spnCount[chr], SIGNAL(valueChanged(int)),
					 mapperSpinBox, SLOT(map()));
			mapperSpinBox->setMapping(levels[level].spnCount[chr],
						((level << 8) | chr));
		}
#endif
	}
}

/** SAAdventure **/

SAAdventure::SAAdventure(QWidget *parent)
	: QWidget(parent)
	, d_ptr(new SAAdventurePrivate(this))
{
	Q_D(SAAdventure);
	d->ui.setupUi(this);

	// Fix alignment of the header labels.
	d->ui.gridLevels->setAlignment(d->ui.lblCharacter, Qt::AlignTop);
	d->ui.gridLevels->setAlignment(d->ui.lblLives,     Qt::AlignTop);
	d->ui.gridLevels->setAlignment(d->ui.lblCompleted, Qt::AlignTop);
	d->ui.gridLevels->setAlignment(d->ui.lblTimeOfDay, Qt::AlignTop);
	d->ui.gridLevels->setAlignment(d->ui.lblUnknown1,  Qt::AlignTop);
	d->ui.gridLevels->setAlignment(d->ui.lblUnknown2,  Qt::AlignTop);
	d->ui.gridLevels->setAlignment(d->ui.lblEntrance,  Qt::AlignTop);
	d->ui.gridLevels->setAlignment(d->ui.lblLevel,     Qt::AlignTop);
	d->ui.gridLevels->setAlignment(d->ui.lblUnknown3,  Qt::AlignTop);

	// Initialize the character listing.
	d->initCharacters();
}

SAAdventure::~SAAdventure()
{
	Q_D(SAAdventure);
	delete d;
}

/** Events. **/

/**
 * Widget state has changed.
 * @param event State change event.
 */
void SAAdventure::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange) {
		// Retranslate the UI.
		Q_D(SAAdventure);
		d->ui.retranslateUi(this);
	}

	// Pass the event to the base class.
	this->QWidget::changeEvent(event);
}

/** Public functions. **/

/**
 * Load data from a Sonic Adventure save slot.
 * @param sa_save Sonic Adventure save slot.
 * The data must have already been byteswapped to host-endian.
 * @return 0 on success; non-zero on error.
 */
int SAAdventure::load(const sa_save_slot *sa_save)
{
	Q_D(SAAdventure);
#if 0
	memcpy(&d->clear_count, &sa_save->clear_count, sizeof(d->clear_count));

	// Update the display.
	d->updateDisplay();
#endif
	return 0;
}

/**
 * Save data to a Sonic Adventure save slot.
 * @param sa_save Sonic Adventure save slot.
 * The data will be in host-endian format.
 * @return 0 on success; non-zero on error.
 */
int SAAdventure::save(sa_save_slot *sa_save) const
{
	Q_D(const SAAdventure);
#if 0
	memcpy(&sa_save->clear_count, &d->clear_count, sizeof(sa_save->clear_count));
#endif
	return 0;
}