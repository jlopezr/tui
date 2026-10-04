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