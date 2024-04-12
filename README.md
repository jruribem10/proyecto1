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
## Arquitectura
La arquitectura del proyecto consta de los siguientes componentes:

* Aplicación Cliente: Envía solicitudes HTTP y recibe respuestas.
* HTTP Proxy + Balanceador de Carga: Actúa como intermediario entre el cliente y los servidores, distribuyendo las solicitudes entre varios servidores backend.
* Servidores de Aplicación Web: Un conjunto de tres servidores donde cada uno ejecuta la misma aplicación web replicada. Se puede usar NGINX o Apache como servidor web.
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

## Instalación de Librerías

## Ejecución
