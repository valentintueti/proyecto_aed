#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <map>
#include <SFML/Graphics.hpp>

template <typename T> struct Node;

template <typename T> struct HeadNode;

template <typename T> class SparseMatrix {
private:
  HeadNode<T> *rowHead;
  HeadNode<T> *colHead;

  // Tamaño de la Matriz
  int maxRow;
  int maxCol;

  // Consulta o creación de índice de fila o columna
  HeadNode<T> *getOrCreateHead(HeadNode<T> *&list, int idx) {
    HeadNode<T> *prev = nullptr;
    HeadNode<T> *curr = list;

    while (curr != nullptr && curr->index < idx) {
      prev = curr;
      curr = curr->next;
    }

    if (curr != nullptr && curr->index == idx)
      return curr;

    HeadNode<T> *new_header = new HeadNode<T>(idx);
    new_header->next = curr;

    if (prev == nullptr)
      list = new_header;
    else
      prev->next = new_header;

    return new_header;
  }

public:
  SparseMatrix() : maxRow(-1), maxCol(-1), colHead(nullptr), rowHead(nullptr) {}

  // Insertar celda
  void set(int i, int j, T value) {
    HeadNode<T> *row = getOrCreateHead(rowHead, i);
    HeadNode<T> *column = getOrCreateHead(colHead, j);

    Node<T> *prevRow = nullptr;
    Node<T> *currRow = row->first;

    while (currRow and currRow->col < j) {
      prevRow = currRow;
      currRow = currRow->nextInRow;
    }

    if (currRow and currRow->col == j) {
      currRow->data = value;
      return;
    }

    Node<T> *newNode = new Node<T>(value, i, j);

    newNode->nextInRow = currRow;
    newNode->prevInRow = prevRow;

    if (prevRow)
      prevRow->nextInRow = newNode;
    else
      row->first = newNode;

    if (currRow)
      currRow->prevInRow = newNode;

    Node<T> *prevCol = nullptr;
    Node<T> *currCol = column->first;

    while (currCol and currCol->row < i) {
      prevCol = currCol;
      currCol = currCol->nextInCol;
    }

    newNode->nextInCol = currCol;
    newNode->prevInCol = prevCol;

    if (prevCol)
      prevCol->nextInCol = newNode;
    else
      column->first = newNode;

    if (currCol)
      currCol->prevInCol = newNode;

    if (i > maxRow)
      maxRow = i;
    if (j > maxCol)
      maxCol = j;
  }

  // Consultar celda
  T get(int i, int j) {
    if (i < 0 || j < 0 || i > maxRow ||
        j > maxCol) { // Índices negativos o fuera del tamaño de la matriz
      throw std::out_of_range("Indexes out of bounds");
    }

    HeadNode<T> *temp1 = rowHead;
    while (temp1 != nullptr && temp1->index < i) { // Recorrer índices de fila
      temp1 = temp1->next;
    }

    if (temp1 == nullptr || temp1->index != i) {
      return T{};
    }

    Node<T> *temp2 = temp1->first;
    while (temp2 != nullptr && temp2->col < j) { // Recorrer celdas en la fila
      temp2 = temp2->nextInRow;
    }

    if (temp2 == nullptr || temp2->col != j) {
      return T{};
    }

    return temp2->data;
  }

  // Modificar celda
  bool update(int i, int j, T value) {
    if (i < 0 || j < 0 || i > maxRow ||
        j > maxCol) { // Índices negativos o fuera del tamaño de la matriz
      return false;
    }

    HeadNode<T> *temp1 = rowHead;
    while (temp1 != nullptr && temp1->index < i) { // Recorrer índices de fila
      temp1 = temp1->next;
    }

    if (temp1 == nullptr || temp1->index != i) {
      return false;
    }

    Node<T> *temp2 = temp1->first;
    while (temp2 != nullptr && temp2->col < j) { // Recorrer celdas en la fila
      temp2 = temp2->nextInRow;
    }

    if (temp2 == nullptr || temp2->col != j) {
      return false;
    }

    temp2->data = value; // Actualización de celda
    return true;
  }

  // Eliminar celda
  bool remove(int i, int j) {
    if (i < 0 || j < 0 || i > maxRow || j > maxCol)
      throw std::out_of_range("Indexes out of bounds");

    HeadNode<T> *row = rowHead;

    while (row && row->index < i) {
      row = row->next;
    }

    if (!row || row->index != i)
      return false;

    Node<T> *curr = row->first;
    while (curr && curr->col < j) {
      curr = curr->nextInRow;
    }

    if (!curr || curr->col != j)
      return false;

    HeadNode<T> *col = colHead;

    while (col && col->index < j) {
      col = col->next;
    }

    if (!col || col->index != j)
      return false;

    if (curr->prevInRow)
      curr->prevInRow->nextInRow = curr->nextInRow;
    else
      row->first = curr->nextInRow;

    if (curr->nextInRow)
      curr->nextInRow->prevInRow = curr->prevInRow;

    if (curr->prevInCol)
      curr->prevInCol->nextInCol = curr->nextInCol;
    else
      col->first = curr->nextInCol;

    if (curr->nextInCol)
      curr->nextInCol->prevInCol = curr->prevInCol;

    delete curr;

    return true;
  }

  // Eliminar fila
  bool remove_row(int i) {
    if (i < 0 || i > maxRow)
      throw std::out_of_range("Index out of bounds");
    HeadNode<T> *rowPrev = nullptr;
    HeadNode<T> *row = rowHead;
    while (row && row->index < i) {
      rowPrev = row;
      row = row->next;
    }
    if (!row || row->index != i)
      return false;
    Node<T> *curr = row->first;
    while (curr) {
      Node<T> *next = curr->nextInRow;
      HeadNode<T> *col = colHead;
      while (col && col->index < curr->col)
        col = col->next;

      if (col) {
        if (curr->prevInCol)
          curr->prevInCol->nextInCol = curr->nextInCol;
        else
          col->first = curr->nextInCol;
        if (curr->nextInCol)
          curr->nextInCol->prevInCol = curr->prevInCol;
      }
      delete curr;
      curr = next;
    }
    if (rowPrev)
      rowPrev->next = row->next;
    else
      rowHead = row->next;
    delete row;
    return true;
  }

  // Eliminar columna
  bool remove_col(int j) {
    if (j < 0 || j > maxCol)
      throw std::out_of_range("Index out of bounds");
    HeadNode<T> *colPrev = nullptr;
    HeadNode<T> *col = colHead;
    while (col && col->index < j) {
      colPrev = col;
      col = col->next;
    }
    if (!col || col->index != j)
      return false;
    Node<T> *curr = col->first;
    while (curr) {
      Node<T> *next = curr->nextInCol;
      HeadNode<T> *row = rowHead;
      while (row && row->index < curr->row)
        row = row->next;
      if (row) {
        if (curr->prevInRow)
          curr->prevInRow->nextInRow = curr->nextInRow;
        else
          row->first = curr->nextInRow;
        if (curr->nextInRow)
          curr->nextInRow->prevInRow = curr->prevInRow;
      }
      delete curr;
      curr = next;
    }
    if (colPrev)
      colPrev->next = col->next;
    else
      colHead = col->next;
    delete col;
    return true;
  }

  // Eliminar rango de celdas
  bool remove_range(int i1, int j1, int i2, int j2) {
    if (i1 < 0 || j1 < 0 || i2 > maxRow || j2 > maxCol || i1 > i2 || j1 > j2)
      throw std::out_of_range("Indexes out of bounds");

    HeadNode<T> *row = rowHead;

    while (row && row->index < i1)
      row = row->next;

    while (row && row->index <= i2) {

      Node<T> *curr = row->first;

      while (curr) {
        Node<T> *next = curr->nextInRow;

        if (curr->col >= j1 && curr->col <= j2) {

          if (curr->prevInRow)
            curr->prevInRow->nextInRow = curr->nextInRow;
          else
            row->first = curr->nextInRow;

          if (curr->nextInRow)
            curr->nextInRow->prevInRow = curr->prevInRow;

          if (curr->prevInCol)
            curr->prevInCol->nextInCol = curr->nextInCol;
          else {
            HeadNode<T> *col = colHead;
            while (col && col->index < curr->col)
              col = col->next;
            if (col)
              col->first = curr->nextInCol;
          }
          if (curr->nextInCol)
            curr->nextInCol->prevInCol = curr->prevInCol;

          delete curr;
        }
        curr = next;
      }
      row = row->next;
    }

    return true;
  }

  // Suma de todos los elementos en una fila
  T sum_row(int i) {
    static_assert(std::is_arithmetic<T>::value, "T must be a numeric type");

    if (i < 0 || i > maxRow) { // Índice negativo o fuera del índice de fila
                               // máximo en la matriz
      throw std::out_of_range("Index out of bounds");
    }

    HeadNode<T> *temp1 = rowHead;
    while (temp1 != nullptr && temp1->index < i) { // Recorrer índices de fila
      temp1 = temp1->next;
    }

    if (temp1 == nullptr || temp1->index != i) {
      return T{};
    }

    Node<T> *temp2 = temp1->first;
    T sumatoria{};
    while (temp2 != nullptr) { // Recorrer celdas en la fila
      sumatoria += temp2->data;
      temp2 = temp2->nextInRow;
    }

    return sumatoria; // Retornar suma
  }

  // Suma de todos los elementos en una columna
  T sum_col(int j) {
    static_assert(std::is_arithmetic<T>::value, "T must be a numeric type");

    if (j < 0 || j > maxCol) { // Índice negativo o fuera del índice de columna
                               // máximo en la matriz
      throw std::out_of_range("Index out of bounds");
    }

    HeadNode<T> *temp1 = colHead;
    while (temp1 != nullptr &&
           temp1->index < j) { // Recorrer índices de columna
      temp1 = temp1->next;
    }

    if (temp1 == nullptr || temp1->index != j) {
      return T{};
    }

    Node<T> *temp2 = temp1->first;
    T sumatoria{};
    while (temp2 != nullptr) { // Recorrer celdas en la columna
      sumatoria += temp2->data;
      temp2 = temp2->nextInCol;
    }

    return sumatoria; // Retornar suma
  }

  // Suma de valores en un rango de celdas
  T sum_range(int i1, int j1, int i2, int j2) {
    static_assert(std::is_arithmetic<T>::value, "T must be a numeric type");

    if (i1 < 0 || j1 < 0 || i2 > maxRow || j2 > maxCol || i1 > i2 || j1 > j2) {
      throw std::out_of_range("Indexes out of bounds");
    }

    T sumatoria{};
    HeadNode<T> *row = rowHead;
    while (row != nullptr &&
           row->index <
               i1) { // Saltar índices de fila anteriores al inicio del rango
      row = row->next;
    }

    while (row != nullptr &&
           row->index <= i2) { // Recorrer filas dentro del rango

      Node<T> *curr = row->first;
      while (curr != nullptr &&
             curr->col < j1) { // Saltar columnas anteriores al inicio del rango
        curr = curr->nextInRow;
      }

      while (curr != nullptr &&
             curr->col <= j2) { // Sumar valores de celdas dentro del rango
        sumatoria += curr->data;
        curr = curr->nextInRow;
      }

      row = row->next;
    }

    return sumatoria; // Retornar suma
  }

  // Máximo valor en un rango de celdas
  T max_range(int i1, int j1, int i2, int j2) {
    static_assert(std::is_arithmetic<T>::value, "T must be a numeric type");

    if (i1 < 0 || j1 < 0 || i2 > maxRow || j2 > maxCol || i1 > i2 || j1 > j2)
      throw std::out_of_range("Indexes out of bounds");

    bool found = false;
    T maxVal{};

    HeadNode<T> *row = rowHead;
    while (row && row->index < i1)
      row = row->next;

    while (row && row->index <= i2) {
      Node<T> *curr = row->first;
      while (curr) {
        if (curr->col >= j1 && curr->col <= j2) {
          if (!found || curr->data > maxVal) {
            maxVal = curr->data;
            found = true;
          }
        }
        curr = curr->nextInRow;
      }
      row = row->next;
    }

    if (!found)
      throw std::runtime_error("No elements in range");
    return maxVal;
  }

  // Mínimo valor en un rango de celdas
  T min_range(int i1, int j1, int i2, int j2) {
    static_assert(std::is_arithmetic<T>::value, "T must be a numeric type");

    if (i1 < 0 || j1 < 0 || i2 > maxRow || j2 > maxCol || i1 > i2 || j1 > j2)
      throw std::out_of_range("Indexes out of bounds");

    bool found = false;
    T minVal{};

    HeadNode<T> *row = rowHead;
    while (row && row->index < i1)
      row = row->next;

    while (row && row->index <= i2) {
      Node<T> *curr = row->first;
      while (curr) {
        if (curr->col >= j1 && curr->col <= j2) {
          if (!found || curr->data < minVal) {
            minVal = curr->data;
            found = true;
          }
        }
        curr = curr->nextInRow;
      }
      row = row->next;
    }

    if (!found)
      throw std::runtime_error("No elements in range");
    return minVal;
  }

  // Promedio de valores en un rango de celdas
  double avg_range(int i1, int j1, int i2, int j2) {
    static_assert(std::is_arithmetic<T>::value, "T must be a numeric type");

    if (i1 < 0 or j1 < 0 or i2 > maxRow or j2 > maxCol or i1 > i2 or j1 > j2)
      throw std::out_of_range("Indexes out of bounds");

    double suma = 0;
    int count = 0;

    HeadNode<T> *row = rowHead;
    while (row && row->index < i1)
      row = row->next;

    while (row && row->index <= i2) {
      Node<T> *curr = row->first;
      while (curr) {
        if (curr->col >= j1 && curr->col <= j2) {
          suma += curr->data;
          count++;
        }

        curr = curr->nextInRow;
      }
      row = row->next;
    }

    if (count == 0)
      return 0.0;
    return suma / count;
  }

  struct CellInfo {
    int row;
    int col;
    T data;
  };

  std::vector<CellInfo> getAllNodes() {
    std::vector<CellInfo> result;

    HeadNode<T> *row = rowHead;
    while (row != nullptr) {
      Node<T> *curr = row->first;
      while (curr != nullptr) {
        result.push_back({curr->row, curr->col, curr->data});
        curr = curr->nextInRow;
      }
      row = row->next;
    }

    return result;
  }
};

// Clase celda
template <typename T> struct Node {
  T data;
  int row;
  int col;
  Node<T> *nextInRow;
  Node<T> *nextInCol;
  Node<T> *prevInRow;
  Node<T> *prevInCol;

  Node(T data_)
      : data(data_), nextInRow(nullptr), nextInCol(nullptr), prevInCol(nullptr),
        prevInRow(nullptr) {}

  Node(T data_, int row_, int col_)
      : data(data_), row(row_), col(col_), nextInRow(nullptr),
        nextInCol(nullptr), prevInCol(nullptr), prevInRow(nullptr) {}
};

// Clase índice
template <typename T> struct HeadNode {
  int index;
  HeadNode<T> *next;
  Node<T> *first;

  HeadNode(int idx) : index(idx), next(nullptr), first(nullptr) {}
};

// --- Column name helpers ---
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
    while (i < input.size() && std::isalpha(input[i])) i++;
    if (i == 0 || i == input.size()) return {-1,-1};
    std::string colPart = input.substr(0, i);
    std::string rowPart = input.substr(i);
    for (char c : rowPart) if (!std::isdigit(c)) return {-1,-1};
    int col = strToCol(colPart);
    int row = std::stoi(rowPart) - 1;
    if (row < 0 || row >= 1000 || col < 0) return {-1,-1};
    return {row, col};
}

// --- Simple formula evaluator ---
double evalFormula(const std::string& formula, SparseMatrix<double>& mat) {
    std::string f = formula.substr(1); // remove '='
    // =sum_range(A1,B100)
    if (f.find("sum_range(") == 0) {
        size_t p1 = f.find('('), p2 = f.find(','), p3 = f.find(')');
        if (p1==std::string::npos||p2==std::string::npos||p3==std::string::npos) return 0;
        auto c1 = parseCell(f.substr(p1+1, p2-p1-1));
        auto c2 = parseCell(f.substr(p2+1, p3-p2-1));
        if (c1.first<0||c2.first<0) return 0;
        try { return mat.sum_range(c1.first,c1.second,c2.first,c2.second); } catch(...) { return 0; }
    }
    // =sum_col(A)
    if (f.find("sum_col(") == 0) {
        size_t p1 = f.find('('), p2 = f.find(')');
        if (p1==std::string::npos||p2==std::string::npos) return 0;
        std::string cs = f.substr(p1+1, p2-p1-1);
        int col = strToCol(cs);
        try { return mat.sum_col(col); } catch(...) { return 0; }
    }
    // =sum_row(1)
    if (f.find("sum_row(") == 0) {
        size_t p1 = f.find('('), p2 = f.find(')');
        if (p1==std::string::npos||p2==std::string::npos) return 0;
        int row = std::stoi(f.substr(p1+1, p2-p1-1)) - 1;
        try { return mat.sum_row(row); } catch(...) { return 0; }
    }
    // =A1+B2
    size_t plus = f.find('+');
    size_t minus = f.find('-', 1);
    size_t mul = f.find('*');
    size_t div = f.find('/');
    char op = 0; size_t opPos = std::string::npos;
    if (plus != std::string::npos) { op = '+'; opPos = plus; }
    if (minus != std::string::npos && (opPos == std::string::npos || minus < opPos)) { op = '-'; opPos = minus; }
    if (mul != std::string::npos && (opPos == std::string::npos || mul < opPos)) { op = '*'; opPos = mul; }
    if (div != std::string::npos && (opPos == std::string::npos || div < opPos)) { op = '/'; opPos = div; }
    if (op && opPos != std::string::npos) {
        std::string left = f.substr(0, opPos), right = f.substr(opPos+1);
        auto getVal = [&](const std::string& s) -> double {
            auto p = parseCell(s);
            if (p.first >= 0) { try { return mat.get(p.first, p.second); } catch(...) { return 0; } }
            try { return std::stod(s); } catch(...) { return 0; }
        };
        double l = getVal(left), r = getVal(right);
        if (op=='+') return l+r; if (op=='-') return l-r;
        if (op=='*') return l*r; if (op=='/' && r!=0) return l/r;
    }
    // single cell ref
    auto p = parseCell(f);
    if (p.first >= 0) { try { return mat.get(p.first, p.second); } catch(...) { return 0; } }
    try { return std::stod(f); } catch(...) { return 0; }
}

// --- Constants ---
const int CELL_W = 100, CELL_H = 28, HDR_H = 30, HDR_W = 50;
const int TOOLBAR_H = 40, MAX_ROWS = 1000;
const sf::Color LILAC(200, 180, 220), LILAC_LIGHT(230, 220, 245),
    LILAC_DARK(140, 110, 170), WHITE(255,255,255), BLACK(0,0,0),
    SEL_COLOR(180, 150, 210, 120), GRID_COLOR(180, 170, 200),
    TOOLBAR_BG(170, 140, 200), BTN_COLOR(210, 195, 230);

int main() {
    SparseMatrix<double> mat;
    sf::RenderWindow window(sf::VideoMode(1200, 700), "Spreadsheet");
    window.setFramerateLimit(60);
    sf::Font font;
    if (!font.loadFromFile("C:/Windows/Fonts/consola.ttf"))
        if (!font.loadFromFile("C:/Windows/Fonts/arial.ttf"))
            { std::cerr << "Cannot load font\n"; return 1; }

    int scrollX = 0, scrollY = 0;
    int selRow = 0, selCol = 0;
    int selRow2 = -1, selCol2 = -1; // range selection end
    bool editing = false;
    std::string editText;
    std::string toolbarInput, toolbarResult;
    bool toolbarActive = false;

    // Find max used column for rendering
    auto getMaxCol = [&]() {
        int mc = 25; // at least A-Z
        auto nodes = mat.getAllNodes();
        for (auto& n : nodes) if (n.col > mc) mc = n.col;
        return mc;
    };

    while (window.isOpen()) {
        sf::Event ev;
        int maxCol = getMaxCol();
        int visCols = (window.getSize().x - HDR_W) / CELL_W + 1;
        int visRows = (window.getSize().y - TOOLBAR_H - HDR_H) / CELL_H + 1;

        while (window.pollEvent(ev)) {
            if (ev.type == sf::Event::Closed) window.close();

            // Mouse wheel scroll
            if (ev.type == sf::Event::MouseWheelScrolled) {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
                    scrollX = std::max(0, scrollX - (int)(ev.mouseWheelScroll.delta * 3));
                else
                    scrollY = std::max(0, scrollY - (int)(ev.mouseWheelScroll.delta * 3));
            }

            // Mouse click - select cell or toolbar
            if (ev.type == sf::Event::MouseButtonPressed && ev.mouseButton.button == sf::Mouse::Left) {
                int mx = ev.mouseButton.x, my = ev.mouseButton.y;
                if (my < TOOLBAR_H) {
                    // Check toolbar buttons
                    toolbarActive = true;
                    // If clicked on input area, activate toolbar
                } else {
                    toolbarActive = false;
                    if (editing) {
                        // Commit edit
                        if (editText.empty()) {
                            try { mat.remove(selRow, selCol); } catch(...) {}
                        } else if (editText[0] == '=') {
                            double val = evalFormula(editText, mat);
                            mat.set(selRow, selCol, val);
                        } else {
                            try { mat.set(selRow, selCol, std::stod(editText)); } catch(...) {}
                        }
                        editing = false;
                    }
                    int clickCol = (mx - HDR_W) / CELL_W + scrollX;
                    int clickRow = (my - TOOLBAR_H - HDR_H) / CELL_H + scrollY;
                    if (clickCol >= 0 && clickCol <= maxCol && clickRow >= 0 && clickRow < MAX_ROWS) {
                        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) {
                            selRow2 = clickRow; selCol2 = clickCol;
                        } else {
                            selRow = clickRow; selCol = clickCol;
                            selRow2 = -1; selCol2 = -1;
                        }
                    }
                }
            }

            // Keyboard
            if (ev.type == sf::Event::KeyPressed) {
                if (toolbarActive) {
                    if (ev.key.code == sf::Keyboard::Escape) toolbarActive = false;
                    if (ev.key.code == sf::Keyboard::Enter) {
                        // Execute toolbar command
                        std::string cmd = toolbarInput;
                        toolbarResult = "";
                        if (cmd.find("sum_range(") == 0) {
                            size_t p1=cmd.find('('),p2=cmd.find(','),p3=cmd.find(')');
                            auto c1=parseCell(cmd.substr(p1+1,p2-p1-1));
                            auto c2=parseCell(cmd.substr(p2+1,p3-p2-1));
                            if(c1.first>=0&&c2.first>=0) try{toolbarResult=std::to_string(mat.sum_range(c1.first,c1.second,c2.first,c2.second));}catch(...){}
                        } else if (cmd.find("sum_col(") == 0) {
                            size_t p1=cmd.find('('),p2=cmd.find(')');
                            int c=strToCol(cmd.substr(p1+1,p2-p1-1));
                            try{toolbarResult=std::to_string(mat.sum_col(c));}catch(...){}
                        } else if (cmd.find("sum_row(") == 0) {
                            size_t p1=cmd.find('('),p2=cmd.find(')');
                            int r=std::stoi(cmd.substr(p1+1,p2-p1-1))-1;
                            try{toolbarResult=std::to_string(mat.sum_row(r));}catch(...){}
                        } else if (cmd.find("max_range(") == 0) {
                            size_t p1=cmd.find('('),p2=cmd.find(','),p3=cmd.find(')');
                            auto c1=parseCell(cmd.substr(p1+1,p2-p1-1));
                            auto c2=parseCell(cmd.substr(p2+1,p3-p2-1));
                            if(c1.first>=0&&c2.first>=0) try{toolbarResult=std::to_string(mat.max_range(c1.first,c1.second,c2.first,c2.second));}catch(...){}
                        } else if (cmd.find("min_range(") == 0) {
                            size_t p1=cmd.find('('),p2=cmd.find(','),p3=cmd.find(')');
                            auto c1=parseCell(cmd.substr(p1+1,p2-p1-1));
                            auto c2=parseCell(cmd.substr(p2+1,p3-p2-1));
                            if(c1.first>=0&&c2.first>=0) try{toolbarResult=std::to_string(mat.min_range(c1.first,c1.second,c2.first,c2.second));}catch(...){}
                        } else if (cmd.find("avg_range(") == 0) {
                            size_t p1=cmd.find('('),p2=cmd.find(','),p3=cmd.find(')');
                            auto c1=parseCell(cmd.substr(p1+1,p2-p1-1));
                            auto c2=parseCell(cmd.substr(p2+1,p3-p2-1));
                            if(c1.first>=0&&c2.first>=0) try{toolbarResult=std::to_string(mat.avg_range(c1.first,c1.second,c2.first,c2.second));}catch(...){}
                        } else if (cmd.find("delete_row(") == 0) {
                            size_t p1=cmd.find('('),p2=cmd.find(')');
                            int r=std::stoi(cmd.substr(p1+1,p2-p1-1))-1;
                            try{mat.remove_row(r); toolbarResult="Row deleted";}catch(...){toolbarResult="Error";}
                        }
                        toolbarActive = false;
                    }
                } else if (editing) {
                    if (ev.key.code == sf::Keyboard::Escape) { editing = false; }
                    if (ev.key.code == sf::Keyboard::Enter) {
                        if (editText.empty()) {
                            try { mat.remove(selRow, selCol); } catch(...) {}
                        } else if (editText[0] == '=') {
                            double val = evalFormula(editText, mat);
                            mat.set(selRow, selCol, val);
                        } else {
                            try { mat.set(selRow, selCol, std::stod(editText)); } catch(...) {}
                        }
                        editing = false;
                        selRow = std::min(selRow + 1, MAX_ROWS - 1);
                    }
                    if (ev.key.code == sf::Keyboard::BackSpace && !editText.empty())
                        editText.pop_back();
                } else {
                    if (ev.key.code == sf::Keyboard::Up) { selRow = std::max(0, selRow-1); selRow2=selCol2=-1; }
                    if (ev.key.code == sf::Keyboard::Down) { selRow = std::min(MAX_ROWS-1, selRow+1); selRow2=selCol2=-1; }
                    if (ev.key.code == sf::Keyboard::Left) { selCol = std::max(0, selCol-1); selRow2=selCol2=-1; }
                    if (ev.key.code == sf::Keyboard::Right) { selCol = std::min(maxCol, selCol+1); selRow2=selCol2=-1; }
                    if (ev.key.code == sf::Keyboard::Delete) {
                        if (selRow2 >= 0 && selCol2 >= 0) {
                            int r1=std::min(selRow,selRow2),r2=std::max(selRow,selRow2);
                            int c1=std::min(selCol,selCol2),c2=std::max(selCol,selCol2);
                            try{mat.remove_range(r1,c1,r2,c2);}catch(...){}
                        } else {
                            try { mat.remove(selRow, selCol); } catch(...) {}
                        }
                    }
                    if (ev.key.code == sf::Keyboard::F2) {
                        editing = true;
                        try { double v = mat.get(selRow, selCol); editText = (v == 0) ? "" : std::to_string(v); }
                        catch(...) { editText = ""; }
                    }
                    if (ev.key.code == sf::Keyboard::Tab) {
                        toolbarActive = true; toolbarInput = ""; toolbarResult = "";
                    }
                    // Auto-scroll to keep selection visible
                    if (selRow < scrollY) scrollY = selRow;
                    if (selRow >= scrollY + visRows - 1) scrollY = selRow - visRows + 2;
                    if (selCol < scrollX) scrollX = selCol;
                    if (selCol >= scrollX + visCols - 1) scrollX = selCol - visCols + 2;
                }
            }

            // Text input for editing and toolbar
            if (ev.type == sf::Event::TextEntered && ev.text.unicode >= 32 && ev.text.unicode < 128) {
                char c = (char)ev.text.unicode;
                if (toolbarActive) {
                    toolbarInput += c;
                } else if (editing) {
                    editText += c;
                } else {
                    // Start editing on any printable key
                    editing = true;
                    editText = std::string(1, c);
                }
            }
        }

        // --- RENDERING ---
        window.clear(LILAC_LIGHT);

        // Toolbar background
        sf::RectangleShape tbBg(sf::Vector2f(window.getSize().x, TOOLBAR_H));
        tbBg.setFillColor(TOOLBAR_BG);
        window.draw(tbBg);

        sf::Text tbText;
        tbText.setFont(font); tbText.setCharacterSize(14); tbText.setFillColor(WHITE);
        if (toolbarActive) {
            tbText.setString("Query> " + toolbarInput + "_");
            tbText.setPosition(10, 10);
        } else if (!toolbarResult.empty()) {
            tbText.setString("Result: " + toolbarResult + "  |  [Tab] query  [F2] edit  [Del] delete  [Shift+Click] select range");
            tbText.setPosition(10, 10);
        } else {
            std::string cellName = colToStr(selCol) + std::to_string(selRow + 1);
            std::string cellVal;
            try { double v = mat.get(selRow, selCol); cellVal = (v==0) ? "" : std::to_string(v); } catch(...) {}
            tbText.setString(cellName + ": " + (editing ? editText : cellVal) +
                "  |  [Tab] query  [F2] edit  [Del] delete  [Shift+Click] select range");
            tbText.setPosition(10, 10);
        }
        window.draw(tbText);

        // Column headers
        for (int c = scrollX; c <= maxCol && (c - scrollX) * CELL_W < (int)window.getSize().x - HDR_W; c++) {
            float x = HDR_W + (c - scrollX) * CELL_W;
            sf::RectangleShape hdr(sf::Vector2f(CELL_W, HDR_H));
            hdr.setPosition(x, TOOLBAR_H);
            hdr.setFillColor(LILAC);
            hdr.setOutlineColor(GRID_COLOR); hdr.setOutlineThickness(1);
            window.draw(hdr);
            sf::Text t(colToStr(c), font, 13);
            t.setFillColor(BLACK);
            t.setPosition(x + CELL_W/2 - t.getLocalBounds().width/2, TOOLBAR_H + 6);
            window.draw(t);
        }

        // Row headers
        for (int r = scrollY; r < MAX_ROWS && (r - scrollY) * CELL_H < (int)window.getSize().y - TOOLBAR_H - HDR_H; r++) {
            float y = TOOLBAR_H + HDR_H + (r - scrollY) * CELL_H;
            sf::RectangleShape hdr(sf::Vector2f(HDR_W, CELL_H));
            hdr.setPosition(0, y);
            hdr.setFillColor(LILAC);
            hdr.setOutlineColor(GRID_COLOR); hdr.setOutlineThickness(1);
            window.draw(hdr);
            sf::Text t(std::to_string(r + 1), font, 12);
            t.setFillColor(BLACK);
            t.setPosition(HDR_W/2 - t.getLocalBounds().width/2, y + 5);
            window.draw(t);
        }

        // Cells - get all nodes for fast rendering
        auto allNodes = mat.getAllNodes();
        std::map<std::pair<int,int>, double> cellMap;
        for (auto& n : allNodes) cellMap[{n.row, n.col}] = n.data;

        for (int r = scrollY; r < MAX_ROWS && (r - scrollY) * CELL_H < (int)window.getSize().y - TOOLBAR_H - HDR_H; r++) {
            for (int c = scrollX; c <= maxCol && (c - scrollX) * CELL_W < (int)window.getSize().x - HDR_W; c++) {
                float x = HDR_W + (c - scrollX) * CELL_W;
                float y = TOOLBAR_H + HDR_H + (r - scrollY) * CELL_H;

                sf::RectangleShape cell(sf::Vector2f(CELL_W, CELL_H));
                cell.setPosition(x, y);
                cell.setFillColor(WHITE);
                cell.setOutlineColor(GRID_COLOR);
                cell.setOutlineThickness(0.5f);

                // Highlight selection
                bool inSel = false;
                if (selRow2 >= 0 && selCol2 >= 0) {
                    int r1=std::min(selRow,selRow2),r2=std::max(selRow,selRow2);
                    int c1=std::min(selCol,selCol2),c2=std::max(selCol,selCol2);
                    inSel = (r>=r1 && r<=r2 && c>=c1 && c<=c2);
                }
                if (r == selRow && c == selCol) {
                    cell.setOutlineColor(LILAC_DARK);
                    cell.setOutlineThickness(2.f);
                } else if (inSel) {
                    cell.setFillColor(SEL_COLOR);
                }
                window.draw(cell);

                // Cell content
                std::string txt;
                if (editing && r == selRow && c == selCol) {
                    txt = editText + "_";
                } else {
                    auto it = cellMap.find({r, c});
                    if (it != cellMap.end()) {
                        double v = it->second;
                        if (v == (int)v) txt = std::to_string((int)v);
                        else { std::ostringstream os; os << v; txt = os.str(); }
                    }
                }
                if (!txt.empty()) {
                    sf::Text t(txt, font, 12);
                    t.setFillColor(BLACK);
                    t.setPosition(x + 4, y + 5);
                    window.draw(t);
                }
            }
        }

        window.display();
    }
    return 0;
}

