#include <stdio.h>
#include <iostream>
#include <chrono>

//* Agregando y definiendo librerías y parámetros para usar la librería asio. *//
#define ASIO_STANDALONE

#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif

#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
//*****************************************************************************//

/* Utilizando la libreria de tiempo literarios de chrono para medir los tiempos */
using namespace std::chrono_literals;

/************************************************************************************************************
	Los vectores son arreglos o listas dinámicas como las de c#, en este caso se usara un vector de tipo char
	para poder guardar los bytes para después leerlos desde el socket.
	También nos servirá para poder almacenar todos los bytes posibles e imprimirlo directamente a la consola.
	(Es necesario hacerlo así debido a que si queremos esperar a recibir una respuesta, necesitamos un buffer que actúa asincrónicamente)
************************************************************************************************************/
std::vector<char> vBuffer(1 * 1024);

// Esta función será una función sincrónica debido a que esperara a leer la mayor posible de respuesta a tomar del socket a referenciar.
// La función tendrá recursamiento para hacerla perfectamente sincrónica y parar hasta que ya no haya data a leer.
void vdGrabSomeData(asio::ip::tcp::socket& socket) {
	// Se usara una función lambda para poder escribir directamente en el buffer.
	socket.async_read_some(asio::buffer(vBuffer.data(), vBuffer.size()), [&](std::error_code ec, std::size_t length) {
		if (!ec) {
			// Imprimiremos los bytes leídos cada que se repita esta sección.
			printf("\n\nLeyendo %zu bytes\n\n", length);

			// Imprimiremos los bytes almacenados
			for (int i = 0; i < length; i++) {
				printf("%c", vBuffer[i]);
			}

			vdGrabSomeData(socket);
		}
		});
}

int main() {

	// Variable que manejara los errores que llegue arrojar el asio al momento de la ejecución.
	asio::error_code ec;

	// Creando un "contexto" que esencialmente la plataforma especificara la interfaz.
	asio::io_context context;

	// Le damos trabajo falso al contexto para poder ejecutarlo y evitar que interrumpa procesos y esto servira para evitar que finalice.
	asio::io_context::work idleWork(context);

	// Empezamos a correr el contexto para que pueda ser pausado sin que interrumpa procesos principales.
	std::thread thrContext = std::thread([&]() { context.run(); });

	// Consiguiendo la dirección de algún lugar del que queramos conectarnos.
		// Se agrega como segundo parámetro la variable que manejara el error que llegue a recibir la función. 
	asio::ip::tcp::endpoint endpoint(asio::ip::make_address("192.168.0.13", ec), 80);

	// Se crea el socket para llevar a cabo la conexión con el endpoint, el contexto se encargara de entregar las implementaciones.
	asio::ip::tcp::socket socket(context);

	// Se le da la instrucción al socket de "intentar y conectar" a la dirección dada.
	socket.connect(endpoint, ec);

	if (!ec) {
		printf("Conectado\n");
	}
	else {
		std::cout << "Fallo al intentar conectarse a la direccion:\n" << ec.message() << std::endl;
	}

	// Se crea esto para revisar si la dirección es una página ya que si lo es, el servidor esperara una solicitud (HTTP).
	if (socket.is_open()) {

		//Esta función se llama prematuramente para posteriormente darle un delay de tiempo largo para que pueda manejar otros datos.
			// Se llama a esta función para poder leer correctamente los datos recibidos de una forma asíncronomicra.
		vdGrabSomeData(socket);

		std::string sRequest =
			"GET /index.html HTTP/1.1\r\n"
			"Host: example.com\r\n"
			"Connection: close\r\n\r\n";

		// Esta función se encargara de escribir la mayoría de veces la información dada.
			// Los buffers son arreglos que contienen bytes y se le entrega 2 datos, los bytes(información) y el tamaño de ese arreglo.
		socket.write_some(asio::buffer(sRequest.data(), sRequest.size()), ec);

		// Se le indica a este proceso que descanse, mientras asio pueda manejar las transferencias de datos en el fondo.
		std::this_thread::sleep_for(2000ms);
	}

	system("pause");
	return 0;
}