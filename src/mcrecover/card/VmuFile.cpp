/***************************************************************************
 * GameCube Memory Card Recovery Program.                                  *
 * VmuFile.cpp: Dreamcast VMU file entry class.                            *
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

#include "VmuFile.hpp"

#include "vmu.h"
#include "util/byteswap.h"

// VmuCard class.
#include "VmuCard.hpp"

// GcImage class.
#include "DcImageLoader.hpp"

// C includes. (C++ namespace)
#include <cerrno>
#include <cstdlib>

// C++ includes.
#include <string>
#include <vector>
using std::string;
using std::vector;

// Qt includes.
#include <QtCore/QByteArray>
#include <QtCore/QTextCodec>
#include <QtCore/QFile>
#include <QtCore/QIODevice>

#define NUM_ELEMENTS(x) ((int)(sizeof(x) / sizeof(x[0])))

/** VmuFilePrivate **/

#include "File_p.hpp"
class VmuFilePrivate : public FilePrivate
{
	public:
		/**
		 * Initialize the VmuFile private class.
		 * This constructor is for valid files.
		 * @param q VmuFile.
		 * @param card VmuCard.
		 * @param direntry Directory Entry pointer.
		 * @param mc_fat VMU FAT.
		 */
		VmuFilePrivate(VmuFile *q, VmuCard *card,
			const vmu_dir_entry *dirEntry,
			const vmu_fat *mc_fat);

		virtual ~VmuFilePrivate();

	protected:
		Q_DECLARE_PUBLIC(VmuFile)
	private:
		Q_DISABLE_COPY(VmuFilePrivate)

		/**
		 * Load the file information.
		 */
		void loadFileInfo(void);

	public:
		const vmu_fat *mc_fat;	// VMU FAT. (TODO: Do we need to store this?)

		/**
		 * Directory entry.
		 * This points to an entry within card's Directory Table.
		 * NOTE: If this is a lost file, this was allocated by us,
		 * and needs to be freed in the destructor.
		 */
		const vmu_dir_entry *dirEntry;

		// VMU file header.
		vmu_file_header *fileHeader;

		// File descriptions.
		QString vmu_desc;
		QString dc_desc;

		// VMU icons. (ICONDATA_VMS)
		// NOTE: These must NOT be the same as
		// gcBanner or any icon in gcIcons.
		bool isIconData;
		GcImage *vmu_icon_mono;
		GcImage *vmu_icon_color;

		/**
		 * Load the banner image.
		 * @return GcImage containing the banner image, or nullptr on error.
		 */
		virtual GcImage *loadBannerImage(void) override;

		/**
		 * Load the icon images.
		 * @return QVector<GcImage*> containing the icon images, or empty QVector on error.
		 */
		virtual QVector<GcImage*> loadIconImages(void) override;

		/**
		 * Load the icon images.
		 * Special version for ICONDATA_VMS.
		 * NOTE: This also sets internal variables vmu_icon_mono and vmu_icon_color.
		 * @return QVector<GcImage*> containing the icon images, or empty QVector on error.
		 */
		QVector<GcImage*> loadIconImages_ICONDATA_VMS(void);
};

/**
 * Initialize the VmuFile private class.
 * This constructor is for valid files.
 * @param q VmuFile.
 * @param card VmuCard.
 * @param direntry Directory Entry pointer.
 * @param mc_fat VMU FAT.
 */
VmuFilePrivate::VmuFilePrivate(VmuFile *q, VmuCard *card,
		const vmu_dir_entry *dirEntry,
		const vmu_fat *mc_fat)
	: FilePrivate(q, card)
	, mc_fat(mc_fat)
	, dirEntry(dirEntry)
	, fileHeader(nullptr)
	, isIconData(false)
	, vmu_icon_mono(nullptr)
	, vmu_icon_color(nullptr)
{
	if (!dirEntry || !mc_fat) {
		// Invalid data.
		this->dirEntry = nullptr;
		this->mc_fat = nullptr;

		// This file is basically useless now...
		return;
	}

	// Clamp file length to the size of the memory card.
	// This shouldn't happen, but it's possible if either
	// the filesystem is heavily corrupted, or the file
	// isn't actually a GCN Memory Card image.
	int size = dirEntry->size;
	if (size > card->totalUserBlocks())
		size = card->totalUserBlocks();

	// Load the FAT entries.
	fatEntries.clear();
	fatEntries.reserve(size);
	// TODO: Add a 'lastValidBlock' function?
	uint16_t totalUserBlocks = card->totalUserBlocks();
	uint16_t next_block = dirEntry->address;
	if (next_block < totalUserBlocks && next_block != VMU_FAT_BLOCK_LAST_IN_FILE) {
		fatEntries.append(next_block);

		// Go through the rest of the blocks.
		for (int i = size; i > 1; i--) {
			next_block = mc_fat->fat[next_block];
			if (next_block >= totalUserBlocks ||
			    next_block == VMU_FAT_BLOCK_LAST_IN_FILE)
			{
				// Next block is invalid.
				break;
			}
			fatEntries.append(next_block);
		}
	}

	// Load the file information.
	loadFileInfo();
}

VmuFilePrivate::~VmuFilePrivate()
{
	if (lostFile) {
		// dirEntry was allocated by us.
		// Free it.
		free((void*)dirEntry);
	}

	// Delete the allocated VMU file header.
	free(fileHeader);

	// Delete the VMU icons. (ICONDATA_VMS)
	delete vmu_icon_mono;
	delete vmu_icon_color;
}

/**
 * Load the file information.
 */
void VmuFilePrivate::loadFileInfo(void)
{
	// Clear stuff that isn't used by the VMU.
	gameID.clear();

	// Filename.
	// TODO: Are Shift-JIS filenames allowed?
	// TODO: Trim filenames?
	filename = QString::fromLatin1(dirEntry->filename, sizeof(dirEntry->filename));

	// Timestamp.
	// FIXME: This might be ctime, not mtime...
	mtime.setVmuTimestamp(dirEntry->ctime);

	// Mode. (TODO)
	//this->mode = dirEntry->permission;

	// Load the block containing the file header.
	const int blockSize = card->blockSize();
	char *data = (char*)malloc(blockSize);
	int ret = card->readBlock(data, blockSize, fileBlockAddrToPhysBlockAddr(dirEntry->header_addr));
	if (ret != blockSize) {
		// Read error.
		// File is probably invalid.
		free(data);
		return;
	}

	// TODO: VMS descriptions are probably JIS X 0201, not Shift-JIS.
	// JIS X 0201 is ASCII plus single-byte characters only.

	if (filename == QLatin1String("ICONDATA_VMS")) {
		// Icon data.
		// Reference: http://mc.pp.se/dc/vms/icondata.html
		isIconData = true;
		const vmu_card_icon_header *fileHeader = (vmu_card_icon_header*)data;
		// TODO: Load icons and stuff.

		// File description.
		// NOTE: Only a VMU description is present, since the file
		// isn't visible in the DC file manager.
		vmu_desc = decodeText_SJISorCP1252(fileHeader->desc_vmu, sizeof(fileHeader->desc_vmu)).trimmed();
		dc_desc.clear();

		// TODO: Retranslate the description on language change.
		description = filename + QChar(L'\0') +
			VmuFile::tr("Custom VMU icon file.");
	} else {
		// Regular file.
		// Reference: http://mc.pp.se/dc/vms/fileheader.html
		isIconData = false;
		if (!fileHeader)
			fileHeader = (vmu_file_header*)malloc(sizeof(*fileHeader));
		memcpy(fileHeader, data, sizeof(*fileHeader));

		// Byteswap the header.
		fileHeader->icon_count		= le16_to_cpu(fileHeader->icon_count);
		fileHeader->icon_speed		= le16_to_cpu(fileHeader->icon_speed);
		fileHeader->eyecatch_type	= le16_to_cpu(fileHeader->eyecatch_type);
		fileHeader->crc			= le16_to_cpu(fileHeader->crc);
		fileHeader->size		= le32_to_cpu(fileHeader->size);

		// File description.
		vmu_desc = decodeText_SJISorCP1252(fileHeader->desc_vmu, sizeof(fileHeader->desc_vmu)).trimmed();
		dc_desc  = decodeText_SJISorCP1252(fileHeader->desc_dc,  sizeof(fileHeader->desc_dc)).trimmed();

		// NOTE: The DC file manager shows filename and DC description,
		// so we'll show the same thing.
		description = filename + QChar(L'\0') + dc_desc;
	}

	// Load the banner and icon images.
	loadImages();
}

/**
 * Load the banner image.
 * @return GcImage* containing the banner image, or nullptr on error.
 */
GcImage *VmuFilePrivate::loadBannerImage(void)
{
	if (isIconData) {
		// ICONDATA_VMS
		return nullptr;
	}

	if (!fileHeader ||
	    fileHeader->eyecatch_type == VMU_EYECATCH_NONE ||
	    fileHeader->eyecatch_type > VMU_EYECATCH_PALETTE_16)
	{
		// No eyecatch.
		return nullptr;
	}

	if (fileHeader->eyecatch_type != VMU_EYECATCH_PALETTE_16) {
		// FIXME: Not supported yet.
		return nullptr;
	}

	// Load the file into memory.
	// TODO: Optimize by only reading in required data.
	// TODO: Copy over the block code from GcnFile::loadIconImages(),
	// but move the "read from X to Y" code down to File.
	QByteArray data = this->loadFileData();

	// Eyecatch start address.
	int eyecatchStart = (dirEntry->header_addr * card->blockSize());
	eyecatchStart += sizeof(*fileHeader);
	if (fileHeader->icon_count > 0) {
		eyecatchStart += sizeof(vmu_icon_palette);
		eyecatchStart += (sizeof(vmu_icon_data) * fileHeader->icon_count);
	}

	// TODO: Other variants.
	const int eyecatchSize = VMU_EYECATCH_PALETTE_16_LEN;
	if (data.size() < (int)(eyecatchStart + eyecatchSize)) {
		// File is too small.
		// The eyecatch isn't actually there...
		return nullptr;
	}

	const vmu_eyecatch_palette_16 *eyecatch16 = (const vmu_eyecatch_palette_16*)(data.data() + eyecatchStart);
	GcImage *gcImage = DcImageLoader::fromPalette16(
				VMU_EYECATCH_W, VMU_EYECATCH_H,
				eyecatch16->eyecatch, sizeof(eyecatch16->eyecatch),
				eyecatch16->palette, sizeof(eyecatch16->palette));
	return gcImage;
}

/**
 * Load the icon images.
 * @return QVector<GcImage*> containing the icon images, or empty QVector on error.
 */
QVector<GcImage*> VmuFilePrivate::loadIconImages(void)
{
	if (isIconData) {
		// ICONDATA_VMS
		return loadIconImages_ICONDATA_VMS();
	}

	// DC only supports looping icon animations.
	// TODO: Use system-independent values?
	this->iconAnimMode = 0;

	if (!fileHeader || fileHeader->icon_count == 0) {
		// No file header or icons.
		return QVector<GcImage*>();
	}

	// Sanity check: Clamp to 8 icons maximum.
	int iconCount = fileHeader->icon_count;
	if (iconCount > 8)
		iconCount = 8;

	// Load the file into memory.
	// TODO: Optimize by only reading in required data.
	// TODO: Copy over the block code from GcnFile::loadIconImages(),
	// but move the "read from X to Y" code down to File.
	QByteArray data = this->loadFileData();

	// Icon start address.
	int iconStart = (dirEntry->header_addr * card->blockSize());
	iconStart += sizeof(*fileHeader);

	// Calculate the total icon length.
	const int totalIconLen = sizeof(vmu_icon_palette) +
				(sizeof(vmu_icon_data) * iconCount);
	if (data.size() < (int)(iconStart + totalIconLen)) {
		// File is too small.
		// The icons aren't actually there...
		return QVector<GcImage*>();
	}

	const char *pIconStart = (data.data() + iconStart);
	const vmu_icon_palette *palette = (const vmu_icon_palette*)pIconStart;
	const vmu_icon_data *iconData = (const vmu_icon_data*)(pIconStart + sizeof(*palette));
	QVector<GcImage*> gcImages;
	// TODO: Should be part of a struct that's returned...
	this->iconSpeed.clear();
	for (int i = 0; i < iconCount; i++, iconData++) {
		// TODO: Should be part of a struct that's returned...
		// TODO: Convert DC icon speed to system-independent value.
		this->iconSpeed.append(3);
		GcImage *gcImage = DcImageLoader::fromPalette16(
					VMU_ICON_W, VMU_ICON_H,
					iconData->icon, sizeof(iconData->icon),
					palette->palette, sizeof(palette->palette));
		gcImages.append(gcImage);
	}

	return gcImages;
}

/**
 * Load the icon images.
 * Special version for ICONDATA_VMS.
 * NOTE: This also sets internal variables vmu_icon_mono and vmu_icon_color.
 * @return QVector<GcImage*> containing the icon images, or empty QVector on error.
 */
QVector<GcImage*> VmuFilePrivate::loadIconImages_ICONDATA_VMS(void)
{
	// Delete any allocated VMU icons.
	delete vmu_icon_mono;
	vmu_icon_mono = nullptr;
	delete vmu_icon_color;
	vmu_icon_color = nullptr;

	// DC only supports looping icon animations.
	// TODO: Use system-independent values?
	this->iconAnimMode = 0;

	// Load the file into memory.
	// TODO: Optimize by only reading in required data.
	// TODO: Copy over the block code from GcnFile::loadIconImages(),
	// but move the "read from X to Y" code down to File.
	QByteArray data = this->loadFileData();

	// Get the ICONDATA_VMS header.
	const int headerStart = (dirEntry->header_addr * card->blockSize());
	const int headerEnd = (headerStart + sizeof(vmu_card_icon_header));
	if (data.size() < headerEnd) {
		// File is too small.
		// The icons aren't actually there...
		return QVector<GcImage*>();
	}
	vmu_card_icon_header iconHeader;
	memcpy(&iconHeader, (data.data() + headerStart), sizeof(iconHeader));

	// Byteswap the icon header.
	iconHeader.icon_mono_offset	= le32_to_cpu(iconHeader.icon_mono_offset);
	iconHeader.icon_color_offset	= le32_to_cpu(iconHeader.icon_color_offset);

	// Load the mono icon.
	// TODO: Only check for offset != 0?
	if (iconHeader.icon_mono_offset >= (uint32_t)headerEnd) {
		int monoIconEnd = iconHeader.icon_mono_offset + sizeof(vmu_card_icon_mono_data);
		if (data.size() < monoIconEnd) {
			// File is too small.
			// The icon isn't actually here...
			goto colorIcon;
		}

		const vmu_card_icon_mono_data *monoIconData =
			(const vmu_card_icon_mono_data*)(data.data() + iconHeader.icon_mono_offset);
		vmu_icon_mono = DcImageLoader::fromMonochrome(
					VMU_ICON_W, VMU_ICON_H,
					monoIconData->icon, sizeof(monoIconData->icon));
	}

colorIcon:
	// Load the color icon.
	// TODO: Only check for offset != 0?
	if (iconHeader.icon_color_offset >= (uint32_t)headerEnd) {
		int colorIconEnd = iconHeader.icon_color_offset + sizeof(vmu_card_icon_color_data);
		if (data.size() < colorIconEnd) {
			// File is too small.
			// The icon isn't actually here...
			goto doneLoadingIcons;
		}

		const vmu_card_icon_color_data *colorIconData =
			(const vmu_card_icon_color_data*)(data.data() + iconHeader.icon_color_offset);
		vmu_icon_color = DcImageLoader::fromPalette16(
					VMU_ICON_W, VMU_ICON_H,
					colorIconData->icon, sizeof(colorIconData->icon),
					colorIconData->palette, sizeof(colorIconData->palette));
	}

doneLoadingIcons:
	// Check if any icons were loaded.
	GcImage *ret_icon = nullptr;
	if (vmu_icon_color) {
		// Color icon was loaded.
		ret_icon = new GcImage(*vmu_icon_color);
	} else if (vmu_icon_mono) {
		// Monochrome icon was loaded.
		ret_icon = new GcImage(*vmu_icon_mono);
	}

	QVector<GcImage*> ret;
	if (ret_icon)
		ret.append(ret_icon);
	return ret;
}

/** VmuFile **/

/**
 * Create a VmuFile for a VmuCard.
 * This constructor is for valid files.
 * @param q VmuFile.
 * @param card VmuCard.
 * @param direntry Directory Entry pointer.
 * @param mc_fat VMU FAT.
 */
VmuFile::VmuFile(VmuCard *card,
		const vmu_dir_entry *dirEntry,
		const vmu_fat *mc_fat)
	: File(new VmuFilePrivate(this, card, dirEntry, mc_fat), card)
{
	// NOTE: This can't be put in VmuFilePrivate::loadFileInfo(),
	// since VmuCard isn't fully created at that time.
	// TODO: Move to a separate function?
	Q_D(VmuFile);
	if (d->dirEntry->filetype == VMU_DIR_FILETYPE_DATA) {
		// Data file. Verify the header's CRC.
		Checksum::ChecksumDef checksumDef;
		checksumDef.algorithm = Checksum::CHKALG_DREAMCASTVMU;
		checksumDef.address = 0x46;
		checksumDef.param = 0x46;
		checksumDef.start = 0;
		checksumDef.length = (this->size() * card->blockSize());
		checksumDef.endian = Checksum::CHKENDIAN_LITTLE;

		// TODO: Optimize this?
		QVector<Checksum::ChecksumDef> checksumDefs;
		checksumDefs.append(checksumDef);
		setChecksumDefs(checksumDefs);
	}
}

// TODO: Maybe not needed?
VmuFile::~VmuFile()
{ }

/**
 * Get the file's mode as a string.
 * This is system-specific.
 * @return File mode as a string.
 */
QString VmuFile::modeAsString(void) const
{
	// TODO
	return QString();
}

/** Export **/

/**
 * Get the default export filename.
 * @return Default export filename.
 */
QString VmuFile::defaultExportFilename(void) const
{
	// TODO
	return QString();
}

/**
 * Export the file.
 * @param filename Filename for the exported file.
 * @return 0 on success; non-zero on error.
 * TODO: Error code constants.
 */
int VmuFile::exportToFile(const QString &filename)
{
	// NOTE: This function doesn't actually do anything different
	// from the base class function, but gcc-4.9.2 attempts to use
	// the QIODevice version when using a VmuFile* instead of a
	// File*. Hence, we need this virtual function.
	return File::exportToFile(filename);
}

/**
 * Export the file.
 * @param qioDevice QIODevice to write the data to.
 * @return 0 on success; non-zero on error.
 * TODO: Error code constants.
 */
int VmuFile::exportToFile(QIODevice *qioDevice)
{
	// TODO
	Q_UNUSED(qioDevice);
	return -ENOSYS;
}