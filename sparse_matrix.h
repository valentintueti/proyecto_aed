#ifndef SPARSE_MATRIX_H
#define SPARSE_MATRIX_H

#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

template<typename T>
inline double cellToDouble(const T& v) {
    if constexpr (std::is_arithmetic_v<T>)
        return static_cast<double>(v);
    else
        return 0.0;
}
inline double cellToDouble(const std::variant<double, std::string>& v) {
    if (const auto* d = std::get_if<double>(&v)) return *d;
    return 0.0;
}

template <typename T>
struct Node;

template <typename T>
struct HeadNode;


template<typename T>
class SparseMatrix {
private:
    HeadNode<T>* rowHead;
    HeadNode<T>* colHead;

    // Tamaño de la Matriz
    int maxRow;
    int maxCol;

    // Consulta o creación de índice de fila o columna
    // Si k es el tamaño de list, tiene complejidad O(k)
    HeadNode<T>* getOrCreateHead(HeadNode<T>*& list, int idx) {
        HeadNode<T>* prev = nullptr;
        HeadNode<T>* curr = list;

        while (curr != nullptr && curr->index < idx) {
            prev = curr;
            curr = curr->next;
        }

        if (curr != nullptr && curr->index == idx)
            return curr;

        HeadNode<T>* new_header = new HeadNode<T>(idx);
        new_header->next = curr;

        if (prev == nullptr) list = new_header;
        else prev->next = new_header;

        return new_header;
    }
    
public:
    SparseMatrix(): maxRow(-1), maxCol(-1), colHead(nullptr), rowHead(nullptr){}

    // Insertar celda
    // La complejidad es O(i + j)
    void set(int i, int j, T value) {
        HeadNode<T>* row = getOrCreateHead(rowHead, i); // O(i)
        HeadNode<T>* column = getOrCreateHead(colHead, j); //O(j)

        Node<T>* prevRow = nullptr;
        Node<T>* currRow = row->first;

        while(currRow and currRow->col < j) { //O(j)
            prevRow = currRow;
            currRow = currRow->nextInRow;
        }

        if(currRow and currRow->col == j) {
            currRow->data = value;
            return;
        }

        Node<T>* newNode = new Node<T>(value, i, j);

        newNode->nextInRow = currRow;
        newNode->prevInRow = prevRow;

        if(prevRow) prevRow->nextInRow = newNode;
        else row->first = newNode;

        if(currRow) currRow->prevInRow = newNode;

        Node<T>* prevCol = nullptr;
        Node<T>* currCol = column->first;

        while(currCol and currCol->row < i) { // O(i)
            prevCol = currCol;
            currCol = currCol->nextInCol;
        }

        newNode->nextInCol = currCol;
        newNode->prevInCol = prevCol;

        if(prevCol) prevCol->nextInCol = newNode;
        else column->first = newNode;

        if(currCol) currCol->prevInCol = newNode;

        if (i > maxRow) maxRow = i;
        if(j > maxCol) maxCol = j;
    }

    // Consultar celda
    // La complejidad es O(i + j)
    T get(int i, int j) const {
        if (i < 0 || j < 0)
            throw std::out_of_range("Indexes out of bounds");
        if (i > maxRow || j > maxCol)
            return T{}; // celda vacía — más allá de los datos actuales

        HeadNode<T>* temp1 = rowHead;
        while (temp1 != nullptr && temp1->index < i) { // Recorrer índices de fila, O(i)
            temp1 = temp1->next;
        }

        if (temp1 == nullptr || temp1->index != i) {
            return T{};
        }

        Node<T>* temp2 = temp1->first;
        while (temp2 != nullptr && temp2->col < j) { // Recorrer celdas en la fila, O(j)
            temp2 = temp2->nextInRow;
        }

        if (temp2 == nullptr || temp2->col != j) {
            return T{};
        }

        return temp2->data;
    }

    // Modificar celda
    // La complejidad es O(i + j)
    bool update(int i, int j, T value){
        if (i < 0 || j < 0 || i > maxRow || j > maxCol) { // Índices negativos o fuera del tamaño de la matriz
            return false;
        }

        HeadNode<T>* temp1 = rowHead;
        while (temp1 != nullptr && temp1->index < i) { // Recorrer índices de fila, O(i)
            temp1 = temp1->next;
        }

        if (temp1 == nullptr || temp1->index != i) {
            return false;
        }

        Node<T>* temp2 = temp1->first;
        while (temp2 != nullptr && temp2->col < j) { // Recorrer celdas en la fila, O(j)
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

        HeadNode<T>* row = rowHead;

        while (row && row->index < i) { //O(i)
            row = row->next;
        }

        if (!row || row->index != i) return false;

        Node<T>* curr = row->first;
        while (curr && curr->col < j) { // O(j)
            curr = curr->nextInRow;
        }

        if (!curr || curr->col != j) return false;

        HeadNode<T>* col = colHead;

        while (col && col->index < j) { // O(j)
            col = col->next;
        }

        if (!col || col->index != j) return false;

        //Desconexión y eliminación en O(1)
        if (curr->prevInRow)
            curr->prevInRow->nextInRow = curr->nextInRow; // Si existe anterior, lo conectamos con el siguiente
        else
            row->first = curr->nextInRow; // Sino, conectamos la cabecera con el siguiente

        if (curr->nextInRow)
            curr->nextInRow->prevInRow = curr->prevInRow; //Si existe siguiente, lo conectamos con el anterior


        //Hacemos lo mismo para columnas
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
        HeadNode<T>* rowPrev = nullptr;
        HeadNode<T>* row = rowHead;
        while (row && row->index < i) {
            rowPrev = row;
            row = row->next;
        }
        if (!row || row->index != i)
            return false;
        Node<T>* curr = row->first;
        while (curr) {
            Node<T>* next = curr->nextInRow;
            HeadNode<T>* col = colHead;
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
        HeadNode<T>* colPrev = nullptr;
        HeadNode<T>* col = colHead;
        while (col && col->index < j) {
            colPrev = col;
            col = col->next;
        }
        if (!col || col->index != j)
            return false;
        Node<T>* curr = col->first;
        while (curr) {
            Node<T>* next = curr->nextInCol;
            HeadNode<T>* row = rowHead;
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
        if (i1 < 0 || j1 < 0 || i1 > i2 || j1 > j2)
            throw std::out_of_range("Indexes out of bounds");
        // Ajustar al límite real de datos — no hay nada que borrar más allá
        i2 = std::min(i2, maxRow);
        j2 = std::min(j2, maxCol);
        if (i2 < i1 || j2 < j1) return true;


        HeadNode<T>* row = rowHead;

        while (row && row->index < i1)
            row = row->next;

        while (row && row->index <= i2) {

            Node<T>* curr = row->first;

            while (curr) {
                Node<T>* next = curr->nextInRow;

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
                        HeadNode<T>* col = colHead;
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
    double sum_row(int i) {
        if (i < 0 || i > maxRow)
            throw std::out_of_range("Index out of bounds");

        HeadNode<T>* temp1 = rowHead;
        while (temp1 != nullptr && temp1->index < i)
            temp1 = temp1->next;

        if (temp1 == nullptr || temp1->index != i)
            return 0.0;

        Node<T>* temp2 = temp1->first;
        double sumatoria = 0.0;
        while (temp2 != nullptr) {
            sumatoria += cellToDouble(temp2->data);
            temp2 = temp2->nextInRow;
        }
        return sumatoria;
    }

    // Suma de todos los elementos en una columna
    double sum_col(int j) {
        if (j < 0 || j > maxCol)
            throw std::out_of_range("Index out of bounds");

        HeadNode<T>* temp1 = colHead;
        while (temp1 != nullptr && temp1->index < j)
            temp1 = temp1->next;

        if (temp1 == nullptr || temp1->index != j)
            return 0.0;

        Node<T>* temp2 = temp1->first;
        double sumatoria = 0.0;
        while (temp2 != nullptr) {
            sumatoria += cellToDouble(temp2->data);
            temp2 = temp2->nextInCol;
        }
        return sumatoria;
    }

    // Suma de valores en un rango de celdas
    double sum_range(int i1, int j1, int i2, int j2) {
        if (i1 < 0 || j1 < 0 || i2 > maxRow || j2 > maxCol || i1 > i2 || j1 > j2)
            throw std::out_of_range("Indexes out of bounds");

        double sumatoria = 0.0;
        HeadNode<T>* row = rowHead;
        while (row != nullptr && row->index < i1)
            row = row->next;

        while (row != nullptr && row->index <= i2) {
            Node<T>* curr = row->first;
            while (curr != nullptr && curr->col < j1)
                curr = curr->nextInRow;
            while (curr != nullptr && curr->col <= j2) {
                sumatoria += cellToDouble(curr->data);
                curr = curr->nextInRow;
            }
            row = row->next;
        }
        return sumatoria;
    }

    // Máximo valor en un rango de celdas
    double max_range(int i1, int j1, int i2, int j2) {
        if (i1 < 0 || j1 < 0 || i2 > maxRow || j2 > maxCol || i1 > i2 || j1 > j2)
            throw std::out_of_range("Indexes out of bounds");

        bool found = false;
        double maxVal = 0.0;

        HeadNode<T>* row = rowHead;
        while (row && row->index < i1) row = row->next;

        while (row && row->index <= i2) {
            Node<T>* curr = row->first;
            while (curr) {
                if (curr->col >= j1 && curr->col <= j2) {
                    double d = cellToDouble(curr->data);
                    if (!found || d > maxVal) { maxVal = d; found = true; }
                }
                curr = curr->nextInRow;
            }
            row = row->next;
        }

        if (!found) throw std::runtime_error("No elements in range");
        return maxVal;
    }

    // Mínimo valor en un rango de celdas
    double min_range(int i1, int j1, int i2, int j2) {
        if (i1 < 0 || j1 < 0 || i2 > maxRow || j2 > maxCol || i1 > i2 || j1 > j2)
            throw std::out_of_range("Indexes out of bounds");

        bool found = false;
        double minVal = 0.0;

        HeadNode<T>* row = rowHead;
        while (row && row->index < i1) row = row->next;

        while (row && row->index <= i2) {
            Node<T>* curr = row->first;
            while (curr) {
                if (curr->col >= j1 && curr->col <= j2) {
                    double d = cellToDouble(curr->data);
                    if (!found || d < minVal) { minVal = d; found = true; }
                }
                curr = curr->nextInRow;
            }
            row = row->next;
        }

        if (!found) throw std::runtime_error("No elements in range");
        return minVal;
    }

    // Promedio de valores en un rango de celdas
    double avg_range(int i1, int j1, int i2, int j2) {
        if (i1 < 0 || j1 < 0 || i2 > maxRow || j2 > maxCol || i1 > i2 || j1 > j2)
            throw std::out_of_range("Indexes out of bounds");

        double suma = 0.0;
        int count = 0;

        HeadNode<T>* row = rowHead;
        while (row && row->index < i1) row = row->next;

        while (row && row->index <= i2) {
            Node<T>* curr = row->first;
            while (curr) {
                if (curr->col >= j1 && curr->col <= j2) {
                    suma += cellToDouble(curr->data);
                    count++;
                }
                curr = curr->nextInRow;
            }
            row = row->next;
        }

        if (count == 0) return 0.0;
        return suma / count;
    }

    struct CellInfo {
        int row;
        int col;
        T data;
    };

    std::vector<CellInfo> getAllNodes() const {
        std::vector<CellInfo> result;

        HeadNode<T>* row = rowHead;
        while (row != nullptr) {
            Node<T>* curr = row->first;
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
template <typename T>
struct Node {
    T data;
    int row;
    int col;
    Node<T>* nextInRow;
    Node<T>* nextInCol;
    Node<T>* prevInRow;
    Node<T>* prevInCol;

    Node(T data_): data(data_), nextInRow(nullptr), nextInCol(nullptr), prevInCol(nullptr), prevInRow(nullptr){}

    Node(T data_, int row_, int col_): data(data_), row(row_), col(col_), nextInRow(nullptr), nextInCol(nullptr), prevInCol(nullptr), prevInRow(nullptr){}
};

// Clase índice
template<typename T>
struct HeadNode {
    int index;
    HeadNode<T>* next;
    Node<T>* first;

    HeadNode(int idx): index(idx), next(nullptr), first(nullptr){}
};

#endif // SPARSE_MATRIX_H
