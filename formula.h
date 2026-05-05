#ifndef FORMULA_H
#define FORMULA_H

#include <string>
#include <utility>
#include "sparse_matrix.h"

// Column index <-> letter(s): 0=A, 25=Z, 26=AA, ...
std::string colToStr(int c);
int strToCol(const std::string& s);

// "B3" -> {2, 1}  (row is 0-based, col is 0-based). No row limit.
std::pair<int,int> parseCell(const std::string& input);

// Evaluate a cell formula starting with '='.
// Supports: =SUMA(A1:B5), =MAX(A1:B5), =A1+B2, =A1, =42
double evalFormula(const std::string& formula, SparseMatrix<double>& mat);

// Execute a toolbar command (without '=') and return a display string.
// Read-only: SUMA, MAX, MIN, PROMEDIO, LISTAR, VER_FILA, VER_COL
// Returns result string or error message.
std::string execCommand(const std::string& cmd, SparseMatrix<double>& mat);

#endif // FORMULA_H
