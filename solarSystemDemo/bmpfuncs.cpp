#include "bmpfuncs.h"

// reads the contents of a 24-bit RGB bitmap file
unsigned char* readBitmapRGBImage(const char *filename, int* widthOut, int* heightOut)
{
	char fileHeader[54];	// to store the file header, bmp file format bmpheader (14 bytes) + bmpheaderinfo (40 bytes) = 54 bytes 
	int width, height;		// width and height of image
	int offset;				// offset where image data starts in the file
	int imageSize;			// image size in bytes
	int padSize;			// in the bmp file format, each row must be a multiple of 4
	int rowSize;			// size per row in bytes
	int i, j;
	char colour[3];
	unsigned char* imageData;	// pointer to store image data

	// open file stream
	ifstream textureFileStream(filename, ios::in | ios::binary);

	// check whether file opened successfully
	if (!textureFileStream.is_open())
	{
		cout << "Failed to open texture file - " << filename << endl;
		return NULL;
	}

	// get file header
	textureFileStream.read(fileHeader, 54);

	// get offset, width and height information
	offset = fileHeader[10];
	width = fileHeader[21] << 24;
	width |= fileHeader[20] << 16;
	width |= fileHeader[19] << 8;
	width |= fileHeader[18];
	width = abs(width);
	height = fileHeader[25] << 24;
	height |= fileHeader[24] << 16;
	height |= fileHeader[23] << 8;
	height |= fileHeader[22];
	height = abs(height);

	// allocate RGB image data
	imageSize = width * height * 3;
	imageData = new unsigned char[imageSize];

	// move read position by offset
	textureFileStream.seekg(offset, ios::beg);
	 
	// compute amount of row padding
	padSize = width % 4;
	if (padSize != 0) {
		padSize = 4 - padSize;
	}

	// bitmaps are stored in upside-down raster order
	rowSize = width * 3;

	// read each RGB pixel colour
	for (i = 0; i < height; i++) {
		for (j = 0; j < rowSize; j += 3) {
			textureFileStream.read(colour, 3);
			imageData[i*rowSize + j] = colour[0];
			imageData[i*rowSize + j + 1] = colour[1];
			imageData[i*rowSize + j + 2] = colour[2];
		}
		// in the bmp format, each row has to be a multiple of 4
		// read in the junk data and throw it away
		for (j = 0; j < padSize; j++) {
			textureFileStream.read(colour, 3);
		}
	}

	// close file stream
	textureFileStream.close();

	// record width and height, and return pointer to image data
	*widthOut = width;
	*heightOut = height;

	return imageData;
}
