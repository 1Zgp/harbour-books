/*
 * Copyright (C) 2004-2010 Geometer Plus <contact@geometerplus.com>
 * Copyright (C) 2015 Slava Monich <slava.monich@jolla.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <ZLLogger.h>

#include "ZLZipHeader.h"
#include "ZLZDecompressor.h"
#include "../ZLInputStream.h"

const unsigned long ZLZipHeader::SignatureLocalFile = 0x04034B50;
const unsigned long ZLZipHeader::SignatureData = 0x08074B50;

bool ZLZipHeader::readFrom(ZLInputStream &stream) {
	size_t startOffset = stream.offset();
	Signature = readLong(stream);
	switch (Signature) {
		default:
			return false;
		case SignatureLocalFile:
			Version = readShort(stream);
			Flags = readShort(stream);
			CompressionMethod = readShort(stream);
			ModificationTime = readShort(stream);
			ModificationDate = readShort(stream);
			CRC32 = readLong(stream);
			CompressedSize = readLong(stream);
			UncompressedSize = readLong(stream);
			if (CompressionMethod == 0 && CompressedSize != UncompressedSize) {
				ZLLogger::Instance().println("zip", "Different compressed & uncompressed size for stored entry; the uncompressed one will be used.");
				CompressedSize = UncompressedSize;
			}
			NameLength = readShort(stream);
			ExtraLength = readShort(stream);
			return stream.offset() == startOffset + 30 && NameLength != 0;
		case SignatureData:
			CRC32 = readLong(stream);
			CompressedSize = readLong(stream);
			UncompressedSize = readLong(stream);
			NameLength = 0;
			ExtraLength = 0;
			return stream.offset() == startOffset + 16;
	}
}

void ZLZipHeader::skipEntry(ZLInputStream &stream, ZLZipHeader &header) {
	switch (header.Signature) {
		default:
			break;
		case SignatureLocalFile:
			// Only DEFLATED entries can have EXT descriptor
			if (header.CompressionMethod != ZLZipHeader::MethodStored && (header.Flags & 0x08)) {
				stream.seek(header.ExtraLength, false);
				ZLZDecompressor decompressor((size_t)-1);
				size_t size;
				do {
					size = decompressor.decompress(stream, 0, 2048);
					header.UncompressedSize += size;
				} while (size == 2048);
				//stream.seek(16, false);
			} else {
				stream.seek(header.ExtraLength + header.CompressedSize, false);
			}
			break;
	}
}

unsigned short ZLZipHeader::readShort(ZLInputStream &stream) {
#if defined(BYTE_ORDER) && BYTE_ORDER == LITTLE_ENDIAN
	u_int16_t result;
	return (stream.read((char*)&result, 2) == 2) ? result : 0;
#else
	char buffer[2];
	stream.read(buffer, 2);
	return ((((unsigned short)buffer[1]) & 0xFF) << 8) + ((unsigned short)buffer[0] & 0xFF);
#endif
}

unsigned long ZLZipHeader::readLong(ZLInputStream &stream) {
#if defined(BYTE_ORDER) && BYTE_ORDER == LITTLE_ENDIAN
	u_int32_t result;
	return (stream.read((char*)&result, 4) == 4) ? result : 0;
#else
	char buffer[4];
	stream.read(buffer, 4);

	return
		((((unsigned long)buffer[3]) & 0xFF) << 24) +
		((((unsigned long)buffer[2]) & 0xFF) << 16) +
		((((unsigned long)buffer[1]) & 0xFF) << 8) +
		((unsigned long)buffer[0] & 0xFF);
#endif
}
