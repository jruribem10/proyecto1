# Proyecto de telemática


## Estudiante
* **Nombre**: Jaime Uribe
* **Correo institucional**: jruribem@eafit.edu.co

## Descripción
El proyecto tiene como objetivo fundamental desarrollar un sistema de proxy inverso y balanceador de carga específicamente para aplicaciones HTTP. Este sistema actuará como intermediario entre clientes y un conjunto de servidores, optimizando la distribución de solicitudes HTTP para mejorar la eficiencia y el rendimiento general. La implementación se basa en el uso de la API Sockets, permitiendo que las aplicaciones en la red se comuniquen de manera efectiva y concurrente. Se pretende que el sistema maneje múltiples peticiones de manera simultánea, utilizando estrategias de balanceo de carga como Round Robin para distribuir equitativamente las solicitudes entrantes. El resultado esperado es un modelo robusto que mejore la velocidad de respuesta del servidor y proporciona una distribución equilibrada de la carga entre múltiples servidores backend.

## Palabras claves
* **Sockets:** Una interfaz de programación que permite la comunicación entre aplicaciones a través de redes. Los sockets facilitan el envío y recepción de datos entre procesos ejecutados en diferentes sistemas pero conectados a una red.
* **Proxy inverso:** Un tipo de servidor proxy que recibe solicitudes de internet y las transmite a servidores internos. Actúa como intermediario para los recursos de los servidores solicitados por los clientes externos, ofreciendo beneficios de balanceo de carga, seguridad mejorada y caché de contenidos.
* **Balanceador de carga:** Un dispositivo o software que distribuye el tráfico de red o las solicitudes a múltiples servidores basándose en diversos factores como la capacidad de carga, prioridad y disponibilidad de servidor. Su objetivo es optimizar el uso de recursos, maximizar el rendimiento, reducir la latencia y asegurar la disponibilidad.
* **HTTP:** El Protocolo de Transferencia de Hipertexto es el protocolo de comunicación que se utiliza para intercambiar información en la World Wide Web, definiendo cómo se transmiten y formatean los mensajes entre clientes y servidores.
* **Berkeley API:** Una implementación específica de la API de sockets, originaria de la Universidad de Berkeley. Es ampliamente utilizada para la comunicación entre aplicaciones a través de redes en sistemas Unix y sistemas operativos similares a Unix.
* 
## Arquitectura
La arquitectura del proyecto consta de los siguientes componentes:
* Aplicación Cliente: Envía solicitudes HTTP y recibe respuestas.
* HTTP Proxy + Balanceador de Carga: Actúa como intermediario entre el cliente y los servidores, distribuyendo las solicitudes entre varios servidores backend.
* Servidores de Aplicación Web: Un conjunto de tres servidores donde cada uno ejecuta la misma aplicación web replicada. Se puede usar NGINX o Apache como servidor web.
  ![imagen](https://github.com/jruribem10/proyecto1/assets/73508381/7e1ce35e-e627-41bc-a088-f2c45c4bc94d)

## Funcionamiento
* **1. Aplicación Cliente:**
Inicio: El cliente inicia una solicitud HTTP introduciendo la URL y el puerto de destino, usualmente a través de una interfaz de línea de comandos.
Generación de la Solicitud: La aplicación cliente crea una solicitud HTTP sin utilizar librerías externas, construyendo manualmente la solicitud según el método HTTP deseado (GET, HEAD, POST).
Envío de la Solicitud: La solicitud se envía al servidor Proxy + Balanceador de Carga a través de un socket TCP.
Recepción de Respuestas: Una vez que el proxy procesa y responde, el cliente recibe la respuesta HTTP y, dependiendo del tipo de contenido, lo almacena o lo muestra (por ejemplo, archivos HTML se pueden visualizar mientras que otros tipos de archivos como imágenes o PDF se almacenan).
* **2. Servidor HTTP Proxy + Balanceador de Carga:**
Recepción de Solicitudes: Este componente escucha en el puerto configurado (por defecto 8080) y acepta solicitudes entrantes de los clientes.
Distribución de Carga: Utiliza un algoritmo de Round Robin para distribuir las solicitudes entrantes equitativamente entre los servidores de aplicación web disponibles.
Modificación de Solicitudes: Modifica las solicitudes recibidas para ajustarlas a las necesidades de los servidores de aplicación web, incluyendo la alteración de encabezados y la ruta de la solicitud.
Comunicación con Servidores Web: Inicia un nuevo socket para enviar la solicitud modificada a uno de los servidores web seleccionados.
Recepción de Respuestas: Espera la respuesta del servidor web y la recibe a través del socket.
Envío de Respuestas al Cliente: Envía la respuesta recibida del servidor web de vuelta al cliente que hizo la solicitud original.
Manejo de Caché: Si la caché está habilitada y la respuesta ya existe en la caché y es válida, la enviará directamente al cliente sin contactar al servidor web, mejorando la eficiencia y reduciendo la latencia.

* **3. Servidores de Aplicación Web:**
Recepción de Solicitudes del Proxy: Cada servidor web recibe solicitudes del servidor proxy, las cuales han sido equitativamente distribuidas.
Procesamiento de Solicitudes: Cada servidor procesa la solicitud, generando una respuesta apropiada basada en los recursos solicitados (archivos HTML, CSS, imágenes, etc.).
Envío de Respuestas al Proxy: Después de procesar la solicitud, el servidor envía la respuesta de vuelta al servidor proxy para que este la redirija al cliente.


## Entorno de Desarrollo y Configuraciones
**Servidor y Cliente HTTP:**
- **Sistema Operativo**: Ubuntu 20.04 LTS o superior.
- **Compilador**: GCC versión 9.3.0 o superior para C.
- **Intérprete de Python**: Python 3.8 o superior para el cliente HTTP.
- **Librerías adicionales**: No se requieren librerías externas adicionales gracias a la utilización de la API de Berkeley para sockets en C y el módulo socket estándar en Python.

## Ejecución
**Pasos para la configuración del entorno local:**
1. **Instalar Ubuntu 20.04 LTS** en una máquina virtual o física.
2. **Instalar GCC**: Utilizar `sudo apt install build-essential` para instalar GCC y herramientas relacionadas.
3. **Instalar Python**: Ejecutar `sudo apt install python3.8`.
4. **Configurar direcciones IP estáticas** para el desarrollo local en `/etc/network/interfaces`.
5. **Clonar el repositorio del proyecto** desde GitHub utilizando `git clone [URL del repositorio]`.
6. **Compilar el servidor proxy** utilizando el comando `gcc -pthread -o httproxy httproxy.c`.
7. **Ejecutar pruebas unitarias** para asegurarse de que todas las componentes están funcionando correctamente.

**Despliegue en Producción:**
1. **Configurar una instancia de AWS EC2** con Ubuntu 20.04.
2. **Instalar las dependencias** como se describió en la sección de configuración local.
3. **Transferir los archivos del proyecto** al servidor utilizando SCP o similar.
4. **Configurar el balanceador de carga de AWS** (si aplicable) para distribuir las peticiones entre múltiples instancias.
5. **Configurar las reglas de firewall y grupos de seguridad** en AWS para permitir tráfico en el puerto 8080.
6. **Iniciar el servidor proxy** y el cliente para pruebas de conectividad y rendimiento.



## **Aspectos Logrados y No logrados**
* Desarrollo y Despliegue de los Tres Servidores de Aplicación Web:
Hemos desarrollado y desplegado exitosamente tres servidores de aplicación web en AWS, configurados para servir una página web estática replicada. Esto garantiza alta disponibilidad y redundancia, elementos críticos para el mantenimiento de un servicio web confiable.
* Implementación de Round Robin para el Balanceo de Carga:
Implementamos un balanceador de carga utilizando el algoritmo Round Robin, que distribuye de manera equitativa las solicitudes entrantes entre los tres servidores web. Esto mejora la distribución de la carga y optimiza el uso de los recursos.
* Implementación del Cliente en Python con Todos sus Requisitos:
El cliente HTTP fue implementado en Python, cumpliendo con todos los requisitos del proyecto, incluyendo el manejo de métodos GET, HEAD, y POST, y la capacidad de trabajar a través de un servidor proxy.
Manejo de Archivos de Registro:
* Hemos establecido un sistema efectivo de registro que documenta detalladamente todas las operaciones del servidor y del cliente. Los registros incluyen datos como las fechas, horas, solicitudes HTTP y respuestas, proporcionando una herramienta esencial para la monitorización y el análisis de la actividad del sistema.
* Implementación de Balanceador de Carga y Proxy HTTP:
Logramos implementar un servidor proxy que además actúa como balanceador de carga, utilizando un algoritmo Round Robin para distribuir las solicitudes entre tres servidores de aplicación diferentes. Esta funcionalidad es crucial para el manejo eficiente del tráfico y la disponibilidad del servicio.
* Concurrencia y Manejo de Múltiples Clientes:
El servidor puede manejar múltiples conexiones simultáneas gracias a la implementación de hilos, lo que permite procesar las solicitudes de manera concurrente sin bloquear a los usuarios mientras se manejan otras solicitudes.
* Caching de Respuestas:
Implementamos un sistema de caché que almacena respuestas a solicitudes previas, lo que reduce el tiempo de respuesta para solicitudes repetidas y disminuye la carga sobre los servidores de aplicación.
* Registro y Depuración:
Desarrollamos un sistema de registro que documenta todas las solicitudes y respuestas, proporcionando una herramienta vital para la depuración y el monitoreo del sistema.

**Aspecto no logrados**
Optimización de la Caché:
Aunque el sistema de caché funciona, no implementamos políticas avanzadas de expiración o invalidación de caché, ni manejamos adecuadamente los encabezados HTTP que podrían influir en las decisiones de caché, como Cache-Control o ETag.
Manejo de Errores y Recuperación:
La robustez del manejo de errores podría mejorarse. Aunque se manejan errores básicos de red y sistema, no hay una política exhaustiva para recuperarse de todos los tipos posibles de fallos en tiempo de ejecución.
## **Conclusiones**

## **Referencias**
