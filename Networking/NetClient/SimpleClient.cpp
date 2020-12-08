/*********************************************************************
* Este será el cliente que se encargara de hacer todos los procesos  *
* y mandarlos al servidor, también se puede decir que aquí se puede  *
* llevar el sistema que se quiere como un juego, aplicación, etc.    *
*********************************************************************/

#include <iostream>
#include <cap_net.h> // Agregamos la librería estática NetCommon con sus headers.

// Esta es una clase enumerada que se encargara de ser el tipo de mensajes que procesara la librería estática CapNet
enum class CustomMsgTypes : uint32_t {
	// Tiene 5 miembros enumerados de tipo entero de 32 bits sin asignar.
	// Estos serán los mensajes que se podrán manejar al utilizarse con el CapNet.
	// También estos nos serviran para indicarle al sistema del cliente que hacer.
	ServerAccept,
	ServerDeny,
	ServerPing,
	MessageAll,
	ServerMessage,
};

// Ejemplo de como se armaria el juego.

class CustomClient : public cap::net::client_interface<CustomMsgTypes> {
private:

protected:

public:

	// Función que nos permitirá pingear al servidor.
	void PingServer() {
		// Creamos la variable de mensaje que aceptara el tipo de instrucción creada en la clase
		// enumerada de CustomMsgTypes.
		cap::net::message<CustomMsgTypes> msg;

		// Le asignamos el tipo de id con instrucción que queremos.
		msg.header.id = CustomMsgTypes::ServerPing;

		// Para medir el tiempo de respuesta, tenemos que medir el tiempo y he aquí el problema.
		// Necesitamos usar la librería chrono para esto ya que esto tiene unos métodos y clases necesarios,
		// lo que pasa es que esto es un poco "sucio" ya que normalmente se utiliza otros métodos para medir
		// ese tipo de cosas. Aún así hay que tener mucho cuidado con esto.

		// Creamos la variable que medirá el punto del tiempo en el momento de la ejecución
		std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();

		// Ahora lo agregamos al mensaje.
		msg << timeNow;

		// Y lo enviamos.
		Send(msg);
	}

	// Función que permitira mandar un mensaje a todos los clientes.
	void MessageAll() {
		// Creamos la variable de mensaje que aceptara el tipo de instrucción creada en la clase
		// enumerada de CustomMsgTypes.
		cap::net::message<CustomMsgTypes> msg;

		// Le ponemos de Id al mensaje la instrucción de mensaje a todos.
		msg.header.id = CustomMsgTypes::MessageAll;

		// Lo mandamos.
		Send(msg);

	}

};

int main() {
	// Creamos el cliente y lo conectamos a la ip.
	CustomClient client;
	client.Connect("127.0.0.1", 60000);

	// Creamos un arreglo booleano que marcara cuando una tecla especial es presionada.
	bool key[3] = { false, false, false };
	bool old_key[3] = { false, false, false };

	// Para evitar que se cierre directamente crearemos la posibilidad de que el cliente quiera desconectarse.
	bool bQuit = false;

	while (!bQuit) {

		// Verificamos que la ventana es la principal (activa).
		if (GetForegroundWindow() == GetConsoleWindow()) {
			// Y aquí estableceremos si las teclas 1, 2 y 3 han sido activadas.
			key[0] = GetAsyncKeyState((unsigned short)'1') & 0x8000;
			key[1] = GetAsyncKeyState((unsigned short)'2') & 0x8000;
			key[2] = GetAsyncKeyState((unsigned short)'3') & 0x8000;
		}

		// Verificamos que la tecla fue presionada y si si, esta ejecutara algo especial.
		if (key[0] && !old_key[0]) { client.PingServer(); } // Pingearemos el servidor
		if (key[1] && !old_key[1]) { client.MessageAll(); }
		if (key[2] && !old_key[2]) { bQuit = true; } // Cancelaremos la conexión.

		// Y ahora ya que se quedo grabado la tecla antes presionada, ahora la ponemos como antigua, para así generar el evento
		// Al presionar, Mantenida y dejar de presionar.
		for (int i = 0; i < 3; i++) {
			old_key[i] = key[i];
		}

		// Verificamos que el cliente este conectado.
		if (client.IsConnected()) {
			// Verificamos que la cola de mensajes no este vacía.
			if (!client.Incoming().empty()) {

				// Si no esta vacía entonces comenzaremos tomando mensajes.
				auto msg = client.Incoming().pop_front().msg;

				// Ahora verificaremos que tipo de mensaje fue el tomado y haremos un caso dependiente de este.
				switch (msg.header.id) {

					// Caso en el que el server haya aceptado la conexión.
					case CustomMsgTypes::ServerAccept: {
						// Indicamos que el servidor acepto nuestra conexión.
						printf("El servidor ha aceptado la conexión.\n");
					}
					break;

					// Caso en el que se pidio el ping.
					case CustomMsgTypes::ServerPing: {
						// Creamos de nuevo el tiempo actual con la librería chrono y a una variable le llamamos de en eso "then"
						// que tomara el valor del mensaje.
						std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
						std::chrono::system_clock::time_point timeThen;

						// Tomamos el mensaje enviado que guardaba el tiempo en el momento de hacer la petición y lo guardamos.
						msg >> timeThen;

						// He imprimimos el resultado
						printf("Ping: %lf\n", std::chrono::duration<double>(timeNow - timeThen).count());
					}
					break;

					// Caso en el que el mensaje sea de tipo todos.
					case CustomMsgTypes::MessageAll: {
						// Creamos la variable de la ID del cliente.
						uint32_t clientID;
						msg >> clientID;

						// He imprimimos el resultado con un formato bonito.
						printf("[%u] ¡Te ha mandado saludos!.\n", clientID);
					}
					break;

				}
			}
		}
		else {
			// Si no esta conectado, entonces significa que el servidor cerro la conexión, así que informamos y salimos del while.
			printf("Servidor cerrado\n");
			bQuit = true;
		}
	}

	return 0;
}