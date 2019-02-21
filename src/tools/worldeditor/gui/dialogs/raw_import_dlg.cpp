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
#include "worldeditor/gui/dialogs/raw_import_dlg.hpp"
#include "common/format.hpp"


using namespace controls;


BEGIN_MESSAGE_MAP(RawImportDlg, CDialog)
    ON_CBN_SELCHANGE(IDC_RAWIMP_SIZE        , OnUpdateImage)
    ON_BN_CLICKED   (IDC_RAWIMP_LITTLEENDIAN, OnUpdateImage)
    ON_BN_CLICKED   (IDC_RAWIMP_BIGENDIAN   , OnUpdateImage)
END_MESSAGE_MAP()


/*explicit*/ RawImportDlg::RawImportDlg(char const *filename):
	CDialog(IDD),
	filename_(filename),
	data_(NULL),
	dataSize_(0),
	selWidth_(0),
	selHeight_(0),
	littleEndian_(true)
{
    CWaitCursor waitCursor; // this may take a little while

    // Read the RAW file into a buffer:
    FILE *file = NULL;
    try
    {
        file = fopen(filename_.c_str(), "rb");
        if (file != NULL)
        {
            fseek(file, 0, SEEK_END);
            dataSize_ = ftell(file);
            fseek(file, 0, SEEK_SET);
            data_ = new uint8[dataSize_];
            fread(data_, sizeof(uint8), dataSize_, file);
            fclose(file);
        }
    }
    catch (...)
    {
        fclose(file); file = NULL;
        delete[] data_; data_ = NULL;
        throw;
    }
}


/*virtual*/ RawImportDlg::~RawImportDlg()
{
    delete[] data_; data_ = NULL;
}


void RawImportDlg::getResult
(
    unsigned int    &width, 
    unsigned int    &height, 
    bool            &littleEndian
) const
{
    width           = selWidth_;
    height          = selHeight_;
    littleEndian    = littleEndian_;
}


/*virtual*/ BOOL RawImportDlg::OnInitDialog()
{
    BOOL result = CDialog::OnInitDialog();
    if (result != FALSE)
    {
        filenameEdit_.SetWindowText(filename_.c_str());

        // Add the prime factors of dataSize/2 into the size array:
        size_t numWords = dataSize_/2;
        for (size_t i = 1; i <= numWords; ++i)
        {
            if (numWords%i == 0)
            {
                std::string sizeStr = 
                    sformat("{0} x {1}", i, numWords/i);
                int idx = sizeCB_.AddString(sizeStr.c_str());
                // Store the width in the item data.  The height is easy to
                // calculate from this.
                sizeCB_.SetItemData(idx, i);
            }
        }
        sizeCB_.SetCurSel(sizeCB_.GetCount()/2); // roughly the sqrt posn.

        bmpImage_.subclass(IDC_RAWIMP_BMP, this);
        bmpImage_.borderPadding(2);

        littleEndianButton_.SetCheck(BST_CHECKED);

        OnUpdateImage();

        INIT_AUTO_TOOLTIP();
    }
    return result;
}


/*virtual*/ void RawImportDlg::DoDataExchange(CDataExchange *dx)
{
    DDX_Control(dx, IDC_RAWIMP_FILENAME    , filenameEdit_      );
    DDX_Control(dx, IDC_RAWIMP_FILENAME    , filenameEdit_      );
    DDX_Control(dx, IDC_RAWIMP_SIZE        , sizeCB_            );
    DDX_Control(dx, IDC_RAWIMP_LITTLEENDIAN, littleEndianButton_);
    DDX_Control(dx, IDC_RAWIMP_BIGENDIAN   , bigEndianButton_   );
}


/*virtual*/ void RawImportDlg::OnOK()
{
    int idx = sizeCB_.GetCurSel();

    selWidth_     = sizeCB_.GetItemData(idx);
    selHeight_    = (dataSize_/2)/selWidth_;
    littleEndian_ = (littleEndianButton_.GetCheck() == BST_CHECKED);

    CDialog::OnOK();
}


void RawImportDlg::OnUpdateImage()
{
    CWaitCursor waitCursor; // this may take a little while

    int          idx    = sizeCB_.GetCurSel();
    unsigned int width  = sizeCB_.GetItemData(idx);
    unsigned int height = (dataSize_/2)/width;
    DibSection8  &image = bmpImage_.image();
    image.resize(width, height);
    if (littleEndianButton_.GetCheck() == BST_CHECKED)
    {
        // Find the range of the data:
        uint16 minv = 65535;
        uint16 maxv =     0;
        for (unsigned int i = 0; i < image.height(); ++i)
        {
            uint16 *q = (uint16 *)data_ + i*width;
            for (unsigned int j = 0; j < image.width(); ++j, ++q)
            {
                minv = std::min(minv, *q);
                maxv = std::max(maxv, *q);
            }
        }
        // Draw the normalized data:
        for (unsigned int i = 0; i < image.height(); ++i)
        {
            uint8    *p = image.getRow(i);
            uint16   *q = (uint16 *)data_ + i*width;
            for (unsigned int j = 0; j < image.width(); ++j, ++q, ++p)
            {
                *p = (uint8)Math::lerp(*q, minv, maxv, 0, 255);
            }
        }
    }
    else
    {
        // Find the range of the data:
        uint16 minv = 65535;
        uint16 maxv =     0;
        for (unsigned int i = 0; i < image.height(); ++i)
        {
            uint16 *q = (uint16 *)data_ + i*width;
            for (unsigned int j = 0; j < image.width(); ++j, ++q)
            {
                uint16 value = *q;
                value = ((value & 0xff) << 8) + (value >> 8);
                minv = std::min(minv, value);
                maxv = std::max(maxv, value);
            }
        }
        // Draw the normalized data:
        for (unsigned int i = 0; i < image.height(); ++i)
        {
            uint8    *p = image.getRow(i);
            uint16   *q = (uint16 *)data_ + i*width;
            for (unsigned int j = 0; j < image.width(); ++j, ++q, ++p)
            {
                uint16 value = *q;
                value = ((value & 0xff) << 8) + (value >> 8);
                uint8 grey = (uint8)Math::lerp(value, minv, maxv, 0, 255);
                *p = grey;
            }
        }
    }
    bmpImage_.Invalidate();
    bmpImage_.RedrawWindow();
}
