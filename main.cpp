#include <iostream>
#include <stdexcept>
#include <type_traits>

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
    void set(int i, int j, T value) {
        HeadNode<T>* row = getOrCreateHead(rowHead, i);
        HeadNode<T>* column = getOrCreateHead(colHead, j);

        Node<T>* prevRow = nullptr;
        Node<T>* currRow = row->first;

        while(currRow and currRow->col < j) {
            prevRow = currRow;
            currRow = currRow->nextInRow;
        }

        if(currRow and currRow->col == j) {
            currRow->data = value;
            return;
        }

        Node<T>* newNode = new Node(value, i, j);

        newNode->nextInRow = currRow;
        newNode->prevInRow = prevRow;

        if(prevRow) prevRow->nextInRow = newNode;
        else row->first = newNode;

        if(currRow) currRow->prevInRow = newNode;

        Node<T>* prevCol = nullptr;
        Node<T>* currCol = column->first;

        while(currCol and currCol->row < i) {
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
    T get(int i, int j) {
        if (i < 0 || j < 0 || i > maxRow || j > maxCol) { // Índices negativos o fuera del tamaño de la matriz
            throw std::out_of_range("Indexes out of bounds");
        }

        HeadNode<T>* temp1 = rowHead;
        while (temp1 != nullptr && temp1->index < i) { // Recorrer índices de fila
            temp1 = temp1->next;
        }

        if (temp1 == nullptr || temp1->index != i) {
            return T{};
        }

        Node<T>* temp2 = temp1->first;
        while (temp2 != nullptr && temp2->col < j) { // Recorrer celdas en la fila
            temp2 = temp2->nextInRow;
        }

        if (temp2 == nullptr || temp2->col != j) {
            return T{};
        }

        return temp2->data;
    }

    // Modificar celda
    bool update(int i, int j, T value){
        if (i < 0 || j < 0 || i > maxRow || j > maxCol) { // Índices negativos o fuera del tamaño de la matriz
            return false;
        }

        HeadNode<T>* temp1 = rowHead;
        while (temp1 != nullptr && temp1->index < i) { // Recorrer índices de fila
            temp1 = temp1->next;
        }

        if (temp1 == nullptr || temp1->index != i) {
            return false;
        }

        Node<T>* temp2 = temp1->first;
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

        HeadNode<T>* row = rowHead;

        while (row && row->index < i) {
            row = row->next;
        }

        if (!row || row->index != i) return false;

        Node<T>* curr = row->first;
        while (curr && curr->col < j) {
            curr = curr->nextInRow;
        }

        if (!curr || curr->col != j) return false;

        HeadNode<T>* col = colHead;

        while (col && col->index < j) {
            col = col->next;
        }

        if (!col || col->index != j) return false;

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
        if (i1 < 0 || j1 < 0 || i2 > maxRow || j2 > maxCol || i1 > i2 || j1 > j2)
            throw std::out_of_range("Indexes out of bounds");


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
    T sum_row(int i){
        static_assert(std::is_arithmetic<T>::value, "T must be a numeric type");

        if (i < 0 || i > maxRow) { // Índice negativo o fuera del índice de fila máximo en la matriz
            throw std::out_of_range("Index out of bounds");
        }

        HeadNode<T>* temp1 = rowHead;
        while (temp1 != nullptr && temp1->index < i) { // Recorrer índices de fila
            temp1 = temp1->next;
        }

        if (temp1 == nullptr || temp1->index != i) {
            return T{};
        }

        Node<T>* temp2 = temp1->first;
        T sumatoria{};
        while (temp2 != nullptr) { // Recorrer celdas en la fila
            sumatoria += temp2->data;
            temp2 = temp2->nextInRow;
        }

        return sumatoria; // Retornar suma
    }

    // Suma de todos los elementos en una columna
    T sum_col(int j){
        static_assert(std::is_arithmetic<T>::value, "T must be a numeric type");

        if (j < 0 || j > maxCol) { // Índice negativo o fuera del índice de columna máximo en la matriz
            throw std::out_of_range("Index out of bounds");
        }

        HeadNode<T>* temp1 = colHead;
        while (temp1 != nullptr && temp1->index < j) { // Recorrer índices de columna
            temp1 = temp1->next;
        }

        if (temp1 == nullptr || temp1->index != j) {
            return T{};
        }

        Node<T>* temp2 = temp1->first;
        T sumatoria{};
        while (temp2 != nullptr) { // Recorrer celdas en la columna
            sumatoria += temp2->data;
            temp2 = temp2->nextInCol;
        }

        return sumatoria; // Retornar suma
    }

    // Suma de valores en un rango de celdas
    T sum_range(int i1, int j1, int i2, int j2){
        static_assert(std::is_arithmetic<T>::value, "T must be a numeric type");

        if (i1 < 0 || j1 < 0 || i2 > maxRow || j2 > maxCol || i1 > i2 || j1 > j2) {
            throw std::out_of_range("Indexes out of bounds");
        }

        T sumatoria{};
        HeadNode<T>* row = rowHead;
        while (row != nullptr && row->index < i1){ // Saltar índices de fila anteriores al inicio del rango
            row = row->next;
        }

        while (row != nullptr && row->index <= i2) { // Recorrer filas dentro del rango

            Node<T>* curr = row->first;
            while (curr != nullptr && curr->col < j1) { // Saltar columnas anteriores al inicio del rango
                curr = curr->nextInRow;
            }

            while (curr != nullptr && curr->col <= j2) { // Sumar valores de celdas dentro del rango
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

        HeadNode<T>* row = rowHead;
        while (row && row->index < i1) row = row->next;

        while (row && row->index <= i2) {
            Node<T>* curr = row->first;
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

        if (!found) throw std::runtime_error("No elements in range");
        return maxVal;
    }

    // Mínimo valor en un rango de celdas
    T min_range(int i1, int j1, int i2, int j2) {
        static_assert(std::is_arithmetic<T>::value, "T must be a numeric type");

        if (i1 < 0 || j1 < 0 || i2 > maxRow || j2 > maxCol || i1 > i2 || j1 > j2)
            throw std::out_of_range("Indexes out of bounds");

        bool found = false;
        T minVal{};

        HeadNode<T>* row = rowHead;
        while (row && row->index < i1) row = row->next;

        while (row && row->index <= i2) {
            Node<T>* curr = row->first;
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

        if (!found) throw std::runtime_error("No elements in range");
        return minVal;
    }

    // Promedio de valores en un rango de celdas
    double avg_range(int i1, int j1, int i2, int j2) {
        static_assert(std::is_arithmetic<T>::value, "T must be a numeric type");

        if (i1 < 0 or j1 < 0 or i2 > maxRow or j2 > maxCol or i1 > i2 or j1 > j2) throw std::out_of_range("Indexes out of bounds");

        double suma = 0;
        int count = 0;

        HeadNode<T>* row = rowHead;
        while (row && row->index < i1) row = row->next;

        while (row && row->index <= i2) {
            Node<T>* curr = row->first;
            while(curr) {
                if(curr->col >= j1 && curr->col <=j2) {
                    suma+= curr->data;
                    count++;
                }

                curr = curr->nextInRow;
            }
            row = row->next;
        }

        if (count == 0) return 0.0;
        return suma / count;
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


int main() {
    std::cout << "Hello, World!" << std::endl;
    return 0;
}
