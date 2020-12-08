// Agregamos las librerías necesarias.
#include <iostream>
#include <stdio.h>
#include <cap_net.h>

// Creamos el tipo de mensaje enumerado que el servidor podra leer.
enum class CustomMsgTypes : uint32_t {
	ServerAccept,
	ServerDeny,
	ServerPing,
	MessageAll,
	ServerMessage,
};

// Crearemos la subclase que usara el servidor con la interfaz del servidor y con el tipo de mensajes a manejar.
// También esta clase es donde implementaremos el resultado o el proceso de los eventos que hay en la interfaz. 
class CustomServer : public cap::net::server_interface<CustomMsgTypes> {
private:

protected:
	// Sobrescribimos el evento cuando el cliente se conecta al indicado.

		// Evento el cual ejecutara un proceso antes de la aceptación del cliente.
	virtual bool OnClientConnect(std::shared_ptr<cap::net::connection<CustomMsgTypes>> client) {
		// Crearemos un mensaje el cual mandaremos al cliente para indicarle que fue aceptado.
		cap::net::message<CustomMsgTypes> msg;
		msg.header.id = CustomMsgTypes::ServerAccept;
		client->Send(msg);

		return true;
	}

	// Sobrescribimos el evento cuando el cliente aparenta haberse desconectado.

		// En este evento nos ayudara a notificar que un cliente se desconecto con su ID respectivo.
	virtual void OnClientDisconnect(std::shared_ptr<cap::net::connection<CustomMsgTypes>> client) {
		printf("Eliminando cliente [%u]\n", client->GetID());
	}

	// Sobrescribimos el evento cuando un mensaje es recibido por el cliente.

		// En este evento nos encargaremos de leer los ID especiales del mensaje para indicarle que hacer.
	virtual void OnMessage(std::shared_ptr<cap::net::connection<CustomMsgTypes>> client, cap::net::message<CustomMsgTypes>& msg) {
		// Creamos un switch para leer y ejecutar un caso dependiendo del ID del mensaje, y de igual forma
		// informar que un cliente en especifico ha hecho una petición especial.
		switch (msg.header.id) {

			// Caso en el que el cliente pidio el tiempo de respuesta del servidor.
			case CustomMsgTypes::ServerPing: {
				printf("[%u]: Pingeando al Servidor.\n", client->GetID());

				// Regresamos el mensaje al cliente ya que la petición es para medir el tiempo de respuesta.
				client->Send(msg);
			}
			break;

			// Caso en el que un cliente pidio mandar un mensaje a todos.
			case CustomMsgTypes::MessageAll: {
				// Notificamos que el servidor recibio la petición.
				printf("[%u]: Mando un mensaje a todos.\n", client->GetID());

				// Creamos el mensaje con su ID de respuesta para ejecutar la acción pedida.
				cap::net::message<CustomMsgTypes> msg;
				msg.header.id = CustomMsgTypes::MessageAll;

				// Le agregamos el mensaje a ser transmitido.
				msg << client->GetID();

				// Y ejecutamos la función que nos ayudara a ejecutar la acción que necesitamos.
				this->MessageAllClients(msg, client);
			}
			break;
		}
	}

public:
	// Constructor del server que pide como parametro el puerto.
	CustomServer(uint16_t nPort) : cap::net::server_interface<CustomMsgTypes>(nPort) {}


	virtual void OnClientValidated(std::shared_ptr<cap::net::connection<CustomMsgTypes>> client) {
	}
};

int main() {

	// Crearemos el servidor en puerto 60000 y lo iniciamos.
	CustomServer server(60000);
	server.Start();

	// Creamos un while donde se estara ejecutando el Update()
	while (1) {
		server.Update(-1, true);
	}

	return 0;
}