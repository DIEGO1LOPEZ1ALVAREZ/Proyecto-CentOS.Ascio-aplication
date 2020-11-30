#pragma once

// Agregando todas las librer�as a usar.
#include "net_common.h"

// Crearemos la librer�a cap (CentOS Ascio Project) que llevara la librer�a net para hacer las conexiones de mensajes.
namespace cap {
	// Librer�a net que se encargara de las conexiones y el tipo de mensajes a procesar.
	namespace net {

		// Crearemos una estructura enumera que se encargara de poder darle una identificaci�n y al tipo de mensaje a procesar, entre otros.
			// Tambi�n el encabeza de los mensajes se enviara al inicio de todos los mensajes
			// Por el cual la plantilla nos permitir� usar clases num�ricas para asegurarnos que los mensajes son validos a la hora de compilarlos.
			// Tambi�n se hace esto para hacer flexible esta clase num�rica en caso de que haya diferentes tipos de mensajes.
		template <typename T>
		struct message_header {
			// Id Del mensaje con formato a especificar con tama�o sin asignar de 32 bits.
			T id{};
			uint32_t size = 0
		};

		// Es la estructura del mensaje del cuerpo, que ya posee el encabezado del mensaje.
		template <typename T>
		struct message {
			// Como antes se menciono, cada mensaje tiene un encabezado que ser� el primero en ser procesado y por ende hay que agregarlo.
			message_header<T> header{};

			// Usaremos un arreglo din�mico para poder almacenar el cuerpo con enteros sin asignar de hasta 256 (1 Byte).
			std::vector<uint8_t> body;

			// Cada que queremos leer o escribir un mensaje, el socket debe de recibir el tama�o que se desea, por ende
			// es necesario crear este m�todo que se encargara de retornar el tama�o completo del paquete del mensaje, en bytes.
			size_t size() const {
				// Solo tenemos que conseguir el tama�o del encabezado del mensaje y sumarle el tama�o del cuerpo, el cual es m�s sencillo
				// debido a que es un arreglo din�mico.
				return sizeof(message_header<T>) + body.size();
			}

			// Sobrescribimos la compatibilidad del std::cout para producir una descripci�n amistosa del mensaje
				// Principalmente esto se hace para poder depurar.
			friend std::ostream& operator << (std::ostream& os, const message<T>& msg) {
				os << "ID:" << int(msg.header.id) << " Tama�o:" << msg.header.size;
				return os;
			}

			// Empuja cualquier cosa de POD (Programming Output Data) como datos hacia el buffer del mensaje
				// Y retorna los datos para poder encajarlos de nuevo.
			template<typename DataType>
			friend message<T>& operator << (message<T>& msg, const DataType& data) {
				// Revisa si el tipo de los datos esta siendo empujado es trivialmente copiable.
				static_assert(std::is_standard_layout<DataType>::value, "Los datos son muy complejos para ser empujados dentro del vector");

				// Consigue el tama�o actual del cache del vector, debido a que este ser� el punto donde ingresaremos datos.
				size_t i = msg.body.size();

				// Aumentamos el tama�o del vector con el tama�o de los datos que est�n siendo empujados.
				msg.body.resize(msg.body.size() + sizeof(DataType));

				// F�sicamente copiamos los datos dentro del nuevo espacio del vector.
				std::memcpy(msg.body.data() + i, &data, sizeof(DataType));

				// Recalculamos el tama�o del mensaje.
				msg.header.size = msg.size();

				// Retornamos el mensaje dado para que pueda ser concatenado.
				return msg;
			}

			// Este m�todo ser� usado para poder sacar datos de los mensajes.
				// Y retorna los datos para poder encajarlos de nuevo.
			template<typename DataType>
			friend message<T>& operator >> (message<T>& msg, DataType& data) {
				// Revisa si el tipo de los datos esta siendo empujado es trivialmente copiable.
				static_assert(std::is_standard_layout<DataType>::value, "Los datos son muy complejos para ser empujados dentro del vector");

				//  Consigue la localizaci�n cache hacia el final del vector donde los datos extra�dos empiezan.
				size_t i = msg.body.size() - sizeof(DataType);

				// F�sicamente copea los datos del vector dentro de las variables del usuario.
				std::memcpy(&data, msg.body.data() + i, sizeof(DataType));

				// Se encoje el vector para remover los bytes le�dos, y para resetear la posici�n final.
				msg.body.resize(i);

				// Recalculando el tama�o del mensaje.
				msg.header.size = msg.size();

				// Retornamos el mensaje dado para que pueda ser concatenado.
				return msg;
			}

		};

	}
}