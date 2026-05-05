#include "formula.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>
#include <stdexcept>

std::string colToStr(int c) {
    std::string s;
    do { s = char('A' + c % 26) + s; c = c / 26 - 1; } while (c >= 0);
    return s;
}

int strToCol(const std::string& s) {
    int c = 0;
    for (char ch : s) c = c * 26 + (std::toupper(ch) - 'A' + 1);
    return c - 1;
}

std::pair<int,int> parseCell(const std::string& input) {
    size_t i = 0;
    while (i < input.size() && std::isalpha((unsigned char)input[i])) i++;
    if (i == 0 || i == input.size()) return {-1, -1};
    std::string colPart = input.substr(0, i);
    std::string rowPart = input.substr(i);
    for (char c : rowPart) if (!std::isdigit((unsigned char)c)) return {-1, -1};
    int col = strToCol(colPart);
    int row = std::stoi(rowPart) - 1;
    if (row < 0 || col < 0) return {-1, -1};
    return {row, col};
}

static std::string toUpper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::toupper(c); });
    return s;
}

static std::string trim(std::string s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return {};
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static bool isAllAlpha(const std::string& s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), [](unsigned char c){ return std::isalpha(c); });
}

static bool isAllDigit(const std::string& s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), [](unsigned char c){ return std::isdigit(c); });
}

static std::string formatNum(double v) {
    if (v >= -1e15 && v <= 1e15 && (double)(long long)v == v)
        return std::to_string((long long)v);
    std::ostringstream os;
    os << v;
    return os.str();
}

struct FuncCall {
    std::string name; // uppercase
    std::string arg1;
    std::string arg2; // empty if single-arg
    bool valid = false;
};

static FuncCall parseFuncCall(const std::string& expr) {
    size_t paren = expr.find('(');
    if (paren == std::string::npos) return {};
    size_t close = expr.rfind(')');
    if (close == std::string::npos || close < paren) return {};

    FuncCall fc;
    fc.name = toUpper(trim(expr.substr(0, paren)));
    std::string inside = trim(expr.substr(paren + 1, close - paren - 1));

    size_t sep = inside.find(':');
    if (sep == std::string::npos) sep = inside.find(',');

    if (sep != std::string::npos) {
        fc.arg1 = trim(inside.substr(0, sep));
        fc.arg2 = trim(inside.substr(sep + 1));
    } else {
        fc.arg1 = inside;
    }
    fc.valid = !fc.name.empty();
    return fc;
}

static double dispatchFunc(const FuncCall& fc, SparseMatrix<double>& mat) {
    const auto& name = fc.name;
    const auto& a1 = fc.arg1;
    const auto& a2 = fc.arg2;

    if (name == "SUMA" || name == "SUM") {
        if (!a2.empty()) {
            auto c1 = parseCell(a1), c2 = parseCell(a2);
            if (c1.first < 0 || c2.first < 0) return 0.0;
            try { return mat.sum_range(c1.first, c1.second, c2.first, c2.second); } catch(...) { return 0.0; }
        }
        if (isAllAlpha(a1)) { try { return mat.sum_col(strToCol(a1)); } catch(...) { return 0.0; } }
        if (isAllDigit(a1)) { try { return mat.sum_row(std::stoi(a1) - 1); } catch(...) { return 0.0; } }
        return 0.0;
    }
    if (name == "PROMEDIO" || name == "AVG" || name == "AVERAGE") {
        if (!a2.empty()) {
            auto c1 = parseCell(a1), c2 = parseCell(a2);
            if (c1.first < 0 || c2.first < 0) return 0.0;
            try { return mat.avg_range(c1.first, c1.second, c2.first, c2.second); } catch(...) { return 0.0; }
        }
        return 0.0;
    }
    if (name == "MAX" || name == "MAXIMO") {
        if (!a2.empty()) {
            auto c1 = parseCell(a1), c2 = parseCell(a2);
            if (c1.first < 0 || c2.first < 0) return 0.0;
            try { return mat.max_range(c1.first, c1.second, c2.first, c2.second); } catch(...) { return 0.0; }
        }
        return 0.0;
    }
    if (name == "MIN" || name == "MINIMO") {
        if (!a2.empty()) {
            auto c1 = parseCell(a1), c2 = parseCell(a2);
            if (c1.first < 0 || c2.first < 0) return 0.0;
            try { return mat.min_range(c1.first, c1.second, c2.first, c2.second); } catch(...) { return 0.0; }
        }
        return 0.0;
    }
    return 0.0;
}

double evalFormula(const std::string& formula, SparseMatrix<double>& mat) {
    if (formula.empty() || formula[0] != '=') {
        try { return std::stod(formula); } catch(...) { return 0.0; }
    }
    std::string f = formula.substr(1);

    // Try function call first
    auto fc = parseFuncCall(f);
    if (fc.valid) {
        try { return dispatchFunc(fc, mat); } catch(...) { return 0.0; }
    }

    // Try binary arithmetic: find first operator (+, -, *, /)
    // Start minus search at 1 to skip a leading negative sign
    size_t plus  = f.find('+');
    size_t minus = f.find('-', 1);
    size_t mul   = f.find('*');
    size_t div   = f.find('/');

    char op = 0; size_t opPos = std::string::npos;
    auto consider = [&](char o, size_t pos) {
        if (pos != std::string::npos && (opPos == std::string::npos || pos < opPos)) { op = o; opPos = pos; }
    };
    consider('+', plus); consider('-', minus); consider('*', mul); consider('/', div);

    if (op && opPos != std::string::npos) {
        std::string left = f.substr(0, opPos), right = f.substr(opPos + 1);
        auto getVal = [&](const std::string& s) -> double {
            auto p = parseCell(s);
            if (p.first >= 0) return mat.get(p.first, p.second);
            try { return std::stod(s); } catch(...) { return 0.0; }
        };
        double l = getVal(left), r = getVal(right);
        if (op == '+') return l + r;
        if (op == '-') return l - r;
        if (op == '*') return l * r;
        if (op == '/' && r != 0) return l / r;
        return 0.0;
    }

    // Single cell ref or literal
    auto p = parseCell(f);
    if (p.first >= 0) return mat.get(p.first, p.second);
    try { return std::stod(f); } catch(...) { return 0.0; }
}

std::string execCommand(const std::string& input, SparseMatrix<double>& mat) {
    std::string cmd = trim(input);
    if (cmd.empty()) return "";

    auto fc = parseFuncCall(cmd);
    if (!fc.valid) return "Error: sintaxis invalida. Ejemplo: SUMA(A1:B5)";

    const auto& name = fc.name;
    const auto& a1 = fc.arg1;
    const auto& a2 = fc.arg2;

    if (name == "SUMA" || name == "SUM") {
        if (!a2.empty()) {
            auto c1 = parseCell(a1), c2 = parseCell(a2);
            if (c1.first < 0 || c2.first < 0) return "Error: rango invalido";
            try { return "SUMA = " + formatNum(mat.sum_range(c1.first,c1.second,c2.first,c2.second)); }
            catch(...) { return "SUMA = 0"; }
        }
        if (isAllAlpha(a1)) {
            try { return "SUMA col " + toUpper(a1) + " = " + formatNum(mat.sum_col(strToCol(a1))); }
            catch(...) { return "SUMA col " + toUpper(a1) + " = 0"; }
        }
        if (isAllDigit(a1)) {
            try { return "SUMA fila " + a1 + " = " + formatNum(mat.sum_row(std::stoi(a1)-1)); }
            catch(...) { return "SUMA fila " + a1 + " = 0"; }
        }
        return "Error: argumento invalido";
    }
    if (name == "PROMEDIO" || name == "AVG" || name == "AVERAGE") {
        if (!a2.empty()) {
            auto c1 = parseCell(a1), c2 = parseCell(a2);
            if (c1.first < 0 || c2.first < 0) return "Error: rango invalido";
            try { return "PROMEDIO = " + formatNum(mat.avg_range(c1.first,c1.second,c2.first,c2.second)); }
            catch(...) { return "PROMEDIO = 0"; }
        }
        return "Error: argumento invalido";
    }
    if (name == "MAX" || name == "MAXIMO") {
        if (!a2.empty()) {
            auto c1 = parseCell(a1), c2 = parseCell(a2);
            if (c1.first < 0 || c2.first < 0) return "Error: rango invalido";
            try { return "MAX = " + formatNum(mat.max_range(c1.first,c1.second,c2.first,c2.second)); }
            catch(const std::exception& e) { return std::string("Error: ") + e.what(); }
        }
        return "Error: argumento invalido";
    }
    if (name == "MIN" || name == "MINIMO") {
        if (!a2.empty()) {
            auto c1 = parseCell(a1), c2 = parseCell(a2);
            if (c1.first < 0 || c2.first < 0) return "Error: rango invalido";
            try { return "MIN = " + formatNum(mat.min_range(c1.first,c1.second,c2.first,c2.second)); }
            catch(const std::exception& e) { return std::string("Error: ") + e.what(); }
        }
        return "Error: argumento invalido";
    }
    if (name == "LISTAR" || name == "LIST") {
        auto nodes = mat.getAllNodes();
        if (nodes.empty()) return "Sin celdas ocupadas";
        std::string r = "Celdas (" + std::to_string(nodes.size()) + "):";
        for (auto& n : nodes)
            r += " " + colToStr(n.col) + std::to_string(n.row+1) + "=" + formatNum(n.data);
        return r;
    }
    if (name == "VER_FILA" || name == "VIEW_ROW") {
        if (!isAllDigit(a1)) return "Error: argumento invalido";
        int row = std::stoi(a1) - 1;
        auto nodes = mat.getAllNodes();
        std::string r = "Fila " + a1 + ":";
        bool found = false;
        for (auto& n : nodes) if (n.row == row) { r += " " + colToStr(n.col) + "=" + formatNum(n.data); found = true; }
        if (!found) r += " (vacia)";
        return r;
    }
    if (name == "VER_COL" || name == "VIEW_COL") {
        if (!isAllAlpha(a1)) return "Error: argumento invalido";
        int col = strToCol(a1);
        auto nodes = mat.getAllNodes();
        std::string r = "Col " + toUpper(a1) + ":";
        bool found = false;
        for (auto& n : nodes) if (n.col == col) { r += " " + std::to_string(n.row+1) + "=" + formatNum(n.data); found = true; }
        if (!found) r += " (vacia)";
        return r;
    }

    return "Cmd desconocido. Disponibles: SUMA, MAX, MIN, PROMEDIO, LISTAR, VER_FILA, VER_COL";
}
