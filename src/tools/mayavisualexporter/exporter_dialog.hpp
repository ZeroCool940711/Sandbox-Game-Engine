/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef _EXPORTER_DIALOG_HPP__
#define _EXPORTER_DIALOG_HPP__

#include <string>
#include "expsets.hpp"

template<typename Dialog, int DlgResource>
class ExporterDialogBase
{
protected:
	HWND hwnd_;
	static Dialog*& instance()
	{
		static Dialog* instance;
		return instance;
	}
	ExporterDialogBase()
	{
		hwnd_ = NULL;
		instance() = (Dialog*)this;
	}
	~ExporterDialogBase()
	{
		instance() = NULL;
	}
public:
	int doModal( HINSTANCE hinst, HWND parent )
	{
		return (int)DialogBox( hinst, MAKEINTRESOURCE( DlgResource ),
			parent, dialogProc );
	}
	virtual void prepareWindow()
	{
		SetWindowText( hwnd_, "Visual Exporter" );

		HWND parent = GetParent( hwnd_ );
		RECT parentRect;
		GetWindowRect( parent, &parentRect );
		int centerX = parentRect.left + ( parentRect.right - parentRect.left ) / 2;
		int centerY = parentRect.top + ( parentRect.bottom - parentRect.top ) / 2;

		RECT rect;
		GetWindowRect( hwnd_, &rect );
		int width = rect.right - rect.left;
		int height = rect.bottom - rect.top;
		MoveWindow( hwnd_, centerX - width / 2, centerY - height / 2, width, height, FALSE );

		setControlStatus();
	}
	void enable( int child, BOOL enable )
	{
		EnableWindow( GetDlgItem( hwnd_, child ) , enable );
	}
	void check( int child, BOOL check )
	{
		SendMessage( GetDlgItem( hwnd_, child ), BM_SETCHECK,
			check ? BST_CHECKED : BST_UNCHECKED, 0 );
	}
	bool checked( int child )
	{
		return SendMessage( GetDlgItem( hwnd_, child ), BM_GETCHECK, 0, 0 )
			== BST_CHECKED;
	}
	virtual void setControlStatus(){};
	virtual INT_PTR processMessage( UINT msg, WPARAM wParam, LPARAM lParam )
	{
		return FALSE;
	}
	static INT_PTR CALLBACK dialogProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
	{
		if( msg == WM_INITDIALOG )
		{
			instance()->hwnd_ = hwnd;
			instance()->prepareWindow();
		}
		return instance()->processMessage( msg, wParam, lParam );
	}
};

class SettingDialog : public ExporterDialogBase<SettingDialog, IDD_SETTINGSDIALOG>
{
	int boneCount_;
	bool disableVisualChecker_;	
	bool useLegacyScaling_;
	bool useLegacyOrientation_;
	bool sceneRootAdded_;
	bool fixCylindrical_;
public:
	SettingDialog()
	{
		boneCount_ = ExportSettings::instance().maxBones();
		disableVisualChecker_ = ExportSettings::instance().disableVisualChecker();
		useLegacyScaling_ = ExportSettings::instance().useLegacyScaling();
		useLegacyOrientation_ = ExportSettings::instance().useLegacyOrientation();
		sceneRootAdded_ = ExportSettings::instance().sceneRootAdded();
		fixCylindrical_ = ExportSettings::instance().fixCylindrical();
	}
	virtual INT_PTR processMessage( UINT msg, WPARAM wParam, LPARAM lParam )
	{
		if( INT_PTR result = ExporterDialogBase<SettingDialog, IDD_SETTINGSDIALOG>::processMessage( msg, wParam, lParam ) )
			return result;
		if( msg == WM_COMMAND )
		{
			if( LOWORD( wParam ) == IDOK )
			{
				gatherSettings();
				boneCount_ = ExportSettings::instance().maxBones();
				disableVisualChecker_ = ExportSettings::instance().disableVisualChecker();
				useLegacyScaling_ = ExportSettings::instance().useLegacyScaling();
				useLegacyOrientation_ = ExportSettings::instance().useLegacyOrientation();
				sceneRootAdded_ = ExportSettings::instance().sceneRootAdded();
				fixCylindrical_ = ExportSettings::instance().fixCylindrical();
				EndDialog( hwnd_, IDOK );
			}
			else if( LOWORD( wParam ) == IDCANCEL )
			{
				ExportSettings::instance().maxBones( boneCount_ );
				ExportSettings::instance().disableVisualChecker( disableVisualChecker_ );
				ExportSettings::instance().useLegacyScaling( useLegacyScaling_ );
				ExportSettings::instance().useLegacyOrientation( useLegacyOrientation_ );
				ExportSettings::instance().sceneRootAdded( sceneRootAdded_ );
				ExportSettings::instance().fixCylindrical( fixCylindrical_ );
				EndDialog( hwnd_, IDCANCEL );
			}
			else if( LOWORD( wParam ) == IDC_VISUALCHECKER || LOWORD( wParam ) == IDC_LEGACY_SCALING ||
				LOWORD( wParam ) == IDC_LEGACY_ORIENTATION || LOWORD( wParam ) == IDC_CYLINDRICAL ||
				LOWORD( wParam ) == IDC_BONE_STEP && HIWORD( wParam ) == BN_KILLFOCUS )
			{
				gatherSettings();
				setControlStatus();
			}
		}
		return FALSE;
	}
	void gatherSettings()
	{
		char num[ 1024 ];
		GetDlgItemText( hwnd_, IDC_BONE_STEP, num, 1024 );
		int boneCount = atoi( num );
		if( boneCount < 3 )
			boneCount = 3;
		if( boneCount > 17 )
			boneCount = 17;
		ExportSettings::instance().maxBones( boneCount );
		ExportSettings::instance().disableVisualChecker( checked( IDC_VISUALCHECKER ) );
		ExportSettings::instance().useLegacyScaling( checked( IDC_LEGACY_SCALING ) );
		ExportSettings::instance().useLegacyOrientation( checked( IDC_LEGACY_ORIENTATION ) );
		ExportSettings::instance().sceneRootAdded( checked( IDC_SCENE_ROOT_ADDED ) );
		ExportSettings::instance().fixCylindrical( checked( IDC_CYLINDRICAL ) );
	}
	virtual void setControlStatus()
	{
		if( ExportSettings::instance().exportAnim() )
		{
			enable( IDC_BONE_STEP_SPIN, FALSE );
			enable( IDC_BONE_STEP, FALSE );
			enable( IDC_VISUALCHECKER, FALSE );
			enable( IDC_LEGACY_SCALING, FALSE );
			enable( IDC_LEGACY_ORIENTATION, TRUE );
			enable( IDC_SCENE_ROOT_ADDED, TRUE );
			enable( IDC_CYLINDRICAL, FALSE );
		}
		else
		{
			enable( IDC_BONE_STEP_SPIN, TRUE );
			enable( IDC_BONE_STEP, TRUE );
			enable( IDC_VISUALCHECKER, TRUE );
			enable( IDC_LEGACY_SCALING, TRUE );
			enable( IDC_LEGACY_ORIENTATION, TRUE );
			enable( IDC_SCENE_ROOT_ADDED, FALSE );
			enable( IDC_CYLINDRICAL, TRUE );
		}

		SendMessage( GetDlgItem( hwnd_, IDC_BONE_STEP_SPIN ), UDM_SETRANGE, 0, MAKELONG( 17, 3 ) );
		SetDlgItemInt( hwnd_, IDC_BONE_STEP, ExportSettings::instance().maxBones(), FALSE );
		check( IDC_VISUALCHECKER, ExportSettings::instance().disableVisualChecker() );
		check( IDC_LEGACY_SCALING, ExportSettings::instance().useLegacyScaling() );
		check( IDC_LEGACY_ORIENTATION, ExportSettings::instance().useLegacyOrientation() );
		check( IDC_SCENE_ROOT_ADDED, ExportSettings::instance().sceneRootAdded() );
		check( IDC_CYLINDRICAL, ExportSettings::instance().fixCylindrical() );
	}
};

class VisualExporterDialog : public ExporterDialogBase<VisualExporterDialog, IDD_VISUALEXPORT_DLG>
{
public:
	virtual INT_PTR processMessage( UINT msg, WPARAM wParam, LPARAM lParam )
	{
		if( INT_PTR result = ExporterDialogBase<VisualExporterDialog, IDD_VISUALEXPORT_DLG>::processMessage( msg, wParam, lParam ) )
			return result;
		if( msg == WM_COMMAND && HIWORD( wParam ) == BN_CLICKED )
		{
			switch( LOWORD( wParam ) )
			{
			case IDOK:
				gatherSettings();
				ExportSettings::instance().writeSettings();
				EndDialog( hwnd_, IDOK );
				break;
			case IDCANCEL:
				ExportSettings::instance().readSettings();
				EndDialog( hwnd_, IDCANCEL );
				break;
			case IDSETTINGS:
				{
					SettingDialog sd;
					sd.doModal( (HINSTANCE)GetWindowLong( hwnd_, GWL_HINSTANCE ), hwnd_ );
				}
				break;
			default:
				gatherSettings();
				setControlStatus();
				break;
			}
		}
		return FALSE;
	}
	void gatherSettings()
	{
		ExportSettings::instance().exportAnim( checked( IDC_ANIMATION ) );

		if( checked( IDC_EXPORT_NORMAL ) )
			ExportSettings::instance().exportMode( ExportSettings::NORMAL );
		else if( checked( IDC_EXPORT_STATIC ) )
			ExportSettings::instance().exportMode( ExportSettings::STATIC );
		else if( checked( IDC_EXPORT_STATIC_WITH_NODES ) )
			ExportSettings::instance().exportMode( ExportSettings::STATIC_WITH_NODES );
		else if( checked( IDC_EXPORT_MESH_PARTICLES ) )
			ExportSettings::instance().exportMode( ExportSettings::MESH_PARTICLES );

		ExportSettings::instance().allowScale( checked( IDC_ALLOWSCALE ) );
		ExportSettings::instance().bumpMapped( checked( IDC_BUMPMAP ) );
		ExportSettings::instance().keepExistingMaterials( checked( IDC_KEEP_EXISTING_MATERIALS ) );
		ExportSettings::instance().referenceNode( checked( IDC_USE_REFERENCE_NODE ) );
		ExportSettings::instance().snapVertices( checked( IDC_SNAP_VERTICES ) );
		ExportSettings::instance().stripRefPrefix( checked( IDC_STRIP_REF_PREFIX ) );
	}
	virtual void setControlStatus()
	{
		SetDlgItemText( hwnd_, IDC_VISUAL_TYPE_ID,
			( "Checking against: " + ExportSettings::instance().visualTypeIdentifier() ).c_str() );
		if( ExportSettings::instance().exportAnim() )
		{
			enable( IDC_VISUAL, TRUE );
			enable( IDC_ANIMATION, TRUE );
			check( IDC_VISUAL, FALSE );
			check( IDC_ANIMATION, TRUE );
			enable( IDC_EXPORT_NORMAL, FALSE );
			enable( IDC_EXPORT_STATIC, FALSE );
			enable( IDC_EXPORT_STATIC_WITH_NODES, FALSE );
			enable( IDC_EXPORT_MESH_PARTICLES, FALSE );
			enable( IDC_ALLOWSCALE, FALSE );
			enable( IDC_BUMPMAP, FALSE );
			enable( IDC_KEEP_EXISTING_MATERIALS, FALSE );
			enable( IDC_SNAP_VERTICES, FALSE );
			enable( IDC_USE_REFERENCE_NODE, TRUE );
			enable( IDC_STRIP_REF_PREFIX, TRUE );
			enable( IDSETTINGS, TRUE );
		}
		else
		{
			enable( IDC_VISUAL, TRUE );
			enable( IDC_ANIMATION, TRUE );
			check( IDC_VISUAL, TRUE );
			check( IDC_ANIMATION, FALSE );
			enable( IDC_EXPORT_NORMAL, TRUE );
			enable( IDC_EXPORT_STATIC, TRUE );
			enable( IDC_EXPORT_STATIC_WITH_NODES, TRUE );
			enable( IDC_EXPORT_MESH_PARTICLES, TRUE );
			enable( IDC_ALLOWSCALE, TRUE );
			enable( IDC_BUMPMAP, TRUE );
			enable( IDC_KEEP_EXISTING_MATERIALS, TRUE );
			enable( IDC_SNAP_VERTICES, TRUE );
			enable( IDC_USE_REFERENCE_NODE, FALSE );
			enable( IDC_STRIP_REF_PREFIX, FALSE );
			enable( IDSETTINGS, TRUE );
		}
		check( IDC_EXPORT_NORMAL, ExportSettings::instance().exportMode() == 0 );
		check( IDC_EXPORT_STATIC, ExportSettings::instance().exportMode() == 1 );
		check( IDC_EXPORT_STATIC_WITH_NODES, ExportSettings::instance().exportMode() == 2 );
		check( IDC_EXPORT_MESH_PARTICLES, ExportSettings::instance().exportMode() == 3 );

		check( IDC_ALLOWSCALE, ExportSettings::instance().allowScale() );
		check( IDC_BUMPMAP, ExportSettings::instance().bumpMapped() );
		check( IDC_KEEP_EXISTING_MATERIALS, ExportSettings::instance().keepExistingMaterials() );
		check( IDC_USE_REFERENCE_NODE, ExportSettings::instance().referenceNode() );
		check( IDC_SNAP_VERTICES, ExportSettings::instance().snapVertices() );
		check( IDC_STRIP_REF_PREFIX, ExportSettings::instance().stripRefPrefix() );
	}
};

#endif//_EXPORTER_DIALOG_HPP__
