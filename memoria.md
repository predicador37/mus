# Entorno de desarrollo


# Estructura y compilación del programa

El programa está dividido en los siguientes archivos:

- `juego.c`: código del proceso maestro.
- `jugador.c`: código de cada uno de los procesos jugadores o workers.
- `mus.h`: cabeceras de funciones auxiliares utilizadas para la lógica del juego.
- `mus.c`: implementación de las funciones descritas en el archivo anterior.
- `CMakeLists.txt`: configuración de CMake para la compilación del programa.

# Memoria descriptiva

## Arquitectura de la solución

Se opta por una arquitectura de procesos maestro - esclavos, con el siguiente reparto de responsabilidades:

### Proceso maestro

- Inicialización del juego y entorno (mazo, jugadores, etc.)
- Creación de los procesos jugadores
- Selección aleatoria de jugador a dar el corte
- Intercambio de información entre jugadores
- Coordinación de turnos de jugadores
- Gestión de la entrada y salida estándar
- Cálculo de puntuaciones y jugadas

### Procesos jugadores
- Reparto de cartas según las reglas del mus
- Recepción de cartas en la mano de cada jugador
- Evaluación de la mano de cada jugador
- Lógica de mus corrido
- Lógica de descartes
- Lógica de envites

## Desarrollo de la práctica

### Lógica inicial

En primer lugar, se implementó la lógica de creación de procesos a partir del proceso maestro utilizando la primitiva `MPI_Comm_spawn`. También se definieron dos intercomunicadores:

- `parent`: para la comunicación de los jugadores con el maestro.
- `juego_comm`: para la comunicación del maestro con los jugadores.

Las primeras decisiones de diseño se orientaron a las estructuras de datos para almacenar las cartas, el mazo o baraja y las manos. Para todo ello, se definió la siguiente estructura:

        struct carta {

            char *cara;
            char *palo;
            int id;
            int orden; // orden en cada palo
            int valor; // puntos para el cálculo de juego
            int equivalencia; // tipo de carta a efectos de grande, chica y pares
            int estado; // 0: en mazo, 1: en mano, 2: descartada
        };
        /* fin struct carta */
        typedef struct carta Carta;

Inmediatamente se codificaron las funciones para crear un mazo, imprimirlo por pantalla, barajarlo (aleatoriamente) y cortarlo. 

La generación de números aleatorios para el barajado y la elección de jugadores o cartas se llevó a cabo usando la función `srand()` de la biblioteca `time.h`, aplicada a intervalos de números enteros en base al caso concreto (cartas, jugadores...).

### Comunicaciones entre procesos

El envío de los mazos (por ejemplo, entre el maestro y un jugador para repartir o entre dicho jugador y el maestro para actualizar las cartas este último) se empaquetó en funciones usando una llamada `MPI_Send()` y `MPI_Recv()` para cada elemento de la estructura `Carta`. De esta forma, se consiguió evitar la lógica compleja para enviar `structs` del lenguaje C usando MPI, pudiendo enviar tipos de datos sencillos como `arrays` de enteros o caracteres o enteros simples.

La actualización de parámetros entre todos los jugadores desde el maestro (como por ejemplo, qué jugador reparte o qué jugador es mano), se llevó a cabo utilizando la primitiva `MPI_Bcast()`.

Para enviar masivamente información al maestro desde los jugadores (por ejemplo, los conteos de cartas para los cálculos de jugadas) se utilizó la primitiva `MPI_Gather` con envíos constantes de N datos, descartando los N últimos por pertenecer al proceso maestro, que no juega.

### Lógica del juego

Para iniciar el juego, se selecciona aleatoriamente un jugador para cortar la baraja. Según el palo que salga, se elige el jugador repartidor: 

- Oros: el de su derecha.
- Copas: el de "en frente".
- Espadas: el de su izquierda.
- Bastos: él mismo.

Una vez repartidas las cartas, se inicia el mus corrido. La lógica para cortar el mus es la siguiente:

- Si el jugador tiene 31 en juego.
- Si el jugador tiene duples o medias.
- Si el jugador tiene pares y juego, aunque no sea 31.

De esta forma se intenta maximizar que el jugador gane a pares y juego. 

Si no hay mus, se produce la fase de descartes. La lógica de decisiones para descartar es muy simple: se descartan todas las cartas que no sean "chones" (reyes o treses). El objetivo de esta estrategia es intentar conseguir más reyes para poder tener jugada a grande y pares.

Una vez terminados los descartes y roto el mus, tiene lugar la fase de envites. La lógica de decisión para los envites es la siguiente:

- A grande: con 3 reyes o más, se envida 5. Con 2 reyes y una apuesta en vigor desconocida, nula o envite normal, se envida 2. En cualquier otro caso, no se envida.
- A chica: con 3 ases o más, se envida 5. Con 2 ases y una apuesta en vigor desconocida, nula o envite normal, se envida 2. En cualquier otro caso, no se envida.
- A pares: con 3 reyes o más, se envida 5. Con 2 reyes y una apuesta en vigor desconocida, nula o envite normal, se envida 2. En cualquier otro caso, no se envida.
- A juego: con 31, se envida 5. Con 32 y una apuesta en vigor desconocida, nula o envite normal, se envida 2. En cualquier otro caso, no se envida.
- Al punto: con 27 o más, se envida 5. Entre 24 y 27 y una apuesta en vigor desconocida, nula o envite normal, se envida 2. En cualquier otro caso, no se envida.

En cualquier lance existe una probabilidad aleatoria muy baja de lanzar un órdago. La idea de las reglas de envite descritas es que los jugadores manejados por la máquina sean muy conservadores y como mucho igualen las jugadas. De esta forma, se simplifica ligeramente la lógica del programa, entendiendo que no afecta a la complejidad de interés de la asignatura (la comunicación entre procesos).

Los envites y conteos de cartas son enviados al maestro, que se encarga de calcular los ganadores en cada lance y apuntar las piedras. 

### Desarrollo de la práctica

Se siguieron las fases descritas en el enunciado de la siguiente forma:

- Implementación de lances sin tener en cuenta mus corrido y descartes en modo automático.
- Implementación de fase de descarte (mus corrido y mus) en modo automático.
- Implementación de envites no deterministas en modo automático.
- Inclusión de lógica para interactividad con el jugador humano.
- Iteración de las vacas, juegos y rondas.

# Conclusiones

Las **dificultades encontradas** para el desarrollo de la práctica han sido realmente reseñables. Partiendo de la falta de costumbre de desarrollar en C (el autor de esta memoria hace más de diez años que no lo usa) y de la novedad de la materia de MPI, se citan las siguientes:

- Puesta en marcha de un entorno de desarrollo en C que permitiese agilizar la programación incluyendo avisos y resaltado de errores sin necesidad de compilación.
- Sincronización entre procesos: **sin duda la parte con mayor dificultad de la práctica**, debido a la necesidad de pensar como 5 programas en uno. En muchos casos los procesos quedaban bloqueados y, pese a la separación de la lógica en jugadores y maestro, es difícil superponer mentalmente la lógica de cada jugador en un mismo código. Igualmente, la dificultad de la depuración se multiplica por el número de procesos existentes.
- Dificultad de depuración en C: en muchos casos errores debido a un tamaño de buffer incorrecto por descuido o una errata de intercambiar un paréntesis con un signo igual pasaban desapercibidos provocando comportamientos inesperado y errores de segmentación, siendo realmente difíciles de corregir.
- Dificultad del lenguaje de programación C: al tratarse de un lenguaje de bajo nivel muy tipado, cualquier tarea sencilla requiere de la programación de una función específica (por ejemplo, invertir un `array` o calcular su máximo). Esto hace que el número de líneas del código crezca y posteriormente sea realmente complejo navegar entre ellas.
- Dificultad del juego del mus: para los que, como el autor de esta práctica, nunca hemos jugado al mus, la comprensión y asimilación de sus reglas supone un esfuerzo adicional bastante notable.
- Problemas con la E/S: se han dado problemas tanto con la entrada como con la salida de datos por pantalla, siendo muy útiles en este caso los foros de la asignatura para su resolución.

Como posibles mejoras se proponen las siguientes:

- Mejora de la lógica de decisión (órdagos, mus corrido, envites, etc.) incluyendo más jugadas.
- Tratamiento de errores de E/S.
- Corrección de posibles bugs.
- Las propuestas en el enunciado de la práctica.

Por último, en la opinión de este alumno, el peso global de esta práctica en el cómputo de nota la asignatura debería ser bastante mayor de dos puntos (al menos el doble). 
