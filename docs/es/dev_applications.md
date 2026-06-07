# Guía del Desarrollador: Aplicaciones Personalizadas

Synth3x soporta la compilación y ejecución de herramientas gráficas propias utilizando backends ligeros (GTK+3, Cairo o SDL2) y lenguajes compilados nativamente.

---

## 1. Integración en el Build System

Coloque el código de su aplicación bajo el directorio `prog/nombre-app/` y cree un Makefile que genere el ejecutable final.

### Modificar el Makefile Principal:
Añada la regla de compilación y copiado al `Makefile` en la raíz del proyecto para asegurar que el binario se cree y compile dentro de `build/prog/` de manera integrada:
```makefile
$(BUILD)/prog/mi-app:
	@mkdir -p $(BUILD)/prog
	@make -C prog/mi-app
	@cp prog/mi-app/mi-app $@
```
Añada el ejecutable a la lista `PROGS` para empaquetarlo automáticamente en el sistema de instalación.
