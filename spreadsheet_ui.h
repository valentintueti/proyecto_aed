#ifndef SPREADSHEET_UI_H
#define SPREADSHEET_UI_H

#include <SFML/Graphics.hpp>
#include <deque>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include "sparse_matrix.h"
#include "formula.h"

// Layout constants
inline constexpr int CELL_W    = 100;
inline constexpr int CELL_H    = 28;
inline constexpr int HDR_H     = 30;
inline constexpr int HDR_W     = 55;
inline constexpr int TOOLBAR_H = 42;
inline constexpr int STATUS_H  = 24;

// Pastel lavender palette
inline const sf::Color C_LILAC      {221, 210, 242}; // header cells
inline const sf::Color C_LILAC_LT   {250, 247, 255}; // page background
inline const sf::Color C_LILAC_DK   {148, 112, 198}; // accent / cursor border
inline const sf::Color C_WHITE      {255, 255, 255}; // cell background
inline const sf::Color C_BLACK      { 40,  28,  62}; // text
inline const sf::Color C_SEL        {190, 168, 228, 110}; // range selection overlay
inline const sf::Color C_GRID       {212, 202, 228}; // grid lines
inline const sf::Color C_TOOLBAR    {200, 180, 232}; // toolbar bar
inline const sf::Color C_TBINPUT    {255, 255, 255}; // formula bar input bg
inline const sf::Color C_ERR        {200,  50,  50}; // error text
inline const sf::Color C_STATUS     {232, 224, 248}; // status bar bg

class SpreadsheetUI {
public:
    SpreadsheetUI();
    int run();

private:
    // ---- data ----
    SparseMatrix<CellValue> mat;
    std::map<std::pair<int,int>, std::string> cellText; // formula-only map

    // Undo stack — each entry is a full snapshot of mat + cellText
    struct CellEntry { int row, col; CellValue val; std::string formula; };
    std::deque<std::vector<CellEntry>> undoStack;

    // Clipboard (tab/newline separated text)
    std::string clipboard;

    sf::RenderWindow window;
    sf::Font font;

    // ---- selection state ----
    int scrollX = 0, scrollY = 0;
    int selRow = 0, selCol = 0;
    int selRow2 = -1, selCol2 = -1;
    bool isDragging = false;
    // anchor for formula range selection (row, col of the formula cell)
    int formulaCellRow = -1, formulaCellCol = -1;
    // anchor for dragging within formula selection mode
    int formulaDragAnchorRow = -1, formulaDragAnchorCol = -1;

    // ---- editing ----
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

    bool isFormulaOpen() const; // true when editText has unbalanced '('
    void insertFormulaRef(int r, int c);        // append single cell ref
    void insertFormulaRange(int r1, int c1, int r2, int c2); // append range

    // Undo / clipboard / delete
    void pushUndo();
    void undo();
    void copySelection();
    void pasteAt(int startRow, int startCol);
    void recalcAllFormulas();
    void executeDeleteSelection(); // borra el rango actualmente seleccionado

    // Cell data
    void   setCellValue(int r, int c, const std::string& text);
    void   deleteCell(int r, int c);
    void   deleteRow(int r);
    void   deleteCol(int c);
    void   deleteRange(int r1, int c1, int r2, int c2);
    std::string getCellFormulaText(int r, int c) const;
    std::string getCellDisplayText(int r, int c) const;

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
    void drawStatusBar();
    void drawRect(float x, float y, float w, float h, sf::Color fill, sf::Color outline, float thick = 1.f);
    void drawText(const std::string& s, float x, float y, unsigned sz, sf::Color col);
};

#endif // SPREADSHEET_UI_H
