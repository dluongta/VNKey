// BẮT BUỘC đặt ở đầu file để mở khóa hBalloonIcon, NIIF_LARGE_ICON và SetProcessDPIAware
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
#include <cmath> // Thêm cmath để dùng powf (bù sáng Gamma)

// Tự động liên kết thư viện khi biên dịch bằng Visual Studio (MSVC)
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")

using namespace std;

// Định nghĩa các hằng số giao diện System Tray
#define WM_TRAYICON (WM_USER + 100)
#define ID_TRAY_TOGGLE_VE 1001
#define ID_TRAY_MODERN_TONE 1002
#define ID_TRAY_CHROME_FIX 1003
#define ID_TRAY_EXIT 1004

// Biến toàn cục quản lý trạng thái
HHOOK g_hKeyboardHook = NULL;
HHOOK g_hMouseHook = NULL;
HWND g_hWnd = NULL;
NOTIFYICONDATAW g_nid = {0};

// Tách riêng 2 bộ Icon: Bản nhỏ (chống răng cưa) và Bản to (để hiện giữa thông báo)
HICON g_hIconV = NULL;       // Icon nhỏ siêu nét (Dùng cho khay hệ thống và góc trái trên thông báo)
HICON g_hIconE = NULL;
HICON g_hIconLargeV = NULL;  // Icon lớn 64x64 siêu nét (Bên trong nội dung thông báo)
HICON g_hIconLargeE = NULL;

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
            if (i > 0 && (word[i - 1] == L'q' || word[i - 1] == L'Q') && base == 9)
            {
                if (i + 1 < len)
                {
                    int nr, nt;
                    if (GetVowelInfo(word[i + 1], nr, nt))
                        continue;
                }
            }
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

    if (vowelIndices.size() >= 3)
    {
        return vowelIndices[1];
    }

    bool hasEndingConsonant = (vowelIndices.back() < len - 1);
    if (hasEndingConsonant)
    {
        return vowelIndices[1];
    }

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
        if (lowerKey == L's') targetTone = 1;
        else if (lowerKey == L'f') targetTone = 2;
        else if (lowerKey == L'r') targetTone = 3;
        else if (lowerKey == L'x') targetTone = 4;
        else if (lowerKey == L'j') targetTone = 5;
        else if (lowerKey == L'z') targetTone = 0;

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

// Hàm vẽ Icon đa kích thước (Tự động chống răng cưa tùy theo kích thước)
// Phân giải 64x64 siêu nét cho thông báo và phân giải 16x16 riêng để tiêu đề không bị lưỡi cưa
HICON CreateTrayIcon(bool isViet, int size)
{
    if (size < 16) size = 16;
    if (size > 128) size = 128;

    vector<DWORD> pixels(size * size, 0);
    // Tính số byte mỗi dòng cho mask (Bắt buộc chia hết cho 2 byte)
    int maskRowBytes = ((size + 15) / 16) * 2;
    vector<BYTE> andMask(maskRowBytes * size, 0);

    const int subSamples = 16; // 256 điểm lấy mẫu mỗi pixel
    const float invSub = 1.0f / (float)subSamples;
    const float normScale = 64.0f / (float)size;

    // Ở kích thước nhỏ nét chữ cần dày hơn một chút để luôn được nhìn rõ ràng
    const float vHalfWidthBase = (size <= 28) ? 5.25f : 4.6f;

    for (int y = 0; y < size; y++)
    {
        for (int x = 0; x < size; x++)
        {
            int fgHits = 0;
            int bgHits = 0;

            for (int sy = 0; sy < subSamples; sy++)
            {
                float py = (y + (sy + 0.5f) * invSub) * normScale;
                for (int sx = 0; sx < subSamples; sx++)
                {
                    float px = (x + (sx + 0.5f) * invSub) * normScale;

                    float dx = 0.0f, dy = 0.0f;
                    if (px < 10.0f) dx = 10.0f - px;
                    else if (px > 54.0f) dx = px - 54.0f;
                    if (py < 10.0f) dy = 10.0f - py;
                    else if (py > 54.0f) dy = py - 54.0f;

                    if (dx * dx + dy * dy <= 100.0f)
                    {
                        bgHits++;
                        bool inText = false;

                        if (isViet)
                        {
                            if (py >= 14.0f && py <= 50.0f)
                            {
                                float t = (py - 14.0f) / 36.0f;
                                float leftCenter = 15.5f + t * 16.5f;
                                float rightCenter = 48.5f - t * 16.5f;
                                float halfWidth = vHalfWidthBase - t * 0.35f;

                                float dL = px - leftCenter;  if (dL < 0) dL = -dL;
                                float dR = px - rightCenter; if (dR < 0) dR = -dR;

                                if (dL <= halfWidth || dR <= halfWidth)
                                {
                                    inText = true;
                                }
                            }
                        }
                        else
                        {
                            if (py >= 14.0f && py <= 50.0f)
                            {
                                if (px >= 17.0f && px <= 25.0f)
                                    inText = true;
                                else if (px >= 25.0f && px <= 47.0f && py <= 21.5f)
                                    inText = true;
                                else if (px >= 25.0f && px <= 47.0f && py >= 28.25f && py <= 35.75f)
                                    inText = true;
                                else if (px >= 25.0f && px <= 47.0f && py >= 42.5f)
                                    inText = true;
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
                andMask[(y * maskRowBytes) + (x / 8)] |= (0x80 >> (x % 8));
            }
            else
            {
                float rawTextAlpha = (float)fgHits / (float)bgHits;
                // Bù sáng Gamma triệt để hiệu ứng răng cưa khi hiển thị icon 16x16
                float textAlpha = (size <= 28) ? powf(rawTextAlpha, 0.62f) : powf(rawTextAlpha, 0.80f);
                float boxAlpha = (float)bgHits / (float)(subSamples * subSamples);

                int bgR = isViet ? 215 : 25;
                int bgG = isViet ? 30 : 110;
                int bgB = isViet ? 35 : 215;

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
    g_nid.uFlags = NIF_ICON | NIF_TIP; // Cập nhật không hiện thông báo
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
                g_ctrlShiftPrimed = false;
            }

            if (ctrlDown || altDown || winDown || !g_isVietnamese)
            {
                g_word.Clear();
                return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
            }

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

            if (pKey->vkCode >= 'A' && pKey->vkCode <= 'Z')
            {
                bool capsLock = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
                bool isUpper = shiftDown ^ capsLock;
                wchar_t ch = isUpper ? (wchar_t)pKey->vkCode : (wchar_t)(pKey->vkCode + 32);

                if (ProcessKey(ch))
                {
                    return 1;
                }
            }
            else if (pKey->vkCode != VK_CAPITAL)
            {
                g_word.Clear();
            }
        }
    }
    return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
}

// Xóa bộ đệm từ khi người dùng click chuột
LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && (wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN || wParam == WM_MBUTTONDOWN))
    {
        g_word.Clear();
    }
    return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
}

// Hiển thị menu chuột phải
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

// Xử lý sự kiện cửa sổ
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

// Hàm khởi chạy chính
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    FreeConsole();

    // Bật nhận diện độ phân giải thực tế (DPI Awareness) để icon không bị hệ thống bóp lại mờ nhòe
    SetProcessDPIAware();

    HANDLE hMutex = CreateMutexW(NULL, TRUE, L"VNKey_SingleInstance_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        MessageBoxW(NULL, L"Bộ gõ VNKey đang chạy ở khay hệ thống (góc dưới bên phải màn hình)!",
                    L"VNKey", MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    // Lấy kích thước chuẩn xác của Icon nhỏ của hệ thống (thường là 16x16 hoặc to hơn nếu dùng màn DPI cao)
    int smallIconSize = GetSystemMetrics(SM_CXSMICON);
    if (smallIconSize <= 0) smallIconSize = 16;

    // TẠO RIÊNG BẢN NHỎ (chuẩn pixel) cho khay hệ thống và TIÊU ĐỀ THÔNG BÁO (góc trái trên)
    g_hIconV = CreateTrayIcon(true, smallIconSize);
    g_hIconE = CreateTrayIcon(false, smallIconSize);

    // TẠO RIÊNG BẢN TO (64x64 siêu nét) để hiển thị trong NỘI DUNG THÔNG BÁO
    g_hIconLargeV = CreateTrayIcon(true, 64);
    g_hIconLargeE = CreateTrayIcon(false, 64);

    // Gắn bản nhỏ vào hIconSm để hệ thống gọi khi hiện trên tiêu đề thông báo
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = g_hIconLargeV;
    wc.hIconSm = g_hIconV;    // Cực kỳ quan trọng: Gắn icon nhỏ chống vỡ nét vào đây
    wc.lpszClassName = L"VNKeyHiddenWndClass";
    RegisterClassExW(&wc);

    g_hWnd = CreateWindowExW(0, wc.lpszClassName, L"VNKey", 0, 0, 0, 0, 0,
                             HWND_MESSAGE, NULL, hInstance, NULL);

    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd = g_hWnd;
    g_nid.uID = 2026; // Sử dụng số ID hoàn toàn mới để xóa Cache lưu trữ icon răng cưa trước đó của Windows
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = g_hIconV; // Dùng icon bản vẽ riêng (không bị răng cưa) để gắn cạnh tên file trên đầu thông báo
    wcscpy_s(g_nid.szTip, L"VNKey: Tiếng Việt (Ctrl+Shift)");
    wcscpy_s(g_nid.szInfoTitle, L"Bộ gõ VNKey đã bật");
    wcscpy_s(g_nid.szInfo, L"Nhấn Ctrl+Shift để chuyển chế độ V/E.\nClick chuột phải vào icon [V] dưới khay hệ thống để tùy chỉnh.");

    // Gọi hiển thị bản 64x64 bên dưới thông báo
    g_nid.dwInfoFlags = 0x00000004 | 0x00000020;
    g_nid.hBalloonIcon = g_hIconLargeV;

    Shell_NotifyIconW(NIM_ADD, &g_nid);

    g_hKeyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0);
    g_hMouseHook = SetWindowsHookExW(WH_MOUSE_LL, LowLevelMouseProc, hInstance, 0);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    Shell_NotifyIconW(NIM_DELETE, &g_nid);
    if (g_hKeyboardHook)
        UnhookWindowsHookEx(g_hKeyboardHook);
    if (g_hMouseHook)
        UnhookWindowsHookEx(g_hMouseHook);
    if (g_hIconV) DestroyIcon(g_hIconV);
    if (g_hIconE) DestroyIcon(g_hIconE);
    if (g_hIconLargeV) DestroyIcon(g_hIconLargeV);
    if (g_hIconLargeE) DestroyIcon(g_hIconLargeE);
    if (hMutex)
        CloseHandle(hMutex);

    return 0;
}
