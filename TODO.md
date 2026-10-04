


no veo porque se intenta redibujar todo (ahora ya solo un control en algunos casos)... si un solo control recibe la tecla y hace algunos putch, que necesidad de repintar todo? si ya hemos pintado lo que hace "falta" en las posiciones que hacen falta. tui_mini escribe directamente en la consola, verdad? no en un buffer intermedio. 
    Que hace TUI_LOCAL

creo que el tui_console deberia detectar que tenemos disponible en inicializacion, si solo hay
uart, pues getch funciona y tal... si hay teclado y raton (input es?) pues tenemos mas cosas.

a nivel de depuracion y pruebas automaticas, esta bien el sistema de "monitor input" o es mas
comodo el de uart? deberiamos añadir algo para poder automatizar pruebas en input?

tiene sentido que el programa que captura mouse siempre envie los eventos de forma que aparezcan en la pantalla en la posicion donde estamos "pintando" en la aplicacion, ahora muchas
veces es asi, pero al salir y entrar de la app a veces hay un cierto offset.

vale la pena poner la pila al final de la memoria? 32MB en casi todos los sitios excepto en la
gpu con ebr que hay algunos kb, no se si 32 o 128?

---

Sí, hay varias. Son ideas, no he medido ninguna. Las ordeno por lo que creo que rinde más respecto al esfuerzo.

1. Procesar todos los eventos pendientes y redibujar una vez. Hoy cada tecla o movimiento de ratón hace su propio repintado. Si hay varios eventos en la FIFO de INPUT (repetición de tecla, ráfaga de ratón, alguien escribiendo rápido), se podrían gestionar todos acumulando las áreas a repintar y repintar una sola vez al vaciar la cola. Hace falta una función de consola que diga si hay más eventos esperando. Es lo que más ayuda a la sensación de fluidez, porque evita que la cola se llene y que el repintado vaya siempre por detrás.

2. No pintar el fondo que luego tapa el control. Al repintar solo la zona de un control, la ventana pinta primero su fondo en ese rectángulo y luego el control lo reescribe. Cada celda se escribe dos veces. Si el control es opaco (TUI_LOCAL ya implica que pinta todo su rectángulo) y la región cae entera dentro de él, el núcleo puede saltarse el dibujo de los padres. Es un cambio pequeño en tui_draw_tree y debería casi partir por la mitad el coste de los repintados de un solo control.

3. Repintar la barra de estado solo si su texto cambió. Ahora se repinta tras casi cada evento aunque diga lo mismo. Basta comparar el texto antes y después.

4. Rectángulos sucios dentro de cada control. El Edit repinta sus 20 celdas aunque solo cambie un carácter. El TextArea repinta las filas enteras para mover el cursor. El TAB es lo más caro hoy, porque repinta dos controles grandes solo para quitar y poner el indicador de foco. Si cada control publicara qué sub-rectángulo cambió (una fila de la lista, una línea del área de texto, el cursor), la demo repintaría esa parte. No es el truco de celdas sucias del PC: lo decide el control, que sabe qué hizo.

5. Revisar el camino caliente del backend. El 70 % del tiempo son esperas de memoria de datos, no el MMIO. Sospecho de tres cosas:

La tabla de comparación mini_shadow lee y escribe memoria en cada celda. Como ya vimos que no ayudaba, quizá solo cuesta. Quitarla se puede probar en minutos.
El texto se escribe celda a celda. Una función que reciba una cadena y un atributo calcularía el atributo una sola vez.
El compilador mete todo por la pila. Sacar del bucle interno el recorte y las llamadas ahorraría accesos.
6. Cambios de hardware, los que más ganan pero cuestan más. Los contadores muestran muchas esperas de búsqueda de instrucciones (unos 37 % de fallos de caché de instrucciones). Una caché mayor o un prefetch aceleraría todo el sistema, no solo el TUI. Además, un comando MMIO de «rellenar rectángulo» o «copiar bloque» en la RAM de texto haría que un fondo de ventana o un desplazamiento de lista fueran una sola operación en vez de cientos de escrituras.

Mi recomendación: hacer 1, 2 y 3. Son cambios pequeños en la demo y el núcleo. Después medir antes de decidir si merece la pena el 4 o el 5. El 6 solo si queremos que todo vaya más rápido que el TUI.