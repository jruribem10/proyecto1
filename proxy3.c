//Servidor HTTP Proxy + Balanceador de Carga
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <stdarg.h> // Para va_list, va_start, va_end


#define PORT_MIN 1024  // Puerto mínimo permitido
#define PORT_MAX 65535 // Puerto máximo permitido
#define LOG_PATH_MAX 256 // Longitud máxima para la ruta del archivo de log 
#define PORT 8080
#define BUFFER_SIZE 16384  
#define SERVER_COUNT 3
#define MAX_LOG_LENGTH 2048
#define LOG_FILE "request.log"
#define CACHE_DIRECTORY "/home/jruribem/proyecto_http_proxy"
#define TTL 300
// Definición de la variable global para el archivo de log
FILE *log_file = NULL;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

char *server_addresses[SERVER_COUNT] = {"54.84.15.182", "3.219.37.52", "54.88.135.62"}; //ruta de las tres aplicaciones web
//char *server_addresses[SERVER_COUNT] = {"127.0.0.1", "127.0.0.1", "127.0.0.1"};
int server_ports[SERVER_COUNT] = {8081, 8082, 8083};
int current_server = 0;
pthread_mutex_t round_robin_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int client_socket;
} thread_args_t;

typedef struct cache_entry {
    char *url;          // La URL completa del recurso
    char *file_path;    // Ruta al archivo en el disco que almacena los datos en caché
    time_t expiration;  // Tiempo de expiración del caché
} cache_entry_t;

/**
 * Convierte una URL en un nombre de archivo válido para almacenar en caché.
 * @param url La URL a convertir.
 * @param filename El nombre de archivo resultante.
 * @param max_length La longitud máxima del nombre de archivo.
 * @return No retorna un valor directo, pero modifica el parámetro `filename`.
 */
void url_to_filename(const char *url, char *filename, size_t max_length) {
    // Iterar sobre cada carácter de la URL
    for (size_t i = 0; url[i] != '\0' && i < max_length - 1; ++i) {
        // Reemplazar caracteres no permitidos con un guión bajo
        if (url[i] == '/' || url[i] == '\\' || url[i] == ':' || url[i] == '?' || url[i] == '&' || url[i] == '=') {
            if (i < max_length - 1) {
                filename[i] = '_';
            } else {
                break; // Superó el límite de tamaño, salir del bucle
            }
        } else {
            filename[i] = url[i];
        }
    }
    // Añadir el terminador nulo al final del nombre del archivo
    filename[max_length - 1] = '\0';
}
/**
 * Abre el archivo de log especificado en modo de anexar, asegurando el acceso exclusivo mediante mutex.
 * @param log_path La ruta del archivo de log.
 */
void open_log_file(const char *log_path) {
    pthread_mutex_lock(&log_mutex); // Bloquear el acceso al archivo de log
    log_file = fopen(log_path, "a");
    if (log_file == NULL) {
        perror("Failed to open log file");
    }
    pthread_mutex_unlock(&log_mutex); // Desbloquear el acceso al archivo de log
}

// Función para cerrar el archivo de log
void close_log_file() {
    pthread_mutex_lock(&log_mutex); // Bloquear el acceso al archivo de log
    if (log_file != NULL) {
        fclose(log_file);
        log_file = NULL;
    }
    pthread_mutex_unlock(&log_mutex); // Desbloquear el acceso al archivo de log

}
// Función para registrar un mensaje en el archivo de log y en la salida estándar
void log_message(const char *format, ...) {
    pthread_mutex_lock(&log_mutex); // Bloquear el acceso al archivo de log

    // Inicializar el argumento de lista variable
    va_list args;
    va_start(args, format);

    // Registrar el mensaje en la salida estándar
    vprintf(format, args);

    // Registrar el mensaje en el archivo de log si está abierto
    if (log_file != NULL) {
        vfprintf(log_file, format, args);
        fflush(log_file); // Forzar la escritura en el archivo de log
    }

    // Finalizar el uso del argumento de lista variable
    va_end(args);

    pthread_mutex_unlock(&log_mutex); // Desbloquear el acceso al archivo de log
}

/**
 * Almacena una respuesta HTTP en caché en disco.
 * @param url La URL de la respuesta.
 * @param response La respuesta HTTP.
 * @param response_length La longitud de la respuesta.
 */
void store_response_in_cache(const char *url, const char *response, ssize_t response_length) {
    char filename[256];
    url_to_filename(url, filename, sizeof(filename));

    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/%s", CACHE_DIRECTORY, filename);

    FILE *file = fopen(filepath, "wb");
    if (file == NULL) {
        perror("Failed to open cache file for writing");
        return;
    }

    // Truncar el archivo si ya existe para eliminar su contenido anterior
    if (ftruncate(fileno(file), 0) < 0) {
        perror("Failed to truncate cache file");
        fclose(file);
        return;
    }

    fwrite(response, 1, response_length, file);
    fclose(file);
}

/**
 * Envía una respuesta HTTP almacenada en caché al cliente.
 * 
 * @param url La URL de la respuesta en caché.
 * @param client_socket El socket del cliente.
 * @return 1 si se encontró y envió la respuesta en caché, 0 en caso contrario.
 */

int send_cached_response_to_client(const char *url, int client_socket) {
    char filename[256];
    url_to_filename(url, filename, sizeof(filename));

    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/%s", CACHE_DIRECTORY, filename);

    FILE *file = fopen(filepath, "rb");
    if (file == NULL) {
        return 0; // No se encontró la respuesta en caché
    }

    // Leer la respuesta desde el archivo y enviarla al cliente
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        send(client_socket, buffer, bytes_read, 0);
    }

    fclose(file);
    return 1; // Se encontró y envió la respuesta en caché
}

/**
 * Registra una solicitud y su respuesta en la salida estándar y en el archivo de log.
 * @param server_address La dirección del servidor donde se envió la solicitud.
 * @param log_msg Mensaje contextual para el registro.
 * @param request La solicitud HTTP enviada.
 * @param response La respuesta HTTP recibida.
 */
void log_request_response(const char *server_address, const char *log_msg, const char *request, const char *response) {
    time_t now = time(NULL);
    char *time_str = ctime(&now);
    time_str[strlen(time_str) - 1] = '\0'; // Eliminar el salto de línea al final
    
    // Formatear y registrar el mensaje en el archivo de log y la salida estándar
    log_message("%s - Servidor: %s - %s\n", time_str, server_address, log_msg);
    log_message("Request:\n%s\n", request);
    log_message("Response:\n%s\n", response);

    FILE *log_file = fopen(LOG_FILE, "a");
    if (log_file != NULL) {
        fprintf(log_file, "%s - Servidor: %s - %s\n", time_str, server_address, log_msg);
        fprintf(log_file, "Request:\n%s\n", request);
        fprintf(log_file, "Response:\n%s\n", response);
        fclose(log_file);
    }
    printf("%s - Servidor: %s - %s\n", time_str, server_address, log_msg);
}

/**
 * Envía una respuesta de error HTTP al cliente y registra el evento.
 * @param client_socket El socket del cliente a quien enviar la respuesta.
 * @param status_code El código de estado HTTP para la respuesta.
 * @param reason_phrase La frase de estado que acompaña al código de estado.
 */
void send_error_response(int client_socket, const char *status_code, const char *reason_phrase) {
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), 
             "HTTP/1.1 %s %s\r\n"
             "Content-Type: text/html\r\n"
             "Connection: close\r\n"
             "\r\n"
             "<html><head><title>%s %s</title></head><body><h1>%s %s</h1></body></html>\r\n",
             status_code, reason_phrase, status_code, reason_phrase, status_code, reason_phrase);

    send(client_socket, buffer, strlen(buffer), 0);
    log_request_response("N/A", "Error response sent to client", "", "");

}

/**
 * Modifica una solicitud HTTP para redirigirla a un servidor específico. Esto incluye cambiar la URL y posiblemente otros encabezados.
 * @param request La solicitud HTTP original.
 * @param req_length La longitud de la solicitud original.
 */
void modify_http_request(char *request, ssize_t req_length) {
    char method[10];  // GET, HEAD, etc.
    char url[2048];   // La URL completa será reemplazada
    char http_version[10]; // HTTP/1.1
    char host[1024];
    char path[2048] = "/test/test.html";  // Path destino de las aplicaciones

    // Intentar extraer el método, la URL, y la versión de HTTP de la solicitud
    if (sscanf(request, "%s %s %s", method, url, http_version) < 3) {
        // No se pudo analizar correctamente la solicitud
        perror("Error al analizar la solicitud HTTP");
        return;
    }

    // Asumir que cualquier solicitud GET/HEAD debe redirigirse a la URL específica
    // Agregar el encabezado Via que identifica este servidor como un proxy
    char via_header[256];
    snprintf(via_header, sizeof(via_header), "Via: 1.1 %s\r\n", "44.219.246.217");
    //snprintf(via_header, sizeof(via_header), "Via: 1.1 %s\r\n", "127.0.0.4");
    // Construye la nueva solicitud apuntando a la URL deseada.
    int new_request_length = snprintf(request, req_length, "%s http://%s%s %s\r\nHost: %s\r\n%sConnection: close\r\n\r\n", 
         method, host, path, http_version, host, via_header);

    // Truncar la solicitud si excede el tamaño del buffer
    if (new_request_length >= req_length) {
        request[req_length - 1] = '\0'; // Asegurar que la solicitud esté terminada correctamente
    }
}

/**
 * Busca en el caché una entrada que coincida con la URL especificada y verifica su validez.
 * @param url La URL del recurso solicitado.
 * @return Retorna un puntero a una estructura `cache_entry_t` si se encuentra una coincidencia válida, de lo contrario retorna NULL.
 */
cache_entry_t *cache_lookup(const char *url) {
    char filename[256], file_path[1024];
    struct stat statbuf;

    url_to_filename(url, filename, sizeof(filename));
    snprintf(file_path, sizeof(file_path), "%s/%s", CACHE_DIRECTORY, filename);

    if (stat(file_path, &statbuf) == 0) {
        // Verifica si el caché aún es válido basándose en el TTL.
        if (time(NULL) - statbuf.st_mtime < TTL) {
            cache_entry_t *entry = (cache_entry_t *)malloc(sizeof(cache_entry_t));
            if (entry) {
                entry->url = strdup(url);
                entry->file_path = strdup(file_path);
                entry->expiration = statbuf.st_mtime + TTL;
                return entry;
            }
        }
    }
    return NULL; // No se encontró una entrada válida en caché o falló la asignación de memoria.
}

/**
 * Maneja la solicitud del cliente, redirigiéndola a los servidores de backend utilizando el algoritmo de balanceo de carga Round Robin.
 * Este proceso incluye la modificación de la solicitud, verificación y utilización del caché, y la comunicación con el servidor backend.
 * @param args Los argumentos pasados al hilo, que incluyen el socket del cliente.
 * @return Retorna NULL siempre, pero procesa la solicitud y genera respuestas adecuadas.
 */

void *handle_client_request(void *args) {
    if (args == NULL) {
        perror("Thread data is NULL");
        return NULL;
    }
    thread_args_t *thread_data = (thread_args_t *)args;
    int client_socket = thread_data->client_socket;
    free(thread_data);

    char buffer[BUFFER_SIZE] = {0};
    ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (bytes_received < 1) {
        close(client_socket);
        return NULL;
    }

    // Loguear la solicitud recibida del cliente
    log_request_response("","Solicitud recibida del cliente", "", "");

    char modified_request[BUFFER_SIZE];
    int request_length = snprintf(modified_request, BUFFER_SIZE, "GET /newpath HTTP/1.1\r\nHost: backend-host\r\n%s", "Connection: close\r\n\r\n");
    if (request_length >= BUFFER_SIZE) {
        // Handle error: Request too large for the buffer
        fprintf(stderr, "Error: Request exceeds buffer size.\n");
        close(client_socket);
        return NULL;
    }

    // Identificar el método HTTP (GET o HEAD)
    char method[5]; // Suficiente para almacenar "GET" o "HEAD"
    sscanf(buffer, "%s", method);

    if (strcmp(method, "GET") == 0 || strcmp(method, "HEAD") == 0) {
        // Modificar la solicitud HTTP para ambos métodos GET y HEAD
        modify_http_request(buffer, bytes_received);

        // Verificar si la respuesta está en caché y es válida según el TTL
        char url[2048]; // Suficiente para almacenar una URL
        sscanf(buffer, "%*s %s %*s", url); // Extraer la URL de la solicitud (simplificado)
        cache_entry_t *cached_response = cache_lookup(url);
        if (cached_response && time(NULL) < cached_response->expiration) {
            // Si está en caché y es válido según el TTL, enviar la respuesta al cliente desde el caché
            if (send_cached_response_to_client(url, client_socket)) {
                log_request_response("","Respuesta enviada al cliente desde caché", "", "");
                close(client_socket);
                free(cached_response->url);
                free(cached_response->file_path);
                free(cached_response);
                return NULL; // Finalizar la ejecución, ya que la respuesta se ha servido desde el caché
            }
        }

        // Continuar con el procesamiento normal...
        // Elegir el servidor para reenviar la solicitud usando el balanceo de carga Round Robin
        static int current_server = 0;
        int server_index = current_server;
        current_server = (current_server + 1) % SERVER_COUNT;

        // Log para indicar en qué servidor se está basado en el índice de Round Robin
        printf("Redirigiendo solicitud al servidor %d: %s\n", server_index + 1, server_addresses[server_index]);
    
        // Establecer una conexión con el servidor de backend seleccionado
        int server_socket = socket(AF_INET, SOCK_STREAM, 0);

        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_ports[server_index]);
        server_addr.sin_addr.s_addr = inet_addr(server_addresses[server_index]);

        if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("Error al conectar con el servidor de backend");
            close(client_socket);
            return NULL;
        }

        // Reenviar la solicitud modificada al servidor de backend
        send(server_socket, buffer, bytes_received, 0);

        // Recibir la respuesta del servidor de backend y enviarla al cliente
        // Para el método HEAD, no enviamos el cuerpo de la respuesta
        while ((bytes_received = recv(server_socket, buffer, BUFFER_SIZE, 0)) > 0) {
            if (strcmp(method, "HEAD") != 0) { // Si no es HEAD, enviar todo
                send(client_socket, buffer, bytes_received, 0);
            } else {
                // Si es HEAD, enviar solo los encabezados (hasta la primera línea vacía)
                char *end_of_headers = strstr(buffer, "\r\n\r\n");
                if (end_of_headers) {
                    send(client_socket, buffer, end_of_headers - buffer + 4, 0); // +4 para incluir "\r\n\r\n"
                    break; // Detener después de enviar los encabezados
                }
            }
        }

        // Loguear la respuesta enviada de vuelta al cliente
        log_request_response("","Respuesta enviada al cliente", "", "");
        // Almacenar la respuesta en caché si corresponde
        if (strcmp(method, "GET") == 0) { // Solo almacenar respuestas GET en caché
            store_response_in_cache(url, buffer, bytes_received);
        }

        close(server_socket);
    } else {
        // Si el método no es GET o HEAD, devolver un error 501 Not Implemented
        char *errMsg = "HTTP/1.1 501 Not Implemented\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
        send(client_socket, errMsg, strlen(errMsg), 0);
        log_request_response("","Respuesta de error 501 Not Implemented enviada al cliente", "", "");
        close(client_socket);
    }

    return NULL;
}




int main(int argc, char *argv[]) {
    // Verificar la cantidad de argumentos
    if (argc != 3) {
        printf("Uso: %s <puerto> <ruta del archivo de log>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Obtener el puerto y la ruta del archivo de log de los argumentos
    int port = atoi(argv[1]);
    char *log_path = argv[2];

    // Validar el puerto
    if (port < PORT_MIN || port > PORT_MAX) {
        printf("Error: El puerto debe estar en el rango %d-%d.\n", PORT_MIN, PORT_MAX);
        exit(EXIT_FAILURE);
    }

    // Validar la longitud de la ruta del archivo de log
    if (strlen(log_path) > LOG_PATH_MAX) {
        printf("Error: La longitud de la ruta del archivo de log supera el límite permitido.\n");
        exit(EXIT_FAILURE);
    }

    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    
    inet_pton(AF_INET, "44.219.246.217", &address.sin_addr);
    //inet_pton(AF_INET, "127.0.0.4", &address.sin_addr);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listening failed");
        exit(EXIT_FAILURE);
    }

    printf("Proxy server running on port %d\n", port);

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("Acceptance failed");
            exit(EXIT_FAILURE);
        }

        pthread_t thread;
        thread_args_t *thread_data = malloc(sizeof(thread_args_t));
        if (!thread_data) {
            perror("Failed to allocate thread data");
            close(new_socket);
            continue;
        }
        thread_data->client_socket = new_socket;

        if (pthread_create(&thread, NULL, handle_client_request, (void *)thread_data) != 0) {
            perror("Failed to create thread");
            free(thread_data);
            close(new_socket);
        }
        pthread_detach(thread); // Liberar recursos automáticamente cuando el hilo termina
    }

    return 0;
}