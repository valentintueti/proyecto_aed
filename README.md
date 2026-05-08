# Proyecto 1 - Algoritmos y Estructuras de Datos (CS2023) 

**Integrantes:**
Daniela Villacorta Sotelo - 100%
Valentin Tuesta Barrantes - 100%
Fatima Isabella Pacheco Vera - 100%

---

## Estructura del Proyecto

```
proyecto_aed/
├── sparse_matrix.h       # Estructura de datos principal (Matriz Dispersa)
├── formula.h / .cpp      # Motor de fórmulas y comandos
├── spreadsheet_ui.h/.cpp # Interfaz gráfica SFML
├── main.cpp              # Punto de entrada
└── CMakeLists.txt        # Configuración de compilación
```


---

## Descripción de Archivos

### `sparse_matrix.h`
Implementación completa de la Matriz Dispersa. Define las estructuras `Node<T>` (celda), `HeadNode<T>` (índice de fila/columna) y la clase `SparseMatrix<T>` con todas las operaciones requeridas. Es el único archivo que maneja la memoria dinámica del proyecto.

### `formula.h / formula.cpp`
Motor de evaluación de expresiones. Convierte texto como `=SUMA(A1:B5)` en resultados numéricos llamando a las funciones de `SparseMatrix`. También implementa los comandos de la barra de herramientas (`LISTAR`, `VER_FILA`, `ELIMINAR_FILA`, etc.) y las funciones `CONTAR` y `PROMEDIO` extendidas.

### `spreadsheet_ui.h / spreadsheet_ui.cpp`
Capa visual completa usando SFML. Maneja:
- Renderizado de la grilla, headers y barra de estado
- Eventos de teclado y mouse
- Modo de edición de celdas y barra de fórmulas
- Undo/Redo, Copiar/Pegar, selección por arrastre
- Recalculación automática de fórmulas al modificar celdas

### `main.cpp`
Punto de entrada. Instancia `SpreadsheetUI` y llama a `run()`.

---

## Justificación: ¿Por qué Matriz Dispersa?

En una hoja de cálculo real, la gran mayoría de celdas están vacías. Usar una matriz densa `T[N][M]` reservaría memoria para **todas** las celdas posibles, incluyendo las vacías. La Matriz Dispersa con listas enlazadas cruzadas solo crea nodos para celdas con contenido, logrando:

- **Memoria:** `O(k)` donde `k` = celdas ocupadas (vs `O(N×M)` de una matriz densa)
- **Inserción/eliminación eficiente:** sin necesidad de desplazar elementos
- **Navegación natural:** cada nodo apunta al siguiente en su misma fila Y en su misma columna

---

## Funciones de `SparseMatrix<T>` y Complejidad

Sea:
- `R` = número de filas con datos (HeadNodes de fila)
- `C` = número de columnas con datos (HeadNodes de columna)
- `k` = nodos en la fila involucrada
- `m` = nodos en la columna involucrada
- `K` = total de nodos en la matriz

---

### Operaciones básicas de celda

#### `set(i, j, value)` — Insertar/actualizar celda
Busca o crea el HeadNode de fila `i` y columna `j`, luego inserta el nodo en la posición correcta manteniendo el orden en ambas listas.
- **Complejidad:** `O(R + C + k + m)`
- En la práctica `O(k + m)` ya que `R` y `C` son pequeños respecto al total.

#### `get(i, j)` — Consultar celda
Navega al HeadNode de la fila `i` y recorre los nodos hasta encontrar la columna `j`. Si no existe el nodo, retorna `T{}` (valor vacío).
- **Complejidad:** `O(R + k)`

#### `update(i, j, value)` — Modificar celda existente
Igual que `get` pero actualiza el dato. Retorna `false` si la celda no existe (sin crear un nodo nuevo).
- **Complejidad:** `O(R + k)`

#### `remove(i, j)` — Eliminar celda
Encuentra el nodo y reenlaza los punteros `prevInRow → nextInRow` y `prevInCol → nextInCol`.
- **Complejidad:** `O(R + C + k)`

---

### Operaciones sobre filas y columnas

#### `remove_row(i)` — Eliminar fila completa
Recorre todos los nodos de la fila `i` y para cada uno corrige los punteros de columna.
- **Complejidad:** `O(R + kC)` donde `k` = nodos en la fila

#### `remove_col(j)` — Eliminar columna completa
Similar a `remove_row` pero recorriendo la columna.
- **Complejidad:** `O(C + mR)` donde `m` = nodos en la columna

#### `remove_range(i1, j1, i2, j2)` — Eliminar rango rectangular
Itera filas de `i1` a `i2`, y dentro de cada fila elimina los nodos en columnas `j1` a `j2`. El rango se ajusta automáticamente si excede los datos existentes.
- **Complejidad:** `O(R + k_rango × C)` donde `k_rango` = nodos dentro del rango

---

### Operaciones de agregación

#### `sum_row(i)` — Suma de fila
Recorre todos los nodos de la fila `i` acumulando valores numéricos.
- **Complejidad:** `O(R + k)`

#### `sum_col(j)` — Suma de columna
Recorre todos los nodos de la columna `j`.
- **Complejidad:** `O(C + m)`

#### `sum_range(i1, j1, i2, j2)` — Suma de rango rectangular
Itera filas del rango y dentro de cada fila recorre solo las columnas en `[j1, j2]`.
- **Complejidad:** `O(R + k_rango)` donde `k_rango` = nodos en el rango

#### `max_range(i1, j1, i2, j2)` — Máximo en rango
Mismo recorrido que `sum_range`, comparando en cada nodo.
- **Complejidad:** `O(R + k_rango)`

#### `min_range(i1, j1, i2, j2)` — Mínimo en rango
Idéntico a `max_range`.
- **Complejidad:** `O(R + k_rango)`

#### `avg_range(i1, j1, i2, j2)` — Promedio en rango
Acumula suma y conteo en un solo recorrido, luego divide.
- **Complejidad:** `O(R + k_rango)`

---

### Utilidad

#### `getAllNodes()` — Obtener todas las celdas ocupadas
Recorre todos los HeadNodes de fila y lista sus nodos, devolviendo un `vector<CellInfo>`.
- **Complejidad:** `O(R + K)`
- Usado internamente para renderizado, undo, LISTAR y recalculación de fórmulas.

---

## Características de la Interfaz

- **Grilla dinámica:** se expande en filas y columnas sin límite fijo
- **Auto-scroll:** el cursor siempre se mantiene visible
- **Selección de rangos:** arrastre de mouse o `Shift + flechas`
- **Recalculación automática:** al editar cualquier celda, todas las fórmulas dependientes se actualizan
- **Barra de estado:** muestra `SUMA / CONTAR / PROMEDIO` del rango seleccionado en tiempo real
- **Errores tipo Excel:** operaciones con strings muestran `#!VALOR!` en rojo

---

## Atajos de Teclado

| Atajo | Acción |
|---|---|
| `Cmd/Ctrl + C` | Copiar selección al portapapeles del sistema |
| `Cmd/Ctrl + V` | Pegar desde portapapeles |
| `Cmd/Ctrl + Z` | Deshacer (hasta 30 niveles) |
| `Escape` | Volver a la celda A1 |
| `Shift + ↑↓←→` | Extender selección por teclado |
| `F2` | Editar celda actual (muestra fórmula en barra) |
| `Tab` | Abrir barra de comandos |
| `Delete / ⌫` | Borrar celda o rango seleccionado |
| `Enter` | Confirmar edición y bajar |

## Comandos de la Barra 

| Comando | Descripción |
|---|---|
| `SUMA(A1:B5)` | Suma rango |
| `SUMA(A)` / `SUMA(1)` | Suma columna A / fila 1 |
| `MAX(A1:B5)` | Máximo en rango |
| `MIN(A1:B5)` | Mínimo en rango |
| `PROMEDIO(A1:B5)` | Promedio de rango, columna o fila |
| `CONTAR(A1:B5)` | Cuenta celdas numéricas |

---

