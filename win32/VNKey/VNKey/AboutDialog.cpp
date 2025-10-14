
#include "stdafx.h"
#include "AboutDialog.h"
#include "AppDelegate.h"

AboutDialog::AboutDialog(const HINSTANCE & hInstance, const int & resourceId)
	: BaseDialog(hInstance, resourceId) {
}

AboutDialog::~AboutDialog() {
}

void AboutDialog::fillData() {
}

INT_PTR AboutDialog::eventProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
		this->hDlg = hDlg;
		initDialog();
		return TRUE;
	case WM_COMMAND: {
		int wmId = LOWORD(wParam);
		switch (wmId) {
		case IDM_EXIT:
		case IDBUTTON_OK:
			AppDelegate::getInstance()->closeDialog(this);
			break;
		case IDC_BUTTON_CHECK_VERSION:
			onUpdateButton();
			break;
		}
		break;
	}
	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case NM_CLICK:
		case NM_RETURN: {
			PNMLINK link = (PNMLINK)lParam;
			if (link->hdr.idFrom == IDC_SYSLINK_HOME_PAGE)
				ShellExecute(NULL, _T("open"), _T(" "), NULL, NULL, SW_SHOWNORMAL);
			else if (link->hdr.idFrom == IDC_SYSLINK_NEW_VERSION)
				ShellExecute(NULL, _T("open"), _T(" "), NULL, NULL, SW_SHOWNORMAL);
			else if (link->hdr.idFrom == IDC_SYSLINK_FANPAGE)
				ShellExecute(NULL, _T("open"), _T(" "), NULL, NULL, SW_SHOWNORMAL);
		}
		break;
		}
	}
	return FALSE;
}

void AboutDialog::initDialog() {
	//dialog icon
	SET_DIALOG_ICON(IDI_APP_ICON);
	
	hUpdateButton = GetDlgItem(hDlg, IDC_BUTTON_CHECK_VERSION);

	HFONT hFont = CreateFont(48, 0, 0, 0, FW_THIN, FALSE, FALSE, FALSE,
		ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
		DEFAULT_PITCH | FF_SWISS | FF_MODERN, _T("Segoe UI"));
	SendDlgItemMessage(hDlg, IDC_STATIC_APP_TITLE, WM_SETFONT, WPARAM(hFont), TRUE);

	wchar_t buffer[256];
	wsprintfW(buffer, _T("Phiên bản %s cho Windows - Ngày cập nhật: %s"), VNKeyHelper::getVersionString().c_str(), _T(__DATE__));
	SendDlgItemMessage(hDlg, IDC_STATIC_APP_VERSION, WM_SETTEXT, 0, LPARAM(buffer));

	hFont = CreateFont(20, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE,
		ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_SWISS | FF_MODERN, _T("Arial"));
	SendDlgItemMessage(hDlg, IDC_STATIC_APP_SUB_TITLE, WM_SETFONT, WPARAM(hFont), TRUE);
}

void AboutDialog::onUpdateButton() {
	EnableWindow(hUpdateButton, false);
	string newVersion;
	if (VNKeyManager::checkUpdate(newVersion)) {
		WCHAR msg[256];
		wsprintf(msg, 
			TEXT("VNKey Có phiên bản mới (%s), bạn có muốn cập nhật không?"),
			utf8ToWideString(newVersion).c_str());

		int msgboxID = MessageBox(
			hDlg,
			msg,
			_T("VNKey Update"),
			MB_ICONEXCLAMATION | MB_YESNO
		);
		if (msgboxID == IDYES) {
			//Call VNKeyUpdate
			WCHAR path[MAX_PATH];
			GetCurrentDirectory(MAX_PATH, path);
			wsprintf(path, TEXT("%s\\VNKeyUpdate.exe"), path);
			ShellExecute(0, L"", path, 0, 0, SW_SHOWNORMAL);

			AppDelegate::getInstance()->onVNKeyExit();
		}
		
	} else {
		MessageBox(hDlg, _T("Bạn đang dùng phiên bản mới nhất!"), _T("VNKey Update"), MB_OK);
	}
	EnableWindow(hUpdateButton, true);
}