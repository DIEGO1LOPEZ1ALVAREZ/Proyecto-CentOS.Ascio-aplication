#pragma once

#include "net_common.h"
#include "net_tsqueue.h"
#include "net_message.h"

namespace cap {
	namespace net {

		// Clase que nos permitir� crear un pointer compartido dentro del todo el objeto.
		// Esto actuara como la conexi�n.
		template <typename T>
		class connection : public std::enable_shared_from_this<connection<T>> {
		private:

		protected:
			// Cada conexi�n tiene un socket �nico para el control remoto.
			asio::ip::tcp::socket m_socket;

			// Este contexto es compartido con toda la instancia asio, ya que se debe usar el mismo contexto y no m�ltiples.
			asio::io_context& m_asioContext;

			// Esta cola de subprocesos sostiene todos los mensajes a ser enviado hacia el control remoto de esta conexi�n.
			tsqueue<message<T>> m_qMessagesOut;

			// Esta cola de sobprocesos sostiene todos los mensajes a ser recividos del control remoto de esta conexi�n.
			// Notar que es una referenc�a como el "propietario" de esta conexi�n, se espera que se provea una cola de subprocesos.
			tsqueue<owned_message>& m_qMessagesIn;

		public:
			connection() {}

			virtual ~connection() {}

			// M�todo que retorna un valor booleano en caso de que se haya completo la conexi�n al servidor.
			bool ConnectToServer();
			
			// M�todo que retorna un valor booleano en caso de que la conexi�n se haya cerrado exitosamente.
			bool Disconnect();

			// M�todo que retorna un valor booleano en caso de que hay una conexi�n establecida al servidor.
			bool IsConnected() const;

			// M�todo que retorna un valor booleano en caso de que se haya enviado el mensaje correctamente.
			bool Send(const message<T>& msg);
		};

	}
}