
#pragma once
#include "BaseDialog.h"

class ConvertToolDialog : public BaseDialog {
private:
	HWND checkAllCaps, checkNonAllCaps, checkRemoveMark, checkCapsEarchWord, checkCapsFirstLetter, checkAlertWhenCompleted,
		checkConvertClipboard, checkCtrl, checkAlt, checkWin, checkShift, checkKey;
	HWND comboboxSource, comboboxDest;
private:
	void onComboBoxSelected(const HWND& hCombobox, const int& comboboxId);
	void onCheckboxClicked(const HWND& hWnd);
	void onCharacter(const HWND& hWnd, const UINT16& keyCode);
	void setSwitchKeyText(const HWND & hWnd, const UINT16 & keyCode);
	void setLogic(const HWND& exceptWnd);
protected:
	INT_PTR eventProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void initDialog();
public:
	ConvertToolDialog(const HINSTANCE & hInstance, const int & resourceId);
	~ConvertToolDialog();
	virtual void fillData() override;
public: //event
	void onSwitchButton();
	void onConvertButton();
};

