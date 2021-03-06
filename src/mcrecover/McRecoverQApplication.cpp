/***************************************************************************
 * GameCube Memory Card Recovery Program.                                  *
 * McRecoverQApplication.cpp: QApplication subclass.                       *
 *                                                                         *
 * Copyright (c) 2013-2018 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "config.mcrecover.h"
#include "McRecoverQApplication.hpp"
#include "util/array_size.h"

// Qt includes.
#include <QtCore/QTranslator>
#include <QtCore/QLocale>
#include <QtCore/QLibraryInfo>
#include <QtCore/QDir>
#include <QtGui/QIcon>
#include <QStyle>

// Translation Manager.
#include "TranslationManager.hpp"

// Import Qt plugins in static builds.
#if defined(QT_IS_STATIC) && defined(HAVE_QT_STATIC_PLUGIN_QJPCODECS)
#include <QtCore/QtPlugin>
Q_IMPORT_PLUGIN(qjpcodecs)
#endif

/** McRecoverQApplicationPrivate **/

class McRecoverQApplicationPrivate
{
	private:
		McRecoverQApplicationPrivate() { }
		~McRecoverQApplicationPrivate() { }
		Q_DISABLE_COPY(McRecoverQApplicationPrivate)

	public:
		// Initialize McRecoverQApplication.
		static void mcrqaInit(void);
};

/**
 * McRecoverQApplication initialization function.
 * The same code is used in all three McRecoverQApplication() constructors.
 */
void McRecoverQApplicationPrivate::mcrqaInit(void)
{
	// Set application information.
	QCoreApplication::setOrganizationName(QLatin1String("GerbilSoft"));
	QCoreApplication::setApplicationName(QLatin1String("GCN MemCard Recover"));

	// Version number.
	const QString sVersion = QString::fromLatin1(MCRECOVER_VERSION_STRING);
	QCoreApplication::setApplicationVersion(sVersion);

	// Set the application icon.
	QIcon mcrIcon = McRecoverQApplication::IconFromProgram(QLatin1String("mcrecover"));
	McRecoverQApplication::setWindowIcon(mcrIcon);

	// Check if an icon theme is available.
	if (!QIcon::hasThemeIcon(QLatin1String("application-exit"))) {
		// Icon theme is not available.
		// Use the built-in Oxygen icon theme.
		// Reference: http://tkrotoff.blogspot.com/2010/02/qiconfromtheme-under-windows.html
		QIcon::setThemeName(QLatin1String("oxygen"));
	}

	// Initialize the TranslationManager.
	TranslationManager *tsm = TranslationManager::instance();
	tsm->setTranslation(QLocale::system().name());
}

/** McRecoverQApplication **/

McRecoverQApplication::McRecoverQApplication(int &argc, char **argv)
	: super(argc, argv)
{
	McRecoverQApplicationPrivate::mcrqaInit();
}

/**
 * Get an icon from the system theme.
 * @param name Icon name.
 * @return QIcon.
 */
QIcon McRecoverQApplication::IconFromTheme(const QString &name)
{
	if (QIcon::hasThemeIcon(name))
		return QIcon::fromTheme(name);

	// System icon doesn't exist.
	// Get the fallback icon.
	struct IconSz_t {
		const char *path;
		int sz;
	};

	static const IconSz_t iconSz[] = {
		{":/oxygen/256x256/", 256},
		{":/oxygen/128x128/", 128},
		{":/oxygen/64x64/", 64},
		{":/oxygen/48x48/", 48},
		{":/oxygen/32x32/", 32},
		{":/oxygen/24x24/", 24},
		{":/oxygen/22x22/", 22},
		{":/oxygen/16x16/", 16}
	};

	QIcon icon;
	for (int i = 0; i < ARRAY_SIZE(iconSz); i++) {
		QPixmap pxm(QLatin1String(iconSz[i].path) + name + QLatin1String(".png"));
		if (!pxm.isNull()) {
			icon.addPixmap(pxm);
		}
	}

	return icon;
}

/**
 * Get an icon from the MemCard Recover icon set.
 * @param name Icon name.
 * @return QIcon.
 */
QIcon McRecoverQApplication::IconFromProgram(const QString &name)
{
	if (QIcon::hasThemeIcon(name))
		return QIcon::fromTheme(name);

	// System icon doesn't exist.
	// Get the fallback icon.
	struct IconSz_t {
		const char *path;
		int sz;
	};

	static const IconSz_t iconSz[] = {
		{":/mcrecover/256x256/", 256},
		{":/mcrecover/128x128/", 128},
		{":/mcrecover/64x64/", 64},
		{":/mcrecover/48x48/", 48},
		{":/mcrecover/32x32/", 32},
		{":/mcrecover/24x24/", 24},
		{":/mcrecover/22x22/", 22},
		{":/mcrecover/16x16/", 16}
	};

	QIcon icon;
	for (int i = 0; i < ARRAY_SIZE(iconSz); i++) {
		QPixmap pxm(QLatin1String(iconSz[i].path) + name + QLatin1String(".png"));
		if (!pxm.isNull()) {
			icon.addPixmap(pxm);
		}
	}

	return icon;
}

/**
 * Get a standard icon.
 * @param standardIcon Standard pixmap.
 * @param option QStyleOption.
 * @param widget QWidget.
 * @return QIcon.
 */
QIcon McRecoverQApplication::StandardIcon(QStyle::StandardPixmap standardIcon,
				const QStyleOption *option,
				const QWidget *widget)
{
	QStyle *style = QApplication::style();
	QIcon icon;
	const char *xdg_icon = nullptr;

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
	// Always use StandardPixmap.
	// Qt will use the xdg icon if the StandardPixmap isn't found.
	// TODO: Verify this behavior on old systems.
	switch (standardIcon) {
		case QStyle::SP_MessageBoxQuestion:
			xdg_icon = "dialog-question";
			break;
		default:
			xdg_icon = nullptr;
			break;
	}
#else
	// Other systems.
	// TODO: Check icons on Mac.
	switch (standardIcon) {
		// Windows only.
		case QStyle::SP_MessageBoxQuestion:
#if defined(Q_OS_WIN)
			icon = style->standardIcon(standardIcon, option, widget);
#else /* !Q_OS_WIN */
			xdg_icon = "dialog-question";
#endif /* Q_OS_WIN */
			break;

		// Neither Windows nor Mac OS X.
		case QStyle::SP_DialogApplyButton:
			xdg_icon = "dialog-ok-apply";
			break;
		case QStyle::SP_DialogCloseButton:
			xdg_icon = "dialog-close";
			break;

		// Available on all systems.
		case QStyle::SP_MessageBoxInformation:
		case QStyle::SP_MessageBoxWarning:
		case QStyle::SP_MessageBoxCritical:
		default:
			// TODO: Add more icons.
			icon = style->standardIcon(standardIcon, option, widget);
			break;
	}
#endif /* defined(Q_OS_UNIX) && !defined(Q_OS_MAC) */

	if (icon.isNull()) {
		if (xdg_icon)
			icon = IconFromTheme(QLatin1String(xdg_icon));
		if (icon.isNull()) {
			// We don't have a custom icon for this one.
			return style->standardIcon(standardIcon, option, widget);
		}
	}

	return icon;
}
