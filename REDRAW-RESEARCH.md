# Cómo resuelven el redibujado otros sistemas (punto 16 del TODO)

Hecho el 6/10/2026 con consultas a la documentación y al código de cada sistema.
Cada afirmación lleva su fuente; lo que **no** se pudo comprobar está al final.

## Qué se quería saber

1. ¿Llega al pintado la información de **qué ha ocurrido** («Enter: baja todo»), o solo
   qué zona está sucia? (punto 15 del TODO)
2. ¿Quién decide **cuándo** se pinta: tras cada evento o cuando la cola está vacía?
   (punto 5)
3. ¿Cómo se **desplaza** contenido sin reescribirlo? (puntos 8 y 15)

## Resumen

| Sistema | Modelo | Qué sabe el pintado | Desplazamientos | Cuándo pinta |
|---|---|---|---|---|
| **ncurses** | Pantalla virtual y física; solo se transmite la diferencia | Líneas «tocadas» (`wnoutrefresh` copia solo las tocadas) | Los **detecta solo**: compara líneas por hash y usa insertar/borrar línea del terminal (`hashmap.c`, `hardscroll.c`); `idlok`/`scrollok` lo permiten | A petición: `doupdate` una sola vez tras varias ventanas |
| **Win32 (GDI)** | Región de actualización y `WM_PAINT` | La región sucia (`rcPaint`); nada sobre el motivo | **Operación explícita** `ScrollWindowEx`: mueve los píxeles y devuelve la región que queda por pintar | `WM_PAINT` solo cuando **no hay otros mensajes en la cola** |
| **Qt** | `update()` y evento de pintado con región | La región | `scrollContentsBy(dx, dy)` es un **gancho del manejador**: por defecto hace `update()` de todo el visor; se puede reimplementar para desplazar | `update()` aplaza y varias llamadas dan un solo pintado |
| **Turbo Vision** (tvision, libre) | Árbol de vistas con recorte | El recorte; `exposed()` antes de pintar | Una vista que se mueve o se oculta **repinta lo de debajo** (`drawUnderRect`) | Tras el evento; `lock`/`unlock` para agrupar |
| **Wayland** | El cliente da «daño» al compositor | Rectángulos dañados, con doble búfer hasta `commit` | — | En `commit` |
| **Textual** (Python) | Compositor de segmentos visibles | Qué región ocupa el widget que cambió | — | — |
| **Ratatui** (Rust) | Modo inmediato: se redibuja todo en un búfer y se compara con el anterior | Nada: la aplicación no marca zonas sucias | Ninguno explícito | Cada `terminal.draw()` |
| **Bubble Tea** (Go) | Modo inmediato: la vista es una cadena; se comparan líneas con la última | Nada | Ninguno explícito | Cada render |
| **z.tui** | Árbol retenido con rectángulos sucios y `draw` con recorte; la consola compara contra su propia copia (`mini_shadow`) | La zona sucia; el recorte | Ninguno aún | Tras cada evento |

## Hallazgos

### 1. Nadie le pasa al pintado «qué ha ocurrido»

En ninguno de los sistemas revisados el código que pinta recibe una descripción del
cambio. Lo que recibe es una región:

- Win32: el ejemplo de `WM_PAINT` pinta dentro de `ps.rcPaint` (la región a pintar) y
  `DefWindowProc` valida la región de actualización.
- Wayland: el cliente da rectángulos de daño que describen «las regiones donde el
  búfer nuevo es distinto del contenido actual».
- Turbo Vision: `drawUnderRect` fija el recorte del dueño y deja que cada vista de debajo
  se pinte dentro de él.

Esto coincide con la conclusión del punto 15: **el `draw` debe ser una función del estado
y del recorte**, sin saber por qué se llama.

### 2. La información semántica va a una operación que llama el manejador

Dónde sí viaja «ha ocurrido un desplazamiento de dy filas»:

- **Win32 `ScrollWindowEx(hWnd, dx, dy, …)`**: la aplicación dice cuánto se desplaza; el
  sistema mueve lo que ya está en pantalla y devuelve la región que ha quedado sin
  pintar (`prcUpdate`/`hrgnUpdate`), que se invalida con `SW_INVALIDATE`.
- **Qt `scrollContentsBy(dx, dy)`**: la documentación dice que se llama cuando las barras
  se mueven, que la implementación por defecto hace `update()` de todo el visor y que
  una subclase puede reimplementarla «para optimizar», con dx y dy «útiles al desplazar
  píxeles». El dato semántico llega al **manejador**, no al pintado.
- **ncurses**: `scrollok`/`idlok` y las operaciones de desplazamiento que usa el terminal
  (insertar/borrar línea).

Es el mismo patrón que se proponía en el punto 15: el manejador invoca una operación de
la librería que mueve las celdas ya dibujadas y solo invalida lo que queda al descubierto.

### 3. Mover contenido necesita comprobar que no lo tapa nada

La documentación de `ScrollWindowEx` es cuidadosa con esto: el rectángulo de recorte
(`prcClip`) limita lo que se mueve («los bits que pasan de fuera a dentro se pintan; los
que pasan de dentro a fuera, no»), con `SW_SCROLLCHILDREN` avisa de que si parte de una
ventana hija queda fuera del rectángulo origen, «no se actualiza bien», y recomienda otra
función. Turbo Vision pinta solo `if (exposed())`. Textual descarta lo oculto por un
widget superior al componer.

Traducido: copiar celdas desplazadas **solo es correcto si el control está a la vista**
(sin ventanas encima ni recorte por un padre). Si no, hay que caer a invalidar el
rectángulo, que ya es correcto. Es la comprobación de visibilidad que se discutió para
el atajo directo.

### 4. Pintar cuando la cola está vacía es lo habitual

- Win32: «El sistema envía este mensaje cuando no hay otros mensajes en la cola de
  mensajes de la aplicación.» `GetMessage` devuelve `WM_PAINT` solo entonces.
- Qt: `update()` no pinta de inmediato; programa un evento de pintado, y varias llamadas
  dan normalmente uno solo.

El lote de eventos del punto 5 es exactamente esto. La medición dijo que ya no hacía
falta (la cola máxima bajó a 1), pero el diseño no es una rareza: es la práctica
común, y se puede añadir sin riesgo cuando haga falta (con tope de eventos por lote).

### 5. El modo inmediato con comparación no sirve a 4,5 MIPS

Ratatui y Bubble Tea redibujan todo en cada fotograma y comparan con el anterior (el
primero por celdas, el segundo por líneas). La aplicación no marca zonas sucias. Es
cómodo, pero el coste por fotograma es proporcional a la pantalla. Un `tui_draw()`
completo en la MiniCPU son ~1,3 M de instrucciones (~0,3 s), así que no sirve.
Nuestra consola ya hace la parte de comparar (`mini_shadow`) y es justo donde se ve que
comparar cuesta casi lo mismo que escribir (punto 6 del TODO).

ncurses es un híbrido útil: **marca líneas tocadas** (`wnoutrefresh` copia solo esas),
compara contra la pantalla física, y encima **detecta por sí mismo** que unas líneas se
han movido (modifica el algoritmo de Heckel sobre hashes de las líneas) para usar
insertar/borrar línea. Esa detección automática evita que el manejador tenga que decir
nada, a costa de mantener hashes de línea.

### 6. Turbo Vision es el pariente más cercano

El árbol de vistas con recorte, `exposed()` antes de pintar, `drawUnderRect` para repintar
lo de debajo con el recorte fijado en el rectángulo afectado y `drawUnderView` ampliando
ese rectángulo por sombras y marcos: es la misma arquitectura que `tui_draw_region`,
`tui_find_cover` y `tui_invalidate_frame`. No hay motivo para cambiar de modelo.

## Qué implica para z.tui

1. **El modelo está bien.** Árbol retenido, rectángulos sucios y `draw` puro con recorte es
   lo que hacen Turbo Vision, Win32 y Qt. Los modos inmediatos no valen para esta CPU.
2. **Punto 15, decidido en principio:** una operación que llama el manejador, p. ej.
   `tui_scroll_rect(control, x, y, w, h, dy)`, que devuelve 1 si desplazó celdas y 0 si el
   llamante debe invalidar. Condiciones para desplazar: control entero a la vista y sin
   rectángulos sucios pendientes que lo toquen; si no, 0. La consola necesitaría una
   operación opcional de copiar celdas (`tui_console_...`); con ella, el punto 8 del TODO
   (copiar bloque por hardware) sería su implementación rápida. No hacerlo sin medir: hoy
   un Enter en el editor repinta desde la fila del cursor hasta la última línea.
3. **Punto 5:** mantenerlo como seguro. Es la práctica de Win32 y Qt.
4. **No copiar** la detección automática de líneas desplazadas de ncurses: exige hashes de
   línea y comparaciones por fotograma.

## Lo que no se ha podido comprobar

- **notcurses**: las páginas del manual consultadas (`notcurses(3)`, `notcurses_render(3)`)
  no dicen cómo evita repintar lo que no cambia; no se ha mirado el código.
- **Qt**: la frase de `update()` («varias llamadas dan normalmente un solo `paintEvent()`»)
  y la de `QWidget::scroll` salen de resultados de búsqueda, no de la página oficial (la
  consulta directa de la página de `QWidget` no devolvió el texto de esas funciones).
  `scrollContentsBy` sí se leyó en la documentación oficial.
- **Wayland**: la descripción del daño sale de resultados de búsqueda del protocolo, no de
  la especificación.
- **Turbo Vision**: el código de `drawView` y `drawUnderRect` salió de un resumen de la
  herramienta sobre `tview.cpp`; `exposed()` no se leyó (el extracto no lo incluía), y lo
  que se dice de él es una suposición razonable. La documentación de Free Vision
  consultada (`Lock`, `Exposed`, `DrawView`) solo trae las declaraciones, sin texto; lo
  que se dice de `lock`/`unlock` (agrupar actualizaciones) sale de un resumen de
  búsqueda.
- **ncurses**: que cada línea guarde la primera y la última columna tocadas no se ha
  comprobado (se recuerda de la estructura de ventana, no de lo consultado).
- **No examinados:** Dear ImGui, React, Flutter y los demás.
- **`ScrollConsoleScreenBuffer`** (copiar celdas en la consola de Windows) existe, pero no
  se ha leído su documentación.

## Fuentes

- ncurses, [`curs_refresh(3x)`](https://invisible-island.net/ncurses/man/curs_refresh.3x.html), [`curs_outopts(3x)`](https://invisible-island.net/ncurses/man/curs_outopts.3x.html) y la [guía de internos](https://invisible-island.net/ncurses/hackguide.html) («The Engine Room»: `hashmap.c`, `hardscroll.c`, `lib_doupdate.c`).
- Win32: [`WM_PAINT`](https://learn.microsoft.com/en-us/windows/win32/gdi/wm-paint) y [`ScrollWindowEx`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-scrollwindowex).
- Qt: [`QAbstractScrollArea::scrollContentsBy`](https://doc.qt.io/qt-6/qabstractscrollarea.html) y [`QWidget`](https://doc.qt.io/qt-6/qwidget.html).
- Turbo Vision: [`tview.cpp` de tvision](https://github.com/magiblot/tvision/blob/master/source/tvision/tview.cpp) y la [documentación de Free Vision](https://docs.freepascal.org/daily/packages/fv/views/tgroup.lock.html).
- [Ratatui: bajo el capó](https://ratatui.rs/concepts/rendering/under-the-hood/).
- [Textual: algoritmos para aplicaciones de terminal de alto rendimiento](https://textual.textualize.io/blog/2024/12/12/algorithms-for-high-performance-terminal-apps/).
- [Bubble Tea, `standard_renderer.go` (v1.3.4)](https://github.com/charmbracelet/bubbletea/blob/v1.3.4/standard_renderer.go).
- [notcurses(3)](https://notcurses.com/notcurses.3.html) y [notcurses_render(3)](https://notcurses.com/notcurses_render.3.html).
- Wayland: [protocolo `wl_surface`](https://wayland.app/protocols/wayland#wl_surface:request:damage_buffer).
