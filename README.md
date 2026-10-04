# Minimal C89 TUI demo

Primera prueba del núcleo TUI discutido:

- C89 para el core.
- Sin `malloc`: los objetos los proporciona el caller.
- `TuiControl` como base de `Window`, `Label` y `Button`.
- `TuiTextArea`: editor multilínea sobre un buffer de la aplicación (`\n` como
  separador, cursor como offset, sin word wrap), con scrollbars vertical y
  horizontal automáticos, ratón y modo solo lectura (`tui_textarea_set_readonly`).
  La demo incluye tres ejemplos.
- `TuiTextModel`: interfaz mínima de texto por offsets (`length`, `read`,
  `insert`, `delete`), sin líneas ni cursor. `TuiLinearTextModel` la implementa
  sobre un `char[]` externo, sin malloc.
- `TuiEditor`: editor sobre un `TuiTextModel` (no es propietario del texto).
  Cursor como offset absoluto, scroll automático, ratón, `tui_editor_get_position`
  (línea/columna/offset, base 0), `tui_editor_is_modified/set_modified` y un
  comando (`tui_editor_set_command`) al cambiar cursor o texto por acción del
  usuario. Demo: menú Demo > Editor.
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
- Backend Windows mediante la API de consola Win32, sin ncurses.
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
seleccionar/proporcionar el backend compatible con ese target. El backend
MMIO se valida directamente con `mini-lcc`:

```powershell
..\tools\mini-lcc.ps1 .\console_mini.c -o .\_build\console_mini.s
```

Si el enlazador no encuentra ncurses, instala ncurses con tu gestor habitual
o ajusta `LDLIBS`/rutas en el Makefile.

## Compilar en Windows

El backend `console_win32.c` usa únicamente la API de consola de Windows y no
requiere ncurses ni otra biblioteca externa. Con MinGW-w64:

```sh
make win32
./demo-win32.exe
```

También se puede seleccionar otro compilador con `WIN32_CC`, por ejemplo:

```sh
make win32 WIN32_CC=x86_64-w64-mingw32-gcc
```

Con Visual Studio, importa las variables de MSVC en la terminal actual
ejecutando el script con dot-sourcing:

```powershell
. .\import-vsdev-env.ps1
cl
```

El primer punto es necesario: si se ejecuta como `.\import-vsdev-env.ps1`,
las variables sólo existirán en el proceso hijo y no quedarán disponibles en
la terminal. Se puede elegir otra arquitectura:

```powershell
. .\import-vsdev-env.ps1 -Architecture x86 -HostArchitecture x64
```

Después, compila con NMAKE usando el makefile específico de MSVC:

```powershell
nmake /f Makefile.msvc
```

Para usar el backend VT, recomendado en el terminal integrado de VS Code:

```powershell
nmake /f Makefile.msvc vt
.\demo-vt-win32.exe
```

Para comparar automáticamente dirty rendering con un frame completo:

```powershell
nmake /f Makefile.msvc profile
```

El benchmark escribe una línea de texto durante 1000 frames y muestra el
tiempo medio de presentación para ambas variantes.

Este backend usa la API Win32 sólo para entrada y secuencias ANSI/VT para
salida. Entra en el alternate screen buffer, evitando los problemas de
desplazamiento de filas que pueden aparecer con ConPTY.

La suite de tests, que usa la consola simulada y no necesita una terminal
interactiva, también se puede compilar y ejecutar con MSVC:

```powershell
nmake /f Makefile.msvc test
```

El ejecutable de tests queda en `test\test_controls.exe`.

Para limpiar los artefactos:

```powershell
nmake /f Makefile.msvc clean
```

El backend conserva el contrato de `console.h`: teclado, ratón, doble clic,
colores DOS, cursor, tamaño de consola y caracteres gráficos. Al cerrar la
aplicación, la ventana visible se limpia y el cursor vuelve a su origen.

## Tests

```sh
make test
```

La suite organiza las pruebas por control (`test_listbox.c`, `test_edit.c`,
`test_button.c`, `test_checkbox.c`, `test_radiobutton.c`, `test_combobox.c`, `test_scrollbar.c`, `test_textarea.c`, `test_textmodel.c`, `test_editor.c`,
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
- `tui_textarea.c`: editor multilínea.
- `tui_textmodel.c`: interfaz `TuiTextModel` y modelo lineal `TuiLinearTextModel`.
- `tui_editor.c`: `TuiEditor` sobre un `TuiTextModel`.
- `tui_edit.c`, `tui_listbox.c`: controles de edición y lista.
- `tui_checkbox.c`, `tui_radiobutton.c`, `tui_combobox.c`: selección
  booleana, exclusiva por grupo y desplegable.
- `tui_menu.c`: MenuBar y PopupMenu.
- `tui_statusbar.c`: StatusBar.
- `console.h`: contrato del backend.
- `console_ncurses.c`: backend de PC/macOS.
- `console_win32.c`: backend de Windows sin dependencias externas.
- `console_vt_win32.c`: backend VT de Windows para ConPTY.
- `console_mini.c`: backend MMIO 80x30 para la consola de la máquina MiniCPU.
- `demo.c`: punto de entrada y aplicación de ejemplo.
- `test/test_*.c`: pruebas unitarias separadas por control y core.
- `test/test_support.c`, `test/test_support.h`: consola simulada y helpers.
- `Makefile`: build.
- `Makefile.msvc`: build de Windows con MSVC/NMAKE.

`console_mini.c` implementa el backend MMIO 80x30 para la máquina real:
entrada por SERIAL y salida por `VIDEO.TEXT`. El build unity puede seleccionarlo
con `-DTUI_BACKEND_MMIO`.

La demo completa también puede compilarse para MiniCPU con `mini-lcc`: en ese
modo desactiva temporalmente los textos formateados que dependen de `stdio`
(`sprintf`) y muestra etiquetas estáticas. Los builds de escritorio conservan
los textos dinámicos:

```powershell
..\tools\mini-lcc.ps1 .\tui_unity_mini.c -o .\_build\tui_mini.s
..\tools\mini-asm.ps1 .\_build\tui_mini.s -o .\_build\tui_mini.bin
```

Los dos pasos, más probar y subir, están como targets del makefile (en Windows,
tras `. .\import-vsdev-env.ps1`; en Unix, `make` con `../tools` en el `PATH`):

```powershell
nmake /f Makefile.msvc mini                 # compila y ensambla a _build\tui_mini.bin
nmake /f Makefile.msvc mini-sim             # la ejecuta en el simulador y muestra la pantalla
nmake /f Makefile.msvc mini-run             # la sube a la placa (prototipo 30) y la arranca
nmake /f Makefile.msvc mini-run PORT=COM3   # igual, con el puerto a mano
```

`mini-sim` usa el simulador funcional, que modela la consola de texto. La demo
lee el teclado por la serie, así que `--serial-input` hace de teclado (aquí,
solo `Esc`, que cierra la demo) y `--console-output` vuelca la pantalla al
terminar. A mano:

```powershell
[IO.File]::WriteAllBytes("keys.bin", [byte[]](27))
..\tools\cpusim.ps1 .\_build\tui_mini.bin --serial-input keys.bin --console-output pantalla.txt --run-limit 50000000
```

`mini-run` usa `run-board`: comprueba que la placa tenga el bitstream de la 30
(y lo sube si no), carga el `.bin` y arranca la CPU. Para manejar la demo hace
falta un terminal serie en el mismo puerto, que `run-board` libera al terminar.

### Teclado y ratón en la MiniCPU

`console_mini.c` lee el bloque INPUT (`mmio.md` §25) además de la UART. Las dos
entradas conviven: lo que llegue antes. La entrada nueva sale de
`monitor.py input` (el teclado y el ratón del PC, sin nada conectado a la FPGA).
Si `SYSTEM.DEVICES` no declara INPUT, la consola funciona como antes, solo con
serie.

- **Teclado español** (`mini_keys.h`). INPUT entrega teclas físicas; la tabla
  las convierte en CP437: `ñ Ñ ç Ç ¡ ¿ º ª`, AltGr (`@ # | ~ [ ] { } \`), acentos
  muertos (`´ ` ^ ¨` + vocal), teclado numérico, F1–F12 y navegación (Supr,
  Inicio, Fin, RePág, AvPág, Insert, Shift+Tab). Está en un `.h` con funciones
  `static` para que lo ejecute la placa y lo pruebe el PC (`test/test_mini_keys.c`).
  Las mayúsculas acentuadas que CP437 no tiene (Á Í Ó Ú) salen sin acento.
- **Repetición de tecla**: el contrato no define typematic, así que la hace la
  consola (400 ms y luego unas 30 por segundo), mirando `KEY_STATE`.
- **Ratón**: relativo, 1 punto = 1 píxel de la pantalla de 640×480. El puntero
  invierte los colores de la celda; el doble clic (500 ms, misma celda) lo
  calcula la consola. El TUI solo se entera cuando cambia la celda o un botón.
- **Texto de 8 bits**: los controles preguntan a la consola qué es imprimible
  (`tui_console_printable`). Los backends de PC siguen en ASCII; la MiniCPU
  admite 128..255.

Probar sin placa: `nmake /f Makefile.msvc mini-sim-input`.

#### Redibujado por invalidación

`tui_draw()` reescribe las 2400 celdas, y en la MiniCPU cuesta ~1,3 millones de
instrucciones (casi 0,4 s: mide la placa con `CYCLES`/`RETIRED`; el 74 % son
esperas de memoria, no de MMIO). Con teclado y ratón cada evento lo disparaba y
la demo iba a ~3 eventos por segundo. Ahora los controles dicen qué cambian y el
escritorio repinta solo eso, como el `InvalidateRect` de Windows:

```c
tui_dispatch(&desktop, &event);
tui_draw_pending(&desktop);       /* en vez de tui_draw() */
```

- **`tui_invalidate(control)`** y `tui_invalidate_rect()` (en coordenadas del
  control) marcan un rectángulo como sucio. El escritorio recuerda hasta
  `TUI_DIRTY_MAX` rectángulos; con más, el siguiente dibujo es completo.
- **Cada control invalida lo suyo**: Edit, TextArea, Editor, ListBox, CheckBox,
  RadioButton (y los hermanos que desmarca), ComboBox, ScrollBar, Label y
  StatusBar, tanto al gestionar un evento como en sus `set_*`. Añadir
  (`tui_add`), quitar (`tui_remove`) y subir al frente una ventana, mover una
  ventana y cambiar el foco (el control que lo pierde, el que lo gana y los marcos
  de las dos ventanas) invalidan solos.
- **`tui_event_done(control)`** dice «lo gestioné y no cambió nada visible» (un
  clic sobre un botón, un movimiento sobre una lista desplegada).
- **Un evento gestionado sin ninguna invalidación se toma como «puede haber
  cambiado cualquier cosa»** y `tui_dispatch` invalida la pantalla entera. Así un
  control que no sabe de esto (o uno nuevo) sigue siendo correcto, solo más lento:
  hoy lo son los menús.
- Un evento que **nadie gestiona** no invalida nada: un movimiento de ratón sobre
  el fondo o una tecla sin dueño cuestan 0.
- Lo que haya **encima** de un rectángulo sucio se vuelve a pintar con él
  (`tui_draw_begin` / `tui_draw_region` / `tui_draw_end` recorren el árbol en
  orden y se saltan lo que no toca la región), así que el solape no importa.

Medido en la placa (ms, con ida y vuelta por el puerto serie): letra en un
cuadro de texto ~17; espacio en checkbox, radio o botón ~10–30; flecha en una
lista 10–48; TAB entre controles 18–108 (repinta los dos controles y los marcos);
abrir o cerrar la lista de un combo ~55–70; ratón sobre una lista desplegada 2–4;
redibujado completo ~256. `tui_draw()` no cambia. `tui_fill` y `tui_text` recortan
el rectángulo una vez en vez de celda a celda.

**Trampa de `mini-lcc`**: `(unsigned char)x` sobre un `int` no enmascara. Un
carácter ≥128 leído de un buffer de `char` llega con el signo extendido
(`0xFFFFFFA4`) y la RAM de texto lo rechaza con un error de MMIO. En
`console_mini.c` se enmascara con `& 0xff`.

La pila de arranque de `mini-lcc` es de 8 KiB: las estructuras grandes no pueden
ser locales de `main` (por eso `App` es `static` en `demo.c`). Si la pila se
desborda pisa el código y la CPU se para con error 5 (codificación inválida).
