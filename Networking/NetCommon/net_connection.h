#pragma once

#include "net_common.h"
#include "net_tsqueue.h"
#include "net_message.h"

namespace cap {
	namespace net {

		// Clase que nos permitirá crear un pointer compartido dentro del todo el objeto.
		// Esto actuara como la conexión.
		template <typename T>
		class connection : public std::enable_shared_from_this<connection<T>> {
		private:

		protected:
			// Cada conexión tiene un socket único para el control remoto.
			asio::ip::tcp::socket m_socket;

			// Este contexto es compartido con toda la instancia asio, ya que se debe usar el mismo contexto y no múltiples.
			asio::io_context& m_asioContext;

			// Esta cola de subprocesos sostiene todos los mensajes a ser enviado hacia el control remoto de esta conexión.
			tsqueue<message<T>> m_qMessagesOut;

			// Esta cola de sobprocesos sostiene todos los mensajes a ser recividos del control remoto de esta conexión.
			// Notar que es una referencía como el "propietario" de esta conexión, se espera que se provea una cola de subprocesos.
			tsqueue<owned_message>& m_qMessagesIn;

		public:
			connection() {}

			virtual ~connection() {}

			// Método que retorna un valor booleano en caso de que se haya completo la conexión al servidor.
			bool ConnectToServer();
			
			// Método que retorna un valor booleano en caso de que la conexión se haya cerrado exitosamente.
			bool Disconnect();

			// Método que retorna un valor booleano en caso de que hay una conexión establecida al servidor.
			bool IsConnected() const;

			// Método que retorna un valor booleano en caso de que se haya enviado el mensaje correctamente.
			bool Send(const message<T>& msg);
		};

	}
}