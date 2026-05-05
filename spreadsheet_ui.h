#ifndef SPREADSHEET_UI_H
#define SPREADSHEET_UI_H

#include <SFML/Graphics.hpp>
#include <map>
#include <string>
#include <utility>
#include "sparse_matrix.h"
#include "formula.h"

// Layout constants
inline constexpr int CELL_W    = 100;
inline constexpr int CELL_H    = 28;
inline constexpr int HDR_H     = 30;
inline constexpr int HDR_W     = 55;
inline constexpr int TOOLBAR_H = 42;

inline const sf::Color C_LILAC      {200, 180, 220};
inline const sf::Color C_LILAC_LT   {230, 220, 245};
inline const sf::Color C_LILAC_DK   {140, 110, 170};
inline const sf::Color C_WHITE      {255, 255, 255};
inline const sf::Color C_BLACK      {  0,   0,   0};
inline const sf::Color C_SEL        {180, 150, 210, 120};
inline const sf::Color C_GRID       {180, 170, 200};
inline const sf::Color C_TOOLBAR    {130,  90, 170};
inline const sf::Color C_TBINPUT    {255, 255, 220};
inline const sf::Color C_ERR        {220,  60,  60};

class SpreadsheetUI {
public:
    SpreadsheetUI();
    int run();

private:
    SparseMatrix<double> mat;
    // formula text / text-cell values keyed by {row, col}
    std::map<std::pair<int,int>, std::string> cellText;

    sf::RenderWindow window;
    sf::Font font;

    int scrollX = 0, scrollY = 0;
    int selRow = 0, selCol = 0;
    int selRow2 = -1, selCol2 = -1; // range selection end (-1 = none)

    bool editing = false;
    std::string editText;

    bool toolbarActive = false;
    std::string toolbarInput;
    std::string toolbarResult;
    bool toolbarError = false;

    // Autocomplete
    std::vector<std::string> suggestions;
    int sugIdx = 0;

    // ---- helpers ----
    bool loadFont();
    int  visibleMaxCol() const;
    void updateSuggestions();
    void applySuggestion();
    void drawAutocomplete();

    // Cell data
    void   setCellValue(int r, int c, const std::string& text);
    void   deleteCell(int r, int c);
    void   deleteRow(int r);
    void   deleteCol(int c);
    void   deleteRange(int r1, int c1, int r2, int c2);
    std::string getCellFormulaText(int r, int c) const; // for formula bar
    std::string getCellDisplayText(int r, int c) const; // for cell render

    // Input
    void commitEdit();
    std::string runToolbarCommand(const std::string& cmd);
    void handleEvent(const sf::Event& ev, int visCols, int visRows);
    void handleKey(const sf::Event::KeyEvent& key, int visCols, int visRows);
    void handleText(uint32_t unicode);

    // Render
    void render(int maxCol, int visCols, int visRows);
    void drawToolbar();
    void drawHeaders(int maxCol);
    void drawCells(int maxCol, int visRows);
    void drawRect(float x, float y, float w, float h, sf::Color fill, sf::Color outline, float thick = 1.f);
    void drawText(const std::string& s, float x, float y, unsigned sz, sf::Color col);
};

#endif // SPREADSHEET_UI_H
