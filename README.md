# Tableau semantico en espanol (C++17)

Aplicacion de consola que convierte premisas de logica proposicional escritas en espanol a un AST, construye un tableau semantico y reporta satisfacibilidad y modelos.

## Requisitos y compilacion

Se necesita un compilador C++17 (`g++` o `clang++`) y GNU Make. En Windows se recomienda MSYS2 MinGW-w64 o WSL. En una terminal donde `make` y el compilador esten disponibles:

```sh
make
make test
```

El ejecutable queda en `build/tableau-es`.

## Uso

```sh
./build/tableau-es "Si llueve entonces la calle se moja; llueve"
./build/tableau-es --archivo premisas.txt
./build/tableau-es --sin-arbol "llueve o nieva"
./build/tableau-es --incompatible "es de dia|es de noche" "es de dia; es de noche"
```

El archivo debe ser UTF-8, con una premisa por linea. Tambien se puede pasar entrada por stdin. `make run` ejecuta el programa leyendo stdin y `make clean` elimina los binarios generados.

## Gramatica soportada

- Negacion: `no A`
- Conjuncion: `A y B`
- Disyuncion inclusiva: `A o B`
- Condicional: `si A entonces B`, `A implica B`
- Condicional inverso: `A solo si B` (equivale a `A implica B`)
- Bicondicional: `A si y solo si B`, `A equivale a B`
- Agrupacion: parentesis; precedencia de mayor a menor: `no`, `y`, `o`, implicacion, bicondicional.
- Varias premisas: una por linea; en argumentos CLI se admite `;` como separador.

Las palabras reservadas separan clausulas atomicas, por lo que estas deben escribirse como frases simples sin conectores reservados en su interior. No se interpretan referencias, cuantificadores, tiempos verbales ni lenguaje irrestricto; entradas fuera de la gramatica producen un error.

### Incompatibilidades del escenario

La logica proposicional no conoce automaticamente hechos del mundo: por defecto, dos atomos distintos pueden ser verdaderos al mismo tiempo. Declara los pares que no pueden cumplirse juntos con `--incompatible "proposicion A|proposicion B"`. Puedes repetir la opcion para agregar otras reglas. Los textos deben coincidir con atomos extraidos de las premisas. Por ejemplo:

```sh
./build/tableau-es --incompatible "es de dia|es de noche" "es de dia; es de noche"
./build/tableau-es --incompatible "sensor activado|sensor desactivado" "sensor activado; sensor desactivado"
```

Tambien puedes pasar pares al objetivo Make: `make run ARGS='--incompatible "A|B" "A; B"'`.

## Estructura

- `include/semantic_tableau/`: tipos del AST y API del pipeline.
- `src/logic.cpp`: construccion y formato de formulas.
- `src/pipeline.cpp`: lexer, extractor, parser, motor tableau y reporte.
- `src/main.cpp`: interfaz de consola.
- `tests/test_pipeline.cpp`: pruebas de satisfacibilidad, traduccion, errores y visualizacion.
- `Makefile`: objetivos `all`, `run`, `test` y `clean`.