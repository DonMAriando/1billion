# Dependencias de terceros

El juego depende de [SDL 3.4.18](https://github.com/libsdl-org/SDL/releases/tag/release-3.4.18), de Sam Lantinga y colaboradores, distribuido bajo licencia zlib. Su aviso está en [third_party/SDL-LICENSE.txt](third_party/SDL-LICENSE.txt).

La descarga oficial `SDL3-3.4.18.tar.gz` se verifica con SHA-256 `9c75cf16330322c217dedd2e0609f1124f1b54b8633e763467b4684d0f4334a3`.

SDL incluye componentes de terceros con sus propios avisos, conservados en su distribución de fuentes, entre ellos código de HIDAPI, stb y conversión de color. CMake descarga el archivo oficial completo sin modificar esos avisos. Se incluyen también los avisos [BSD de HIDAPI](third_party/SDL-HIDAPI-LICENSE-bsd.txt) y [original de HIDAPI](third_party/SDL-HIDAPI-LICENSE-orig.txt). Consultá `LICENSE.txt`, `src/hidapi/LICENSE*.txt` y los encabezados de los componentes correspondientes en la fuente de SDL.

Los bloques, la interfaz, el atlas de caracteres y el audio de este prototipo se construyen por código; no se descargan assets adicionales.
