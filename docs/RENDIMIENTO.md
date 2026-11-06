# Rendimiento y memoria

## Medición inicial — 6 de noviembre de 2026

Linux x86-64, GCC 13.3.0, CMake Release (`-O3 -DNDEBUG`), máquina virtual compartida con CPU identificada como **AMD EPYC 9V74**. La simulación es monohilo; no se usaron `-ffast-math`, extensiones SIMD obligatorias ni ajustes específicos de esa CPU.

Cada ejecución inicializa una escena de carga, calienta 150 ticks y mide 1.800 ticks: **60 segundos simulados a 30 Hz**, ejecutados tan rápido como permite el procesador. Hay 24 torretas de nivel 3 de distintos tipos, escuadras y reposición de bajas. Los muros y el núcleo son invulnerables únicamente en esta escena. Después de cada reposición, las bajas del tick pueden reducir la población hasta el siguiente tick; por eso se informa el mínimo activo realmente medido.

| Cupo solicitado | Mínimo activo | Media por tick | Percentil 95 | Peor tick observado | Memoria propia del núcleo |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 32.768 | 31.887 | 0,9452 ms | 1,2633 ms | 5,0234 ms | 1,087 MiB |
| 65.536 | 62.893 | 2,2601 ms | 3,1221 ms | 19,1545 ms | 2,087 MiB |
| 131.072 | 127.268 | 5,3105 ms | 7,3887 ms | 12,8820 ms | 4,087 MiB |

Resultados sin redondear y hashes: [benchmark.csv](benchmark.csv).

**Estos tiempos miden exclusivamente la simulación. No son FPS del juego, tiempos de GPU ni una garantía para otra computadora.** La variación de los máximos también incluye la planificación del sistema en un entorno compartido. Las capturas y pruebas gráficas locales se hicieron con el renderer de software de SDL y el driver de video `dummy`; falta medir una partida visible con la GPU de destino.

Reproducir desde una compilación Release:

```bash
siege_benchmark --enemies 32768 --seconds 60
siege_benchmark --enemies 65536 --seconds 60
siege_benchmark --enemies 131072 --seconds 60
```

Los hashes sirven para comparar repeticiones en el mismo ejecutable y entorno. El modelo usa coma flotante y no promete determinismo binario entre distintas arquitecturas o compiladores.

## Estructura del costo

- **Enemigos:** arrays separados de posiciones, posiciones anteriores, salud, ralentización, tipo, tramo objetivo y estado interior/exterior. Se reservan al crear la simulación. No hay objetos polimórficos ni una reserva por enemigo.
- **Borrado:** reemplazar al muerto por el último elemento vivo. El índice no es una identidad persistente; ningún proyectil conserva referencias a elementos que puedan moverse.
- **Movimiento:** un destino de perímetro y otro de núcleo. Solo se permite entrar tras alcanzar el tramo destruido asignado; no se atraviesa un muro vecino porque otro esté roto.
- **Presión local:** grilla de densidad de la actualización anterior; evita comparar cada individuo contra todos los demás. Es una aproximación de multitud, no una colisión rígida.
- **Targeting:** grilla espacial de 64 × 64 con listas enlazadas por índices preasignados. Las torretas consultan las celdas de su alcance cuando corresponde disparar. Los enemigos dentro del refugio tienen prioridad sobre los jefes exteriores.
- **Daño:** impacto inmediato y consultas de área sobre la misma grilla. Los trazos son efectos visuales, no proyectiles con una simulación de trayectoria adicional.
- **Orden de actualización:** spawn, movimiento, reconstrucción de grilla, fuego y eliminación. La compactación sucede después de terminar todas las consultas; nunca se recorren enlaces que quedaron inválidos por un borrado.
- **Profundidad:** 384 buckets reutilizados en lugar de ordenar todos los enemigos con `std::sort` cada frame. Es una aproximación suficiente para bloques, con posibles empates de profundidad a resolver si se agregan sprites grandes.
- **Render:** geometría coloreada por lotes, culling de cámara y atlas de caracteres generado una vez. Las letras y las formas se agrupan en buffers distintos. SDL sigue controlando la implementación de su backend gráfico.
- **Reloj:** 30 Hz de simulación, interpolación visual y objetivo de presentación de 60 Hz. Se limita a cinco ticks de recuperación por frame para evitar trabajo acumulado ilimitado; bajo sobrecarga el juego se ralentiza.
- **Audio:** bloques fijos de PCM mono generados por código y una cola acotada a aproximadamente 85 ms. Puede deshabilitarse sin cambiar la lógica del juego.

El recorrido de movimiento es lineal en enemigos activos. Las consultas espaciales dependen de la cantidad de objetivos dentro del alcance; una multitud extremadamente densa todavía puede hacerlas costosas. No se afirma costo constante para toda la simulación.

## Propiedad y liberación

| Recurso | Propietario | Liberación |
| --- | --- | --- |
| Arrays de enemigos y enlaces de grilla | `Simulation` / `EnemyPool` | Destructores de `std::vector` |
| Muros, escuadras, efectos y celdas | Contenedores con tamaño fijo | Vida de su objeto propietario |
| Ventana | `unique_ptr<SDL_Window, WindowDelete>` | `SDL_DestroyWindow` |
| Renderer | `unique_ptr<SDL_Renderer, RendererDelete>` | `SDL_DestroyRenderer`, antes de la ventana |
| Atlas de caracteres | `unique_ptr<SDL_Texture, TextureDelete>` | `SDL_DestroyTexture`, antes del renderer |
| Superficies temporales y capturas | `SurfacePtr` | `SDL_DestroySurface`, incluso ante un error |
| Audio | `unique_ptr<SDL_AudioStream, AudioDelete>` | `SDL_DestroyAudioStream`, que también cierra el dispositivo asociado |
| Subsistemas SDL | `SdlLifetime` | `SDL_Quit`, después de destruir la presentación |

Un error de construcción también destruye los miembros ya inicializados. Si falla la inicialización de SDL, se copia su mensaje, se llama a `SDL_Quit` y después se propaga el error.

Los buffers mantienen su reserva mientras la partida está abierta; **reiniciar reutiliza la memoria**. Liberar y volver a reservar todo por oleada produciría trabajo innecesario. Cerrar el programa destruye sus propietarios y devuelve la memoria.

El panel F3 distingue memoria propia del núcleo y buffers propios de presentación en CPU. No incluye reservas internas de SDL, el driver, la biblioteca estándar, las superficies del sistema ni VRAM. No debe interpretarse como RSS total.

## Qué se verifica

`siege_tests` cuenta las llamadas a `operator new` / `operator new[]` durante actualizaciones de carga, reinicio y arranque de una partida: el resultado debe ser cero. La prueba no pretende instrumentar los allocators internos en C de SDL, que además no se inicializa en ese ejecutable.

Las demás pruebas comprueban:

- Saturación exacta del pool y copia de todos los campos al compactarlo.
- Que las incorporaciones de una oleada esperan cuando el cupo está ocupado.
- Conservación de población: `reserva + vivos + bajas = 1.000.000.000`.
- Economía, potencia, desbloqueos, órdenes inválidas y reinicio.
- Repeticiones con la misma semilla y secuencia de órdenes.
- Una derrota real por brechas y daño al núcleo.
- Una victoria de primera oleada y una campaña completa con un comandante programado, tres etapas tecnológicas y cuatro jefes derrotados.
- Entrada por teclado/ratón y renderizado con SDL en `ui_smoke`.

AddressSanitizer y UndefinedBehaviorSanitizer se habilitan con `ONEBILLION_SANITIZERS=ON`. El workflow ejecuta también LeakSanitizer en Linux. En el entorno local de creación, LeakSanitizer no pudo leer `/proc/<pid>/task` y abortó su inspección de fugas; esto es una limitación del entorno, no un informe de ausencia o presencia de fugas. Las comprobaciones ASan/UBSan se pueden ejecutar aquí con `detect_leaks=0`; la comprobación de fugas queda separada en CI.

## Escala futura

El siguiente salto razonable no es crear un array de mil millones de agentes. Es simular cantidades y flujos en regiones distantes a menor frecuencia, convertir flujos en agentes al acercarse al perímetro y mantener la conservación de población entre capas. El prototipo ya separa reserva estratégica, individuos y representación del horizonte, pero todavía **no implementa** una simulación de flujos lejanos.

Antes de agregar hilos o una API de GPU más compleja, medir el renderer con una GPU real, revisar las consultas más densas y decidir el mínimo de hardware objetivo. Más complejidad no es una mejora de rendimiento por sí misma.
