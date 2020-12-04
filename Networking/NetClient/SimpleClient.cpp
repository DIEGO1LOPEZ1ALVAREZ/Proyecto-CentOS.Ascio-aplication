/*********************************************************************
* Este será el cliente que se encargara de hacer todos los procesos  *
* y mandarlos al servidor, también se puede decir que aquí se puede  *
* llevar el sistema que se quiere como un juego, aplicación, etc.    *
*********************************************************************/

#include <iostream>
#include <cap_net.h> // Agregamos la librería estática NetCommon con sus headers.

// Esta es una clase enumerada que se encargara de ser el tipo de mensajes que procesara la librería estática CapNet
enum class CustomMsgTypes : uint32_t {
	// Tiene 2 miembros enumerados de tipo entero de 32 bits sin asignar.
	// Estos serán los mensajes que se podrán manejar al utilizarse con el CapNet.
	Player,
	MovePlayer
};

// Ejemplo de como se armaria el juego.

class CustomClient : public cap::net::client_interface<CustomMsgTypes> {
private:

protected:

public:

	bool MovePlayer(float x, float y) {
		cap::net::message<CustomMsgTypes> msg;
		msg.header.id = CustomMsgTypes::MovePlayer;
		msg << x << y;
	}

};

int main() {
	CustomClient c();

	return 0;
}