#include "spreadsheet_ui.h"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <cmath>
#include <map>

// ---- autocomplete data ----

static const std::vector<std::string> CELL_COMPLETIONS = {
    "SUMA(", "MAX(", "MIN(", "PROMEDIO(", "MAXIMO(", "MINIMO(", "CONTAR("
};

static const std::vector<std::string> CMD_COMPLETIONS = {
    "SUMA(", "MAX(", "MIN(", "PROMEDIO(", "CONTAR(",
    "ELIMINAR_FILA(", "ELIMINAR_COL(", "ELIMINAR(",
    "LISTAR", "VER_FILA(", "VER_COL("
};

static const std::map<std::string, std::string> FUNC_HINTS = {
    {"SUMA",          "SUMA(A1:B5)   o   SUMA(A)   o   SUMA(1)"},
    {"MAX",           "MAX(A1:B5)"},
    {"MIN",           "MIN(A1:B5)"},
    {"MAXIMO",        "MAXIMO(A1:B5)"},
    {"MINIMO",        "MINIMO(A1:B5)"},
    {"PROMEDIO",      "PROMEDIO(A1:B5)   o   PROMEDIO(A)   o   PROMEDIO(1)"},
    {"CONTAR",        "CONTAR(A1:B5)   o   CONTAR(A)   o   CONTAR(1)"},
    {"ELIMINAR_FILA", "ELIMINAR_FILA(3)   → borra fila 3"},
    {"ELIMINAR_COL",  "ELIMINAR_COL(B)    → borra col B"},
    {"ELIMINAR",      "ELIMINAR(A1:C4)    → borra rango"},
    {"VER_FILA",      "VER_FILA(2)        → muestra fila 2"},
    {"VER_COL",       "VER_COL(B)         → muestra col B"},
};

// ---- undo / clipboard ----

static constexpr int MAX_UNDO = 30;

void SpreadsheetUI::pushUndo() {
    std::vector<CellEntry> snap;
    for (auto& n : mat.getAllNodes()) {
        std::string formula;
        auto it = cellText.find({n.row, n.col});
        if (it != cellText.end()) formula = it->second;
        snap.push_back({n.row, n.col, n.data, formula});
    }
    undoStack.push_back(snap);
    if ((int)undoStack.size() > MAX_UNDO) undoStack.pop_front();
}

void SpreadsheetUI::undo() {
    if (undoStack.empty()) return;
    auto snap = undoStack.back();
    undoStack.pop_back();

    // Remove all current nodes
    auto current = mat.getAllNodes();
    for (auto& n : current) try { mat.remove(n.row, n.col); } catch(...) {}
    cellText.clear();

    // Restore snapshot
    for (auto& e : snap) {
        mat.set(e.row, e.col, e.val);
        if (!e.formula.empty()) cellText[{e.row, e.col}] = e.formula;
    }
}

void SpreadsheetUI::copySelection() {
    int r1 = std::min(selRow, selRow2 < 0 ? selRow : selRow2);
    int r2 = std::max(selRow, selRow2 < 0 ? selRow : selRow2);
    int c1 = std::min(selCol, selCol2 < 0 ? selCol : selCol2);
    int c2 = std::max(selCol, selCol2 < 0 ? selCol : selCol2);

    clipboard.clear();
    for (int r = r1; r <= r2; r++) {
        for (int c = c1; c <= c2; c++) {
            if (c > c1) clipboard += '\t';
            clipboard += getCellFormulaText(r, c);
        }
        clipboard += '\n';
    }
    sf::Clipboard::setString(clipboard);
}

void SpreadsheetUI::pasteAt(int startRow, int startCol) {
    std::string clip = sf::Clipboard::getString();
    if (clip.empty()) clip = clipboard;
    if (clip.empty()) return;
    pushUndo();
    int r = startRow, c = startCol;
    std::string cell;
    for (char ch : clip) {
        if (ch == '\t') { setCellValue(r, c++, cell); cell.clear(); }
        else if (ch == '\n') { setCellValue(r++, c, cell); cell.clear(); c = startCol; }
        else cell += ch;
    }
}

// ---- formula range selection helpers ----

bool SpreadsheetUI::isFormulaOpen() const {
    if (!editing || editText.empty() || editText[0] != '=') return false;
    int depth = 0;
    for (char ch : editText) { if (ch == '(') depth++; else if (ch == ')') depth--; }
    return depth > 0;
}

static size_t findInsertPos(const std::string& editText) {
    size_t p = editText.rfind('(');
    size_t q = editText.rfind(',');
    return (q != std::string::npos && q > p) ? q + 1 : p + 1;
}

void SpreadsheetUI::insertFormulaRef(int r, int c) {
    std::string ref = colToStr(c) + std::to_string(r + 1);
    size_t pos = findInsertPos(editText);
    editText = editText.substr(0, pos) + ref;
    updateSuggestions();
}

void SpreadsheetUI::insertFormulaRange(int r1, int c1, int r2, int c2) {
    int minR = std::min(r1,r2), maxR = std::max(r1,r2);
    int minC = std::min(c1,c2), maxC = std::max(c1,c2);
    std::string ref;
    if (minR == maxR && minC == maxC)
        ref = colToStr(minC) + std::to_string(minR + 1);
    else
        ref = colToStr(minC) + std::to_string(minR+1) + ":" +
              colToStr(maxC) + std::to_string(maxR+1);
    size_t pos = findInsertPos(editText);
    editText = editText.substr(0, pos) + ref;
    updateSuggestions();
}

// ---- delete selection ----

void SpreadsheetUI::executeDeleteSelection() {
    int r1 = std::min(selRow, selRow2 < 0 ? selRow : selRow2);
    int r2 = std::max(selRow, selRow2 < 0 ? selRow : selRow2);
    int c1 = std::min(selCol, selCol2 < 0 ? selCol : selCol2);
    int c2 = std::max(selCol, selCol2 < 0 ? selCol : selCol2);
    pushUndo();
    deleteRange(r1, c1, r2, c2);
    recalcAllFormulas();
    selRow2 = selCol2 = -1;
}

// ---- formula recalculation ----

void SpreadsheetUI::recalcAllFormulas() {
    // Primero eliminar todos los valores de las celdas fórmula para evitar
    // que una fórmula se incluya a sí misma en el cálculo (ej. =SUMA(C) en C5)
    for (auto& [key, _] : cellText)
        try { mat.remove(key.first, key.second); } catch(...) {}

    // Luego re-evaluar con la matriz limpia de resultados anteriores
    for (auto& [key, formula] : cellText) {
        double val = evalFormula(formula, mat);
        mat.set(key.first, key.second, CellValue{val});
    }
}

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
    if (std::isnan(v))  return "#!VALOR!";
    if (std::isinf(v))  return "#!DIV/0!";
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
        // update() modifica si ya existe el nodo; set() lo crea si no existe
        if (!mat.update(r, c, CellValue{val}))
            mat.set(r, c, CellValue{val});
        cellText[{r, c}] = text;
    } else {
        cellText.erase({r, c});
        CellValue val;
        try { val = CellValue{std::stod(text)}; }
        catch(...) { val = CellValue{text}; }
        if (!mat.update(r, c, val))
            mat.set(r, c, val);
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
        CellValue v = mat.get(r, c);
        if (auto* d = std::get_if<double>(&v)) return (*d == 0.0) ? "" : formatNum(*d);
        if (auto* s = std::get_if<std::string>(&v)) return *s;
        return "";
    } catch(...) { return ""; }
}

std::string SpreadsheetUI::getCellDisplayText(int r, int c) const {
    auto it = cellText.find({r, c});
    if (it != cellText.end()) {
        // Formula cell: display the computed double stored in mat
        try {
            CellValue v = mat.get(r, c);
            if (auto* d = std::get_if<double>(&v)) return formatNum(*d);
            return "0";
        } catch(...) { return "0"; }
    }
    try {
        CellValue v = mat.get(r, c);
        if (auto* d = std::get_if<double>(&v)) return (*d == 0.0) ? "" : formatNum(*d);
        if (auto* s = std::get_if<std::string>(&v)) return *s;
        return "";
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
    pushUndo();
    setCellValue(selRow, selCol, editText);
    recalcAllFormulas();
    editing = false;
    editText.clear();
    suggestions.clear();
    formulaCellRow = formulaCellCol = -1;
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
        try { int r = std::stoi(a1)-1; pushUndo(); deleteRow(r); recalcAllFormulas(); ok = true; } catch(...) {}
        return ok ? "Fila " + a1 + " eliminada" : "Error: fila no encontrada";
    }
    if ((uc.find("ELIMINAR_COL(") == 0 || uc.find("DELETE_COL(") == 0) && parseParen(a1, a2)) {
        std::string ua1 = a1; std::transform(ua1.begin(),ua1.end(),ua1.begin(),[](unsigned char c){return std::toupper(c);});
        pushUndo(); deleteCol(strToCol(ua1)); recalcAllFormulas();
        return "Columna " + ua1 + " eliminada";
    }
    if ((uc.find("ELIMINAR(") == 0 || uc.find("DELETE(") == 0) && parseParen(a1, a2)) {
        if (!a2.empty()) {
            auto c1 = parseCell(a1), c2 = parseCell(a2);
            if (c1.first < 0 || c2.first < 0) return "Error: rango invalido";
            pushUndo(); deleteRange(c1.first,c1.second,c2.first,c2.second); recalcAllFormulas();
            return "Rango eliminado";
        }
        auto c = parseCell(a1);
        if (c.first < 0) return "Error: celda invalida";
        pushUndo(); deleteCell(c.first, c.second); recalcAllFormulas();
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
        if (key.code == sf::Keyboard::Delete) {
            editText.clear(); suggestions.clear();
            editing = false;
            executeDeleteSelection();
        }
        if (key.code == sf::Keyboard::BackSpace) {
            if (!editText.empty()) { editText.pop_back(); updateSuggestions(); }
            else { editing = false; executeDeleteSelection(); }
        }
        return;
    }

    // Ctrl / Cmd shortcuts (Cmd = LSystem on macOS)
    bool ctrl = key.control || key.system;
    if (ctrl && key.code == sf::Keyboard::C) { copySelection(); return; }
    if (ctrl && key.code == sf::Keyboard::V) { pasteAt(selRow, selCol); return; }
    if (ctrl && key.code == sf::Keyboard::Z) { undo(); return; }
    // Ctrl+Home (Ctrl+Fn+← en Mac) o Ctrl+Escape → volver a A1
    if (ctrl && (key.code == sf::Keyboard::Home || key.code == sf::Keyboard::Escape)) {
        selRow = selCol = scrollX = scrollY = 0;
        selRow2 = selCol2 = -1; return;
    }
    // Escape solo (sin editar) → volver a A1 también
    if (key.code == sf::Keyboard::Escape) {
        selRow = selCol = scrollX = scrollY = 0;
        selRow2 = selCol2 = -1; return;
    }
    // Home solo (Fn+←) → inicio de la fila (col A)
    if (key.code == sf::Keyboard::Home) {
        selCol = scrollX = 0; selRow2 = selCol2 = -1; return;
    }

    // Navigation — plain arrows move cursor; Shift+arrows extend selection
    bool shift = key.shift;
    if (key.code == sf::Keyboard::Up) {
        if (shift) { if (selRow2 < 0) { selRow2 = selRow; selCol2 = selCol; } selRow2 = std::max(0, selRow2-1); }
        else       { selRow = std::max(0, selRow-1); selRow2 = selCol2 = -1; }
    }
    if (key.code == sf::Keyboard::Down) {
        if (shift) { if (selRow2 < 0) { selRow2 = selRow; selCol2 = selCol; } selRow2++; }
        else       { selRow++;         selRow2 = selCol2 = -1; }
    }
    if (key.code == sf::Keyboard::Left) {
        if (shift) { if (selRow2 < 0) { selRow2 = selRow; selCol2 = selCol; } selCol2 = std::max(0, selCol2-1); }
        else       { selCol = std::max(0, selCol-1); selRow2 = selCol2 = -1; }
    }
    if (key.code == sf::Keyboard::Right) {
        if (shift) { if (selRow2 < 0) { selRow2 = selRow; selCol2 = selCol; } selCol2++; }
        else       { selCol++;          selRow2 = selCol2 = -1; }
    }

    // En macOS la tecla ⌫ es BackSpace en SFML, no Delete
    if (key.code == sf::Keyboard::Delete || key.code == sf::Keyboard::BackSpace) {
        executeDeleteSelection();
    }

    if (key.code == sf::Keyboard::F2) {
        editing = true;
        formulaCellRow = selRow; formulaCellCol = selCol;
        editText = getCellFormulaText(selRow, selCol);
    }

    // Tab → command toolbar
    if (key.code == sf::Keyboard::Tab) {
        toolbarActive = true;
        toolbarInput.clear();
        toolbarResult.clear();
        toolbarError = false;
    }

    // Auto-scroll
    int activeR = (selRow2 >= 0) ? selRow2 : selRow;
    int activeC = (selCol2 >= 0) ? selCol2 : selCol;
    if (activeR < scrollY) scrollY = activeR;
    if (activeR >= scrollY + visRows - 1) scrollY = activeR - visRows + 2;
    if (activeC < scrollX) scrollX = activeC;
    if (activeC >= scrollX + visCols - 1) scrollX = activeC - visCols + 2;
}

void SpreadsheetUI::handleEvent(const sf::Event& ev, int visCols, int visRows) {
    if (ev.type == sf::Event::Closed) { window.close(); return; }

    if (ev.type == sf::Event::MouseWheelScrolled) {
        bool horizontal = (ev.mouseWheelScroll.wheel == sf::Mouse::HorizontalWheel)
                       || sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)
                       || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);
        if (horizontal)
            scrollX = std::max(0, scrollX - (int)(ev.mouseWheelScroll.delta * 3));
        else
            scrollY = std::max(0, scrollY - (int)(ev.mouseWheelScroll.delta * 3));
        return;
    }

    if (ev.type == sf::Event::MouseButtonPressed && ev.mouseButton.button == sf::Mouse::Left) {
        int mx = ev.mouseButton.x, my = ev.mouseButton.y;
        if (my < TOOLBAR_H) {
            // Click formula bar → edit current cell's formula (like Excel)
            if (editing) commitEdit();
            toolbarActive = false;
            toolbarResult.clear();
            toolbarError = false;
            editing = true;
            editText = getCellFormulaText(selRow, selCol);
            suggestions.clear();
            updateSuggestions();
        } else {
            int clickCol = (mx - HDR_W) / CELL_W + scrollX;
            int clickRow = (my - TOOLBAR_H - HDR_H) / CELL_H + scrollY;
            if (clickCol >= 0 && clickRow >= 0) {
                // Formula range selection: if editing a formula with open '('
                if (isFormulaOpen()) {
                    insertFormulaRef(clickRow, clickCol);
                    formulaDragAnchorRow = clickRow;
                    formulaDragAnchorCol = clickCol;
                    isDragging = true;
                    return;
                }
                if (editing) commitEdit();
                toolbarActive = false;
                toolbarResult.clear();
                toolbarError = false;
                suggestions.clear();
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift))
                    { selRow2 = clickRow; selCol2 = clickCol; }
                else {
                    selRow = clickRow; selCol = clickCol;
                    selRow2 = selCol2 = -1;
                    isDragging = true;
                }
            }
        }
        return;
    }

    if (ev.type == sf::Event::MouseMoved && isDragging) {
        int mx = ev.mouseMove.x, my = ev.mouseMove.y;
        int dragCol = (mx - HDR_W) / CELL_W + scrollX;
        int dragRow = (my - TOOLBAR_H - HDR_H) / CELL_H + scrollY;
        if (dragCol >= 0 && dragRow >= 0) {
            if (isFormulaOpen() && formulaDragAnchorRow >= 0) {
                // Update formula range reference live
                insertFormulaRange(formulaDragAnchorRow, formulaDragAnchorCol, dragRow, dragCol);
            } else {
                selRow2 = dragRow;
                selCol2 = dragCol;
            }
        }
        return;
    }

    if (ev.type == sf::Event::MouseButtonReleased && ev.mouseButton.button == sf::Mouse::Left) {
        isDragging = false;
        formulaDragAnchorRow = formulaDragAnchorCol = -1;
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
    for (int r = scrollY; (r - scrollY) * CELL_H < H - TOOLBAR_H - HDR_H - STATUS_H; r++) {
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
        if (cellText.count(key)) {
            // Formula cell: show computed double
            if (auto* d = std::get_if<double>(&n.data)) display[key] = formatNum(*d);
        } else if (auto* d = std::get_if<double>(&n.data)) {
            display[key] = formatNum(*d);
        } else if (auto* s = std::get_if<std::string>(&n.data)) {
            display[key] = *s;
        }
    }

    int r1sel = (selRow2 < 0) ? selRow : std::min(selRow, selRow2);
    int r2sel = (selRow2 < 0) ? selRow : std::max(selRow, selRow2);
    int c1sel = (selCol2 < 0) ? selCol : std::min(selCol, selCol2);
    int c2sel = (selCol2 < 0) ? selCol : std::max(selCol, selCol2);

    for (int r = scrollY; (r - scrollY) * CELL_H < H - TOOLBAR_H - HDR_H - STATUS_H; r++) {
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
                bool isErr = (txt.size() > 1 && txt[0] == '#' && txt[1] == '!');
                t.setFillColor(isErr ? C_ERR : C_BLACK);
                t.setPosition(x + 4, y + 5);
                window.draw(t);
            }
        }
    }
}

void SpreadsheetUI::drawStatusBar() {
    float W = (float)window.getSize().x;
    float H = (float)window.getSize().y;
    float y  = H - STATUS_H;

    drawRect(0, y, W, STATUS_H, C_STATUS, C_LILAC_DK, 0.f);

    // Only show aggregates when a range (more than 1 cell) is selected
    int r1 = std::min(selRow,  selRow2 < 0 ? selRow  : selRow2);
    int r2 = std::max(selRow,  selRow2 < 0 ? selRow  : selRow2);
    int c1 = std::min(selCol,  selCol2 < 0 ? selCol  : selCol2);
    int c2 = std::max(selCol,  selCol2 < 0 ? selCol  : selCol2);
    bool isRange = (r1 != r2 || c1 != c2);

    if (isRange) {
        // Count numeric cells and compute sum in one pass (avoids exception on empty range)
        double sum = 0.0;
        int    cnt = 0;
        for (auto& n : mat.getAllNodes()) {
            if (n.row >= r1 && n.row <= r2 && n.col >= c1 && n.col <= c2) {
                if (auto* d = std::get_if<double>(&n.data)) { sum += *d; cnt++; }
            }
        }
        std::string msg = "SUMA: " + formatNum(sum) +
                          "   CONTAR: " + std::to_string(cnt);
        if (cnt > 0)
            msg += "   PROMEDIO: " + formatNum(sum / cnt);
        drawText(msg, 8, y + 4, 12, C_LILAC_DK);
    } else {
        // Single cell: show coordinate + shortcuts hint
        std::string ref = colToStr(selCol) + std::to_string(selRow + 1);
        drawText(ref + "   |   Ctrl+Home=A1   Ctrl+C copiar   Ctrl+V pegar   Ctrl+Z deshacer   Tab=comandos   F2=editar",
                 8, y + 4, 11, C_LILAC_DK);
    }
}

void SpreadsheetUI::render(int maxCol, int visCols, int visRows) {
    window.clear(C_LILAC_LT);
    drawToolbar();
    drawHeaders(maxCol);
    drawCells(maxCol, visRows);
    drawAutocomplete(); // siempre encima del grid
    drawStatusBar();
    window.display();
}

// ---- main loop ----

int SpreadsheetUI::run() {
    while (window.isOpen()) {
        int maxCol = visibleMaxCol();
        int visCols = ((int)window.getSize().x - HDR_W) / CELL_W + 2;
        int visRows = ((int)window.getSize().y - TOOLBAR_H - HDR_H - STATUS_H) / CELL_H + 2;

        sf::Event ev;
        while (window.pollEvent(ev))
            handleEvent(ev, visCols, visRows);

        render(maxCol, visCols, visRows);
    }
    return 0;
}
