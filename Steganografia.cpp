#include <iostream>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <cmath>
#include <bitset>
#include <vector>
#include <random>
#include <conio.h>
#include <stdlib.h>
#include <windows.h>
#include <commdlg.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

// ============ ОБЪЯВЛЕНИЯ ФУНКЦИЙ ============
void infoHider();
void infoViewer();
void infoDetector();
void programTester();
void changeSettings();
void programHelper();
void exitProgram();

// ============ БИБЛИОТЕКИ / ЗАГОЛОВОЧНЫЕ ФАЙЛЫ ============
using namespace std;
using namespace Gdiplus;

// ============ ГЛОБАЛЬНЫЕ НАСТРОЙКИ ============
struct ProgramSettings {
    bool useEncryption = true;
    unsigned int currentSeed = 0;
    bool useSuffix = true;
    bool useSeedSuffix = true;
    int packLevel = 1;
};

ProgramSettings g_settings;

// ============ ОСНОВНАЯ ПРОГРАММА ============
int main() {
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    int choice;
    do {
        system("cls");
        cout << "=== SteganoMIX v1.06 ===";
        cout << "\nHide and view your information in a BMP file\n";
        cout << "\n=== Main menu ===\n";
        cout << "1. Hide information\n";
        cout << "2. View information\n";
        cout << "3. Detect secret in BMP\n";
        cout << "4. Tests this program\n";
        cout << "5. Settings\n";
        cout << "6. Help\n";
        cout << "0. Exit\n\n";
        choice = _getch() - '0';

        switch (choice) {
        case 1: infoHider(); break;
        case 2: infoViewer(); break;
        case 3: infoDetector(); break;
        case 4: programTester(); break;
        case 5: changeSettings(); break;
        case 6: programHelper(); break;
        case 0: exitProgram(); break;
        default: break;
        }
    } while (choice != 0);

    GdiplusShutdown(gdiplusToken);
    return 0;
}

// ============ СТРУКТУРА ЦВЕТА ============
struct PixelColor {
    unsigned char R, G, B;
    PixelColor(unsigned char r = 0, unsigned char g = 0, unsigned char b = 0) : R(r), G(g), B(b) {}
};

// ============ БАЗОВЫЕ ФУНКЦИИ ============
bitset<8> ByteToBit(unsigned char src) {
    return bitset<8>(src);
}

unsigned char BitToByte(const bitset<8>& scr) {
    return static_cast<unsigned char>(scr.to_ulong());
}

PixelColor EmbedSymbolToColor(const PixelColor& curColor, unsigned char symbol) {
    bitset<8> symbolBits = ByteToBit(symbol);
    PixelColor result = curColor;

    bitset<8> tempR = ByteToBit(curColor.R);
    tempR[0] = symbolBits[0];
    tempR[1] = symbolBits[1];
    result.R = BitToByte(tempR);

    bitset<8> tempG = ByteToBit(curColor.G);
    tempG[0] = symbolBits[2];
    tempG[1] = symbolBits[3];
    tempG[2] = symbolBits[4];
    result.G = BitToByte(tempG);

    bitset<8> tempB = ByteToBit(curColor.B);
    tempB[0] = symbolBits[5];
    tempB[1] = symbolBits[6];
    tempB[2] = symbolBits[7];
    result.B = BitToByte(tempB);

    return result;
}

bool isEncryption(const PixelColor& pixelColor) {
    unsigned char extracted = 0;
    extracted |= (pixelColor.R & 0x03);
    extracted |= ((pixelColor.G & 0x07) << 2);
    extracted |= ((pixelColor.B & 0x07) << 5);
    return (extracted == '/');
}

// ============ СТЕПЕНЬ УПАКОВКИ ============
int GetBitsPerChannel(int packLevel) {
    if (packLevel <= 0 || packLevel > 3) {
        packLevel = 1;
    }

    switch (packLevel) {
    case 1: return 3;
    case 2: return 6;
    case 3: return 8;
    default: return 3;
    }
}

int GetBitsPerPixel(int packLevel) {
    if (packLevel <= 0 || packLevel > 3) {
        packLevel = 1;
    }
    return GetBitsPerChannel(packLevel) * 3;
}

int GetMaxCapacity(int width, int height, int packLevel) {
    if (packLevel <= 0 || packLevel > 3) {
        packLevel = 1;
    }

    int availablePixels = width * height - width * 2;

    if (availablePixels <= 0) return 0;

    return availablePixels * packLevel;
}

unsigned char GetBitMask(int packLevel) {
    if (packLevel <= 0 || packLevel > 3) {
        packLevel = 1;
    }

    int bitsPerChannel = GetBitsPerChannel(packLevel);
    if (bitsPerChannel >= 8) return 0xFF;
    return (1 << bitsPerChannel) - 1;
}

PixelColor EmbedSymbolToColorLevel(const PixelColor& curColor, unsigned char symbol, int packLevel) {
    if (packLevel <= 0 || packLevel > 3) {
        packLevel = 1;
    }

    PixelColor result = curColor;
    int bitsPerChannel = GetBitsPerChannel(packLevel);
    unsigned char mask = GetBitMask(packLevel);

    result.R = (curColor.R & ~mask) | (symbol & mask);
    result.G = (curColor.G & ~mask) | ((symbol >> bitsPerChannel) & mask);
    result.B = (curColor.B & ~mask) | ((symbol >> (bitsPerChannel * 2)) & mask);

    return result;
}

unsigned char ExtractSymbolFromColorLevel(const PixelColor& color, int packLevel) {
    if (packLevel <= 0 || packLevel > 3) {
        packLevel = 1;
    }

    int bitsPerChannel = GetBitsPerChannel(packLevel);
    unsigned char mask = GetBitMask(packLevel);

    unsigned char extracted = 0;
    extracted |= (color.R & mask);
    extracted |= ((color.G & mask) << bitsPerChannel);
    extracted |= ((color.B & mask) << (bitsPerChannel * 2));

    return extracted;
}

// ============ РАБОТА С BMP ============
PixelColor GetPixelColor(Bitmap* bPic, int x, int y) {
    Color color;
    bPic->GetPixel(x, y, &color);
    return PixelColor(
        static_cast<unsigned char>(color.GetR()),
        static_cast<unsigned char>(color.GetG()),
        static_cast<unsigned char>(color.GetB())
    );
}

void SetPixelColor(Bitmap* bPic, int x, int y, const PixelColor& color) {
    Color gdiColor(color.R, color.G, color.B);
    bPic->SetPixel(x, y, gdiColor);
}

// ============ ПРОВЕРКА УРОВНЯ УПАКОВКИ ============
int ReadPackLevel(Bitmap* bPic) {
    PixelColor color = GetPixelColor(bPic, 0, 0);
    unsigned char ch = ExtractSymbolFromColorLevel(color, 1);

    if (ch >= '1' && ch <= '3') {
        return ch - '0';
    }
    return 1;
}

// ============ ЧТЕНИЕ КОЛИЧЕСТВА СИМВОЛОВ ============
int ReadCountTextDynamic(Bitmap* src, int packLevel, int startPos = 5) {
    if (packLevel <= 0 || packLevel > 3) {
        packLevel = 1;
    }

    string countStr = "";
    int i = startPos;

    while (true) {
        PixelColor color = GetPixelColor(src, 0, i);
        unsigned char ch = ExtractSymbolFromColorLevel(color, packLevel);

        if (ch == ':') {
            break;
        }

        if (ch >= '0' && ch <= '9') {
            countStr += static_cast<char>(ch);
        }
        else {
            return 0;
        }

        i++;
    }

    if (countStr.empty()) {
        return 0;
    }

    return stoi(countStr);
}

// ============ ДУБЛИРОВАНИЕ СЛУЖЕБНОЙ ИНФОРМАЦИИ ============
void CopyServiceDataToLastRow(Bitmap* bPic) {
    int width = bPic->GetWidth();
    int height = bPic->GetHeight();
    int lastRow = height - 1;

    int packLevel = ReadPackLevel(bPic);
    if (packLevel < 1 || packLevel > 3) {
        packLevel = 1;
    }

    // Читаем служебную информацию из первой строки
    PixelColor color00 = GetPixelColor(bPic, 0, 0);
    unsigned char packLevelChar = ExtractSymbolFromColorLevel(color00, 1);

    PixelColor color01 = GetPixelColor(bPic, 0, 1);
    unsigned char marker = ExtractSymbolFromColorLevel(color01, packLevel);

    string extension = "";
    for (int i = 2; i <= 4; i++) {
        PixelColor color = GetPixelColor(bPic, 0, i);
        extension += static_cast<char>(ExtractSymbolFromColorLevel(color, packLevel));
    }

    int textSize = ReadCountTextDynamic(bPic, packLevel, 5);
    string sizeStr = to_string(textSize) + ":";

    // Записываем служебную информацию в последнюю строку
    PixelColor dest00 = GetPixelColor(bPic, 0, lastRow);
    SetPixelColor(bPic, 0, lastRow, EmbedSymbolToColorLevel(dest00, packLevelChar, 1));

    PixelColor dest01 = GetPixelColor(bPic, 1, lastRow);
    SetPixelColor(bPic, 1, lastRow, EmbedSymbolToColorLevel(dest01, marker, packLevel));

    for (unsigned i = 0; i < extension.length(); i++) {
        PixelColor destColor = GetPixelColor(bPic, 2 + i, lastRow);
        SetPixelColor(bPic, 2 + i, lastRow,
            EmbedSymbolToColorLevel(destColor, extension[i], packLevel));
    }

    for (unsigned i = 0; i < sizeStr.length(); i++) {
        PixelColor destColor = GetPixelColor(bPic, 5 + i, lastRow);
        SetPixelColor(bPic, 5 + i, lastRow,
            EmbedSymbolToColorLevel(destColor, sizeStr[i], packLevel));
    }
}

// ============ ПРОВЕРКА РАЗМЕРА ИЗОБРАЖЕНИЯ ============
bool CheckImageSize(Bitmap* bPic) {
    int width = bPic->GetWidth();
    int height = bPic->GetHeight();

    const int MIN_WIDTH = 20;

    if (width < MIN_WIDTH) {
        cout << "\n=== WARNING: Image is too small! ===" << endl;
        cout << "Width: " << width << " pixels" << endl;
        cout << "Minimum width required: " << MIN_WIDTH << " pixels" << endl;
        cout << "Please use a larger image." << endl;
        cout << "=====================================" << endl;
        system("pause");
        return false;
    }

    return true;
}

// ============ ПРОВЕРКА ПРИЗНАКА ============
bool isEncryptionInBMPLevel(Bitmap* bPic) {
    int packLevel = ReadPackLevel(bPic);
    PixelColor color = GetPixelColor(bPic, 0, 1);
    return (ExtractSymbolFromColorLevel(color, packLevel) == '/');
}

// ============ ГЕНЕРАЦИЯ SEED ============
unsigned int GenerateSeed(int width, int height) {
    unsigned int seed = (width * 31) ^ (height * 17);
    seed ^= static_cast<unsigned int>(time(nullptr));
    seed ^= static_cast<unsigned int>(GetTickCount64());
    seed = (seed << 13) ^ seed;
    seed = (seed >> 7) ^ seed;
    seed = (seed << 17) ^ seed;
    return seed;
}

// ============ СТРУКТУРА ДЛЯ ПОЗИЦИЙ ПИКСЕЛЕЙ ============
struct PixelPosition {
    int x, y;
};

// ============ ГЕНЕРАЦИЯ СЛУЧАЙНЫХ ПОЗИЦИЙ ============
vector<PixelPosition> GenerateRandomPositions(int width, int height, int count, unsigned int seed) {
    vector<PixelPosition> positions;
    positions.reserve(count);

    mt19937 rng(seed);

    vector<PixelPosition> allPositions;
    for (int i = 1; i < width; i++) {
        for (int j = 0; j < height - 1; j++) {
            allPositions.push_back({ i, j });
        }
    }

    for (int i = static_cast<int>(allPositions.size()) - 1; i > 0; i--) {
        uniform_int_distribution<int> dist(0, i);
        int j = dist(rng);
        swap(allPositions[i], allPositions[j]);
    }

    for (int i = 0; i < count && i < static_cast<int>(allPositions.size()); i++) {
        positions.push_back(allPositions[i]);
    }

    return positions;
}

// ============ ЗАПИСЬ КОЛИЧЕСТВА СИМВОЛОВ ============
void WriteCountTextDynamic(int count, Bitmap* src, int packLevel) {
    if (packLevel <= 0 || packLevel > 3) {
        packLevel = 1;
    }

    string countStr = to_string(count) + ":";
    int startPos = 5;

    for (unsigned i = 0; i < countStr.length(); i++) {
        unsigned char symbol = static_cast<unsigned char>(countStr[i]);
        PixelColor curColor = GetPixelColor(src, 0, startPos + static_cast<int>(i));
        PixelColor newColor = EmbedSymbolToColorLevel(curColor, symbol, packLevel);
        SetPixelColor(src, 0, startPos + static_cast<int>(i), newColor);
    }
}

// ============ ЗАПИСЬ СИМВОЛОВ В BMP (БЕЗ ШИФРОВАНИЯ) ============
void HideTextInBMP(Bitmap* bPic, const string& text, const char* extension) {
    string extensionStr = "bmp";

    vector<unsigned char> bList;
    for (int i = 0; i < (int)text.length(); i++) {
        bList.push_back(static_cast<unsigned char>(text[i]));
    }

    int CountText = static_cast<int>(bList.size());
    int width = bPic->GetWidth();
    int height = bPic->GetHeight();
    int packLevel = g_settings.packLevel;
    int capacity = GetMaxCapacity(width, height, packLevel);

    if (CountText > capacity) {
        MessageBox(NULL, L"No free space in the image", L"Information", MB_OK);
        return;
    }

    if (isEncryptionInBMPLevel(bPic)) {
        MessageBox(NULL, L"File is already encrypted", L"Information", MB_OK);
        return;
    }

    // 1. packLevel в (0,0) с packLevel=1
    PixelColor pixel00 = GetPixelColor(bPic, 0, 0);
    char packLevelChar = '0' + packLevel;
    SetPixelColor(bPic, 0, 0, EmbedSymbolToColorLevel(pixel00, packLevelChar, 1));

    // 2. Признак в (0,1) с packLevel из настроек
    PixelColor pixel01 = GetPixelColor(bPic, 0, 1);
    SetPixelColor(bPic, 0, 1, EmbedSymbolToColorLevel(pixel01, '/', packLevel));

    // 3. Расширение в (0,2)-(0,4) с packLevel из настроек
    for (unsigned i = 0; i < extensionStr.length(); i++) {
        PixelColor curColor = GetPixelColor(bPic, 0, 2 + static_cast<int>(i));
        SetPixelColor(bPic, 0, 2 + static_cast<int>(i),
            EmbedSymbolToColorLevel(curColor, extensionStr[i], packLevel));
    }

    // 4. Размер в (0,5)... с packLevel из настроек
    WriteCountTextDynamic(CountText, bPic, packLevel);

    // 5. Текст
    int index = 0;
    for (int i = 4; i < width && index < CountText; i++) {
        for (int j = 0; j < height - 1 && index < CountText; j++) {
            PixelColor pixelColor = GetPixelColor(bPic, i, j);
            PixelColor newColor = EmbedSymbolToColorLevel(pixelColor, bList[index], packLevel);
            SetPixelColor(bPic, i, j, newColor);
            index++;
        }
    }

    // 6. Дублируем служебную информацию
    CopyServiceDataToLastRow(bPic);
}

// ============ ЗАПИСЬ СИМВОЛОВ В BMP (С ШИФРОВАНИЕМ) ============
void HideTextInBMP_Encrypted(Bitmap* bPic, const string& text, unsigned int seed, const char* extension) {
    string extensionStr = "bmp";

    vector<unsigned char> bList;
    for (unsigned i = 0; i < text.length(); i++) {
        bList.push_back(static_cast<unsigned char>(text[i]));
    }

    int CountText = static_cast<int>(text.length());
    int width = bPic->GetWidth();
    int height = bPic->GetHeight();
    int packLevel = g_settings.packLevel;
    int capacity = GetMaxCapacity(width, height, packLevel);

    if (CountText > capacity) {
        string msg = "Text too large!\nText size: " + to_string(CountText) + " bytes\nCapacity: " + to_string(capacity) + " bytes\nPack level: " + to_string(packLevel);
        MessageBoxA(NULL, msg.c_str(), "Information", MB_OK);
        return;
    }

    if (isEncryptionInBMPLevel(bPic)) {
        MessageBox(NULL, L"File is already encrypted", L"Information", MB_OK);
        return;
    }

    // 1. packLevel в (0,0) с packLevel=1
    PixelColor pixel00 = GetPixelColor(bPic, 0, 0);
    char packLevelChar = '0' + packLevel;
    SetPixelColor(bPic, 0, 0, EmbedSymbolToColorLevel(pixel00, packLevelChar, 1));

    // 2. Признак в (0,1) с packLevel из настроек
    PixelColor pixel01 = GetPixelColor(bPic, 0, 1);
    SetPixelColor(bPic, 0, 1, EmbedSymbolToColorLevel(pixel01, '/', packLevel));

    // 3. Расширение в (0,2)-(0,4) с packLevel из настроек
    for (unsigned i = 0; i < extensionStr.length(); i++) {
        PixelColor curColor = GetPixelColor(bPic, 0, 2 + static_cast<int>(i));
        PixelColor newColor = EmbedSymbolToColorLevel(curColor, extensionStr[i], packLevel);
        SetPixelColor(bPic, 0, 2 + static_cast<int>(i), newColor);
    }

    // 4. Размер в (0,5)... с packLevel из настроек
    WriteCountTextDynamic(CountText, bPic, packLevel);

    // 5. Текст в случайные позиции
    // Вычисляем сколько пикселей нужно для всех байт
    int pixelsNeeded = (CountText + packLevel - 1) / packLevel;  // Округление вверх

    vector<PixelPosition> positions = GenerateRandomPositions(width, height, pixelsNeeded, seed);

    // Записываем байты в пиксели
    int byteIndex = 0;
    for (int i = 0; i < pixelsNeeded && byteIndex < CountText; i++) {

        PixelColor pixelColor = GetPixelColor(bPic, positions[i].x, positions[i].y);
        PixelColor newColor = EmbedSymbolToColorLevel(pixelColor, bList[byteIndex], packLevel);
        SetPixelColor(bPic, positions[i].x, positions[i].y, newColor);
        byteIndex++;
    }

    // 6. Дублируем служебную информацию
    CopyServiceDataToLastRow(bPic);
}

// ============ ПРОВЕРКА СЛУЖЕБНОЙ ИНФОРМАЦИИ ============
bool CheckServiceData(Bitmap* bPic, int& packLevel, int& countSymbol) {
    int height = bPic->GetHeight();
    int lastRow = height - 1;

    // Читаем packLevel из (0,0) с packLevel=1
    packLevel = ReadPackLevel(bPic);
    if (packLevel < 1 || packLevel > 3) {
        packLevel = 1;
    }

    // Читаем первую строку
    PixelColor colorStart1 = GetPixelColor(bPic, 0, 1);
    unsigned char chStart1 = ExtractSymbolFromColorLevel(colorStart1, packLevel);

    string extStart = "";
    for (int i = 2; i <= 4; i++) {
        PixelColor color = GetPixelColor(bPic, 0, i);
        unsigned char ch = ExtractSymbolFromColorLevel(color, packLevel);
        extStart += static_cast<char>(ch);
    }

    int sizeStart = ReadCountTextDynamic(bPic, packLevel, 5);
    bool hasStart = (chStart1 == '/');

    // Читаем последнюю строку
    PixelColor colorEnd1 = GetPixelColor(bPic, 1, lastRow); // ВНИМАНИЕ: x=1, а не x=0!
    unsigned char chEnd1 = ExtractSymbolFromColorLevel(colorEnd1, packLevel);

    string extEnd = "";
    for (int i = 2; i <= 4; i++) {
        PixelColor color = GetPixelColor(bPic, i, lastRow);
        unsigned char ch = ExtractSymbolFromColorLevel(color, packLevel);
        extEnd += static_cast<char>(ch);
    }

    int sizeEnd = ReadCountTextDynamic(bPic, packLevel, 5);
    bool hasEnd = (chEnd1 == '/');

    // Принимаем решение
    if (hasStart && hasEnd) {
        if (extStart == extEnd && sizeStart == sizeEnd) {
            countSymbol = sizeStart;
            cout << "Service data matches!" << endl;
            return true;
        }
        else {
            countSymbol = sizeStart;
            return false;
        }
    }
    else if (hasStart) {
        cout << "\n=== WARNING: Service data mismatch! ===" << endl;
        cout << "First row:  marker='" << (char)chStart1
            << "' (0x" << hex << (int)chStart1 << dec << ")"
            << ", ext='" << extStart
            << "', size=" << sizeStart << endl;
        cout << "Last row:   marker='" << (char)chEnd1
            << "' (0x" << hex << (int)chEnd1 << dec << ")"
            << ", ext='" << extEnd
            << "', size=" << sizeEnd << endl;
        cout << "Using first row data." << endl;
        countSymbol = sizeStart;
        return true;
    }
    else if (hasEnd) {
        cout << "\n=== WARNING: Service data mismatch! ===" << endl;
        cout << "First row:  marker='" << (char)chStart1
            << "' (0x" << hex << (int)chStart1 << dec << ")"
            << ", ext='" << extStart
            << "', size=" << sizeStart << endl;
        cout << "Last row:   marker='" << (char)chEnd1
            << "' (0x" << hex << (int)chEnd1 << dec << ")"
            << ", ext='" << extEnd
            << "', size=" << sizeEnd << endl;
        cout << "Using last row data." << endl;
        countSymbol = sizeEnd;
        return true;
    }
    else {
        cout << "No marker found. Using first row data." << endl;
        countSymbol = sizeStart;
        return true;
    }
}

// ============ ЧТЕНИЕ СИМВОЛОВ ИЗ BMP (БЕЗ ШИФРОВАНИЯ) ============
string ReadTextFromBMP(Bitmap* bPic) {
    int packLevel = ReadPackLevel(bPic);
    if (packLevel < 1 || packLevel > 3) {
        packLevel = 1;
    }

    if (!isEncryptionInBMPLevel(bPic)) {
        MessageBox(NULL, L"No hidden information", L"Information", MB_OK);
        return "";
    }

    string extension = "";
    int i = 2;
    while (true) {
        PixelColor color = GetPixelColor(bPic, 0, i);
        unsigned char ch = ExtractSymbolFromColorLevel(color, packLevel);

        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '.') {
            extension += static_cast<char>(ch);
            i++;
        }
        else {
            break;
        }
    }

    int countSymbol = ReadCountTextDynamic(bPic, packLevel);
    if (countSymbol <= 0) {
        return "";
    }

    int packLevelFromCheck = packLevel;
    int countSymbolFromCheck = countSymbol;
    CheckServiceData(bPic, packLevelFromCheck, countSymbolFromCheck);

    vector<unsigned char> message;
    message.reserve(countSymbol);

    int width = bPic->GetWidth();
    int height = bPic->GetHeight();

    for (int x = 4; x < width && (int)message.size() < countSymbol; x++) {
        for (int y = 0; y < height - 1 && (int)message.size() < countSymbol; y++) {
            PixelColor pixelColor = GetPixelColor(bPic, x, y);
            message.push_back(ExtractSymbolFromColorLevel(pixelColor, packLevel));
        }
    }

    return string(message.begin(), message.end());
}

// ============ ЧТЕНИЕ СИМВОЛОВ ИЗ BMP (С ШИФРОВАНИЕМ) ============
string ReadTextFromBMP_Encrypted(Bitmap* bPic, unsigned int seed, const char* filePath) {
    int packLevel = ReadPackLevel(bPic);
    if (packLevel < 1 || packLevel > 3) {
        packLevel = 1;
    }

    if (!isEncryptionInBMPLevel(bPic)) {
        MessageBox(NULL, L"No hidden information", L"Information", MB_OK);
        return "";
    }

    string extension = "";
    int i = 2;
    while (true) {
        PixelColor color = GetPixelColor(bPic, 0, i);
        unsigned char ch = ExtractSymbolFromColorLevel(color, packLevel);

        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '.') {
            extension += static_cast<char>(ch);
            i++;
        }
        else {
            break;
        }
    }

    int countSymbol = ReadCountTextDynamic(bPic, packLevel);
    if (countSymbol <= 0) {
        return "";
    }

    int packLevelFromCheck = packLevel;
    int countSymbolFromCheck = countSymbol;
    CheckServiceData(bPic, packLevelFromCheck, countSymbolFromCheck);

    packLevel = packLevelFromCheck;
    countSymbol = countSymbolFromCheck;

    int width = bPic->GetWidth();
    int height = bPic->GetHeight();

    // Количество байт для чтения = countSymbol
    // Количество пикселей для чтения = ceil(countSymbol / packLevel)
    int pixelsToRead = (countSymbol + packLevel - 1) / packLevel;  // Округление вверх

    vector<PixelPosition> positions = GenerateRandomPositions(width, height, pixelsToRead, seed);
    vector<unsigned char> fullData;
    fullData.reserve(countSymbol);

    // Читаем все байты из пикселей
    for (int i = 0; i < pixelsToRead && i < (int)positions.size(); i++) {
        PixelColor pixelColor = GetPixelColor(bPic, positions[i].x, positions[i].y);
        unsigned char byte = ExtractSymbolFromColorLevel(pixelColor, packLevel);

        // Извлекаем все байты из этого пикселя
        // При packLevel=2, пиксель содержит 2 байта
        // При packLevel=3, пиксель содержит 3 байта
        for (int b = 0; b < packLevel; b++) {
            if (fullData.size() < countSymbol) {
                // Извлекаем байт по частям
                unsigned char extractedByte = 0;
                if (b == 0) {
                    // Первый байт - младшие биты
                    extractedByte = byte & 0xFF;
                }
                else {
                    extractedByte = (byte >> (b * 8)) & 0xFF;
                }
                fullData.push_back(extractedByte);
            }
        }
    }

    return string(fullData.begin(), fullData.end());
}

// ============ ВЫБОР ФАЙЛА ============
bool SelectBMPFile(char* outPath, int maxPath) {
    OPENFILENAMEA ofn = { 0 };
    char file[MAX_PATH] = { 0 };
    char filter[] =
        "BMP files (*.bmp;*.BMP)\0*.bmp;*.BMP\0"
        "All files (*.*)\0*.*\0";

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetDesktopWindow();
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = "Select BMP file";
    ofn.lpstrFilter = filter;
    ofn.lpstrInitialDir = ".";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    if (GetOpenFileNameA(&ofn)) {
        strcpy_s(outPath, maxPath, file);
        return true;
    }
    return false;
}

// ============ СОЗДАНИЕ ПАПКИ ============
bool CreateDirectoryIfNotExists(const char* path) {
    DWORD attribs = GetFileAttributesA(path);
    if (attribs != INVALID_FILE_ATTRIBUTES) {
        return true;
    }

    char tempPath[MAX_PATH];
    strcpy_s(tempPath, sizeof(tempPath), path);

    for (unsigned i = 0; i < strlen(tempPath); i++) {
        if (tempPath[i] == '\\' || tempPath[i] == '/') {
            tempPath[i] = '\0';
            CreateDirectoryA(tempPath, NULL);
            tempPath[i] = '\\';
        }
    }
    CreateDirectoryA(tempPath, NULL);
    return true;
}

// ============ ИДЕНТИФИКАТОР (GUID) ДЛЯ BMP ФАЙЛОВ ============
const CLSID CLSID_BMP = { 0x557cf400, 0x1a04, 0x11d3, { 0x9a, 0x73, 0x0, 0x0, 0xf8, 0x1e, 0xf3, 0x2e } };

// ============ СОХРАНЕНИЕ ВЫХОДНОГО ФАЙЛА ============
bool SaveBMPWith(Bitmap* bPic, const char* originalPath, bool encrypted, unsigned int seed) {
    char drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];

    _splitpath_s(originalPath, drive, _MAX_DRIVE, dir, _MAX_DIR,
        fname, _MAX_FNAME, ext, _MAX_EXT);

    char outputDir[MAX_PATH];
    sprintf_s(outputDir, sizeof(outputDir), "%s%soutput_img\\", drive, dir);
    CreateDirectoryIfNotExists(outputDir);

    char fullName[MAX_PATH];

    if (g_settings.useSuffix) {
        if (encrypted) {
            sprintf_s(fullName, sizeof(fullName), "%s%s_encrypted", outputDir, fname);
            if (g_settings.useSeedSuffix) {
                char seedStr[16];
                sprintf_s(seedStr, sizeof(seedStr), "_%u", seed);
                strcat_s(fullName, seedStr);
            }
        }
        else {
            sprintf_s(fullName, sizeof(fullName), "%s%s_hidden", outputDir, fname);
        }
    }
    else {
        sprintf_s(fullName, sizeof(fullName), "%s%s", outputDir, fname);
    }

    strcat_s(fullName, ext);

    WCHAR wnewPath[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, fullName, -1, wnewPath, MAX_PATH);

    if (bPic->Save(wnewPath, &CLSID_BMP, NULL) != Ok) {
        return false;
    }

    cout << "Saved as: " << fullName << endl;
    return true;
}

// ============ ОТОБРАЖЕНИЕ НАСТРОЕК ============
void showSettings() {
    system("cls");
    cout << "=== Settings ===\n";
    cout << "1. Encryption: " << (g_settings.useEncryption ? "ON" : "OFF") << "\n";
    cout << "2. Add suffix to filename: " << (g_settings.useSuffix ? "ON" : "OFF") << "\n";
    cout << "3. Add seed to filename: " << (g_settings.useSeedSuffix ? "ON" : "OFF") << "\n";
    cout << "4. Pack level (1-3): " << g_settings.packLevel << "\n";
    cout << "   Bytes per pixel: " << g_settings.packLevel << "\n";
    cout << "0. Return\n\n";
}

// ============ ИЗМЕНЕНИЕ НАСТРОЕК ============
void changeSettings() {
    int choice;
    do {
        showSettings();
        choice = _getch() - '0';

        switch (choice) {
        case 1:
            g_settings.useEncryption = !g_settings.useEncryption;
            break;
        case 2:
            g_settings.useSuffix = !g_settings.useSuffix;
            break;
        case 3:
            g_settings.useSeedSuffix = !g_settings.useSeedSuffix;
            break;
        case 4: {
            cout << "\nEnter pack level (1-3): ";
            int level = _getch() - '0';
            if (level >= 1 && level <= 3) {
                g_settings.packLevel = level;
                cout << "\nPack level set to: " << level << "\n";
                cout << "Bytes per pixel: " << level << "\n";
            }
            else {
                cout << "Invalid level! Use 1-3.\n";
            }
            system("pause");
            break;
        }
        case 0:
            return;
        default:
            break;
        }
    } while (choice != 0);
}

// ============ СОКРЫТИЕ СИМВОЛОВ В BMP ============
void infoHider() {
    system("cls");
    cout << "=== Welcome to InfoHider ===\n";
    cout << "1. LSB Algorithm (Least Significant Bit)\n";
    cout << "2. FF Algorithm (For Future)\n";
    cout << "3. Settings\n";
    cout << "0. Return to menu\n\n";

    int alg = _getch() - '0';

    if (alg == 0) {
        return;
    }

    if (alg == 3) {
        changeSettings();
        return;
    }

    if (alg == 1) {
        system("cls");
        cout << "=== LSB Algorithm ===\n\n";

        char path[MAX_PATH];
        cout << "Select a BMP file to hide information...\n";

        if (!SelectBMPFile(path, sizeof(path))) {
            cout << "No file selected. Operation cancelled.\n";
            system("pause");
            return;
        }

        WCHAR wpath[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, path, -1, wpath, MAX_PATH);

        Bitmap* bPic = new Bitmap(wpath);
        if (bPic->GetLastStatus() != Ok) {
            cout << "Error loading BMP!\n";
            delete bPic;
            system("pause");
            return;
        }

        // ============ ПРОВЕРКА РАЗМЕРА ============
        if (!CheckImageSize(bPic)) {
            delete bPic;
            return;
        }

        int packLevel = g_settings.packLevel;

        if (isEncryptionInBMPLevel(bPic)) {
            cout << "\nThis file already contains hidden information!\n";
            cout << "Use 'View information' to read the hidden text.\n";
            delete bPic;
            system("pause");
            return;
        }

        int width = bPic->GetWidth();
        int height = bPic->GetHeight();

        int capacity = GetMaxCapacity(width, height, packLevel);

        cout << "File successfully loaded: " << path << endl;
        cout << "Size: " << width << "x" << height << endl;
        cout << "Max capacity: " << capacity << " bytes" << endl;
        cout << "Encryption: " << (g_settings.useEncryption ? "ON" : "OFF") << endl;
        cout << "Pack level: " << packLevel << " (" << packLevel << " bytes per pixel)\n";

        cout << "\nInput your text: ";
        string text;
        cin.seekg(0, ios::end);
        cin.clear();
        getline(cin, text);

        if (text.empty()) {
            cout << "Text cannot be empty!\n";
            delete bPic;
            system("pause");
            return;
        }

        int textBytes = static_cast<int>(text.length());
        cout << "\nText length: " << text.length() << " symbols (" << textBytes << " bytes)" << endl;

        bool encrypted = g_settings.useEncryption;
        unsigned int seed = 0;

        if (encrypted) {
            seed = GenerateSeed(width, height);
            g_settings.currentSeed = seed;
            cout << "Seed generated: " << seed << endl;
            cout << "SAVE THIS SEED to decrypt later!\n";

            string extension = "";
            const char* p = strrchr(path, '.');
            if (p != nullptr) {
                extension = string(p + 1);
            }
            if (extension.empty()) {
                extension = "NULL";
            }

            HideTextInBMP_Encrypted(bPic, text, seed, extension.c_str());
        }
        else {
            string extension = "";
            const char* p = strrchr(path, '.');
            if (p != nullptr) {
                extension = string(p + 1);
            }
            if (extension.empty()) {
                extension = "NULL";
            }

            HideTextInBMP(bPic, text, extension.c_str());
        }

        if (!SaveBMPWith(bPic, path, encrypted, seed)) {
            cout << "\nError saving file!\n";
        }
        else {
            cout << "\nInformation successfully hidden and saved!\n";
        }

        delete bPic;
        system("pause");
        return;
    }

    if (alg == 2) {
        system("cls");
        cout << "=== FF Algorithm ===\n";
        cout << "Error 404\n";
        system("pause");
        return;
    }

    cout << "Invalid choice!\n";
    system("pause");
}

// ============ ЧТЕНИЕ СИМВОЛОВ ИЗ BMP ============
void infoViewer() {
    system("cls");
    cout << "=== Welcome to InfoViewer ===\n";
    cout << "1. Read hidden text\n";
    cout << "0. Return to menu\n\n";

    int choice = _getch() - '0';

    if (choice == 0) {
        return;
    }

    if (choice == 1) {
        char path[MAX_PATH];
        cout << "Select a BMP file to read hidden information...\n";

        if (!SelectBMPFile(path, sizeof(path))) {
            cout << "No file selected.\n";
            system("pause");
            return;
        }

        WCHAR wpath[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, path, -1, wpath, MAX_PATH);

        Bitmap* bPic = new Bitmap(wpath);
        if (bPic->GetLastStatus() != Ok) {
            cout << "Error loading BMP!\n";
            delete bPic;
            system("pause");
            return;
        }

        cout << "\nEnter seed (or press Enter for non-encrypted text): ";
        string seedInput;
        cin.seekg(0, ios::end);
        cin.clear();
        getline(cin, seedInput);

        string hiddenText;

        if (!seedInput.empty()) {
            bool isNumber = true;
            for (char c : seedInput) {
                if (!isdigit(c)) {
                    isNumber = false;
                    break;
                }
            }

            if (isNumber) {
                try {
                    unsigned int seed = static_cast<unsigned int>(stoul(seedInput));
                    cout << "Using seed: " << seed << " (encrypted mode)\n";
                    hiddenText = ReadTextFromBMP_Encrypted(bPic, seed, path);
                }
                catch (const out_of_range&) {
                    cout << "Seed is too large!\n";
                    hiddenText = "";
                }
            }
            else {
                cout << "Invalid seed! Please enter a number.\n";
                hiddenText = "";
            }
        }
        else {
            hiddenText = ReadTextFromBMP(bPic);
        }

        if (!hiddenText.empty()) {
            cout << "\n=== Hidden text ===\n";
            cout << hiddenText << endl;
            cout << "==================\n";
            cout << "Text length: " << hiddenText.length() << " symbols\n";
        }
        else {
            cout << "\nNo text found.\n";
        }

        delete bPic;
        system("pause");
        return;
    }

    cout << "Invalid choice!\n";
    system("pause");
}

// ============ ПРОВЕРКА BMP НА СКРЫТЫЕ СИМВОЛЫ ============
void infoDetector() {
    system("cls");
    cout << "=== Welcome to InfoDetector ===\n";
    cout << "1. Check BMP file\n";
    cout << "0. Return to menu\n\n";

    int choice = _getch() - '0';

    if (choice == 0) {
        return;
    }

    if (choice == 1) {
        char path[MAX_PATH];
        cout << "Select a BMP file to check...\n";

        if (!SelectBMPFile(path, sizeof(path))) {
            cout << "No file selected.\n";
            system("pause");
            return;
        }

        WCHAR wpath[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, path, -1, wpath, MAX_PATH);

        Bitmap* bPic = new Bitmap(wpath);
        if (bPic->GetLastStatus() != Ok) {
            cout << "Error loading BMP!\n";
            delete bPic;
            system("pause");
            return;
        }

        int packLevel = ReadPackLevel(bPic);
        if (packLevel < 1 || packLevel > 3) {
            packLevel = 1;
        }

        // Проверяем маркер в первой строке
        bool hasSecret = isEncryptionInBMPLevel(bPic);

        // Если в первой строке нет маркера, проверяем в последней
        if (!hasSecret) {
            int height = bPic->GetHeight();
            int lastRow = height - 1;
            PixelColor colorEnd = GetPixelColor(bPic, 1, lastRow);
            unsigned char chEnd = ExtractSymbolFromColorLevel(colorEnd, packLevel);
            hasSecret = (chEnd == '/');
        }

        if (!hasSecret) {
            cout << "No hidden information found.\n";
            delete bPic;
            system("pause");
            return;
        }

        cout << "Hidden information detected!\n";
        cout << "   Pack level from file: " << packLevel << "\n";

        string extension = "";
        int i = 2;
        while (true) {
            PixelColor color = GetPixelColor(bPic, 0, i);
            unsigned char ch = ExtractSymbolFromColorLevel(color, packLevel);

            if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '.') {
                extension += static_cast<char>(ch);
                i++;
            }
            else {
                break;
            }
        }
        cout << "   File type: ." << extension << endl;

        int count = ReadCountTextDynamic(bPic, packLevel);
        if (count > 0) {
            cout << "   Text size: " << count << " symbols\n";
        }

        int packLevelCheck = packLevel;
        int countSymbolCheck = count;
        CheckServiceData(bPic, packLevelCheck, countSymbolCheck);

        delete bPic;
        system("pause");
        return;
    }

    cout << "Invalid choice!\n";
    system("pause");
}

// ============ АВТОМАТИЧЕСКОЕ ТЕСТИРОВАНИЕ ============
void programTester() {
    int choice;
    do {
        system("cls");
        cout << "=== Welcome to ProgramTester ===\n";
        cout << "1. Auto all tests\n";
        cout << "2. Manual select tests\n";
        cout << "0. Return to menu\n\n";
        choice = _getch() - '0';
        switch (choice) {
        case 0: return;
        case 1:
            system("cls");
            cout << "=== Auto all tests ===\n";
            cout << "Error 404\n";
            system("pause");
            return;
        case 2:
            system("cls");
            cout << "=== Manual select tests ===\n";
            cout << "Error 404\n";
            system("pause");
            return;
        default: break;
        }
    } while (choice != 0);
}

// ============ ПОМОЩЬ ПОЛЬЗОВАТЕЛЮ ============
void programHelper() {
    system("cls");
    cout << "=== SteganoMIX v1.06 - Help ===" << endl;
    cout << endl;

    cout << "=== Program description ===" << endl;
    cout << "The program is designed to hide and extract text information" << endl;
    cout << "BMP images using the LSB (Least Significant Bit) method." << endl;
    cout << endl;

    cout << "=== Main features ===" << endl;
    cout << "1. Hide text in BMP images (with/without encryption)" << endl;
    cout << "2. Extract hidden text from BMP images" << endl;
    cout << "3. Detect the presence of hidden information" << endl;
    cout << "4. Configurable packing level (1, 2, or 3 bytes per pixel)" << endl;
    cout << "5. Encryption with seed (random pixel distribution)" << endl;
    cout << "6. Automatic duplication of service data in the last row for reliability" << endl;
    cout << endl;

    cout << "=== Pack levels ===" << endl;
    cout << "Level 1: 1 byte per pixel (best quality, lowest capacity)" << endl;
    cout << "Level 2: 2 bytes per pixel (balanced)" << endl;
    cout << "Level 3: 3 bytes per pixel (max capacity, lower quality)" << endl;
    cout << "Note: Pack level is stored in the file and read automatically." << endl;
    cout << endl;

    cout << "=== File structure ===" << endl;
    cout << "Row 0 (first row) - service data:" << endl;
    cout << "  Pixel (0,0) - pack level (1, 2, or 3)" << endl;
    cout << "  Pixel (0,1) - marker '/' (indicates presence of a secret)" << endl;
    cout << "  Pixel (0,2 - 0.4) - file extension ':'" << endl;
    cout << "  Pixel (0,5...) - text size with delimiter ':'" << endl;
    cout << "  Pixel (4,0...) - hidden text data" << endl;
    cout << "Row (height-1) (last row) - exact copy of service data" << endl;
    cout << endl;

    cout << "=== How to hide text ===" << endl;
    cout << "1. Select 'Hide information' in the main menu" << endl;
    cout << "2. Choose LSB Algorithm" << endl;
    cout << "3. Select a BMP image (minimum width: 20 pixels)" << endl;
    cout << "4. Enter the text to hide" << endl;
    cout << "5. If encryption is ON, save the generated seed" << endl;
    cout << "6. The file will be saved in the 'output_img' folder" << endl;
    cout << endl;

    cout << "=== How to extract text ===" << endl;
    cout << "1. Select 'View information' in the main menu" << endl;
    cout << "2. Select the BMP image with hidden text" << endl;
    cout << "3. If the file was encrypted, enter the seed" << endl;
    cout << "4. If the file is not encrypted, press Enter" << endl;
    cout << "5. The hidden text will be displayed" << endl;
    cout << endl;

    cout << "=== How to detect a secret ===" << endl;
    cout << "1. Select 'Detect secret in BMP' in the main menu" << endl;
    cout << "2. Select the BMP image to check" << endl;
    cout << "3. The program will report the presence of hidden information" << endl;
    cout << endl;

    cout << "=== Settings ===" << endl;
    cout << "1. Encryption ON/OFF - enables/disables random pixel placement" << endl;
    cout << "2. Add suffix to filename - adds _hidden or _encrypted_seed" << endl;
    cout << "3. Add seed to filename - adds the seed to the file name" << endl;
    cout << "4. Pack level (1-3) - number of bytes per pixel" << endl;
    cout << "   (higher = more capacity, lower quality)" << endl;
    cout << endl;

    cout << "=== Service data verification ===" << endl;
    cout << "When reading, the program checks both first and last rows." << endl;
    cout << "If the data matches, extraction proceeds normally." << endl;
    cout << "If the data differs, a warning is displayed and" << endl;
    cout << "the program uses data from the first row." << endl;
    cout << "This ensures reliable recovery even if one row is damaged." << endl;
    cout << endl;

    cout << "=== Minimum image requirements ===" << endl;
    cout << "Minimum width: 20 pixels (for service data)" << endl;
    cout << "Recommended: 64x64 pixels or larger" << endl;
    cout << "If the image is too small, you will see a warning." << endl;
    cout << endl;

    cout << "=== Supported file formats ===" << endl;
    cout << "BMP" << endl;
    cout << endl;

    cout << "=== Recommendations ===" << endl;
    cout << "Use large images (640x480 or more) to hide large texts." << endl;
    cout << "Always save the seed when encrypting - it is required for decryption." << endl;
    cout << "Use pack level 1 for best image quality." << endl;
    cout << "The program automatically duplicates service data for reliability." << endl;
    cout << "Maximum text size depends on image dimensions and pack level." << endl;
    cout << endl;

    cout << "Press any key to return to the main menu..." << endl;
    (void)_getch();
}

// ============ ВЫХОД ИЗ ПРОГРАММЫ ============
void exitProgram() {
    exit(0);
}