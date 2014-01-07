/***************************************************************************
 * GameCube Memory Card Recovery Program.                                  *
 * TranslationManager.cpp: Qt translation manager.                         *
 *                                                                         *
 * Copyright (c) 2014 by David Korth.                                      *
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

#include "config.mcrecover.h"
#include "TranslationManager.hpp"

// Qt includes.
#include <QtCore/QTranslator>
#include <QtCore/QCoreApplication>
#include <QtCore/QVector>
#include <QtCore/QString>
#include <QtCore/QDir>
#include <QtCore/QLibraryInfo>

// McRecoverQApplication. (Needed for translation context.)
#include "McRecoverQApplication.hpp"

/** TranslationManagerPrivate **/

class TranslationManagerPrivate
{
	public:
		TranslationManagerPrivate(TranslationManager *q);
		~TranslationManagerPrivate();

	private:
		TranslationManager *const q;
		Q_DISABLE_COPY(TranslationManagerPrivate);

	public:
		static TranslationManager *instance;

		// QTranslators.
		QTranslator *qtTranslator;
		QTranslator *prgTranslator;

		// List of paths to check for translations.
		// NOTE: qtTranslator also checks QLibraryInfo::TranslationsPath.
		QVector<QString> pathList;
};

// Singleton instance.
TranslationManager *TranslationManagerPrivate::instance = nullptr;

TranslationManagerPrivate::TranslationManagerPrivate(TranslationManager *q)
	: q(q)
	, qtTranslator(new QTranslator())
	, prgTranslator(new QTranslator())
{
	// Install the QTranslators.
	QCoreApplication::installTranslator(qtTranslator);
	QCoreApplication::installTranslator(prgTranslator);

	// Determine which paths to check for translations.
	pathList.clear();

#ifdef Q_OS_WIN
	// Win32: Search the program's /translations/ and main directories.
	pathList.append(QCoreApplication::applicationDirPath() + QLatin1String("/translations"));
	pathList.append(QCoreApplication::applicationDirPath());
#else /* !Q_OS_WIN */
	// Check if the program's directory is within the user's home directory.
	bool isPrgDirInHomeDir = false;
	QDir prgDir = QDir(QCoreApplication::applicationDirPath());
	QDir homeDir = QDir::home();

	do {
		if (prgDir == homeDir) {
			isPrgDirInHomeDir = true;
			break;
		}

		prgDir.cdUp();
	} while (!prgDir.isRoot());

	if (isPrgDirInHomeDir) {
		// Program is in the user's home directory.
		// This usually means they're working on it themselves.

		// Search the program's /translations/ and main directories.
		pathList.append(QCoreApplication::applicationDirPath() + QLatin1String("/translations"));
		pathList.append(QCoreApplication::applicationDirPath());
	}

	// Search the installed translations directory.
	pathList.append(QString::fromUtf8(MCRECOVER_DATA_DIRECTORY));

	// Search the user's .config directory.
	// TODO: Get default save directory using QSettings?
	pathList.append(homeDir.absoluteFilePath(QLatin1String("/.config/mcrecover/translations")));
	pathList.append(homeDir.absoluteFilePath(QLatin1String("/.config/mcrecover")));
#endif /* Q_OS_WIN */
}

TranslationManagerPrivate::~TranslationManagerPrivate()
{
	delete qtTranslator;
	delete prgTranslator;
}

/** TranslationManager **/

TranslationManager::TranslationManager()
	: d(new TranslationManagerPrivate(this))
{ }

TranslationManager::~TranslationManager()
	{ delete d; }

TranslationManager* TranslationManager::instance(void)
{
	if (!TranslationManagerPrivate::instance)
		TranslationManagerPrivate::instance = new TranslationManager();
	return TranslationManagerPrivate::instance;
}

/**
 * Set the translation.
 * @param locale Locale, e.g. "en_US". (Empty string is untranslated.)
 */
void TranslationManager::setTranslation(QString locale)
{
	// Initialize the Qt translation system.
	QString qtLocale = QLatin1String("qt_") + locale;
	bool isQtSysTranslator = false;
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
	// Qt on Unix (but not Mac) is usually installed system-wide.
	// Check the Qt library path first.
	isQtSysTranslator = d->qtTranslator->load(qtLocale,
		QLibraryInfo::location(QLibraryInfo::TranslationsPath));
#else
	// Suppress warnings that isQtSysTranslator is used but not set.
	Q_UNUSED(isQtSysTranslator)
#endif
	if (!isQtSysTranslator) {
		// System-wide translations aren't installed.
		// Check other paths.
		foreach (QString path, d->pathList) {
			if (d->qtTranslator->load(qtLocale, path))
				break;
		}
	}

	// Initialize the application translator.
	QString prgLocale = QLatin1String("mcrecover_") + locale;
	foreach (QString path, d->pathList) {
		if (d->prgTranslator->load(prgLocale, path))
			break;
	}

	/** Translation file information. **/

	//: Translation file author. Put your name here.
	QString tsAuthor = McRecoverQApplication::tr("David Korth", "ts-author");
	Q_UNUSED(tsAuthor)
	//: Language this translation provides, e.g. "English (US)".
	QString tsLanguage = McRecoverQApplication::tr("Default", "ts-language");
	Q_UNUSED(tsLanguage)
	//: Locale name, e.g. "en_US".
	QString tsLocale = McRecoverQApplication::tr("C", "ts-locale");
	Q_UNUSED(tsLocale)
}
