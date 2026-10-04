# TODO de z.tui

## Redibujado (lo que queda)

- **Procesar todos los eventos pendientes y repintar una vez.** Hoy cada tecla o
  movimiento de ratón gestiona un evento y repinta. Con repetición de tecla o una
  ráfaga de ratón habría que vaciar la FIFO de INPUT acumulando rectángulos y
  repintar al final. Hace falta una función de consola que diga si hay más eventos
  esperando. Es lo que más ayuda a la sensación de fluidez.
- **Rectángulos sucios dentro de cada control.** `tui_invalidate_rect` ya existe,
  pero ningún control lo usa: el Edit repinta sus 20 celdas por una letra, la lista
  todas sus filas por mover la selección, el área de texto y el editor todas sus
  líneas por mover el cursor. Que cada uno invalide solo lo que cambió. TAB entre
  controles grandes (18-108 ms) es lo más caro hoy.
- **Camino caliente del backend.** El 70 % del tiempo son esperas de memoria de
  datos, no MMIO. Probar: quitar la tabla `mini_shadow` (lee y escribe memoria en
  cada celda y se vio que no ayudaba); escribir el texto con una función que reciba
  la cadena y calcule el atributo una vez; sacar del bucle interno el recorte y las
  llamadas.
- **Hardware** (solo si se quiere más velocidad en general). Muchos fallos de
  caché de instrucciones (~37 %): una caché mayor o un prefetch aceleraría todo, no
  solo el TUI. Un comando MMIO de «rellenar rectángulo» o «copiar bloque» en la RAM
  de texto convertiría un fondo de ventana o un desplazamiento en una operación.

## Preguntas

- ¿Vale la pena poner la pila al final de la memoria? 32 MB en casi todos los
  sitios, excepto en la GPU con EBR, que tiene algunos KB (¿32 o 128?).
