# Gentoo Hardened: Seguridad y Aislamiento

Gentoo Hardened es un proyecto que implementa funciones de seguridad avanzadas en Gentoo Linux. Synth3x utiliza este perfil endurecido para garantizar la privacidad y la integridad de los datos en memoria.

---

## 1. Endurecimiento del Compilador (Toolchain)

El compilador hardened aplica las siguientes medidas de seguridad predeterminadas:
*   **PIE (Position Independent Executables):** Carga los programas en direcciones de memoria aleatorias para mitigar ataques de desbordamiento de búfer.
*   **SSP (Stack Smashing Protector):** Inserta variables de control ("canarios") en la pila para detectar y abortar ejecuciones corruptas.

---

## 2. Protección del Espacio de Direcciones

El kernel hardened de Gentoo previene la ejecución de códigos maliciosos:
*   **ASLR (Address Space Layout Randomization):** Aleatoriza el mapa de memoria del proceso (pila, montón y bibliotecas).
*   **NX/W^X (No Ejecutable):** Asegura que las páginas de memoria sean de escritura o de ejecución, pero nunca ambas simultáneamente.

---

## 3. Privacidad y Red Tor

Synth3x redirige todo el tráfico de la red a través de la red distribuida Tor.
*   **Proxy Transparente:** La tabla de reglas del firewall (`/etc/nftables.rules`) intercepta y desvía la comunicación TCP al puerto local de Tor (9040).
