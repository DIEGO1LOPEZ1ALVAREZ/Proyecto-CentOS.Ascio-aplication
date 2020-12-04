#pragma once
#include "net_common.h"
#include "net_message.h"
#include "net_tsqueue.h"
#include "net_connection.h"

namespace cap {
	namespace net {

		// Esta clase es responsable de establecer y configurar asio y la conexión,
		// también se comparte con un pointer para poder usar la aplicación para comunicarse
		// con el server.

		template <typename T>
		class client_interface {

			client_interface() : m_socket(m_context) {
				// Se inicializa el socket con el contexto, para que así pueda hacer cosas.
			}

			virtual ~client_interface() {
				// Si el cliente es destruido, se desconectarse del server.
				this->Disconnect();
			}

		private:

			// Este es el proceso seguro de la cola de subprocesos con mensajes entrantes del servidor.
			tsqueue<owned_message<T>> m_qMessagesIn;

		protected:

			// Los contextos de asio manejas las transferencias de datos.
			// Pero por cada contexto, este necesita su propio proceso para ejecutar sus comandos.

			asio::io_context m_context;
			std::thread thrContext;

			// Este es el socket físico que esta conectado al servidor.
			asio::ip::tcp::socket m_socket;

			// El cliente tiene una única instancia de un objeto de "connection", el cual maneja la transferencia de datos.
			std::unique_ptr<connection<T>> m_connection;

		public:

			// Conecta al servidor con hostname o ip y con su puerto, retornara si la conexión fue posible.
			bool Connect(const std::string& host, const uint16_t port) {
				try {
					// Creando la conexión
					this->m_connection = std::make_unique<connection<T>>(); // TODO

					// Resuelve el hostname/ip en una dirección física tangible.
					asio::ip::tcp::resolver resolver(this->m_context);
					
					// Consigue el host y lo guarda en el punto final de la conexión.
					m_endpoints = resolver.resolve(host, std::to_string(port));

					// Le indica a la conexión que se conecte al server.
					this->m_connection->ConnectToServer(m_endpoints);

					// Empieza un proceso con el contexto.
					this->thrContext = std::thread([this]() { m_context.run(); });

				}
				catch (std::exception& e) {
					// Imprimimos la excepción encontrada.
					std::cerr << "Excepción del Cliente: " << e.what() << "\n";
					return false;
				}

				return true;
			}

			// Desconecta del servidor.
			void Disconnect() {
				// Si la conexión existe, y esta conectada nos desconectamos.
				if (IsConnected()) {
					this->m_connection->Disconnect();
				}

				// Como acabamos con la conexión, también es necesario acabar con el contexto de asio y sus procesos.
				this->m_context.stop();
				if (this->thrContext.joinable()) {
					this->thrContext.join();
				}

				// Finalmente destruimos el objeto de la conexión.
				this->m_connection.release();
			}

			// Revisa si el cliente esta conectado al servidor.
			bool IsConnected() {
				if (this->m_connection) {
					return this->m_connection->IsConnected();
				}
				else {
					return false;
				}
			}
			
			// Recupera la cola de mensajes del server.
			tsqueue<owned_message<T>>& Incoming() {
				return this->m_qMessagesIn;
			}

		};
	}
}