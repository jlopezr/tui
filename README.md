# Minimal C89 TUI demo

Primera prueba del núcleo TUI discutido:

- C89 para el core.
- Sin `malloc`: los objetos los proporciona el caller.
- `TuiControl` como base de `Window`, `Label` y `Button`.
- `TuiListBox` con selección por teclado/ratón, activación y type-to-select.
- La demo muestra Label, Button, CheckBox, RadioButton, Edit, ComboBox y
  ListBox en ventanas separadas, más un ScrollBar vertical y otro horizontal
  en el panel derecho; el StatusBar muestra el último comando o
  el control con el que se interactuó.
- El menú `Demo` permite alternar durante la ejecución entre esa demo de
  controles y un mini IDE que muestra Project, Editor, Inspector y Output
  organizados con docking. MenuBar y StatusBar permanecen activos al cambiar.
- Ambas vistas se almacenan en `App` y se conectan/desconectan del árbol de
  controles al alternar; no se asigna memoria dinámicamente ni se duplica la
  inicialización de sus controles.
- El ComboBox abre y navega sus opciones con las flechas; su valor y el fondo
  del desplegable heredan el atributo de color de su padre.
- `TuiScrollBar` (vertical/horizontal): `min`/`max` es el rango lógico total,
  `page` lo visible y `value` la primera posición visible; el máximo efectivo
  es `max - page` (o `min`). Los setters normalizan y nunca emiten comandos;
  solo la interacción del usuario que cambia el valor emite `TUI_EV_COMMAND`.
  Flechas ±1, pista ±`page`, arrastre del thumb con captura, teclas
  flechas/RePág/AvPág/Inicio/Fin.
- Vtable (`TuiClass`) compartida por tipo.
- Árbol intrusivo de controles.
- Coordenadas de hijos relativas al área cliente de la ventana.
- Clipping en la primitiva de escritura.
- Backend de consola separado.
- Backend macOS/Unix mediante ncurses.
- Tab cambia el foco.
- Enter activa el botón enfocado.
- Esc sale.
- "Aceptar" cambia el texto del label.
- "Salir" termina.

## Compilar en macOS

```sh
make
./demo
```

El build habitual compila los módulos en translation units separadas. Para
compiladores sin linker que necesiten una sola unidad de traducción:

```sh
make unity
./demo-unity
```

`tui_unity.c` agrega los mismos módulos y selecciona el backend igual que el
build normal. El target actual de unity usa ncurses; para LCC se necesita
seleccionar/proporcionar el backend compatible con ese target.

Si el enlazador no encuentra ncurses, instala ncurses con tu gestor habitual
o ajusta `LDLIBS`/rutas en el Makefile.

## Tests

```sh
make test
```

La suite organiza las pruebas por control (`test_listbox.c`, `test_edit.c`,
`test_button.c`, `test_checkbox.c`, `test_radiobutton.c`, `test_combobox.c`, `test_scrollbar.c`,
`test_window.c`, `test_menu.c`, `test_statusbar.c` y `test_label.c`) y
`test_core.c` cubre árbol, layout, hit-testing y foco. `test_support.c`
comparte la consola simulada y los helpers; no requiere ncurses ni una
terminal interactiva. Se compila con los mismos flags C89 estrictos que el
resto del proyecto.

Para generar un informe de cobertura de líneas del core y los controles con
LLVM. El informe excluye los ficheros de tests y los headers:

```sh
make coverage
```

El informe se muestra en la terminal; el ejecutable instrumentado y los
perfiles quedan en `coverage/`, que se puede limpiar con `make clean`.

## Estructura

- `tui.h`: API y estructuras públicas.
- `tui_internal.h`: declaraciones compartidas sólo por la implementación.
- `tui.c`: core TUI (árbol, layout, dibujo, foco y dispatch).
- `tui_unity.c`: agregador opcional para un solo translation unit.
- `tui_window.c`, `tui_button.c`, `tui_label.c`: controles básicos.
- `tui_edit.c`, `tui_listbox.c`: controles de edición y lista.
- `tui_checkbox.c`, `tui_radiobutton.c`, `tui_combobox.c`: selección
  booleana, exclusiva por grupo y desplegable.
- `tui_menu.c`: MenuBar y PopupMenu.
- `tui_statusbar.c`: StatusBar.
- `console.h`: contrato del backend.
- `console_ncurses.c`: backend de PC/macOS.
- `demo.c`: punto de entrada y aplicación de ejemplo.
- `test/test_*.c`: pruebas unitarias separadas por control y core.
- `test/test_support.c`, `test/test_support.h`: consola simulada y helpers.
- `Makefile`: build.

El siguiente backend puede ser `console_mmio.c`, implementando exactamente
las mismas funciones de `console.h`.
