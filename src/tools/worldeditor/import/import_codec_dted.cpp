/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#include "pch.hpp"
#include "worldeditor/import/import_codec_dted.hpp"
#include "worldeditor/import/import_image.hpp"


namespace
{
    // This is the header for DT2 files:
    struct DTEDHeader
    {
        char    userHeaderLabel_[  80];
        char    dataDescription_[ 648];
        char    accuracyRecord_ [2700];

        uint32 numRecords() const
        {
            uint32 nRecordsFromHeader;
            char charRecords[5];
            strncpy( charRecords,&userHeaderLabel_[47], 4 );
            charRecords[4] = 0;
            sscanf( charRecords, "%d", &nRecordsFromHeader );
            return nRecordsFromHeader;
        };

        uint32 recordSize() const
        {
            uint32 recordSizeFromHeader;
            char charRecords[5];
            strncpy( charRecords,&userHeaderLabel_[51], 4 );
            charRecords[4] = 0;
            sscanf( charRecords, "%d", &recordSizeFromHeader );
            return recordSizeFromHeader;
        }
    };

    // This is a helper struct to convert from DT2 records to more standard
    // formats.
    class DTEDRecord
    {
    public: 
        int16 asInt16(uint32 index) const
        {
            uint16 b1    = (uint16)data_[index*2 +     prefixBytes()] & 0xff;
            uint16 b2    = (uint16)data_[index*2 + 1 + prefixBytes()] & 0xff;
            int16  value = (int16)(b1<<8 | b2);
            // Take care of null (-32767) and incorrect large -ve values
            // -12000 is global minimum given in spec
            if (value < -12000)
                value = 0;
            return value;
        }

        float asFloat(uint32 index) const
        {
            return (float)asInt16(index);
        }

        //Each row has a sentinel and sequential block count
        static int prefixBytes()    { return    6; }
        static int numEntries()     { return 3601; }

    private:
        char        data_[7214];
    };
}


/**
 *  This function returns true if the extension of filename is "dt2".
 *
 *  @param filename     The name of the file.
 *  @returns            True if the filename ends in "dt2", ignoring case.
 */
/*virtual*/ bool ImportCodecDTED::canHandleFile(std::string const &filename)
{
    return strcmpi("dt2", BWResource::getExtension(filename).c_str()) == 0;
}


/**
 *  This function loads the DT2 file in filename into image.  
 *
 *  @param filename     The file to load.
 *  @param image        The image to read.
 *	@param left			The left coordinate that the image should be used at.
 *	@param top			The top coordinate that the image should be used at.
 *	@param right		The right coordinate that the image should be used at.
 *	@param bottom		The bottom coordinate that the image should be used at.
 *	@param absolute		If true then the values are absolute.
 *  @returns            True if the image could be read, false otherwise.
 */
/*virtual*/ ImportCodec::LoadResult 
ImportCodecDTED::load
(
    std::string         const &filename, 
    ImportImage			&image,
    float               * /*left           = NULL*/,
    float               * /*top            = NULL*/,
    float               * /*right          = NULL*/,
    float               * /*bottom         = NULL*/,
	bool				* /*absolute	   = NULL*/,
    bool                /*loadConfigDlg    = false*/
)
{
    FILE        *file       = NULL;
    bool        worked      = false;
    DTEDRecord  *records    = NULL;
    try
    {
        size_t numRead;

        // Open the file:
        file = fopen(filename.c_str(), "rb");
        if (file == NULL)
            return LR_BAD_FORMAT;

        // Get the size of the file:
        fseek(file, 0, SEEK_END);
        long sz = ftell(file);
        fseek(file, 0, SEEK_SET);

        // Read the header:
        DTEDHeader header;
        numRead = fread(&header, 1, sizeof(header), file);
        if (numRead != sizeof(header))
        {
            fclose(file); file = NULL;
            return LR_BAD_FORMAT;
        }

        unsigned int width  = (unsigned int)((sz - sizeof(header))/sizeof(DTEDRecord));
        unsigned int height = header.recordSize();
        image.resize(width, height);
        records = new DTEDRecord[height];
        for (unsigned int x = 0; x < width; ++x)
        {
            fread(records, height, sizeof(DTEDRecord), file);
            DTEDRecord *r = records;
            for (unsigned int y = 0; y < height; ++y)
            {
				uint16 v = r->asInt16(y) + 32768;
                image.set(x, y, v);
            }
        }

        image.normalize();
        image.flip(true); // x is back to front

        // Cleanup:
        delete[] records; records = NULL;
        fclose(file); file = NULL;
        worked = true;
    }
    catch (...)
    {
        if (file != NULL)
        {
            fclose(file); 
            file = NULL;
        }
        throw;
    }
    return worked ? LR_OK : LR_BAD_FORMAT;
}


/**
 *  This function is where DT2 saving should be implemented if it ever is.
 *  Currently it return false, meaning the save failed.
 *
 *  @param filename     The name of the file to save.
 *  @param image        The image to save.
 *	@param left			The left coordinate that the image should be used at.
 *	@param top			The top coordinate that the image should be used at.
 *	@param right		The right coordinate that the image should be used at.
 *	@param bottom		The bottom coordinate that the image should be used at.
 *	@param absolute		If true then the values are absolute.
 *  @returns            false.
 */
/*virtual*/ bool 
ImportCodecDTED::save
(
    std::string         const &filename, 
    ImportImage			const &image,
    float               * /*left           = NULL*/,
    float               * /*top            = NULL*/,
    float               * /*right          = NULL*/,
    float               * /*bottom         = NULL*/,
	bool				* /*absolute	   = NULL*/,
	float				* /*minVal		   = NULL*/,
	float				* /*maxVal		   = NULL*/ 
)
{
    return false; // not yet implemented
}


/**
 *  This function indicates that we can read DT2 files.
 *
 *  @returns            True.
 */
/*virtual*/ bool ImportCodecDTED::canLoad() const
{
    return true;
}
