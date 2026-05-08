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


inline constexpr int CELL_W    = 100;
inline constexpr int CELL_H    = 28;
inline constexpr int HDR_H     = 30;
inline constexpr int HDR_W     = 55;
inline constexpr int TOOLBAR_H = 42;
inline constexpr int STATUS_H  = 24;


inline const sf::Color C_LILAC      {221, 210, 242};
inline const sf::Color C_LILAC_LT   {250, 247, 255};
inline const sf::Color C_LILAC_DK   {148, 112, 198};
inline const sf::Color C_WHITE      {255, 255, 255};
inline const sf::Color C_BLACK      { 40,  28,  62};
inline const sf::Color C_SEL        {190, 168, 228, 110};
inline const sf::Color C_GRID       {212, 202, 228};
inline const sf::Color C_TOOLBAR    {200, 180, 232};
inline const sf::Color C_TBINPUT    {255, 255, 255};
inline const sf::Color C_ERR        {200,  50,  50};
inline const sf::Color C_STATUS     {232, 224, 248};

class SpreadsheetUI {
public:
    SpreadsheetUI();
    int run();

private:

    SparseMatrix<CellValue> mat;
    std::map<std::pair<int,int>, std::string> cellText;


    struct CellEntry { int row, col; CellValue val; std::string formula; };
    std::deque<std::vector<CellEntry>> undoStack;


    std::string clipboard;

    sf::RenderWindow window;
    sf::Font font;


    int scrollX = 0, scrollY = 0;
    int selRow = 0, selCol = 0;
    int selRow2 = -1, selCol2 = -1;
    bool isDragging = false;

    int formulaCellRow = -1, formulaCellCol = -1;

    int formulaDragAnchorRow = -1, formulaDragAnchorCol = -1;


    bool editing = false;
    std::string editText;

    bool toolbarActive = false;
    std::string toolbarInput;
    std::string toolbarResult;
    bool toolbarError = false;


    std::vector<std::string> suggestions;
    int sugIdx = 0;

    // ---- helpers ----
    bool loadFont();
    int  visibleMaxCol() const;
    void updateSuggestions();
    void applySuggestion();
    void drawAutocomplete();

    bool isFormulaOpen() const;
    void insertFormulaRef(int r, int c);
    void insertFormulaRange(int r1, int c1, int r2, int c2);


    void pushUndo();
    void undo();
    void copySelection();
    void pasteAt(int startRow, int startCol);
    void recalcAllFormulas();
    void executeDeleteSelection();


    void   setCellValue(int r, int c, const std::string& text);
    void   deleteCell(int r, int c);
    void   deleteRow(int r);
    void   deleteCol(int c);
    void   deleteRange(int r1, int c1, int r2, int c2);
    std::string getCellFormulaText(int r, int c) const;
    std::string getCellDisplayText(int r, int c) const;


    void commitEdit();
    std::string runToolbarCommand(const std::string& cmd);
    void handleEvent(const sf::Event& ev, int visCols, int visRows);
    void handleKey(const sf::Event::KeyEvent& key, int visCols, int visRows);
    void handleText(uint32_t unicode);


    void render(int maxCol, int visCols, int visRows);
    void drawToolbar();
    void drawHeaders(int maxCol);
    void drawCells(int maxCol, int visRows);
    void drawStatusBar();
    void drawRect(float x, float y, float w, float h, sf::Color fill, sf::Color outline, float thick = 1.f);
    void drawText(const std::string& s, float x, float y, unsigned sz, sf::Color col);
};

#endif // SPREADSHEET_UI_H
