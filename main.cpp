#include <iostream>

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
    SparseMatrix(): n_rows(0), n_col(0), colHead(nullptr), rowHead(nullptr){}
    void set(int i, int j, T value);
    T get(int i, int j);
    bool update(int i, int j, T value);
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
    Node<T>* nextRow;
    Node<T>* nextCol;

    Node(T data_): data(data_), nextRow(nullptr), nextCol(nullptr){}

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
