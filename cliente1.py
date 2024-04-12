#Cliente HTTP


import socket
import sys
import time
import os
import re

PROXY_HOST = '44.219.246.217'
#PROXY_HOST = '127.0.0.4'
PROXY_PORT = 8080
CACHE_DIRECTORY = "/home/jruribem/proyecto_http_proxy/"



def log_request_response(log_file_path, request, response):
    """
    Registra cada solicitud y respuesta HTTP en un archivo de log especificado.
    Parámetros:
        log_file_path (str): Ruta al archivo de log donde se registrarán las entradas.
        request (str): La solicitud HTTP enviada.
        response (str): La respuesta HTTP recibida; solo los primeros 100 caracteres se registran si no es un método HEAD.
    Retorna:
        None. Escribe el registro en un archivo y muestra el mensaje de log en la consola.
    """
    with open(log_file_path, "a") as log_file:
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        log_msg = f"{timestamp} - Request: {request} - Response: {response[:100]}"
        log_file.write(log_msg + "\n")
    print(log_msg)

def send_http_request(method, url, log_path, data=None):
    """
    Envía una solicitud HTTP al servidor usando un proxy. Soporta los métodos GET, POST y HEAD.
    Parámetros:
        method (str): Método HTTP (GET, POST, HEAD).
        url (str): URL completa a la que se envía la solicitud.
        log_path (str): Ruta al archivo de log donde se registrarán las solicitudes y respuestas.
        data (str, optional): Datos para enviar en el cuerpo de la solicitud para métodos POST.
    Retorna:
        str: La respuesta del servidor o cabeceras, dependiendo del método HTTP utilizado. Retorna None en caso de error.
    """
    parsed_url = re.match(r'http[s]?://([^/]+)(/.*)?', url)
    if not parsed_url:
        print("URL format error.")
        return

    host, path = parsed_url.groups()
    path = path if path else '/'
    request = f"{method} {path} HTTP/1.1\r\nHost: {host}\r\nConnection: close\r\n\r\n"
    if method == 'POST' and data:
        request = f"{method} {path} HTTP/1.1\r\nHost: {host}\r\nContent-Length: {len(data)}\r\n\r\n{data}"

    response_headers = ''
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((PROXY_HOST, PROXY_PORT))
            s.sendall(request.encode())

            if method == 'HEAD':
                while True:
                    part = s.recv(1024)
                    response_headers += part.decode()
                    if "\r\n\r\n" in response_headers:  # Fin de las cabeceras
                        break
            else:
                response = b""
                while True:
                    part = s.recv(1024)
                    if not part:
                        break
                    response += part
                response_headers = response.decode()

    except Exception as e:
        print(f"Error sending HTTP request: {e}")
        return None

    if method == 'HEAD':
        log_request_response(log_path, request, response_headers)
        return response_headers
    else:
        log_request_response(log_path, request, response_headers[:100])
        return response_headers

def cache_response(url, response):
    """
    Guarda la respuesta de una solicitud en un archivo de caché basado en un hash de la URL.
    Parámetros:
        url (str): URL de la solicitud.
        response (str): Respuesta del servidor a guardar en caché.
    Retorna:
        None. Escribe la respuesta en un archivo de caché para su uso futuro.
    """
    hashed_url = hash(url)
    file_path = os.path.join(CACHE_DIRECTORY, f"{hashed_url}.cache")
    with open(file_path, "w") as file:
        file.write(response)

def read_cached_response(url):
    """
    Lee la respuesta guardada en caché para una URL específica, si existe.
    Parámetros:
        url (str): URL de la solicitud cuya respuesta se busca en el caché.
    Retorna:
        str: La respuesta almacenada en caché, o None si no se encuentra en el caché.
    """
    hashed_url = hash(url)
    file_path = os.path.join(CACHE_DIRECTORY, f"{hashed_url}.cache")
    if os.path.exists(file_path):
        with open(file_path, "r") as file:
            return file.read()
    return None

def is_resource_modified(url, response):
    """
    Comprueba si el recurso solicitado ha sido modificado comparando la nueva respuesta con la almacenada en caché.
    Parámetros:
        url (str): URL del recurso solicitado.
        response (str): Respuesta reciente del servidor para comparar con la caché.
    Retorna:
        bool: True si el recurso ha sido modificado, False de lo contrario.
    """
    cached_response = read_cached_response(url)
    if cached_response:
        return cached_response != response
    return False

def flush_cache():
    """
    Elimina todos los archivos en el directorio de caché.
    Retorna:
        None. Borra todos los archivos de caché y muestra un mensaje de confirmación.
    """
    for filename in os.listdir(CACHE_DIRECTORY):
        file_path = os.path.join(CACHE_DIRECTORY, filename)
        try:
            if os.path.isfile(file_path) or os.path.islink(file_path):
                os.unlink(file_path)
        except Exception as e:
            print(f"Failed to delete {file_path}. Reason: {e}")

    print("Cache flushed.")

def main():
    """
    Función principal que procesa los argumentos de la línea de comandos para determinar la acción HTTP a realizar.
    Retorna:
        None. Dependiendo de los argumentos, puede cachear respuestas, servir respuestas de caché, enviar solicitudes o limpiar el caché.
    """
    if len(sys.argv) not in [4, 5]:
        print("Usage: python3 cliente1.py /path/to/request.log <METHOD> <URL> [data]")
        return

    log_path, method, url = sys.argv[1:4]
    data = sys.argv[4] if len(sys.argv) == 5 else None

    if method.lower() == "flush":
        flush_cache()
        return

    if method.upper() not in ["GET", "HEAD", "POST"]:
        print("Invalid HTTP method.")
        return

    cached_response = read_cached_response(url)
    if cached_response and method.upper() != "POST":
        print("Serving from cache.")
        print(cached_response)
        return

    response = send_http_request(method.upper(), url, log_path, data)
    if response:
        if method.upper() == "GET":
            cache_response(url, response)
        print(response)

if __name__ == "__main__":
    main()


