/*********************************************************************
* Este ser� el cliente que se encargara de hacer todos los procesos  *
* y mandarlos al servidor, tambi�n se puede decir que aqu� se puede  *
* llevar el sistema que se quiere como un juego, aplicaci�n, etc.    *
*********************************************************************/

#include <iostream>
#include <cap_net.h> // Agregamos la librer�a est�tica NetCommon con sus headers.

// Esta es una clase enumerada que se encargara de ser el tipo de mensajes que procesara la librer�a est�tica CapNet
enum class CustomMsgTypes : uint32_t {
	// Tiene 5 miembros enumerados de tipo entero de 32 bits sin asignar.
	// Estos ser�n los mensajes que se podr�n manejar al utilizarse con el CapNet.
	// Tambi�n estos nos serviran para indicarle al sistema del cliente que hacer.
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

	// Funci�n que nos permitir� pingear al servidor.
	void PingServer() {
		// Creamos la variable de mensaje que aceptara el tipo de instrucci�n creada en la clase
		// enumerada de CustomMsgTypes.
		cap::net::message<CustomMsgTypes> msg;

		// Le asignamos el tipo de id con instrucci�n que queremos.
		msg.header.id = CustomMsgTypes::ServerPing;

		// Para medir el tiempo de respuesta, tenemos que medir el tiempo y he aqu� el problema.
		// Necesitamos usar la librer�a chrono para esto ya que esto tiene unos m�todos y clases necesarios,
		// lo que pasa es que esto es un poco "sucio" ya que normalmente se utiliza otros m�todos para medir
		// ese tipo de cosas. A�n as� hay que tener mucho cuidado con esto.

		// Creamos la variable que medir� el punto del tiempo en el momento de la ejecuci�n
		std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();

		// Ahora lo agregamos al mensaje.
		msg << timeNow;

		// Y lo enviamos.
		Send(msg);
	}

	// Funci�n que permitira mandar un mensaje a todos los clientes.
	void MessageAll() {
		// Creamos la variable de mensaje que aceptara el tipo de instrucci�n creada en la clase
		// enumerada de CustomMsgTypes.
		cap::net::message<CustomMsgTypes> msg;

		// Le ponemos de Id al mensaje la instrucci�n de mensaje a todos.
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
			// Y aqu� estableceremos si las teclas 1, 2 y 3 han sido activadas.
			key[0] = GetAsyncKeyState((unsigned short)'1') & 0x8000;
			key[1] = GetAsyncKeyState((unsigned short)'2') & 0x8000;
			key[2] = GetAsyncKeyState((unsigned short)'3') & 0x8000;
		}

		// Verificamos que la tecla fue presionada y si si, esta ejecutara algo especial.
		if (key[0] && !old_key[0]) { client.PingServer(); } // Pingearemos el servidor
		if (key[1] && !old_key[1]) { client.MessageAll(); }
		if (key[2] && !old_key[2]) { bQuit = true; } // Cancelaremos la conexi�n.

		// Y ahora ya que se quedo grabado la tecla antes presionada, ahora la ponemos como antigua, para as� generar el evento
		// Al presionar, Mantenida y dejar de presionar.
		for (int i = 0; i < 3; i++) {
			old_key[i] = key[i];
		}

		// Verificamos que el cliente este conectado.
		if (client.IsConnected()) {
			// Verificamos que la cola de mensajes no este vac�a.
			if (!client.Incoming().empty()) {

				// Si no esta vac�a entonces comenzaremos tomando mensajes.
				auto msg = client.Incoming().pop_front().msg;

				// Ahora verificaremos que tipo de mensaje fue el tomado y haremos un caso dependiente de este.
				switch (msg.header.id) {

					// Caso en el que el server haya aceptado la conexi�n.
					case CustomMsgTypes::ServerAccept: {
						// Indicamos que el servidor acepto nuestra conexi�n.
						printf("El servidor ha aceptado la conexi�n.\n");
					}
					break;

					// Caso en el que se pidio el ping.
					case CustomMsgTypes::ServerPing: {
						// Creamos de nuevo el tiempo actual con la librer�a chrono y a una variable le llamamos de en eso "then"
						// que tomara el valor del mensaje.
						std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
						std::chrono::system_clock::time_point timeThen;

						// Tomamos el mensaje enviado que guardaba el tiempo en el momento de hacer la petici�n y lo guardamos.
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
						printf("[%u] �Te ha mandado saludos!.\n", clientID);
					}
					break;

				}
			}
		}
		else {
			// Si no esta conectado, entonces significa que el servidor cerro la conexi�n, as� que informamos y salimos del while.
			printf("Servidor cerrado\n");
			bQuit = true;
		}
	}

	return 0;
}