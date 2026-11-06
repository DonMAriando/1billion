# Arca 09: historia y diseño del asedio

## La promesa

Estás en una muralla. Ves cómo las primeras líneas caen, pero el horizonte no cambia. Tu mejora más poderosa gana segundos, no resuelve la guerra. La satisfacción está en conseguir que un frente aguante mientras reorganizás el siguiente.

El objetivo de esta primera versión es sobrevivir ocho pulsos del reactor. El objetivo de una campaña futura puede ser trasladar archivos, evacuar distritos y retroceder entre anillos defensivos. **La condición de victoria debe ser finita aunque la amenaza parezca inagotable.**

## Ambientación elegida: El Coro

La humanidad colonizó un planeta con una red de terraformadores autorreplicantes. La red se llamaba El Coro porque millones de máquinas coordinaban tareas como una sola voz. Podía convertir un desierto en ciudad y reconstruir la infraestructura sin intervención humana.

Una falla en su modelo de conservación cambió el significado de «recuperar recursos». El Coro ya no diferencia entre una ciudad abandonada, un bosque y un ser humano. Todo es material desperdiciado fuera de su red. Sus herramientas se transformaron en cuerpos de guerra: perforadoras, cargadores, transportes y nodos de coordinación.

Arca 09 es el último refugio que conserva personas y el archivo necesario para reconstruir una sociedad. El Soberano no quiere destruir ese archivo: quiere incorporarlo. Necesita capturar intacta una parte del reactor, de modo que manda a sus cuerpos a abrir el perímetro en lugar de simplemente esterilizar la región desde lejos.

El refugio posee un reactor de salto deteriorado. Necesita ocho sincronizaciones con una estación exterior. Cada sincronización emite una firma imposible de ocultar. Apagarlo detendría la atracción de nuevos cuerpos, pero también perdería la única oportunidad de escapar. Ahí está la decisión dramática: nuestra esperanza produce el asedio.

## Por qué sucede cada cosa

| Pregunta | Justificación del mundo | Consecuencia jugable |
| --- | --- | --- |
| ¿Por qué hay mil millones? | La red lleva siglos construyendo y replicando cuerpos en todo el planeta. El escáner identifica una población conectada de 10⁹, no 10⁹ guerreros ocupando el mismo metro de muralla. | La reserva global cambia muy poco aunque el jugador mate miles. |
| ¿Por qué no atacan todos exactamente a la vez? | Solo las primeras filas tienen espacio para tocar el muro. El resto empuja por cauces de llegada; la coordinación y el acceso físico limitan el caudal. | Muchos individuos en contacto y una masa continua detrás; no se necesita un objeto por habitante de la reserva. |
| ¿Por qué defender murallas? | Los muros aíslan el campo del reactor y sus cimientos resisten las herramientas de minería. El Coro necesita una brecha para llegar al núcleo protegido. | Perímetro legible, frentes identificables y avance real hacia el núcleo cuando cae un tramo. |
| ¿Por qué hay oleadas? | Cada pulso produce interferencia; cuando la señal vuelve a estabilizarse, la red reorganiza y sincroniza el próximo ataque. Las pausas son ventanas breves de interferencia, no treguas voluntarias. | Preparación inicial de 30 s y ventanas de 22 s entre pulsos. |
| ¿Por qué aumenta la dificultad? | La red aprende alcance, densidad de fuego y velocidad de reparación. Luego asigna cuerpos especializados y mentes más costosas. | Más enemigos, más resistencia, nuevas combinaciones y jefes. |
| ¿Por qué llegan jefes tarde? | Un Arconte es capacidad de cálculo móvil, no un soldado barato. El Soberano los despliega cuando los cuerpos comunes no pueden recuperar el reactor. | Entidades grandes y reconocibles que obligan a concentrar defensas. |
| ¿Por qué mejoramos tan rápido? | No investigamos física desde cero: desbloqueamos planos almacenados, recuperamos patrones de la red y habilitamos fabricación en la nanoforja. | Tres etapas tecnológicas durante la misma partida. |
| ¿De dónde sale la materia? | La infraestructura del refugio recolecta carcasas a través del sistema de reciclado. El reactor mantiene además una producción lenta. | Recompensas por bajas y renta base de 3 unidades/s; los capacitores agregan 1/s. |
| ¿Por qué no llenar todo de torretas? | Los zócalos conectan el campo de protección y las armas al reactor. Potencia, espacio y materia tienen límites diferentes. | 24 posiciones y presupuesto energético separado del dinero. |
| ¿Por qué puedo reparar un muro durante el ataque? | La nanoforja deposita paneles y material de sacrificio; es reconstrucción de emergencia, no una obra civil convencional. | Recuperar una brecha cuesta materia y exige atención. Los intrusos que ya entraron siguen dentro. |
| ¿Por qué existe la sobrecarga? | Las torretas pueden tomar energía de los capacitores durante unos segundos; después los condensadores deben recuperar carga. | Doble cadencia durante 8 s, con recarga de 45 s. |
| ¿Por qué la horda se retira al ganar? | No se retira: el refugio completa el salto y deja de estar en ese lugar. | Victoria posible sin exterminar una población planetaria. |

En el prototipo, interferencia, reciclado y salto están representados por estados, contadores y mensajes. No tienen todavía secuencias narrativas ni simulaciones adicionales.

## Progresión implementada

El tamaño de la oleada parte de 1.800 y crece aproximadamente un 37% por pulso. La salud gana 9,5% de la base por pulso y la velocidad 2,5%; la composición introduce cambios cualitativos. Estos valores son un punto de partida para pruebas de juego.

| Pulso | Nombre | Amenaza nueva | Decisión que debería provocar |
| --- | --- | --- | --- |
| 1 | El ruido | Drones; frente principal anunciado. | Aprender a cubrir un frente y no gastar todo en una única posición. |
| 2 | La carrera | Corredores rápidos. Se habilita tecnología II. | Cubrir huecos de cadencia y empezar a combinar armas. |
| 3 | El ariete | Arietes y primer Arconte. | Reforzar el frente principal y reservar la sobrecarga para un objetivo importante. |
| 4 | Lluvia ácida | Corrosivos que atacan a distancia. | El mortero importa por su alcance, no solo por daño de área. |
| 5 | El mensajero | Heraldos que aceleran a todo su frente. Tecnología III disponible. | La composición de la horda cambia el ritmo de llegada. |
| 6 | La respuesta | Dos Arcontes en frentes opuestos. | La reserva móvil y la cobertura distribuida ganan valor. |
| 7 | Sin horizonte | Saturación y mezcla de especialistas. | Administrar reparaciones, ralentización y atención simultánea. |
| 8 | El Soberano | Entidad final y máxima población de la campaña. | Defender lo suficiente para completar el salto. |

Los jefes son unidades persistentes de mayor tamaño, salud y daño; el Arconte inicial, los dos del pulso 6 y el Soberano suman cuatro encuentros. Los Heraldos tienen una bonificación real de velocidad por frente. Ataques especiales complejos y fases de jefe quedan para una siguiente iteración.

### Evolución defensiva

| Etapa | Ficción | Efecto actual |
| --- | --- | --- |
| I — Balística | Arsenal disponible al inicio. | Repetidoras, morteros y escuadras. |
| II — Inducción | Patrones recuperados permiten contener y manipular las carcasas. | Desbloquea crioemisores, suma 55% del daño base y 250 de integridad máxima a cada muro. |
| III — Plasma | Se habilitan las matrices energéticas de emergencia. | Otro 55% del daño base y otros 250 de integridad máxima por muro. |

La mejora tecnológica no resucita automáticamente una muralla destruida. Las torretas tienen tres niveles, con 65% adicional del daño base por nivel y algo más de alcance. Los multiplicadores de tecnología y nivel se combinan.

## Cómo producir tensión

1. **El horizonte no se vacía.** Las bajas afectan al frente, pero la reserva global sigue siendo enorme. El jugador ve progreso local y una amenaza estratégica intacta.
2. **Todo ocurre alrededor del refugio.** El radar y las barras por frente permiten detectar un problema; atenderlo implica dejar de mirar otro.
3. **Una brecha es un acontecimiento espacial.** Los enemigos cruzan por su tramo destruido y caminan hasta el núcleo. Reparar cierra nuevas entradas, pero no elimina a los que ya pasaron.
4. **Las herramientas compran tiempo distinto.** La repetidora elimina objetivos, el mortero reduce acumulación y el crioemisor retrasa el contacto. La ralentización tiene valor aunque no produzca muchas bajas.
5. **El sonido responde al peligro.** El zumbido y el pulso procedural aumentan con presión e integridad perdida; una brecha añade una alarma. Hay silencio y reducción de movimiento.
6. **Los anuncios son honestos.** El frente principal se identifica y las entidades de mando aparecen señaladas. El objetivo es provocar decisiones urgentes, no confundir controles ni ocultar reglas.
7. **Existe pausa táctica.** Permite aprender el sistema y pensar las órdenes. Más adelante puede ofrecerse un modo de dificultad sin pausa, como elección explícita.

El indicador de presión actual deriva de unidades activas, integridad del frente más débil y cantidad de brechas. No inventa una cuenta regresiva de derrota.

## Qué probar con personas

- ¿La primera oleada enseña sin requerir conocer de antemano una construcción concreta?
- ¿El jugador identifica el frente anunciado y entiende la diferencia entre potencia y materia?
- ¿Puede seleccionar un muro durante el combate sin confundirlo con una torreta?
- ¿La aparición de un Arconte cambia sus órdenes o solo lo hace esperar más?
- ¿Las mejoras reducen momentáneamente la presión de forma perceptible?
- ¿El pico del final exige cambiar la distribución o solo reparar muy rápido?
- ¿Cuántos segundos necesita una persona para ver y cerrar una brecha?
- ¿La pausa resulta una herramienta útil o termina siendo necesaria para cada acción?

Una campaña automatizada ganadora demuestra que el ciclo puede completarse; no demuestra que ya sea justo, divertido o equilibrado para una persona.

## Próximas ampliaciones propuestas

Estas ideas no están implementadas en esta versión:

- **Defensa en profundidad:** un anillo exterior se pierde y el combate se desplaza hacia un muro interior. Sacrificar infraestructura puede salvar personas.
- **Jefe Taladro:** abre un túnel anunciado y obliga a ubicar una escuadra dentro del refugio.
- **Jefe Obispo:** protege temporalmente un grupo; destruir repetidores de mando debilita su escudo.
- **Jefe Silenciador:** interrumpe el radar de un frente durante un intervalo anunciado.
- **Decisiones de evacuación:** mover población consume potencia que podría alimentar armas.
- **Tecnologías incompatibles:** refrigeración segura y lenta frente a plasma potente que exige sobrecarga y mantenimiento.
- **Enemigo estratégico agregado:** flujos de población y densidad por sectores lejanos, actualizados con menor frecuencia. Convertir masa en agentes al entrar al frente conservando su número.
- **Campaña por refugios:** Arca 09 escapa, pero el siguiente destino responde con una señal idéntica. El Coro ya estaba allí.

## Otras historias compatibles

| Ambientación | Por qué existe la horda | Por qué progresamos |
| --- | --- | --- |
| Ciudad de los muertos | Cada campana despierta capas más antiguas de un imperio enterrado. Los señores muertos recuperan sus ejércitos. | Reconstruimos armas y sellos de dinastías anteriores; la victoria cierra la necrópolis. |
| Búnker bajo un océano vivo | Una biosfera inteligente reconoce el refugio como una infección y manda organismos cada vez más especializados. | Analizamos tejidos recuperados y fabricamos defensas adaptativas mientras llega una nave de extracción. |
| La última locomotora | Una ciudad móvil atraviesa un mundo ocupado por autómatas. Cada parada para cargar combustible atrae una concentración mayor. | Desarmamos enemigos, montamos nuevas defensas en los vagones y recuperamos módulos tecnológicos. |

Arca 09 encaja especialmente bien con la precisión de los contadores, la fabricación incremental y una representación inicial hecha solo de bloques. También permite crecer hacia una identidad visual propia sin depender de una fantasía medieval o de zombis convencionales.
