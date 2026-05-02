#include <iostream>
#include <stdexcept>

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

    }

    // Consulta
    T get(int i, int j) {
        if (i < 0 || j < 0 || i > maxRow || j > maxCol) { // Índices negativos o fuera del tamaño de la matriz
            throw std::out_of_range("Indices out of bounds");
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

}

    //
    bool update(int i, int j, T value){

    };
    void remove(int i, int j);

    bool remove_row(int i);
    bool remove_col(int j);

    bool remove_range(int i1, int j1, int i2, int j2);

    T sum_range(int i1, int j1, int i2, int j2);
    T max_range(int i1, int j1, int i2, int j2);
    T min_range(int i1, int j1, int i2, int j2);
    double avg_range(int i1, int j1, int i2, int j2);
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
