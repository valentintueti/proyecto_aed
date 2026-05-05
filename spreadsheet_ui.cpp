#include "spreadsheet_ui.h"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <cmath>
#include <map>

// ---- autocomplete data ----

static const std::vector<std::string> CELL_COMPLETIONS = {
    "SUMA(", "MAX(", "MIN(", "PROMEDIO(", "MAXIMO(", "MINIMO("
};

static const std::vector<std::string> CMD_COMPLETIONS = {
    "SUMA(", "MAX(", "MIN(", "PROMEDIO(",
    "ELIMINAR_FILA(", "ELIMINAR_COL(", "ELIMINAR(",
    "LISTAR", "VER_FILA(", "VER_COL("
};

static const std::map<std::string, std::string> FUNC_HINTS = {
    {"SUMA",          "SUMA(A1:B5)   o   SUMA(A)   o   SUMA(1)"},
    {"MAX",           "MAX(A1:B5)"},
    {"MIN",           "MIN(A1:B5)"},
    {"MAXIMO",        "MAXIMO(A1:B5)"},
    {"MINIMO",        "MINIMO(A1:B5)"},
    {"PROMEDIO",      "PROMEDIO(A1:B5)"},
    {"ELIMINAR_FILA", "ELIMINAR_FILA(3)   → borra fila 3"},
    {"ELIMINAR_COL",  "ELIMINAR_COL(B)    → borra col B"},
    {"ELIMINAR",      "ELIMINAR(A1:C4)    → borra rango"},
    {"VER_FILA",      "VER_FILA(2)        → muestra fila 2"},
    {"VER_COL",       "VER_COL(B)         → muestra col B"},
};

// ---- constructor ----

SpreadsheetUI::SpreadsheetUI()
    : window(sf::VideoMode(1280, 720), "Hoja de Calculo - Sparse Matrix")
{
    window.setFramerateLimit(60);
    if (!loadFont()) {
        std::cerr << "No se pudo cargar ninguna fuente.\n";
    }
}

bool SpreadsheetUI::loadFont() {
    const char* paths[] = {
        // macOS
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/Library/Fonts/Arial.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "/System/Library/Fonts/Monaco.ttf",
        "/System/Library/Fonts/Menlo.ttc",
        "/System/Library/Fonts/SFNSMono.ttf",
        // Linux
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        // Windows
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/consola.ttf",
        nullptr
    };
    for (int i = 0; paths[i]; ++i)
        if (font.loadFromFile(paths[i])) return true;
    return false;
}

// ---- cell data helpers ----

static std::string formatNum(double v) {
    if (std::isnan(v) || std::isinf(v)) return "ERR";
    if (v >= -1e15 && v <= 1e15 && (double)(long long)v == v)
        return std::to_string((long long)v);
    std::ostringstream os; os << v; return os.str();
}

static std::string toUpperStr(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::toupper(c); });
    return s;
}

void SpreadsheetUI::setCellValue(int r, int c, const std::string& text) {
    if (text.empty()) { deleteCell(r, c); return; }
    if (text[0] == '=') {
        double val = evalFormula(text, mat);
        mat.set(r, c, val);
        cellText[{r, c}] = text;
    } else {
        try {
            double val = std::stod(text);
            mat.set(r, c, val);
            cellText.erase({r, c});
        } catch(...) {
            // Non-numeric text: store as text cell, no numeric entry in mat
            try { mat.remove(r, c); } catch(...) {}
            cellText[{r, c}] = text;
        }
    }
}

void SpreadsheetUI::deleteCell(int r, int c) {
    try { mat.remove(r, c); } catch(...) {}
    cellText.erase({r, c});
}

void SpreadsheetUI::deleteRow(int r) {
    try { mat.remove_row(r); } catch(...) {}
    for (auto it = cellText.begin(); it != cellText.end(); )
        (it->first.first == r) ? it = cellText.erase(it) : ++it;
}

void SpreadsheetUI::deleteCol(int c) {
    try { mat.remove_col(c); } catch(...) {}
    for (auto it = cellText.begin(); it != cellText.end(); )
        (it->first.second == c) ? it = cellText.erase(it) : ++it;
}

void SpreadsheetUI::deleteRange(int r1, int c1, int r2, int c2) {
    try { mat.remove_range(r1, c1, r2, c2); } catch(...) {}
    for (auto it = cellText.begin(); it != cellText.end(); ) {
        auto [r, c] = it->first;
        (r >= r1 && r <= r2 && c >= c1 && c <= c2) ? it = cellText.erase(it) : ++it;
    }
}

std::string SpreadsheetUI::getCellFormulaText(int r, int c) const {
    auto it = cellText.find({r, c});
    if (it != cellText.end()) return it->second;
    try {
        double v = mat.get(r, c);
        return (v == 0.0) ? "" : formatNum(v);
    } catch(...) { return ""; }
}

std::string SpreadsheetUI::getCellDisplayText(int r, int c) const {
    auto it = cellText.find({r, c});
    if (it != cellText.end()) {
        if (!it->second.empty() && it->second[0] == '=') {
            try { return formatNum(mat.get(r, c)); } catch(...) { return "0"; }
        }
        return it->second;
    }
    try {
        double v = mat.get(r, c);
        return (v == 0.0) ? "" : formatNum(v);
    } catch(...) { return ""; }
}

int SpreadsheetUI::visibleMaxCol() const {
    int mc = 25; // at least A-Z
    for (auto& n : mat.getAllNodes()) if (n.col > mc) mc = n.col;
    return mc;
}

// ---- autocomplete ----

void SpreadsheetUI::updateSuggestions() {
    suggestions.clear();
    sugIdx = 0;

    std::string prefix;
    bool cellMode = false;

    if (editing && !editText.empty() && editText[0] == '=') {
        prefix = editText.substr(1);
        cellMode = true;
    } else if (toolbarActive) {
        prefix = toolbarInput;
    } else {
        return;
    }

    // Already inside a function call — no completion, hint handled in drawAutocomplete
    if (prefix.find('(') != std::string::npos) return;

    std::string up = toUpperStr(prefix);
    const auto& list = cellMode ? CELL_COMPLETIONS : CMD_COMPLETIONS;
    for (const auto& c : list) {
        if (toUpperStr(c).substr(0, up.size()) == up)
            suggestions.push_back(c);
    }
}

void SpreadsheetUI::applySuggestion() {
    if (suggestions.empty()) return;
    const std::string& sug = suggestions[sugIdx % suggestions.size()];
    if (editing && !editText.empty() && editText[0] == '=')
        editText = "=" + sug;
    else if (toolbarActive)
        toolbarInput = sug;
    suggestions.clear();
}

void SpreadsheetUI::drawAutocomplete() {
    std::string prefix;
    bool cellMode = false;

    if (editing && !editText.empty() && editText[0] == '=') {
        prefix = editText.substr(1);
        cellMode = true;
    } else if (toolbarActive) {
        prefix = toolbarInput;
    } else {
        return;
    }

    float dropX = 62.f, dropY = (float)TOOLBAR_H;

    // ---- inside a function: show argument hint ----
    size_t paren = prefix.find('(');
    if (paren != std::string::npos) {
        std::string fname = toUpperStr(prefix.substr(0, paren));
        auto it = FUNC_HINTS.find(fname);
        if (it != FUNC_HINTS.end()) {
            std::string hint = (cellMode ? "=" : "") + it->second;
            float hw = std::max(340.f, (float)hint.size() * 7.5f);
            drawRect(dropX, dropY, hw, 22, sf::Color(255,255,200,240), C_LILAC_DK, 1.f);
            drawText(hint, dropX + 6, dropY + 3, 12, sf::Color(80, 40, 120));
        }
        return;
    }

    // ---- dropdown with completions ----
    if (suggestions.empty()) return;

    float rowH = 22.f;
    float w    = 320.f;
    float h    = suggestions.size() * rowH + 4;
    drawRect(dropX, dropY, w, h, C_WHITE, C_LILAC_DK, 1.f);

    for (int i = 0; i < (int)suggestions.size(); i++) {
        float sy  = dropY + 2 + i * rowH;
        bool  sel = (i == sugIdx % (int)suggestions.size());
        if (sel) drawRect(dropX + 1, sy, w - 2, rowH - 2, C_LILAC, C_LILAC, 0.f);

        // Build display: "=SUMA("  →  "SUMA(A1:B5)"
        std::string s = suggestions[i];
        std::string fname = toUpperStr(s);
        if (!fname.empty() && fname.back() == '(') fname.pop_back();
        auto hit = FUNC_HINTS.find(fname);
        std::string line = (cellMode ? "=" : "") + s;
        if (hit != FUNC_HINTS.end()) line += "   " + hit->second;

        drawText(line, dropX + 6, sy + 3, 12, C_BLACK);
    }
}

// ---- input handling ----

void SpreadsheetUI::commitEdit() {
    setCellValue(selRow, selCol, editText);
    editing = false;
    editText.clear();
    suggestions.clear();
    toolbarResult.clear();
    toolbarError = false;
}

std::string SpreadsheetUI::runToolbarCommand(const std::string& cmd) {
    if (cmd.empty()) return "";
    std::string uc = cmd;
    std::transform(uc.begin(), uc.end(), uc.begin(), [](unsigned char c){ return std::toupper(c); });

    // Delete operations handled here so cellText stays in sync
    auto parseParen = [&](std::string& a1, std::string& a2) -> bool {
        size_t p1 = cmd.find('('), p2 = cmd.rfind(')');
        if (p1 == std::string::npos || p2 == std::string::npos || p2 < p1) return false;
        std::string inside = cmd.substr(p1+1, p2-p1-1);
        size_t sep = inside.find(':');
        if (sep == std::string::npos) sep = inside.find(',');
        if (sep != std::string::npos) { a1 = inside.substr(0,sep); a2 = inside.substr(sep+1); }
        else { a1 = inside; a2 = ""; }
        return true;
    };

    std::string a1, a2;
    if ((uc.find("ELIMINAR_FILA(") == 0 || uc.find("DELETE_ROW(") == 0) && parseParen(a1, a2)) {
        bool ok = false;
        try { int r = std::stoi(a1)-1; deleteRow(r); ok = true; } catch(...) {}
        return ok ? "Fila " + a1 + " eliminada" : "Error: fila no encontrada";
    }
    if ((uc.find("ELIMINAR_COL(") == 0 || uc.find("DELETE_COL(") == 0) && parseParen(a1, a2)) {
        std::string ua1 = a1; std::transform(ua1.begin(),ua1.end(),ua1.begin(),[](unsigned char c){return std::toupper(c);});
        deleteCol(strToCol(ua1));
        return "Columna " + ua1 + " eliminada";
    }
    if ((uc.find("ELIMINAR(") == 0 || uc.find("DELETE(") == 0) && parseParen(a1, a2)) {
        if (!a2.empty()) {
            auto c1 = parseCell(a1), c2 = parseCell(a2);
            if (c1.first < 0 || c2.first < 0) return "Error: rango invalido";
            deleteRange(c1.first,c1.second,c2.first,c2.second);
            return "Rango eliminado";
        }
        auto c = parseCell(a1);
        if (c.first < 0) return "Error: celda invalida";
        deleteCell(c.first, c.second);
        return "Celda eliminada";
    }

    return execCommand(cmd, mat);
}

void SpreadsheetUI::handleText(uint32_t unicode) {
    if (unicode < 32 || unicode > 126) return;
    char c = (char)unicode;
    if (toolbarActive) {
        toolbarInput += c;
        updateSuggestions();
    } else if (editing) {
        editText += c;
        updateSuggestions();
    } else {
        editing = true;
        editText = std::string(1, c);
        updateSuggestions();
    }
}

void SpreadsheetUI::handleKey(const sf::Event::KeyEvent& key, int visCols, int visRows) {
    if (toolbarActive) {
        if (key.code == sf::Keyboard::Escape) {
            if (!suggestions.empty()) { suggestions.clear(); sugIdx = 0; }
            else { toolbarActive = false; toolbarInput.clear(); toolbarResult.clear(); }
        }
        if (key.code == sf::Keyboard::Enter) {
            suggestions.clear();
            toolbarResult = runToolbarCommand(toolbarInput);
            toolbarError = (toolbarResult.find("Error") == 0);
            toolbarActive = false;
            toolbarInput.clear();
        }
        if (key.code == sf::Keyboard::Tab && !suggestions.empty()) {
            applySuggestion();
        }
        if (key.code == sf::Keyboard::Down && !suggestions.empty()) {
            sugIdx = (sugIdx + 1) % (int)suggestions.size();
        }
        if (key.code == sf::Keyboard::Up && !suggestions.empty()) {
            sugIdx = (sugIdx + (int)suggestions.size() - 1) % (int)suggestions.size();
        }
        if (key.code == sf::Keyboard::BackSpace && !toolbarInput.empty()) {
            toolbarInput.pop_back();
            updateSuggestions();
        }
        return;
    }
    if (editing) {
        if (key.code == sf::Keyboard::Escape) {
            if (!suggestions.empty()) { suggestions.clear(); sugIdx = 0; }
            else { editing = false; editText.clear(); }
        }
        if (key.code == sf::Keyboard::Enter)  { commitEdit(); selRow++; }
        if (key.code == sf::Keyboard::Tab) {
            if (!suggestions.empty()) applySuggestion();
            else { commitEdit(); selCol++; }
        }
        if (key.code == sf::Keyboard::Down && !suggestions.empty()) {
            sugIdx = (sugIdx + 1) % (int)suggestions.size();
        }
        if (key.code == sf::Keyboard::Up && !suggestions.empty()) {
            sugIdx = (sugIdx + (int)suggestions.size() - 1) % (int)suggestions.size();
        }
        if (key.code == sf::Keyboard::BackSpace && !editText.empty()) {
            editText.pop_back();
            updateSuggestions();
        }
        return;
    }

    // Navigation
    if (key.code == sf::Keyboard::Up)    { selRow = std::max(0, selRow-1); selRow2 = selCol2 = -1; }
    if (key.code == sf::Keyboard::Down)  { selRow++;                        selRow2 = selCol2 = -1; }
    if (key.code == sf::Keyboard::Left)  { selCol = std::max(0, selCol-1); selRow2 = selCol2 = -1; }
    if (key.code == sf::Keyboard::Right) { selCol++;                        selRow2 = selCol2 = -1; }

    if (key.code == sf::Keyboard::Delete) {
        if (selRow2 >= 0 && selCol2 >= 0) {
            int r1=std::min(selRow,selRow2), r2=std::max(selRow,selRow2);
            int c1=std::min(selCol,selCol2), c2=std::max(selCol,selCol2);
            deleteRange(r1,c1,r2,c2);
        } else {
            deleteCell(selRow, selCol);
        }
        selRow2 = selCol2 = -1;
    }

    if (key.code == sf::Keyboard::F2) {
        editing = true;
        editText = getCellFormulaText(selRow, selCol);
    }

    // Tab → toolbar / formula bar
    if (key.code == sf::Keyboard::Tab) {
        toolbarActive = true;
        toolbarInput.clear();
        toolbarResult.clear();
        toolbarError = false;
    }

    // Auto-scroll
    if (selRow < scrollY) scrollY = selRow;
    if (selRow >= scrollY + visRows - 1) scrollY = selRow - visRows + 2;
    if (selCol < scrollX) scrollX = selCol;
    if (selCol >= scrollX + visCols - 1) scrollX = selCol - visCols + 2;
}

void SpreadsheetUI::handleEvent(const sf::Event& ev, int visCols, int visRows) {
    if (ev.type == sf::Event::Closed) { window.close(); return; }

    if (ev.type == sf::Event::MouseWheelScrolled) {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift))
            scrollX = std::max(0, scrollX - (int)(ev.mouseWheelScroll.delta * 3));
        else
            scrollY = std::max(0, scrollY - (int)(ev.mouseWheelScroll.delta * 3));
        return;
    }

    if (ev.type == sf::Event::MouseButtonPressed && ev.mouseButton.button == sf::Mouse::Left) {
        int mx = ev.mouseButton.x, my = ev.mouseButton.y;
        if (my < TOOLBAR_H) {
            // Click formula bar → activate toolbar
            if (editing) commitEdit();
            toolbarActive = true;
            toolbarInput.clear();
            toolbarResult.clear();
            toolbarError = false;
        } else {
            if (editing) commitEdit();
            toolbarActive = false;
            toolbarResult.clear();
            toolbarError = false;
            suggestions.clear();
            int clickCol = (mx - HDR_W) / CELL_W + scrollX;
            int clickRow = (my - TOOLBAR_H - HDR_H) / CELL_H + scrollY;
            if (clickCol >= 0 && clickRow >= 0) {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift))
                    { selRow2 = clickRow; selCol2 = clickCol; }
                else
                    { selRow = clickRow; selCol = clickCol; selRow2 = selCol2 = -1; }
            }
        }
        return;
    }

    if (ev.type == sf::Event::KeyPressed) handleKey(ev.key, visCols, visRows);
    if (ev.type == sf::Event::TextEntered) handleText(ev.text.unicode);
}

// ---- rendering ----

void SpreadsheetUI::drawRect(float x, float y, float w, float h,
                              sf::Color fill, sf::Color outline, float thick) {
    sf::RectangleShape r({w, h});
    r.setPosition(x, y); r.setFillColor(fill);
    r.setOutlineColor(outline); r.setOutlineThickness(thick);
    window.draw(r);
}

void SpreadsheetUI::drawText(const std::string& s, float x, float y, unsigned sz, sf::Color col) {
    sf::Text t(s, font, sz);
    t.setFillColor(col);
    t.setPosition(x, y);
    window.draw(t);
}

void SpreadsheetUI::drawToolbar() {
    float W = (float)window.getSize().x;
    drawRect(0, 0, W, TOOLBAR_H, C_TOOLBAR, C_TOOLBAR);

    // Cell reference box
    std::string cellRef = colToStr(selCol) + std::to_string(selRow + 1);
    drawRect(4, 4, 54, TOOLBAR_H-8, C_WHITE, C_LILAC_DK, 1.f);
    drawText(cellRef, 8, 8, 13, C_BLACK);

    // Formula bar
    float fbX = 62, fbW = W - fbX - 4;
    drawRect(fbX, 4, fbW, TOOLBAR_H-8, C_TBINPUT, C_LILAC_DK, 1.f);

    if (toolbarActive) {
        std::string prompt = toolbarInput + "_";
        drawText(prompt, fbX + 4, 8, 13, C_BLACK);
    } else if (!toolbarResult.empty()) {
        sf::Color rc = toolbarError ? C_ERR : sf::Color(20, 120, 20);
        drawText(toolbarResult, fbX + 4, 8, 13, rc);
    } else if (editing) {
        drawText(editText + "_", fbX + 4, 8, 13, C_BLACK);
    } else {
        std::string formula = getCellFormulaText(selRow, selCol);
        drawText(formula, fbX + 4, 8, 13, C_BLACK);
    }
}

void SpreadsheetUI::drawHeaders(int maxCol) {
    float W = (float)window.getSize().x;
    // Row-header top-left corner
    drawRect(0, TOOLBAR_H, HDR_W, HDR_H, C_LILAC, C_GRID);

    // Column headers
    for (int c = scrollX; (c - scrollX) * CELL_W < W - HDR_W; c++) {
        float x = HDR_W + (c - scrollX) * CELL_W;
        bool sel = (selRow2 < 0) ? (c == selCol) :
                   (c >= std::min(selCol,selCol2) && c <= std::max(selCol,selCol2));
        drawRect(x, TOOLBAR_H, CELL_W, HDR_H, sel ? C_LILAC_DK : C_LILAC, C_GRID);
        std::string lbl = colToStr(c);
        sf::Text t(lbl, font, 13); t.setFillColor(sel ? C_WHITE : C_BLACK);
        t.setPosition(x + (CELL_W - t.getLocalBounds().width)/2.f, TOOLBAR_H + 6);
        window.draw(t);
    }

    // Row headers
    float H = (float)window.getSize().y;
    for (int r = scrollY; (r - scrollY) * CELL_H < H - TOOLBAR_H - HDR_H; r++) {
        float y = TOOLBAR_H + HDR_H + (r - scrollY) * CELL_H;
        bool sel = (selRow2 < 0) ? (r == selRow) :
                   (r >= std::min(selRow,selRow2) && r <= std::max(selRow,selRow2));
        drawRect(0, y, HDR_W, CELL_H, sel ? C_LILAC_DK : C_LILAC, C_GRID);
        std::string lbl = std::to_string(r + 1);
        sf::Text t(lbl, font, 12); t.setFillColor(sel ? C_WHITE : C_BLACK);
        t.setPosition(HDR_W/2.f - t.getLocalBounds().width/2.f, y + 5);
        window.draw(t);
    }
}

void SpreadsheetUI::drawCells(int maxCol, int visRows) {
    float W = (float)window.getSize().x;
    float H = (float)window.getSize().y;

    // Preload occupied cells into a map for O(1) lookup during render
    std::map<std::pair<int,int>, std::string> display;
    for (auto& n : mat.getAllNodes()) {
        auto key = std::make_pair(n.row, n.col);
        auto it = cellText.find(key);
        if (it != cellText.end() && !it->second.empty() && it->second[0] == '=')
            display[key] = formatNum(n.data);
        else
            display[key] = formatNum(n.data);
    }
    // Override with text cells
    for (auto& [key, txt] : cellText) {
        if (!txt.empty() && txt[0] != '=')
            display[key] = txt;
    }

    int r1sel = (selRow2 < 0) ? selRow : std::min(selRow, selRow2);
    int r2sel = (selRow2 < 0) ? selRow : std::max(selRow, selRow2);
    int c1sel = (selCol2 < 0) ? selCol : std::min(selCol, selCol2);
    int c2sel = (selCol2 < 0) ? selCol : std::max(selCol, selCol2);

    for (int r = scrollY; (r - scrollY) * CELL_H < H - TOOLBAR_H - HDR_H; r++) {
        for (int c = scrollX; (c - scrollX) * CELL_W < W - HDR_W; c++) {
            float x = HDR_W + (c - scrollX) * CELL_W;
            float y = TOOLBAR_H + HDR_H + (r - scrollY) * CELL_H;

            bool isCursor = (r == selRow && c == selCol);
            bool inRange  = (r >= r1sel && r <= r2sel && c >= c1sel && c <= c2sel) && !isCursor;

            sf::Color fill = C_WHITE;
            float thick = 0.5f;
            sf::Color outlineCol = C_GRID;

            if (isCursor) { outlineCol = C_LILAC_DK; thick = 2.f; }
            else if (inRange) fill = C_SEL;

            drawRect(x, y, CELL_W, CELL_H, fill, outlineCol, thick);

            std::string txt;
            if (editing && isCursor) {
                txt = editText + "_";
            } else {
                auto it = display.find({r, c});
                if (it != display.end()) txt = it->second;
            }

            if (!txt.empty()) {
                sf::Text t(txt, font, 12);
                t.setFillColor(C_BLACK);
                // Clip text inside cell (simple: just truncate visually via position)
                t.setPosition(x + 4, y + 5);
                // Simple clipping: only draw if text width fits, otherwise truncate string
                window.draw(t);
            }
        }
    }
}

void SpreadsheetUI::render(int maxCol, int visCols, int visRows) {
    window.clear(C_LILAC_LT);
    drawToolbar();
    drawHeaders(maxCol);
    drawCells(maxCol, visRows);
    drawAutocomplete(); // siempre encima del grid
    window.display();
}

// ---- main loop ----

int SpreadsheetUI::run() {
    while (window.isOpen()) {
        int maxCol = visibleMaxCol();
        int visCols = ((int)window.getSize().x - HDR_W) / CELL_W + 2;
        int visRows = ((int)window.getSize().y - TOOLBAR_H - HDR_H) / CELL_H + 2;

        sf::Event ev;
        while (window.pollEvent(ev))
            handleEvent(ev, visCols, visRows);

        render(maxCol, visCols, visRows);
    }
    return 0;
}
