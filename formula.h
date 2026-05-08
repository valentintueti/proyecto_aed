#ifndef FORMULA_H
#define FORMULA_H

#include <string>
#include <utility>
#include <variant>
#include "sparse_matrix.h"

using CellValue = std::variant<double, std::string>;


std::string colToStr(int c);
int strToCol(const std::string& s);


std::pair<int,int> parseCell(const std::string& input);


double evalFormula(const std::string& formula, SparseMatrix<CellValue>& mat);


std::string execCommand(const std::string& cmd, SparseMatrix<CellValue>& mat);

#endif // FORMULA_H
