/***************************************************************************
 * GameCube Memory Card Recovery Program.                                  *
 * HerpDerpEggListener.cpp: Listener for... something. (shh)               *
 *                                                                         *
 * Copyright (c) 2014 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "HerpDerpEggListener.hpp"
#include "util/array_size.h"

// C includes.
#include <stdint.h>

// C includes. (C++ namespace)
#include <cstring>

// Qt includes.
#include <QtGui/QKeyEvent>
#include <QApplication>
#include <QDesktopWidget>

/** HerpDerpEggListenerPrivate **/

class HerpDerpEggListenerPrivate
{
	public:
		explicit HerpDerpEggListenerPrivate(HerpDerpEggListener *q);
		~HerpDerpEggListenerPrivate();

	protected:
		HerpDerpEggListener *const q_ptr;
		Q_DECLARE_PUBLIC(HerpDerpEggListener)
	private:
		Q_DISABLE_COPY(HerpDerpEggListenerPrivate)

	public:
		HackDetection::DetectType detectType;
		int seq_pos;

		// Set of HackDetection objects.
		// NOTE: Must be QObject, since we can't use
		// qobject_cast<> in the destroyed() slot.
		QSet<QObject*> hdSet;

		/**
		 * Do "Hack Detection" on all monitors.
		 */
		void doHackDetection(void);
};

HerpDerpEggListenerPrivate::HerpDerpEggListenerPrivate(HerpDerpEggListener *q)
	: q_ptr(q)
	, detectType(HackDetection::DT_NONE)
	, seq_pos(0)
{ }	

HerpDerpEggListenerPrivate::~HerpDerpEggListenerPrivate()
{
	// NOTE: QSet::swap() was added in qt-4.8.
	QSet<QObject*> hdSet_del;
	hdSet_del = hdSet;
	hdSet.clear();
	qDeleteAll(hdSet_del);
}

/**
 * Do "Hack Detection" on all monitors.
 */
void HerpDerpEggListenerPrivate::doHackDetection(void)
{
	Q_Q(HerpDerpEggListener);
	QWidget *parent = qobject_cast<QWidget*>(q->parent());

	seq_pos = 0;
	QDesktopWidget *desktop = QApplication::desktop();
	for (int i = 0; i < desktop->numScreens(); i++) {
		HackDetection *hd = new HackDetection(i, parent);
		hd->setDetectType(this->detectType);
		QObject::connect(hd, &QObject::destroyed,
			q, &HerpDerpEggListener::hd_destroyed);
		hdSet.insert(hd);
		hd->show();
	}
}

/** HerpDerpEggListener **/

HerpDerpEggListener::HerpDerpEggListener(QObject *parent)
	: QObject(parent)
	, d_ptr(new HerpDerpEggListenerPrivate(this))
{ }

HerpDerpEggListener::~HerpDerpEggListener()
{
	Q_D(HerpDerpEggListener);
	delete d;
}

/**
 * Set the selected game ID. (ID6)
 * @param gameID Game ID. (gamecode+company)
 */
void HerpDerpEggListener::setSelGameID(const QString &gameID)
{
	Q_D(HerpDerpEggListener);
	d->detectType = HackDetection::DT_NONE;
	d->seq_pos = 0;
	if (gameID.size() != 6) {
		// Invalid game ID.
		return;
	}

	const QByteArray str_data = gameID.toLatin1();
	const char *str = str_data.constData();

	struct hurrMatch_t {
		char gameID[7];
		uint8_t dt;
	};

	static const hurrMatch_t hurrMatch[] = {
		/** HACK DETECTION **/
		{"G2XE8P", HackDetection::DT_H},
		{"G2XP8P", HackDetection::DT_H},
		{"G9SE8P", HackDetection::DT_H},
		{"G9SJ8P", HackDetection::DT_H},
		{"G9SP8P", HackDetection::DT_H},
		{"GSBJ8P", HackDetection::DT_H},
		{"GSNE8P", HackDetection::DT_H},
		{"GSNP8P", HackDetection::DT_H},
		{"GSOE8P", HackDetection::DT_H},
		{"GSOJ8P", HackDetection::DT_H},
		{"GSOP8P", HackDetection::DT_H},
		{"GXEE8P", HackDetection::DT_H},
		{"GXEJ8P", HackDetection::DT_H},
		{"GXEP8P", HackDetection::DT_H},
		{"GXSE8P", HackDetection::DT_H},
		{"GXSP6W", HackDetection::DT_H},
		{"GXSP8P", HackDetection::DT_H},
		{"RELSAB", HackDetection::DT_H},
		/** QUACK DETECTION **/
		{"GDDE41", HackDetection::DT_Q},
		{"GDDP41", HackDetection::DT_Q},
		{"GDOP41", HackDetection::DT_Q},
		/** SNACK DETECTION **/
		{"GKYE01", HackDetection::DT_S},
		{"GKYJ01", HackDetection::DT_S},
		{"GKYP01", HackDetection::DT_S},
	};

	for (int i = (ARRAY_SIZE(hurrMatch)-1); i >= 0; i--) {
		if (!strcmp(str, hurrMatch[i].gameID)) {
			d->detectType = (HackDetection::DetectType)hurrMatch[i].dt;
			break;
		}
	}
}

/**
 * Key press listener.
 * @param event Key press event.
 */
void HerpDerpEggListener::widget_keyPress(QKeyEvent *event)
{
	// Key sequence.
	static const uint8_t hack_seq[] = {
		Qt::Key_1, Qt::Key_9,
		Qt::Key_6, Qt::Key_5,
		Qt::Key_0, Qt::Key_9,
		Qt::Key_1, Qt::Key_7
	};

	Q_D(HerpDerpEggListener);
	if (d->detectType <= HackDetection::DT_NONE ||
	    d->detectType >= HackDetection::DT_MAX) {
		// Invalid detection.
		d->seq_pos = 0;
		return;
	}

	if (d->seq_pos < 0 || d->seq_pos >= ARRAY_SIZE(hack_seq))
		d->seq_pos = 0;

	if (event->key() == (int)hack_seq[d->seq_pos]) {
		d->seq_pos++;
		if (d->seq_pos == ARRAY_SIZE(hack_seq)) {
			d->seq_pos = 0;
			d->doHackDetection();
		}
	} else if (event->key() == (int)hack_seq[0]) {
		d->seq_pos = 1;
	} else {
		d->seq_pos = 0;
	}
}

/**
 * Focus out listener.
 * @param event Focus out event.
 */
void HerpDerpEggListener::widget_focusOut(QFocusEvent *event)
{
	Q_D(HerpDerpEggListener);
	if (event->lostFocus())
		d->seq_pos = 0;
}

/**
 * HackDetection destroyed slot.
 * @param object HackDetection.
 */
void HerpDerpEggListener::hd_destroyed(QObject *object)
{
	Q_D(HerpDerpEggListener);
	if (d->hdSet.contains(object)) {
		// Delete all HackDetections.
		// NOTE: QSet::swap() was added in qt-4.8.
		QSet<QObject*> hdSet_del;
		hdSet_del = d->hdSet;
		d->hdSet.clear();
		hdSet_del.remove(object);
		qDeleteAll(hdSet_del);
	}
}
