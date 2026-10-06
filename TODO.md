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
| 14 | Menos pasadas por el texto en cada evento | Lo que queda del editor: ~1-1,65 fotogramas de lógica por evento | Medio | Ahora que el TextArea es el Editor (punto 19), se optimiza un solo código. Cada evento hace la instantánea de antes (medir y cursor), el manejador (cursor, línea, sync) y la de después (medir y cursor otra vez): unas 5-6 pasadas por trozos con 552 caracteres. Opciones: que el manejador devuelva su `EditorView` para no repetir la sincronización, o cachear la cuenta de líneas y la anchura máxima. **El modelo puede cambiar por fuera** (por eso no hay caché hoy), así que haría falta un contador de versión en `TuiTextModel` que suban `tui_text_model_insert`/`delete`. Medir antes con `profile_editor_*`. |
| 15 | `tui_scroll_rect`: que el manejador desplace celdas en vez de repintarlas | Insertar o borrar líneas (Enter, unir líneas) sin repintar las de debajo | Medio-alto | **Sin decidir.** Hoy un Enter repinta desde la fila del cursor hacia abajo. Propuesta: `tui_scroll_rect(control, x, y, w, h, dy)`, que devuelve 1 si desplazó celdas y 0 si el llamante debe invalidar; solo con el control entero a la vista y sin sucios pendientes que lo toquen. El `draw` sigue siendo una función pura del estado y el recorte (el manejador es quien sabe «qué ha ocurrido»; investigación en `REDRAW-RESEARCH.md`: Win32 `ScrollWindowEx`, Qt `scrollContentsBy`, ncurses con hashes de línea). **Pega:** copiar las celdas en la consola de la MiniCPU sigue escribiendo cada una en la RAM de texto, igual que repintarla, así que se ahorra la lógica del dibujo y no las escrituras; solo ganaría de verdad con la orden de copiar bloque por hardware (punto 8). Antes de decidir: medir cuánto cuesta hoy un Enter en el Editor (guion nuevo). |
| 7 | Hardware: caché de instrucciones mayor o prefetch | Fallos de caché de instrucciones (~37 %) | Alto | Acelera todo, no solo el TUI. Solo si se quiere más velocidad en general. |
| 8 | Hardware: comando MMIO de «rellenar rectángulo» o «copiar bloque» en la RAM de texto | Fondo de ventana, desplazamientos | Alto | Los convertiría en una sola operación. Es el mecanismo del punto 15; el punto 15 es lo que hace falta para poder usarlo. Es el equivalente de la inserción y el borrado de línea que ncurses pide al terminal. |

Orden recomendado: 14 (medir antes el reparto por fase de la lógica del evento; es lo
que queda del editor). El 15 está sin decidir: medir antes cuánto cuesta un Enter, y
depende en buena parte del 8 (copiar bloque por hardware). El 6, el 7 y el 8 solo si
hace falta más velocidad.

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
| 18 | Rectángulos finos en ListBox y Edit | **ListBox** (`listbox_event`): toma una instantánea (selección, desplazamiento, barra visible); si el desplazamiento o la barra cambian invalida todo, si solo cambia la selección invalida las dos filas (la de antes y la de ahora, `listbox_invalidate_row`), y si no cambia nada llama a `tui_event_done`. El clic ya no pasa por `tui_listbox_set_selected` (que invalida entero): usa `listbox_select`, sin invalidar. **Edit** (`edit_event` y `edit_focus_changed`): si el texto se desplaza, todo; si cambia la longitud, desde la primera columna que cambió (el mínimo entre el cursor de antes y el de ahora) hasta el borde; si solo se mueve el cursor, su celda; si no cambia nada, `tui_event_done`. El foco invalida la celda del cursor. Pruebas: `test_pending_listbox_moves_draw_two_rows`, `test_pending_edit_draws_what_changed` y `test_pending_edit_and_listbox_random_events` (1500 pasos, cada uno comparado con un repintado completo). **Comprobado por mutación:** acotar el rectángulo del Edit a la columna del cursor nuevo falla 19 comprobaciones. 7880 comprobaciones, 0 fallos. Probado en la placa: se ve bien. Además `tui_edit_init` ajusta ahora el desplazamiento para que el cursor, que empieza al final del texto, quede a la vista (antes quedaba fuera hasta la primera tecla con un texto más largo que el control). |
| 19 | Unir `TuiTextArea` y `TuiEditor` (punto 13) | Se borra `tui_textarea.c` (578 líneas): `TuiEditor` lo sustituye, sin alias. **Dos constructores:** `tui_editor_init` (modelo propio) y `tui_editor_init_buffer` (buffer de la aplicación, con un `TuiLinearTextModel` dentro del editor, campo `own`). `tui_linear_text_model_adopt` (como `init`, pero conserva la cadena que haya en el buffer y la corta dentro de la capacidad). `tui_editor_set_text` (cursor, desplazamiento y `modified` a 0; solo con el modelo interno), `tui_editor_get_text` (`""` con modelo ajeno) y `tui_editor_reset` (cambios por fuera del modelo: invalida y sincroniza; el recorte del cursor lo hace `editor_sync_view`). Decisiones: `init_buffer` conserva el texto (como Edit y TextArea), `set_text` abre un documento, y el contador de versión para detectar cambios por fuera queda para el punto 14. **Pruebas:** `test_textarea.c` pasó a `test_editor_buffer.c` con solo cambiar nombres, y sus 8 casos pasan sobre el Editor sin tocar una comprobación (7905 antes y después); más `adopt`, `set_text` y `reset` (7931). **Comprobado por mutación:** `set_text` sin poner `modified` a 0, `reset` sin invalidar, `adopt` sin terminar el buffer y `set_text` sobre un modelo ajeno fallan; el recorte del cursor en `reset` sobraba y se quitó. **Tamaño en la MiniCPU:** `.text` de 59 316 a 54 540 bytes (−4,8 KB) y `.bss` +76. Cambios visibles: `modified` y `command` (sin usar) y `set_readonly` ahora invalida. Probado en la placa: se ve bien. |
| 21 | El marco de una ventana se repinta con la mitad de escrituras, y solo si cambia | `window_draw` en `tui_window.c`: rellena solo el interior y deja que el marco escriba el borde (el anillo de 25×5 costaba 122 escrituras por 56 celdas, porque se rellenaba toda la ventana y luego se pintaba el marco, y el título encima; ahora unas 66). La ventana de menos de 2×2 se rellena entera. `tui_desktop_set_focus` (`tui_invalidate_active_frame`, en `tui.c`) solo repinta el marco de una ventana si se dibuja distinto al estar activa (`TUI_WINDOW_ACTIVE_DOUBLE`): «TextArea» y «Scrolling» de la demo no lo tienen, y sus marcos se repintaban para nada. Con la demo, el cambio de foco entre esas dos ventanas baja de 436 celdas a un par (las de los cursores). Pruebas: `test_pending_focus_across_plain_windows_skips_frames` y `test_pending_editor_buffer_focus_draws_only_the_cursor`. Probado en la placa: se ve bien. |
| 22 | Cada celda se escribe una vez al repintar texto (punto 20) | Se rellenaba la fila de espacios y se escribía el texto encima: cada letra cambiaba su celda dos veces, y la consola de la MiniCPU escribe las dos en la RAM de texto aunque la pantalla acabe igual (`mini_shadow` solo se salta las escrituras que no cambian nada). Medido en el banco de pruebas al repintar contenido que no ha cambiado (nuevo contador `test_cells_changed`): Editor 78×23, 2382 escrituras y **1176 cambiaban la celda**; Edit, 30 de 39; ListBox, 119 de 362. Ahora son **0** en los tres. Nuevas `tui_chars` (n caracteres con la fila recortada una vez) y `tui_text_padded` (texto y espacios hasta el ancho) en `tui.c`; las usan `editor_draw` (por filas: texto, resto de la fila; las filas bajo la última línea son solo espacios), `listbox_draw` y `edit_draw`. Pruebas: `test_pending_unchanged_repaint_changes_no_cell` y `test_text_helpers_respect_the_clip`. **Comprobado por mutación:** volver a rellenar antes en el Editor falla 4 comprobaciones, en la lista 3, en el Edit 1, y quitar el recorte izquierdo de `tui_text_padded` 3. **En el simulador** (`make mini-profile`, fotogramas de 75000 instrucciones): el arranque de la demo y el cambio al Editor pasan de 19 a 17 fotogramas, y el arrastre de 21 a 20; flecha (1,5), letra (1,0) y TAB (0,25) no cambian, porque ya repintaban poco y su coste es la lógica del evento (punto 14). Es menos de lo que estimaba por instrucciones (el Editor entero de ~2,4 a ~1,1 fotogramas): sin medir ese caso suelto. Sin probar en la placa. |
| 23 | Tramos de celdas en la consola (punto 6) | `tui_console_fill(x, y, n, ch, attr)` y `tui_console_text(x, y, texto, n, attr)` en `console.h`. La biblioteca (`tui_fill`, `tui_text`, `tui_chars` y `tui_text_padded`, en `tui.c`) calcula el tramo visible y hace **una** llamada por fila, en vez de una por celda. Los backends de PC y el de pruebas usan los bucles por defecto de `tui.c` (sobre `tui_console_cell`, sin `TUI_BACKEND_MMIO`); `console_mini.c` tiene los suyos, que pagan una vez por tramo los límites de pantalla, el atributo, el glifo y ocultar el cursor y el puntero. **Medido en el simulador** (`make mini-bench`, instrucciones por celda, 62 pasadas de 64): sin cambio, `tui_console_cell` 47, `fill` **11**, `text` **15**; con cambio, 100, **21** y **27**. **En la demo** (`make mini-profile`, fotogramas de 75000 instrucciones): el arranque de 17 a **7** fotogramas (de 19 antes del punto 22), y en la pantalla Controls 10 a 4; el coste de flecha, letra y TAB casi no cambia (1,45, 1,0 y 0,25: son lógica del evento, punto 14). **Pruebas:** `test_text_helpers_respect_the_clip` (ahora también `tui_text` y `tui_fill`) y, en el simulador, `make mini-runcheck` (`test/mini_runcheck.c`): compara la RAM de texto escrita con los tramos contra la escrita celda a celda, con recortes por los dos lados, un carácter abstracto, bytes con el bit alto y repeticiones; debe escribir `DIFF 0000`. **Comprobado por mutación:** 6 cambios en `tui.c` fallan en la suite del PC (7963 comprobaciones) y 5 en `console_mini.c` dan `DIFF` distinto de 0 (el que se salía por la derecha pasó la primera vez, por eso las filas 3 y 4 se comprueban). Código: +0,8 KB en la MiniCPU. Probado en la placa (6/10/2026): se ve perfecto. |
| 24 | Sin copia de la pantalla en la consola de la MiniCPU (punto 6) | Se quita `mini_shadow` (9600 bytes): `tui_console_cell`, `tui_console_fill` y `tui_console_text` escriben siempre en la RAM de texto. **Medido en la placa** (`make mini-perf`, contadores de CPU PERFORMANCE; ciclos por celda, repintando lo mismo / algo distinto / espacios y texto encima): con la copia 205 / 405 / 708, dos tablas como los PC 344 / 668 / 432, una tabla leyendo la RAM de texto 377 / 508 / 465, **sin copia 147 / 148 / 245**. Una lectura de memoria de datos cuesta ~60 ciclos y una escritura a la RAM de texto ~6 (`STALL_MMIO`), así que la comparación costaba más que lo que ahorraba, y las esperas son lo que domina (mismas ~14 instrucciones por celda en todas). El cursor y el puntero, que se dibujan encima, se ocultan solo si el tramo (o la celda) cae en su celda: `mini_overlay_hide_range`. Antes se ocultaban cuando cambiaba cualquier celda de la pantalla. **Pruebas:** `make mini-runcheck` ahora cubre los adornos (cursor y puntero con tramos que los solapan, que caen al lado, en la primera y la última celda, y los dos en la misma celda); **comprobado por mutación:** 6 cambios en `mini_overlay_hide_range` y sus llamadas dan `DIFF` distinto de 0 (dos sobrevivían hasta añadir el caso de la última celda). **Tamaño:** `.bss` de 33,6 a 24,0 KB y la imagen de 93,3 a 83,5 KB. Simulador (instrucciones): arranque de 7 a 5 fotogramas; flecha, letra y TAB sin cambio. `test/mini_perf.c` conserva la copia como variante A para poder repetir la medida. Subido a la placa y arranca sin error; falta mirarlo a ojo, sobre todo que el puntero y el cursor no parpadeen. |
| 16 | Investigar cómo lo resuelven otros sistemas TUI y GUI | `REDRAW-RESEARCH.md`. ncurses, Win32, Qt, Turbo Vision, Wayland, Textual, Ratatui y Bubble Tea, con fuentes y con lo que no se pudo comprobar. Conclusiones: nadie pasa «qué ha ocurrido» al pintado, los desplazamientos son una operación o gancho del manejador, pintar con la cola vacía es lo habitual (Win32, Qt), el modo inmediato con comparación no sirve a esta CPU y Turbo Vision es el pariente más cercano (el modelo de z.tui es el correcto). |

### Descartado

| # | Qué | Por qué no |
|---|-----|------------|
| 3b | Repintado fino al cambiar el foco en Button, CheckBox, RadioButton, ComboBox y ScrollBar | Son de una fila y pocas celdas: una tecla en un CheckBox o un RadioButton escribe como mucho 24 celdas (`test_pending_cost`), y un TAB en la pantalla Controls cuesta 0,25 fotogramas. Cada gancho `focus_changed` son ~200 bytes de código en la MiniCPU, que se comerían parte de lo que ganó la unión del punto 19. |
| 20 | Texto y relleno en una pasada en Menu, StatusBar, ComboBox, Button, CheckBox y RadioButton | Los que escriben mucho ya están (Editor, ListBox y Edit, punto 22). Mover la selección de un menú ya toca solo dos filas (`test_pending_menus`), y el resto son filas de pocas celdas o texto centrado. Si en algún momento hace falta en todos los controles a la vez, la alternativa es el doble búfer en la consola de la MiniCPU (`tui_console_cell` solo anota y `tui_console_present` vuelca las celdas distintas), a cambio de otra tabla de 9,6 KB y un recorrido del rango tocado. |
| 5 | Lote de eventos: `tui_pump(desktop, max)` | Con teclear a ~18 teclas por segundo y con la repetición de tecla la cola máxima es 1 (`profile_editor_type` y `profile_editor_repeat`), así que un lote no tiene nada que juntar; se procesan unas 60 teclas por segundo. Pegar texto largo desde el PC no pierde nada: el host hace el control de flujo del UART (`mmio.md` 8.2) y `RX_OVERRUN` no debería levantarse nunca; solo va más lento. Con una cola larga el lote ahorraría el repintado de cada tecla (el 27 % de un evento con flechas y el 65 % con letras, medido), pero requiere un bucle nuevo con tope, cortar al salir y despachar cada evento antes de sondear el siguiente (el ratón vive en estáticas de un solo hueco). Reabrir si pegar texto largo se vuelve un uso real. Es la práctica de Win32 y Qt, ver `REDRAW-RESEARCH.md`. |

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
  ListBox van bien. Después, con la unión de TextArea y Editor (punto 19), el
  repintado fino de ListBox y Edit (punto 18) y el marco de ventana sin repintar al
  cambiar el foco: se ve bien. El arrastre de ventanas con el ratón del PC sigue sin
  medirse.

## Preguntas

- ¿Vale la pena poner la pila al final de la memoria? 32 MB en casi todos los
  sitios, excepto en la GPU con EBR, que tiene algunos KB (¿32 o 128?).
