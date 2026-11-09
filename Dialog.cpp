// Dialog.cpp

#include "dialog.h"

HWND hIntroWnd ;

bool Begin = false;
bool Windowed = false;
bool Music = true;
bool Input = true;
bool Debug = true;
unsigned long Width = 800;
unsigned long Height = 600;

//#define FINAL_VERSION

LRESULT CALLBACK IntroDlgProc(HWND hWndDlg, 
							  UINT Msg, 
							  WPARAM wParam, 
							  LPARAM lParam)
{
	switch(Msg)
	{
	case WM_INITDIALOG:
		CheckDlgButton(hWndDlg,IDC_MUSIC,TRUE);
		CheckDlgButton(hWndDlg,IDC_DEBUG,TRUE);
		CheckDlgButton(hWndDlg,IDC_INPUT,TRUE);
		CheckDlgButton(hWndDlg,IDC_RES_800,TRUE);
		return TRUE;

	case WM_COMMAND:
		switch(wParam)
		{
		case IDOK:
			if (IsDlgButtonChecked(hWndDlg, IDC_RES_800) == BST_CHECKED)
			{
				Width = 800;
				Height = 600;
			}
			else
			{
				Width = 640;
				Height = 480;
			}

			if (IsDlgButtonChecked(hWndDlg, IDC_MUSIC) == BST_CHECKED)
			{
				Music = true;
			}
			else
			{
				Music = false;
			}

#ifndef FINAL_VERSION
				Input = true;
#else
				Input = false;
#endif


			if (IsDlgButtonChecked(hWndDlg, IDC_WINDOWED) == BST_CHECKED)
			{
				Windowed = true;
				//Input = false;
			}


			Debug = true;

			Begin = true;

			EndDialog(hWndDlg, 0);
			return TRUE;

		case IDCANCEL:
			EndDialog(hWndDlg, 0);
			return TRUE;
		}
		break;


	}

	return FALSE;
}
