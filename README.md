# Minimal C89 TUI demo

Primera prueba del núcleo TUI discutido:

- C89 para el core.
- Sin `malloc`: los objetos los proporciona el caller.
- `TuiControl` como base de `Window`, `Label` y `Button`.
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

Si el enlazador no encuentra ncurses, instala ncurses con tu gestor habitual
o ajusta `LDLIBS`/rutas en el Makefile.

## Estructura

- `tui.h`: API y estructuras públicas.
- `tui.c`: core TUI; no contiene ncurses.
- `console.h`: contrato del backend.
- `console_ncurses.c`: backend de PC/macOS.
- `demo.c`: ejemplo.
- `Makefile`: build.

El siguiente backend puede ser `console_mmio.c`, implementando exactamente
las mismas funciones de `console.h`.
