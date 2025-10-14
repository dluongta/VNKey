
#pragma once
#include "BaseDialog.h"

class AboutDialog :	public BaseDialog {
protected:
	INT_PTR eventProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void initDialog();
public:
	AboutDialog(const HINSTANCE & hInstance, const int & resourceId);
	~AboutDialog();
	virtual void fillData() override;
private:
	HWND hUpdateButton;
	void onUpdateButton();
};