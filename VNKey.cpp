#ifndef WINVER
#define WINVER 0x0600
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <cwctype>
#include <cstring>
#include <cmath>

// Tự động liên kết thư viện khi biên dịch bằng Visual Studio (MSVC)
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")

using namespace std;

// Định nghĩa các hằng số giao diện System Tray
#define WM_TRAYICON         (WM_USER + 100)
#define ID_TRAY_TOGGLE_VE   1001
#define ID_TRAY_MODERN_TONE 1002
#define ID_TRAY_CHROME_FIX  1003
#define ID_TRAY_EXIT        1004

// Biến toàn cục quản lý trạng thái
HHOOK g_hKeyboardHook = NULL;
HHOOK g_hMouseHook = NULL;
HWND  g_hWnd = NULL;
NOTIFYICONDATAW g_nid = {0};

HICON g_hIconV = NULL;       // Icon nhỏ siêu nét (System Tray & góc trái trên thông báo cạnh VNKey.exe)
HICON g_hIconE = NULL;       // Icon nhỏ siêu nét (System Tray)
HICON g_hIconLargeV = NULL;  // Icon lớn 64x64 siêu nét (Bên trong nội dung thông báo)
HICON g_hIconLargeE = NULL;  // Icon lớn 64x64 siêu nét

bool g_isVietnamese = true;     // true = Tiếng Việt (V), false = Tiếng Anh (E)
bool g_modernTone = false;      // false = hòa, thúy (kiểu cũ) | true = hoà, thuý (kiểu mới)
bool g_chromeFix = true;        // Sửa lỗi gợi ý thanh địa chỉ trình duyệt Chromium/Firefox
bool g_ctrlShiftPrimed = false; // Theo dõi trạng thái nhấn Ctrl + Shift

// Cấu trúc lưu trạng thái của từ đang gõ
struct WordState
{
    wstring display;              // Chuỗi đang hiển thị trên màn hình
    bool disabled = false;        // Đã hủy bỏ dấu (khi gõ lặp phím aa -> a, off -> of)
    bool lastStandaloneW = false; // Đánh dấu chữ 'ư' cuối cùng được tạo bởi phím 'w' đơn lẻ

    void Clear()
    {
        display.clear();
        disabled = false;
        lastStandaloneW = false;
    }
} g_word;

// Bảng 12 nguyên âm (0..11: thường, 12..23: HOA) x 6 thanh điệu (0: ngang, 1: sắc, 2: huyền, 3: hỏi, 4: ngã, 5: nặng)
// Nhóm nguyên âm: 0:a, 1:ă, 2:â, 3:e, 4:ê, 5:i, 6:o, 7:ô, 8:ơ, 9:u, 10:ư, 11:y
const wchar_t VOWEL_TABLE[24][6] = {
    {L'a', L'\x00E1', L'\x00E0', L'\x1EA3', L'\x00E3', L'\x1EA1'},      // 0: a
    {L'\x0103', L'\x1EAF', L'\x1EB1', L'\x1EB3', L'\x1EB5', L'\x1EB7'}, // 1: ă
    {L'\x00E2', L'\x1EA5', L'\x1EA7', L'\x1EA9', L'\x1EAB', L'\x1EAD'}, // 2: â
    {L'e', L'\x00E9', L'\x00E8', L'\x1EBB', L'\x1EBD', L'\x1EB9'},      // 3: e
    {L'\x00EA', L'\x1EBF', L'\x1EC1', L'\x1EC3', L'\x1EC5', L'\x1EC7'}, // 4: ê
    {L'i', L'\x00ED', L'\x00EC', L'\x1EC9', L'\x0129', L'\x1ECB'},      // 5: i
    {L'o', L'\x00F3', L'\x00F2', L'\x1ECF', L'\x00F5', L'\x1ECD'},      // 6: o
    {L'\x00F4', L'\x1ED1', L'\x1ED3', L'\x1ED5', L'\x1ED7', L'\x1ED9'}, // 7: ô
    {L'\x01A1', L'\x1EDB', L'\x1EDD', L'\x1EDF', L'\x1EE1', L'\x1EE3'}, // 8: ơ
    {L'u', L'\x00FA', L'\x00F9', L'\x1EE7', L'\x0169', L'\x1EE5'},      // 9: u
    {L'\x01B0', L'\x1EE9', L'\x1EEB', L'\x1EED', L'\x1EEF', L'\x1EF1'}, // 10: ư
    {L'y', L'\x00FD', L'\x1EF3', L'\x1EF7', L'\x1EF9', L'\x1EF5'},      // 11: y

    // Chữ HOA (12..23)
    {L'A', L'\x00C1', L'\x00C0', L'\x1EA2', L'\x00C3', L'\x1EA0'},
    {L'\x0102', L'\x1EAE', L'\x1EB0', L'\x1EB2', L'\x1EB4', L'\x1EB6'},
    {L'\x00C2', L'\x1EA4', L'\x1EA6', L'\x1EA8', L'\x1EAA', L'\x1EAC'},
    {L'E', L'\x00C9', L'\x00C8', L'\x1EBA', L'\x1EBC', L'\x1EB8'},
    {L'\x00CA', L'\x1EBE', L'\x1EC0', L'\x1EC2', L'\x1EC4', L'\x1EC6'},
    {L'I', L'\x00CD', L'\x00CC', L'\x1EC8', L'\x0128', L'\x1ECA'},
    {L'O', L'\x00D3', L'\x00D2', L'\x1ECE', L'\x00D5', L'\x1ECC'},
    {L'\x00D4', L'\x1ED0', L'\x1ED2', L'\x1ED4', L'\x1ED6', L'\x1ED8'},
    {L'\x01A0', L'\x1EDA', L'\x1EDC', L'\x1EDE', L'\x1EE0', L'\x1EE2'},
    {L'U', L'\x00DA', L'\x00D9', L'\x1EE6', L'\x0168', L'\x1EE4'},
    {L'\x01AF', L'\x1EE8', L'\x1EEA', L'\x1EEC', L'\x1EEE', L'\x1EF0'},
    {L'Y', L'\x00DD', L'\x1EF2', L'\x1EF6', L'\x1EF8', L'\x1EF4'}
};

// Tra cứu nguyên âm trong bảng
bool GetVowelInfo(wchar_t ch, int &row, int &tone)
{
    for (int r = 0; r < 24; r++)
    {
        for (int t = 0; t < 6; t++)
        {
            if (VOWEL_TABLE[r][t] == ch)
            {
                row = r;
                tone = t;
                return true;
            }
        }
    }
    return false;
}

// Kiểm tra cửa sổ hiện tại có phải là trình duyệt (Chrome, Edge, Brave, Cốc Cốc, Firefox...)
bool IsBrowserWindow()
{
    HWND fg = GetForegroundWindow();
    if (!fg)
        return false;
    wchar_t className[64] = {0};
    GetClassNameW(fg, className, 63);
    return (wcscmp(className, L"Chrome_WidgetWin_1") == 0 ||
            wcscmp(className, L"MozillaWindowClass") == 0);
}

// Phát chuỗi phím xóa lùi (Backspace) và chèn chuỗi Unicode mới
void SendReplacement(int backspaceCount, const wstring &newText)
{
    vector<INPUT> inputs;

    // Sửa lỗi bôi đen gợi ý (Autocomplete) trên thanh địa chỉ trình duyệt
    if (g_chromeFix && backspaceCount > 0 && IsBrowserWindow())
    {
        INPUT zwDown = {0};
        zwDown.type = INPUT_KEYBOARD;
        zwDown.ki.wScan = 0x200C;
        zwDown.ki.dwFlags = KEYEVENTF_UNICODE;
        inputs.push_back(zwDown);

        INPUT zwUp = {0};
        zwUp.type = INPUT_KEYBOARD;
        zwUp.ki.wScan = 0x200C;
        zwUp.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
        inputs.push_back(zwUp);

        backspaceCount += 1;
    }

    for (int i = 0; i < backspaceCount; i++)
    {
        INPUT bsDown = {0};
        bsDown.type = INPUT_KEYBOARD;
        bsDown.ki.wVk = VK_BACK;
        inputs.push_back(bsDown);

        INPUT bsUp = {0};
        bsUp.type = INPUT_KEYBOARD;
        bsUp.ki.wVk = VK_BACK;
        bsUp.ki.dwFlags = KEYEVENTF_KEYUP;
        inputs.push_back(bsUp);
    }

    for (wchar_t ch : newText)
    {
        INPUT chDown = {0};
        chDown.type = INPUT_KEYBOARD;
        chDown.ki.wScan = ch;
        chDown.ki.dwFlags = KEYEVENTF_UNICODE;
        inputs.push_back(chDown);

        INPUT chUp = {0};
        chUp.type = INPUT_KEYBOARD;
        chUp.ki.wScan = ch;
        chUp.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
        inputs.push_back(chUp);
    }

    if (!inputs.empty())
    {
        SendInput((UINT)inputs.size(), inputs.data(), sizeof(INPUT));
    }
}

// Tìm vị trí nguyên âm chuẩn cần đặt dấu thanh trong từ
int FindToneTargetIndex(const wstring &word)
{
    vector<int> vowelIndices;
    int len = (int)word.length();

    for (int i = 0; i < len; i++)
    {
        int r, t;
        if (GetVowelInfo(word[i], r, t))
        {
            int base = r % 12;
            // Bỏ qua 'u' trong cụm 'qu' nếu phía sau còn nguyên âm
            if (i > 0 && (word[i - 1] == L'q' || word[i - 1] == L'Q') && base == 9)
            {
                if (i + 1 < len)
                {
                    int nr, nt;
                    if (GetVowelInfo(word[i + 1], nr, nt))
                        continue;
                }
            }
            // Bỏ qua 'i' trong cụm 'gi' nếu phía sau còn nguyên âm
            if (i > 0 && (word[i - 1] == L'g' || word[i - 1] == L'G') && base == 5)
            {
                if (i + 1 < len)
                {
                    int nr, nt;
                    if (GetVowelInfo(word[i + 1], nr, nt))
                        continue;
                }
            }
            vowelIndices.push_back(i);
        }
    }

    if (vowelIndices.empty())
        return -1;
    if (vowelIndices.size() == 1)
        return vowelIndices[0];

    // Ưu tiên cặp 'ươ': dấu luôn nằm ở 'ơ'
    for (size_t k = 0; k + 1 < vowelIndices.size(); k++)
    {
        int r1, t1, r2, t2;
        GetVowelInfo(word[vowelIndices[k]], r1, t1);
        GetVowelInfo(word[vowelIndices[k + 1]], r2, t2);
        if ((r1 % 12) == 10 && (r2 % 12) == 8)
        {
            return vowelIndices[k + 1];
        }
    }

    // Ưu tiên nguyên âm mang dấu mũ/móc/trăng: ă(1), â(2), ê(4), ô(7), ơ(8), ư(10)
    for (int idx : vowelIndices)
    {
        int r, t;
        GetVowelInfo(word[idx], r, t);
        int b = r % 12;
        if (b == 1 || b == 2 || b == 4 || b == 7 || b == 8 || b == 10)
        {
            return idx;
        }
    }

    // Nếu có 3 nguyên âm (oai, uya, uyên...) -> đặt ở nguyên âm giữa
    if (vowelIndices.size() >= 3)
    {
        return vowelIndices[1];
    }

    // Có 2 nguyên âm: nếu có phụ âm cuối (toan, hoang...) -> đặt ở nguyên âm thứ 2
    bool hasEndingConsonant = (vowelIndices.back() < len - 1);
    if (hasEndingConsonant)
    {
        return vowelIndices[1];
    }

    // Âm tiết mở có 2 nguyên âm: kiểm tra oa, oe, uy theo kiểu bỏ dấu mới/cũ
    int r1, t1, r2, t2;
    GetVowelInfo(word[vowelIndices[0]], r1, t1);
    GetVowelInfo(word[vowelIndices[1]], r2, t2);
    int b1 = r1 % 12, b2 = r2 % 12;
    if ((b1 == 6 && b2 == 0) || (b1 == 6 && b2 == 3) || (b1 == 9 && b2 == 11))
    {
        return g_modernTone ? vowelIndices[1] : vowelIndices[0];
    }

    return vowelIndices[0];
}

// Tự động cân chỉnh lại vị trí dấu thanh trong từ (nếu từ đã có dấu thanh)
void RepositionTone(wstring &tempWord, int &earliestModPos)
{
    int existingTone = 0;
    int oldToneIdx = -1;

    for (int i = 0; i < (int)tempWord.length(); i++)
    {
        int r, t;
        if (GetVowelInfo(tempWord[i], r, t) && t > 0)
        {
            existingTone = t;
            oldToneIdx = i;
            break;
        }
    }

    if (existingTone > 0 && oldToneIdx != -1)
    {
        int newToneIdx = FindToneTargetIndex(tempWord);
        if (newToneIdx != -1 && newToneIdx != oldToneIdx)
        {
            int rOld, tOld, rNew, tNew;
            GetVowelInfo(tempWord[oldToneIdx], rOld, tOld);
            GetVowelInfo(tempWord[newToneIdx], rNew, tNew);
            tempWord[oldToneIdx] = VOWEL_TABLE[rOld][0];
            tempWord[newToneIdx] = VOWEL_TABLE[rNew][existingTone];

            int firstChange = min(oldToneIdx, newToneIdx);
            if (earliestModPos == -1 || firstChange < earliestModPos)
            {
                earliestModPos = firstChange;
            }
        }
    }
}

// Áp dụng thay đổi từ tempWord ra màn hình
void ApplyWordChange(const wstring &tempWord, int modPos)
{
    int bsCount = (int)g_word.display.length() - modPos;
    wstring newSuffix = tempWord.substr(modPos);
    g_word.display = tempWord;
    SendReplacement(bsCount, newSuffix);
}

// Xử lý phím chữ cái A-Z; trả về true nếu bộ gõ đã chặn và xử lý phím đó
bool ProcessKey(wchar_t ch)
{
    if (g_word.disabled)
    {
        g_word.display.push_back(ch);
        return false;
    }

    wchar_t lowerKey = towlower(ch);
    wstring temp = g_word.display;
    int len = (int)temp.length();

    // 1. Xử lý phím 'd' / 'D'
    if (lowerKey == L'd' && len > 0)
    {
        if (temp[0] == L'd' || temp[0] == L'D')
        {
            temp[0] = (temp[0] == L'd') ? L'\x0111' : L'\x0110';
            g_word.lastStandaloneW = false;
            ApplyWordChange(temp, 0);
            return true;
        }
        if (len == 1 && (temp[0] == L'\x0111' || temp[0] == L'\x0110'))
        {
            temp[0] = (temp[0] == L'\x0111') ? L'd' : L'D';
            temp.push_back(ch);
            g_word.disabled = true;
            ApplyWordChange(temp, 0);
            return true;
        }
    }

    // 2. Xử lý phím mũ: 'a' (aa -> â), 'e' (ee -> ê), 'o' (oo -> ô)
    if ((lowerKey == L'a' || lowerKey == L'e' || lowerKey == L'o') && len > 0)
    {
        int targetBase = (lowerKey == L'a') ? 0 : (lowerKey == L'e' ? 3 : 6);
        int circumBase = (lowerKey == L'a') ? 2 : (lowerKey == L'e' ? 4 : 7);

        for (int i = len - 1; i >= 0; i--)
        {
            int r, t;
            if (GetVowelInfo(temp[i], r, t))
            {
                int base = r % 12;
                int upperOffset = (r >= 12) ? 12 : 0;

                if (base == targetBase)
                {
                    temp[i] = VOWEL_TABLE[circumBase + upperOffset][t];
                    int modPos = i;
                    RepositionTone(temp, modPos);
                    g_word.lastStandaloneW = false;
                    ApplyWordChange(temp, modPos);
                    return true;
                }
                if (base == circumBase)
                {
                    temp[i] = VOWEL_TABLE[targetBase + upperOffset][t];
                    temp.push_back(ch);
                    g_word.disabled = true;
                    g_word.lastStandaloneW = false;
                    ApplyWordChange(temp, i);
                    return true;
                }
            }
        }
    }

    // 3. Xử lý phím 'w' / 'W'
    if (lowerKey == L'w')
    {
        if (g_word.lastStandaloneW && len > 0)
        {
            int r, t;
            if (GetVowelInfo(temp.back(), r, t) && (r % 12) == 10)
            {
                temp.back() = (r >= 12) ? L'W' : L'w';
                g_word.disabled = true;
                g_word.lastStandaloneW = false;
                ApplyWordChange(temp, len - 1);
                return true;
            }
        }

        for (int i = len - 2; i >= 0; i--)
        {
            int r1, t1, r2, t2;
            if (GetVowelInfo(temp[i], r1, t1) && GetVowelInfo(temp[i + 1], r2, t2))
            {
                int b1 = r1 % 12, b2 = r2 % 12;
                int u1 = (r1 >= 12) ? 12 : 0, u2 = (r2 >= 12) ? 12 : 0;

                bool isQu = (i > 0 && (temp[i - 1] == L'q' || temp[i - 1] == L'Q'));
                if (!isQu)
                {
                    if ((b1 == 9 || b1 == 10) && (b2 == 6 || b2 == 8) && !(b1 == 10 && b2 == 8))
                    {
                        int combinedTone = (t1 > 0) ? t1 : t2;
                        temp[i] = VOWEL_TABLE[10 + u1][0];
                        temp[i + 1] = VOWEL_TABLE[8 + u2][combinedTone];
                        int modPos = i;
                        RepositionTone(temp, modPos);
                        g_word.lastStandaloneW = false;
                        ApplyWordChange(temp, modPos);
                        return true;
                    }
                    if (b1 == 10 && b2 == 8)
                    {
                        temp[i] = VOWEL_TABLE[9 + u1][t1];
                        temp[i + 1] = VOWEL_TABLE[6 + u2][t2];
                        temp.push_back(ch);
                        g_word.disabled = true;
                        g_word.lastStandaloneW = false;
                        ApplyWordChange(temp, i);
                        return true;
                    }
                }
            }
        }

        for (int i = len - 1; i >= 0; i--)
        {
            int r, t;
            if (GetVowelInfo(temp[i], r, t))
            {
                int b = r % 12;
                int u = (r >= 12) ? 12 : 0;

                if (b == 0 || b == 6 || b == 9)
                {
                    int newBase = (b == 0) ? 1 : (b == 6 ? 8 : 10);
                    temp[i] = VOWEL_TABLE[newBase + u][t];
                    int modPos = i;
                    RepositionTone(temp, modPos);
                    g_word.lastStandaloneW = false;
                    ApplyWordChange(temp, modPos);
                    return true;
                }
                if (b == 1 || b == 8 || b == 10)
                {
                    int origBase = (b == 1) ? 0 : (b == 8 ? 6 : 9);
                    temp[i] = VOWEL_TABLE[origBase + u][t];
                    temp.push_back(ch);
                    g_word.disabled = true;
                    g_word.lastStandaloneW = false;
                    ApplyWordChange(temp, i);
                    return true;
                }
            }
        }

        wchar_t uHorn = iswupper(ch) ? VOWEL_TABLE[22][0] : VOWEL_TABLE[10][0];
        temp.push_back(uHorn);
        g_word.lastStandaloneW = true;
        ApplyWordChange(temp, len);
        return true;
    }

    // 4. Xử lý phím dấu thanh: s(1), f(2), r(3), x(4), j(5), z(0)
    if ((lowerKey == L's' || lowerKey == L'f' || lowerKey == L'r' ||
         lowerKey == L'x' || lowerKey == L'j' || lowerKey == L'z') &&
        len > 0)
    {
        int targetTone = 0;
        if (lowerKey == L's')
            targetTone = 1;
        else if (lowerKey == L'f')
            targetTone = 2;
        else if (lowerKey == L'r')
            targetTone = 3;
        else if (lowerKey == L'x')
            targetTone = 4;
        else if (lowerKey == L'j')
            targetTone = 5;
        else if (lowerKey == L'z')
            targetTone = 0;

        int targetIdx = FindToneTargetIndex(temp);
        if (targetIdx != -1)
        {
            int r, currentTone;
            GetVowelInfo(temp[targetIdx], r, currentTone);

            if (targetTone == 0)
            {
                if (currentTone > 0)
                {
                    temp[targetIdx] = VOWEL_TABLE[r][0];
                    g_word.lastStandaloneW = false;
                    ApplyWordChange(temp, targetIdx);
                    return true;
                }
            }
            else
            {
                if (currentTone == targetTone)
                {
                    temp[targetIdx] = VOWEL_TABLE[r][0];
                    temp.push_back(ch);
                    g_word.disabled = true;
                    g_word.lastStandaloneW = false;
                    ApplyWordChange(temp, targetIdx);
                    return true;
                }
                else
                {
                    temp[targetIdx] = VOWEL_TABLE[r][targetTone];
                    g_word.lastStandaloneW = false;
                    ApplyWordChange(temp, targetIdx);
                    return true;
                }
            }
        }
    }

    // 5. Khi gõ chữ cái bình thường vào cuối từ
    temp.push_back(ch);
    g_word.lastStandaloneW = false;
    int modPos = -1;

    if (len >= 2)
    {
        int r1, t1, r2, t2;
        if (GetVowelInfo(temp[len - 2], r1, t1) && GetVowelInfo(temp[len - 1], r2, t2))
        {
            if ((r1 % 12) == 10 && (r2 % 12) == 6)
            {
                int u2 = (r2 >= 12) ? 12 : 0;
                int combinedTone = (t1 > 0) ? t1 : t2;
                temp[len - 2] = VOWEL_TABLE[r1][0];
                temp[len - 1] = VOWEL_TABLE[8 + u2][combinedTone];
                modPos = len - 2;
            }
        }
    }

    RepositionTone(temp, modPos);

    if (modPos != -1)
    {
        ApplyWordChange(temp, modPos);
        return true;
    }

    g_word.display = temp;
    return false;
}

// Tạo Icon chữ V (đỏ) và chữ E (xanh) với độ phân giải tùy biến (hỗ trợ cả bản nhỏ 16..32px và bản lớn 64px)
// Sử dụng lấy mẫu siêu phân giải SSAA 16x16 (256 mẫu/pixel) + Bù sáng Gamma để nét chéo chữ V liền mạch, tuyệt đối không răng cưa
HICON CreateTrayIcon(bool isViet, int size)
{
    if (size < 16) size = 16;
    if (size > 64) size = 64;

    vector<DWORD> pixels(size * size, 0);
    // Cấp phát mảng mask chuẩn 16-bit aligned cho mọi kích thước icon
    int maskRowBytes = ((size + 15) / 16) * 2;
    vector<BYTE> andMask(maskRowBytes * size, 0);

    const int subSamples = 16; // 16x16 = 256 điểm lấy mẫu trên mỗi pixel
    const float invSub = 1.0f / (float)subSamples;
    const float normScale = 64.0f / (float)size;

    // Tăng nhẹ độ dày nét khi ở kích thước nhỏ (16x16..24x24) để nét chữ V đanh, liền và rõ như icon lớn
    const float vHalfWidthBase = (size <= 28) ? 5.25f : 4.6f;

    for (int y = 0; y < size; y++)
    {
        for (int x = 0; x < size; x++)
        {
            int fgHits = 0;
            int bgHits = 0;

            for (int sy = 0; sy < subSamples; sy++)
            {
                // Quy đổi tọa độ pixel hiện tại về hệ tọa độ chuẩn 64x64
                float py = (y + (sy + 0.5f) * invSub) * normScale;

                for (int sx = 0; sx < subSamples; sx++)
                {
                    float px = (x + (sx + 0.5f) * invSub) * normScale;

                    // Tạo khung hình vuông bo góc mượt (bán kính góc bo R = 10)
                    float dx = 0.0f, dy = 0.0f;
                    if (px < 10.0f)
                        dx = 10.0f - px;
                    else if (px > 54.0f)
                        dx = px - 54.0f;
                    if (py < 10.0f)
                        dy = 10.0f - py;
                    else if (py > 54.0f)
                        dy = py - 54.0f;

                    if (dx * dx + dy * dy <= 100.0f)
                    {
                        bgHits++;
                        bool inText = false;

                        if (isViet)
                        {
                            // Vẽ chữ V cân đối, sắc sảo (cao từ y=14 đến y=50)
                            if (py >= 14.0f && py <= 50.0f)
                            {
                                float t = (py - 14.0f) / 36.0f;
                                float leftCenter = 15.5f + t * 16.5f;  // Nét trái kéo xuống tâm (32)
                                float rightCenter = 48.5f - t * 16.5f; // Nét phải kéo xuống tâm (32)
                                float halfWidth = vHalfWidthBase - t * 0.35f;

                                float dL = px - leftCenter;
                                if (dL < 0) dL = -dL;
                                float dR = px - rightCenter;
                                if (dR < 0) dR = -dR;

                                if (dL <= halfWidth || dR <= halfWidth)
                                {
                                    inText = true;
                                }
                            }
                        }
                        else
                        {
                            // Vẽ chữ E: Cột dọc + 3 gạch ngang dày bằng nhau
                            if (py >= 14.0f && py <= 50.0f)
                            {
                                if (px >= 17.0f && px <= 25.0f)
                                    inText = true; // Cột dọc
                                else if (px >= 25.0f && px <= 47.0f && py <= 21.5f)
                                    inText = true; // Thanh ngang trên
                                else if (px >= 25.0f && px <= 47.0f && py >= 28.25f && py <= 35.75f)
                                    inText = true; // Thanh ngang giữa
                                else if (px >= 25.0f && px <= 47.0f && py >= 42.5f)
                                    inText = true; // Thanh ngang dưới
                            }
                        }

                        if (inText)
                            fgHits++;
                    }
                }
            }

            if (bgHits == 0)
            {
                pixels[y * size + x] = 0x00000000;
            }
            else
            {
                float rawTextAlpha = (float)fgHits / (float)bgHits;
                // Bù sáng Gamma (Gamma Correction) cho biên chữ trắng trên nền đỏ/xanh
                // Giúp triệt tiêu hoàn toàn hiệu ứng thắt cổ chai gây cảm giác "lưỡi cưa" ở kích thước nhỏ
                float textAlpha = (size <= 28) ? powf(rawTextAlpha, 0.62f) : powf(rawTextAlpha, 0.80f);
                float boxAlpha = (float)bgHits / (float)(subSamples * subSamples);

                int bgR = isViet ? 215 : 25;
                int bgG = isViet ? 30 : 110;
                int bgB = isViet ? 35 : 215;

                // Hòa trộn màu với Premultiplied Alpha
                int r = (int)((255.0f * textAlpha + bgR * (1.0f - textAlpha)) * boxAlpha + 0.5f);
                int g = (int)((255.0f * textAlpha + bgG * (1.0f - textAlpha)) * boxAlpha + 0.5f);
                int b = (int)((255.0f * textAlpha + bgB * (1.0f - textAlpha)) * boxAlpha + 0.5f);
                int a = (int)(255.0f * boxAlpha + 0.5f);

                if (r > 255) r = 255;
                if (g > 255) g = 255;
                if (b > 255) b = 255;
                if (a > 255) a = 255;

                pixels[y * size + x] = ((DWORD)a << 24) | ((DWORD)r << 16) | ((DWORD)g << 8) | (DWORD)b;
            }
        }
    }

    return CreateIcon(GetModuleHandleW(NULL), size, size, 1, 32, andMask.data(), (const BYTE *)pixels.data());
}

// Cập nhật trạng thái icon dưới System Tray
void UpdateTrayIcon()
{
    g_nid.uFlags = NIF_ICON | NIF_TIP; // Chỉ cập nhật Icon và Tooltip để không kích hoạt lại Balloon
    g_nid.hIcon = g_isVietnamese ? g_hIconV : g_hIconE;
    wcscpy_s(g_nid.szTip, g_isVietnamese
                              ? L"VNKey: Tiếng Việt (Ctrl+Shift)"
                              : L"VNKey: Tiếng Anh (Ctrl+Shift)");
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

void ToggleVietnameseMode()
{
    g_isVietnamese = !g_isVietnamese;
    g_word.Clear();
    UpdateTrayIcon();
}

// Hook bàn phím toàn cục
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        KBDLLHOOKSTRUCT *pKey = (KBDLLHOOKSTRUCT *)lParam;

        // Bỏ qua các phím giả lập do chính bộ gõ bắn ra
        if (pKey->flags & LLKHF_INJECTED)
        {
            return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
        }

        bool ctrlDown = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        bool shiftDown = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        bool altDown = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
        bool winDown = ((GetAsyncKeyState(VK_LWIN) | GetAsyncKeyState(VK_RWIN)) & 0x8000) != 0;

        bool isShiftKey = (pKey->vkCode == VK_LSHIFT || pKey->vkCode == VK_RSHIFT || pKey->vkCode == VK_SHIFT);
        bool isCtrlKey = (pKey->vkCode == VK_LCONTROL || pKey->vkCode == VK_RCONTROL || pKey->vkCode == VK_CONTROL);

        // Xử lý nhả phím Ctrl + Shift để chuyển chế độ V/E
        if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
        {
            if (g_ctrlShiftPrimed && (isShiftKey || isCtrlKey))
            {
                g_ctrlShiftPrimed = false;
                ToggleVietnameseMode();
            }
            return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
        }

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
        {
            // Kiểm tra tổ hợp Ctrl + Shift (nhận diện tức thì cả khi bấm Ctrl trước hay Shift trước)
            if (isShiftKey || isCtrlKey)
            {
                if (((ctrlDown && isShiftKey) || (shiftDown && isCtrlKey) || (ctrlDown && shiftDown)) && !altDown && !winDown)
                {
                    g_ctrlShiftPrimed = true;
                }
                return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
            }
            else
            {
                // Nếu nhấn thêm bất kỳ phím nào khác khi đang giữ Ctrl+Shift -> hủy cờ chuyển V/E
                g_ctrlShiftPrimed = false;
            }

            // Nếu đang giữ Ctrl, Alt, Win hoặc đang ở chế độ Tiếng Anh -> xóa bộ đệm và bỏ qua
            if (ctrlDown || altDown || winDown || !g_isVietnamese)
            {
                g_word.Clear();
                return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
            }

            // Xử lý phím Backspace
            if (pKey->vkCode == VK_BACK)
            {
                if (!g_word.display.empty())
                {
                    g_word.display.pop_back();
                    g_word.lastStandaloneW = false;
                    if (g_word.display.empty())
                        g_word.disabled = false;
                }
                return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
            }

            // Xử lý các phím chữ cái A..Z
            if (pKey->vkCode >= 'A' && pKey->vkCode <= 'Z')
            {
                bool capsLock = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
                bool isUpper = shiftDown ^ capsLock;
                wchar_t ch = isUpper ? (wchar_t)pKey->vkCode : (wchar_t)(pKey->vkCode + 32);

                if (ProcessKey(ch))
                {
                    return 1; // Chặn phím gốc vì đã được bộ gõ biến đổi
                }
            }
            else if (pKey->vkCode != VK_CAPITAL)
            {
                // Gặp khoảng trắng, Enter, số, dấu câu hoặc phím mũi tên -> kết thúc từ
                g_word.Clear();
            }
        }
    }
    return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
}

// Xóa bộ đệm từ khi người dùng click chuột sang vị trí khác
LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && (wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN || wParam == WM_MBUTTONDOWN))
    {
        g_word.Clear();
    }
    return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
}

// Hiển thị menu chuột phải ở khay hệ thống (System Tray)
void ShowContextMenu(HWND hWnd)
{
    POINT pt;
    GetCursorPos(&pt);
    HMENU hMenu = CreatePopupMenu();

    AppendMenuW(hMenu, MF_STRING | (g_isVietnamese ? MF_CHECKED : 0),
                ID_TRAY_TOGGLE_VE, L"Bật Tiếng Việt (Ctrl+Shift)");
    AppendMenuW(hMenu, MF_STRING | (g_modernTone ? MF_CHECKED : 0),
                ID_TRAY_MODERN_TONE, L"Bỏ dấu kiểu mới");
    AppendMenuW(hMenu, MF_STRING | (g_chromeFix ? MF_CHECKED : 0),
                ID_TRAY_CHROME_FIX, L"Sửa lỗi thanh địa chỉ Chrome/Edge");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Thoát bộ gõ");

    SetForegroundWindow(hWnd);
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
    DestroyMenu(hMenu);
}

// Xử lý sự kiện cửa sổ ẩn quản lý System Tray
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_TRAYICON:
        if (lParam == WM_LBUTTONUP)
        {
            ToggleVietnameseMode();
        }
        else if (lParam == WM_RBUTTONUP)
        {
            ShowContextMenu(hWnd);
        }
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_TRAY_TOGGLE_VE:
            ToggleVietnameseMode();
            break;
        case ID_TRAY_MODERN_TONE:
            g_modernTone = !g_modernTone;
            break;
        case ID_TRAY_CHROME_FIX:
            g_chromeFix = !g_chromeFix;
            break;
        case ID_TRAY_EXIT:
            PostQuitMessage(0);
            break;
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// Hàm khởi chạy chính của ứng dụng Windows GUI (chạy ngầm không hiện Console)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    FreeConsole(); // Tự động ẩn cửa sổ Console đen ngay khi khởi chạy

    // Bật nhận diện độ phân giải thực (DPI Awareness) để Windows không tự co giãn làm vỡ nét Icon nhỏ
    SetProcessDPIAware();

    // Chống mở trùng nhiều tiến trình bộ gõ cùng lúc
    HANDLE hMutex = CreateMutexW(NULL, TRUE, L"VNKey_SingleInstance_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        MessageBoxW(NULL, L"Bộ gõ VNKey đang chạy ở khay hệ thống (góc dưới bên phải màn hình)!",
                    L"VNKey", MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    // Lấy kích thước chuẩn xác của Icon nhỏ theo DPI màn hình hiện tại (16x16, 20x20, 24x24...)
    int smallIconSize = GetSystemMetrics(SM_CXSMICON);
    if (smallIconSize <= 0) smallIconSize = 16;

    // Tạo bộ Icon nhỏ đúng tỷ lệ 1:1 cho vị trí cạnh chữ VNKey.exe và dưới khay hệ thống
    g_hIconV = CreateTrayIcon(true, smallIconSize);
    g_hIconE = CreateTrayIcon(false, smallIconSize);

    // Tạo bộ Icon lớn 64x64 siêu nét cho phần nội dung thông báo Balloon
    g_hIconLargeV = CreateTrayIcon(true, 64);
    g_hIconLargeE = CreateTrayIcon(false, 64);

    // Đăng ký lớp cửa sổ ẩn kèm theo cả Icon lớn và Icon nhỏ chuẩn kích thước
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = g_hIconLargeV;
    wc.hIconSm = g_hIconV;
    wc.lpszClassName = L"VNKeyHiddenWndClass";
    RegisterClassExW(&wc);

    g_hWnd = CreateWindowExW(0, wc.lpszClassName, L"VNKey", 0, 0, 0, 0, 0,
                             HWND_MESSAGE, NULL, hInstance, NULL);

    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd = g_hWnd;
    g_nid.uID = 2026; // Đổi sang ID mới để xóa cache icon răng cưa cũ của Windows Notification
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = g_hIconV; // Icon nhỏ nét liền nằm cạnh chữ VNKey.exe và dưới System Tray
    wcscpy_s(g_nid.szTip, L"VNKey: Tiếng Việt (Ctrl+Shift)");
    wcscpy_s(g_nid.szInfoTitle, L"Bộ gõ VNKey đã bật");
    wcscpy_s(g_nid.szInfo, L"Nhấn Ctrl+Shift để chuyển chế độ V/E.\nClick chuột phải vào icon [V] dưới khay hệ thống để tùy chỉnh.");

    // 0x00000004 = NIIF_USER (dùng icon tùy chỉnh), 0x00000020 = NIIF_LARGE_ICON (hiển thị icon kích thước lớn)
    g_nid.dwInfoFlags = 0x00000004 | 0x00000020;
    g_nid.hBalloonIcon = g_hIconLargeV; // Icon lớn 64x64 bên trong thân thông báo

    Shell_NotifyIconW(NIM_ADD, &g_nid);

    // Cài đặt Hook bàn phím và chuột toàn cục
    g_hKeyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0);
    g_hMouseHook = SetWindowsHookExW(WH_MOUSE_LL, LowLevelMouseProc, hInstance, 0);

    // Vòng lặp thông điệp giữ chương trình hoạt động
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // Dọn dẹp tài nguyên trước khi thoát
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
    if (g_hKeyboardHook)
        UnhookWindowsHookEx(g_hKeyboardHook);
    if (g_hMouseHook)
        UnhookWindowsHookEx(g_hMouseHook);
    if (g_hIconV)
        DestroyIcon(g_hIconV);
    if (g_hIconE)
        DestroyIcon(g_hIconE);
    if (g_hIconLargeV)
        DestroyIcon(g_hIconLargeV);
    if (g_hIconLargeE)
        DestroyIcon(g_hIconLargeE);
    if (hMutex)
        CloseHandle(hMutex);

    return 0;
}
