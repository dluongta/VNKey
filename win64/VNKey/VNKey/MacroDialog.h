
#pragma once
#include "BaseDialog.h"

class MacroDialog :	public BaseDialog {
private:
	HWND listMacro;
	HWND hMacroName, hMacroContent;
	HWND hAddButton, hAutoCaps;
	vector<vector<Uint32>> keys;
	vector<string> macroText;
	vector<string> macroContent;
public:
	MacroDialog(const HINSTANCE & hInstance, const int & resourceId);
	~MacroDialog();
	virtual void fillData() override;
protected:
	INT_PTR eventProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void initDialog();
private:
	void saveAndReload();
	void insertItem(const int& index, LPTSTR macroName, LPTSTR macroContent);

	void onSelectItem(const int& index);
	void onAddMacroButton();
	void onDeleteMacroButton();

	void onImportMacroButton();
	void onExportMacrobutton();

	void onCheckboxClicked(const HWND& hWnd);
};

