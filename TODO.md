# TODO de z.tui

## Redibujado

Lo que se notaba lento: el editor grande y arrastrar ventanas. Los dos están
resueltos en lo esencial. El arrastre con los puntos 1, 2, 10 y 11; el editor ha
pasado de 14,85 a **1,8** fotogramas por flecha y de 11,0 a **1,0** por letra (puntos
12 y 3). Lo que queda del editor es sobre todo lógica del evento (unas pasadas por
el texto), no escribir celdas.

### Pendiente

| # | Arreglo | Qué cura | Esfuerzo | Notas |
|---|---------|----------|----------|-------|
| 3b | Rectángulos finos en el TextArea | TextArea (todo su rectángulo) | Bajo-medio | ListBox y Edit ya están hechos (punto 18). El gancho del foco ya está también en el TextArea (`textarea_focus_changed`, solo la celda del cursor; con la demo, «TextArea» y «Scrolling», el cambio de foco pasó de 436 celdas a unas 250). Falta lo demás del TextArea: al escribir o mover el cursor sigue invalidando todo (se heredaría al unirlo con el editor, punto 13). Quedan con el comportamiento de siempre (invalidar entero al cambiar el foco) Button, CheckBox, RadioButton, ComboBox y ScrollBar: son pequeños o se ven distinto con el foco. **Pendiente medido:** el marco de una ventana cuesta 122 celdas por un anillo de 56, porque `window_draw` rellena toda la ventana y luego pinta el marco encima (y el título encima del marco). Rellenar solo el interior lo dejaría en unas 66. |
| 5 | Lote de eventos: `tui_pump(desktop, max)` | Ráfagas largas de UART (texto pegado) | Bajo-medio | El sondeo (`tui_console_poll`) ya existe: sería un bucle que despacha mientras haya algo, sin repintar, y repinta al final. Con tope de eventos por lote (si no, una ráfaga continua nunca repinta) y cortando al salir (`running` a 0, ESC). Despachar cada evento antes de sondear el siguiente (el ratón vive en estáticas de un solo hueco). **Ya no es urgente:** teclear a ~18 teclas por segundo en el editor perdía 6 de 20 letras; ahora salen las 20 y la cola máxima es 1. La repetición de tecla de la MiniCPU no lo necesita: `mini_input_repeat` genera como mucho una tecla por sondeo. Es la práctica común: Win32 solo envía `WM_PAINT` cuando no hay otros mensajes en la cola y Qt aplaza `update()` y junta varias llamadas en un pintado. |
| 13 | Unir `TuiTextArea` y `TuiEditor` | Código duplicado (~la mitad) | Medio | Tras renombrar, 233 de las 447 líneas de código del editor aparecen idénticas en el textarea (707 y 733 líneas en total). El modelo lineal (`TuiLinearTextModel`) trabaja en sitio sobre el buffer de la aplicación, igual que el textarea, así que este puede quedar como un envoltorio fino de `TuiEditor` + modelo lineal, con la misma API pública. El textarea heredaría las lecturas por trozos (punto 12) y los rectángulos finos (punto 3). Antes de borrar el código: comprobar que pasa `test_textarea.c` (29 usos) y comparar el comportamiento (teclas, solo lectura; el editor además tiene comando y posición). `TuiEdit` (una línea) se queda aparte: es ligero a propósito. |
| 14 | Menos pasadas por el texto en cada evento | Lo que queda del editor: ~1-1,65 fotogramas de lógica por evento | Medio | Cada evento hace la instantánea de antes (medir y cursor), el manejador (cursor, línea, sync) y la de después (medir y cursor otra vez): unas 5-6 pasadas por trozos con 552 caracteres. Opciones: que el manejador devuelva su `EditorView` para no repetir la sincronización, o cachear la cuenta de líneas y la anchura máxima. **El modelo puede cambiar por fuera** (por eso no hay caché hoy), así que haría falta un contador de versión en `TuiTextModel` que suban `tui_text_model_insert`/`delete`. Medir antes con `profile_editor_*`. |
| 15 | Que el `draw` sepa qué ha ocurrido | Desplazar una línea, insertar o borrar líneas sin repintarlas | Medio-alto | **Problema semántico:** el `draw` solo ve el estado y el recorte, y no puede distinguir «han quitado una ventana de encima» de «Enter: hay que bajar todo». Esa información solo la tiene el manejador, que es quien hace el cambio. Pasarla como dato al `draw` (un puntero con una descripción del cambio) tiene problemas: el `draw` se llama también por razones ajenas (ventana movida, repintado completo), los avisos se fusionan y se pierden al combinar rectángulos o lotes, y rompe que `draw` sea una función pura del estado. Alternativa: que el **manejador** invoque una operación de la librería («desplazar esta región una fila»), que mueve las celdas existentes (con la orden de copiar bloque de la consola, punto 8, o celda a celda) y solo invalida lo que queda al descubierto. El `draw` sigue puro. Hoy un Enter repinta desde la fila del cursor hasta la última línea; con esto bastaría la fila nueva. **Investigado (punto 16, `REDRAW-RESEARCH.md`):** nadie pasa «qué ha ocurrido» al pintado; Win32 (`ScrollWindowEx`) y Qt (`scrollContentsBy`) lo reciben en una operación o gancho del manejador, y ncurses lo detecta solo con hashes de línea. Propuesta: `tui_scroll_rect(control, x, y, w, h, dy)`, que devuelve 1 si desplazó celdas y 0 si el llamante debe invalidar; solo si el control está entero a la vista y sin sucios pendientes que lo toquen (Turbo Vision pinta solo `if (exposed())`; `ScrollWindowEx` avisa de lo que pasa con lo tapado). Medir antes: hoy un Enter repinta desde la fila del cursor hasta la última línea. |
| 6 | Camino caliente del backend | Esperas de memoria de datos (70 % del tiempo, no MMIO) | Medio | Probar: quitar la tabla `mini_shadow` (lee y escribe memoria en cada celda; se vio que no ayudaba); escribir el texto con una función que reciba la cadena y calcule el atributo una vez; sacar del bucle interno el recorte y las llamadas. Sin garantía de mejora. |
| 7 | Hardware: caché de instrucciones mayor o prefetch | Fallos de caché de instrucciones (~37 %) | Alto | Acelera todo, no solo el TUI. Solo si se quiere más velocidad en general. |
| 8 | Hardware: comando MMIO de «rellenar rectángulo» o «copiar bloque» en la RAM de texto | Fondo de ventana, desplazamientos | Alto | Los convertiría en una sola operación. Es el mecanismo del punto 15; el punto 15 es lo que hace falta para poder usarlo. Es el equivalente de la inserción y el borrado de línea que ncurses pide al terminal. |

Orden recomendado: probar en la placa lo hecho, luego 13 (unir textarea y editor,
que arrastra 3, 12 y 17 al textarea), y con el punto 16 delante decidir 14 y 15.

### Hecho

| # | Arreglo | Dónde |
|---|---------|-------|
| 1 | Fusionar rectángulos que se solapan en `tui_dirty_add` | `tui.c`. Se unen si la unión no es mayor que la suma de los dos, y se repite hasta que no haya más (cada unión libera un hueco). Pruebas: `test_pending_overlapping_rectangles_merge` y `test_pending_window_drag_is_one_rectangle`. |
| 2 | Juntar los MOVE consecutivos del ratón en uno | `tui_read_event` en `tui.c`, para todos los backends. Tras un MOVE sondea; si el siguiente también lo es, descarta el anterior, y si es otra cosa la guarda en un hueco (`tui_lookahead`) para la llamada siguiente. Teclas, DOWN y UP no se tocan y conservan el orden. Pruebas en `test_core.c` (`test_read_event_*`). |
| 3 | Rectángulos finos en el editor | `tui_editor.c`. Antes cada evento manejado invalidaba el control entero. Ahora `editor_event` toma una instantánea antes (`EditorBefore`) y `editor_invalidate_changes` decide después: viewport movido (scroll, barras que aparecen o se van) = todo; líneas añadidas o quitadas = desde la fila del cambio hasta la última línea que hubo o hay; letra o borrado en una línea = esa fila desde la columna que cambió; solo el cursor = su celda nueva (el cursor es una superposición de la consola, la celda antigua no hace falta); nada cambió = `tui_event_done`. Las barras (los setters no invalidan) se invalidan a mano si cambia su pulgar. Además `editor_draw` usa el recorte: no lee las filas ni las columnas fuera de él y para al pasar la última fila. Pruebas en `test_mini_keys.c`: `test_pending_editor_writes_only_what_changed` (celdas escritas por evento) y `test_pending_editor_random_events` (1500 pasos aleatorios de teclas y clics, cada uno comparado con un redibujado completo). **Comprobado por mutación:** romper a propósito cuatro reglas (una sola celda al teclear, Enter sin las filas de abajo, sin la barra vertical, sin la horizontal) hace fallar 484, 42, 4 y 2 comprobaciones. |
| 4 | `tui_console_poll()`: sondeo sin bloqueo en los cuatro backends | `console.h` documenta el contrato: la tecla o `TUI_KEY_NONE` si no hay nada ahora (lo que el TUI no usa se salta dentro: soltar tecla, modificador solo, movimiento dentro de la misma celda). `tui_console_key()` espera sondeando: `WaitForSingleObject` en win32 y vt, `nodelay` en ncurses, bucle en mini. La consola de pruebas (`test_support.c`) tiene una cola programable. Win32 y vt compilan con MSVC; ncurses y el uso interactivo de los backends de PC **no se han probado**. |
| 10 | Recortar lo invalidado a lo que dejan ver las ventanas ancestro | `tui_invalidate_rect` en `tui.c`. Una ventana solo deja ver a sus hijos dentro de su borde (`tui_get_child_context`), pero el rectángulo sucio no se recortaba: al arrastrar una ventana más allá de su padre se repintaba, capa a capa de abajo arriba y directamente sobre la RAM de texto, lo que hay fuera del padre, y se veía lo de debajo hasta que lo de encima se dibujaba otra vez. Un rectángulo que queda vacío cuenta igualmente como invalidación (si no, `tui_dispatch` invalidaría toda la pantalla). Prueba: `test_pending_window_drag_against_the_parent`, con la consola de pruebas contando las escrituras que cambian una celda fuera del padre (`test_watch_outside`); fallaba en 63 pasos. Los fotogramas del arrastre pasan de 23 a 21. |
| 11 | Arrastrar una ventana limitado al interior de su padre | `window_drag_to` en `tui_window.c`. Antes solo se limitaba al escritorio: una ventana podía salirse entera de su padre (que la recorta), desaparecer y no poder volver a agarrarse, porque la prueba de impacto también recorta. Si el padre es una ventana, quedan al menos 4 celdas del título dentro y la fila dentro de su área interior; con otro padre, solo el escritorio. La prueba comprueba en cada paso que el título se puede agarrar. |
| 12 | El editor lee el texto por trozos y no repite recorridos | `tui_editor.c`. Cada lectura de un carácter pasaba por tres llamadas (`editor_at` → `tui_text_model_read` → `read` del modelo) y una tecla hacía unas 7 500. Ahora se lee de `EDITOR_CHUNK` (32) en 32 caracteres, las líneas y la anchura máxima se miden en una sola pasada (`editor_measure`), `editor_sync_view` deja el resultado (`EditorView`) para el dibujo y el ratón, y `editor_draw` lee cada línea visible de una vez. |
| 17 | El foco invalida solo lo que cambia | Nuevo miembro `focus_changed` en `TuiClass` (`tui.h`), opcional: 0 = invalidar entero, como siempre. `tui_desktop_set_focus` lo llama para el control que pierde el foco y el que lo gana (`tui_invalidate_focus`, en `tui.c`). **Editor** (`editor_focus_changed`): solo cambia el cursor, que `editor_draw` pide mientras tiene el foco, así que invalida la celda del cursor (el escritorio ya lo había ocultado); sin cursor (solo lectura o sin modelo) no pinta nada. No sincroniza el viewport, porque el control puede no estar maquetado todavía: si la celda no cabe, cae a invalidar entero. **ListBox** (`listbox_focus_changed`): solo la fila seleccionada se dibuja distinta con el foco, y es lo único que invalida. Pruebas: `test_pending_focus_draws_only_what_changes` (el cursor se oculta y reaparece donde estaba, y las celdas escritas), `test_pending_listbox_focus_draws_one_row`, y la aleatoria del editor mezcla ahora cambios de foco hacia un botón y vuelta. **Comprobado por mutación:** hacer que el gancho del editor no pinte nada falla 1 comprobación (la del cursor, que la comparación de celdas no ve) y el de la lista 4. |
| 18 | Rectángulos finos en ListBox y Edit | **ListBox** (`listbox_event`): toma una instantánea (selección, desplazamiento, barra visible); si el desplazamiento o la barra cambian invalida todo, si solo cambia la selección invalida las dos filas (la de antes y la de ahora, `listbox_invalidate_row`), y si no cambia nada llama a `tui_event_done`. El clic ya no pasa por `tui_listbox_set_selected` (que invalida entero): usa `listbox_select`, sin invalidar. **Edit** (`edit_event` y `edit_focus_changed`): si el texto se desplaza, todo; si cambia la longitud, desde la primera columna que cambió (el mínimo entre el cursor de antes y el de ahora) hasta el borde; si solo se mueve el cursor, su celda; si no cambia nada, `tui_event_done`. El foco invalida la celda del cursor. Pruebas: `test_pending_listbox_moves_draw_two_rows`, `test_pending_edit_draws_what_changed` y `test_pending_edit_and_listbox_random_events` (1500 pasos, cada uno comparado con un repintado completo). **Comprobado por mutación:** acotar el rectángulo del Edit a la columna del cursor nuevo falla 19 comprobaciones. 7880 comprobaciones, 0 fallos. Sin probar en la placa. Nota: `tui_edit_init` deja el cursor al final del texto sin ajustar el desplazamiento, así que con un texto más largo que el control el cursor queda fuera de la vista hasta la primera tecla (ya era así). |
| 16 | Investigar cómo lo resuelven otros sistemas TUI y GUI | `REDRAW-RESEARCH.md`. ncurses, Win32, Qt, Turbo Vision, Wayland, Textual, Ratatui y Bubble Tea, con fuentes y con lo que no se pudo comprobar. Conclusiones: nadie pasa «qué ha ocurrido» al pintado, los desplazamientos son una operación o gancho del manejador, pintar con la cola vacía es lo habitual (Win32, Qt), el modo inmediato con comparación no sirve a esta CPU y Turbo Vision es el pariente más cercano (el modelo de z.tui es el correcto). |

Resultado en el simulador (fotogramas de 75000 instrucciones, ~16,7 ms a 4,5 MIPS;
guiones `profile_editor_nav`, `_letter` y `_click` menos `_none`, entre 20; cada
acción con tiempo de sobra para terminar su repintado):

| | Original | Con el 12 | Con el 12 y el 3 |
|---|---|---|---|
| Flecha abajo | 14,85 (9,85 repintado + 5,00 evento) | 4,00 (3,25 + 0,75) | **1,80** (0,60 + 1,20) |
| Letra | 11,00 (8,00 + 3,00) | 4,00 (4,00 + 0,00) | **1,00** (0,00 + 1,00) |
| Clic en el texto | — | 4,00 (3,70 + 0,30) | **1,80** (0,15 + 1,65) |

Lecturas del modelo por tecla (host, 552 caracteres): antes 2 946 al manejar + 4 592
al dibujar; con el 12, 42 + 68. Teclear deprisa (`profile_editor_type.txt`, 20 teclas
a ~18 por segundo): original, 14 letras de 20 y cola máxima 16; con el 12, 20 de 20 y
cola máxima 5; con el 12 y el 3, 20 de 20 y cola máxima 1.

Cambiar el foco desde y hacia un editor de 78×21 (host, con un botón en la misma
ventana): **2194 celdas escritas antes, 18 ahora** (la celda del cursor y el botón).
En la pantalla Controls de la demo un TAB ya costaba 1,0 fotograma antes y sigue
costando 1,0 (`profile_tab.txt` menos `profile_tab_none.txt`): cruza controles
pequeños. El cálculo de 18-108 ms que figuraba antes era de cuando el editor
costaba ~15 fotogramas.

Lo que queda por tecla es casi todo el evento (1,0-1,65): el repintado ya es de 0 a
0,6. El evento creció porque ahora hace la instantánea y vuelve a sincronizar para
decidir el rectángulo (punto 14).

### Cómo se repartía el coste del repintado (antes de los puntos 12 y 3)

Experimento: la demo de medición con la escritura de celdas sustituida por una
función vacía en toda la librería (un fichero de unidad que hace
`#define tui_console_cell tui_cell_nop` antes de incluir las fuentes de la
librería y deja la real solo para el informe; no está guardado). Con una tecla cada
1,5 M de instrucciones, para que cada una dé un repintado entero:

| | normal | sin celdas | escribir celdas |
|---|---|---|---|
| Flecha abajo | 9,85 | 7,95 | ~1,9 (19 %) |
| Letra | 8,00 | 6,00 | ~2,0 (25 %) |

El 75-80 % restante era lógica, y casi toda eran las lecturas del modelo (≈ 100-130
instrucciones cada una, como cota superior). Por eso el punto 12 ganó más que el
punto 3 prometía; el 3 quitó después lo que quedaba (celdas y filas fuera del
recorte).

### Arrastre de ventanas, antes y después de los puntos 1 y 2

Resultado en el simulador con `make mini-profile` (arrastre de la ventana Button
15 celdas, un movimiento cada ~10 ms; los fotogramas descuentan los 13 de los dos
primeros repintados, el dibujo completo y activar la ventana):

| | repintados | eventos | cola máx. | fotogramas de arrastre |
|---|---|---|---|---|
| Antes | 17 | 20 | 9 | 29 |
| Con el 1 | 17 | 20 | 8 | 19 |
| Con el 1 y el 2 | 9 | 12 | 1 | 10 |
| Con el 1 y el 2, ya en `tui_read_event` | 9 | 12 | 0 | 10 |

La última fila es la misma medición con el colapso movido de la consola mini a la
librería: sale igual. Antes, cada repintado de arrastre costaba ~1,8 fotogramas
(~30 ms) y llegaba un movimiento cada ~10 ms: la cola crecía hasta 9 y la ventana
iba ~300 ms por detrás del ratón.

## Medición (`make mini-profile`)

`tui_unity_profile.c` compila la demo con `TUI_PROFILE_EVENTS`: antes de cada
repintado cuenta los eventos que esperan en la FIFO de INPUT, mide los fotogramas
que gasta repintando y manejando eventos, y al salir escribe el resultado en las dos
primeras filas de la pantalla (repintados, eventos, repintados con la cola no vacía,
eventos en cola, máximo, fotogramas repintando, fotogramas manejando eventos,
histograma de la cola y celda del último clic).

Guiones en `test/` (`make mini-profile MINI_PROFILE_SCRIPT=test/<guion>`):

| Guion | Qué hace |
|---|---|
| `profile_drag.txt` | Arrastra la ventana Button 15 celdas a la izquierda. |
| `profile_editor_repeat.txt` | Llega al Editor y mantiene la flecha abajo 64 fotogramas. |
| `profile_editor_type.txt` | Llega al Editor y teclea 20 letras a ~18 por segundo. |
| `profile_editor_nav.txt` | Llega al Editor y pulsa 20 veces la flecha abajo, muy espaciadas. |
| `profile_editor_letter.txt` | Llega al Editor y teclea 20 `X`, muy espaciadas. |
| `profile_editor_click.txt` | Llega al Editor y hace 20 clics en el texto, una celda más a la derecha cada vez. |
| `profile_editor_none.txt` | Llega al Editor y sale. Hay que restarlo a nav, letter y click. |
| `profile_tab.txt` | En la pantalla Controls pulsa 20 veces TAB, muy espaciadas. |
| `profile_tab_none.txt` | Arranca la demo y sale. Hay que restarlo a `profile_tab.txt`. |

Coste por acción: `(fotogramas - fotogramas_none) / 20`, sumando las columnas de
repintado y de manejo del evento.

- El reloj del simulador son instrucciones, no tiempo: 75000 por fotograma
  (~4,5 MIPS a 60 Hz). Sirve para comparar antes y después; los milisegundos son
  orientativos. Los fotogramas de un evento suelto tienen un error de ±1 por la
  resolución del reloj; con 20 acciones se promedia.
- La ventana ListBox de la demo es `TUI_WINDOW_FIXED` y no se arrastra. El guion
  usa Button.
- Un movimiento del ratón con el botón pulsado se escribe `mouse report 0b1 dx dy`;
  `mouse move` suelta el botón.
- Falta medir en la placa, con el ratón del PC (`monitor.py input`), que añade su
  propio retardo (~16 ms por ida y vuelta).
- Validado en la placa (6/10/2026): el Editor (teclas, clic, foco) y el foco en el
  ListBox van bien. El arrastre de ventanas con el ratón del PC sigue sin medirse.

## Preguntas

- ¿Vale la pena poner la pila al final de la memoria? 32 MB en casi todos los
  sitios, excepto en la GPU con EBR, que tiene algunos KB (¿32 o 128?).
