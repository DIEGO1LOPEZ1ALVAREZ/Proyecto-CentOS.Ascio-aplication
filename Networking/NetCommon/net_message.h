#pragma once

// Agregando todas las librerías a usar.
#include "net_common.h"

// Crearemos la librería cap (CentOS Ascio Project) que llevara la librería net para hacer las conexiones de mensajes.
namespace cap {
	// Librería net que se encargara de las conexiones y el tipo de mensajes a procesar.
	namespace net {

		// Crearemos una estructura enumera que se encargara de poder darle una identificación y al tipo de mensaje a procesar, entre otros.
			// También el encabeza de los mensajes se enviara al inicio de todos los mensajes
			// Por el cual la plantilla nos permitirá usar clases numéricas para asegurarnos que los mensajes son validos a la hora de compilarlos.
			// También se hace esto para hacer flexible esta clase numérica en caso de que haya diferentes tipos de mensajes.
		template <typename T>
		struct message_header {
			// Id Del mensaje con formato a especificar con tamaño sin asignar de 32 bits.
			T id{};
			uint32_t size = 0
		};

		// Es la estructura del mensaje del cuerpo, que ya posee el encabezado del mensaje.
		template <typename T>
		struct message {
			// Como antes se menciono, cada mensaje tiene un encabezado que será el primero en ser procesado y por ende hay que agregarlo.
			message_header<T> header{};

			// Usaremos un arreglo dinámico para poder almacenar el cuerpo con enteros sin asignar de hasta 256 (1 Byte).
			std::vector<uint8_t> body;

			// Cada que queremos leer o escribir un mensaje, el socket debe de recibir el tamaño que se desea, por ende
			// es necesario crear este método que se encargara de retornar el tamaño completo del paquete del mensaje, en bytes.
			size_t size() const {
				// Solo tenemos que conseguir el tamaño del encabezado del mensaje y sumarle el tamaño del cuerpo, el cual es más sencillo
				// debido a que es un arreglo dinámico.
				return sizeof(message_header<T>) + body.size();
			}

			// Sobrescribimos la compatibilidad del std::cout para producir una descripción amistosa del mensaje
				// Principalmente esto se hace para poder depurar.
			friend std::ostream& operator << (std::ostream& os, const message<T>& msg) {
				os << "ID:" << int(msg.header.id) << " Tamaño:" << msg.header.size;
				return os;
			}

			// Empuja cualquier cosa de POD (Programming Output Data) como datos hacia el buffer del mensaje
				// Y retorna los datos para poder encajarlos de nuevo.
			template<typename DataType>
			friend message<T>& operator << (message<T>& msg, const DataType& data) {
				// Revisa si el tipo de los datos esta siendo empujado es trivialmente copiable.
				static_assert(std::is_standard_layout<DataType>::value, "Los datos son muy complejos para ser empujados dentro del vector");

				// Consigue el tamaño actual del cache del vector, debido a que este será el punto donde ingresaremos datos.
				size_t i = msg.body.size();

				// Aumentamos el tamaño del vector con el tamaño de los datos que están siendo empujados.
				msg.body.resize(msg.body.size() + sizeof(DataType));

				// Físicamente copiamos los datos dentro del nuevo espacio del vector.
				std::memcpy(msg.body.data() + i, &data, sizeof(DataType));

				// Recalculamos el tamaño del mensaje.
				msg.header.size = msg.size();

				// Retornamos el mensaje dado para que pueda ser concatenado.
				return msg;
			}

			// Este método será usado para poder sacar datos de los mensajes.
				// Y retorna los datos para poder encajarlos de nuevo.
			template<typename DataType>
			friend message<T>& operator >> (message<T>& msg, DataType& data) {
				// Revisa si el tipo de los datos esta siendo empujado es trivialmente copiable.
				static_assert(std::is_standard_layout<DataType>::value, "Los datos son muy complejos para ser empujados dentro del vector");

				//  Consigue la localización cache hacia el final del vector donde los datos extraídos empiezan.
				size_t i = msg.body.size() - sizeof(DataType);

				// Físicamente copea los datos del vector dentro de las variables del usuario.
				std::memcpy(&data, msg.body.data() + i, sizeof(DataType));

				// Se encoje el vector para remover los bytes leídos, y para resetear la posición final.
				msg.body.resize(i);

				// Recalculando el tamaño del mensaje.
				msg.header.size = msg.size();

				// Retornamos el mensaje dado para que pueda ser concatenado.
				return msg;
			}

		};

	}
}