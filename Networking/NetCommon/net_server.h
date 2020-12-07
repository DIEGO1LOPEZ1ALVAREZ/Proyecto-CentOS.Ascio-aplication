#pragma once

#include "net_common.h"
#include "net_tsqueue.h"
#include "net_message.h"
#include "net_connection.h"

namespace cap {
	namespace net {

		// Esta clase se encargara de hacer los procesos guardados en la cosa de subprocesos.
		// Tambi�n es la clase principal que nos permitira crear el servidor el cual tambi�n
		// manipulara quienes pueden conectarse y como seran tratados.
		template <typename T>
		class server_interface {
		private:

		protected:

			// Evento que se llama cuando un cliente se conecta, se puede vetar la conexi�n si se retorna falso.
			virtual bool OnClientConnect(std::shared_ptr<connection<T>> client) {
				return false;
			}

			// Evento llamada cuando un cliente aparenta haberse desconectado.
			virtual void OnClientDisconnect(std::shared_ptr<connection<T>> client) {

			}

			// Evento cuando un mensaje es entregado al cliente
			virtual void OnMessage(std::shared_ptr<connection<T>> client, message<T>& msg) {

			}

			// Cola de subprocesos segura que servira para paquetes de mensajes entrantes
			tsqueue<owned_message<T>> m_qMessagesIn;

			// Como siempre, la orden de declaraci�n, contexto y su proceso para que el contexto no cierre.
			// Mejor visto como la orden de inicializaci�n.

			asio::io_context m_asioContext;
			std::thread m_threadContext;

			// Esta variable aceptara un socket que sera reservado para el servidor, pero necesita un contexto.
			asio::ip::tcp::acceptor m_asioAcceptor;

			// Variable que nos permitir� tener una lista din�mica que puede crecer hacia delante o hacia atr�s que nos ayudara a manipular
			// y/o almacenar a los nuevos clientes que se conecten, la cual tendr� una conexi�n compartida.
			std::deque<std::shared_ptr<connection<T>>> m_deqConnections;

			// Clientes ser�n identificados con un sistema m�s aplio por medio de un ID, esta variable indica el maximo de IDs
			uint32_t nIDCounter = 10000;

		public:
			// Crea el servidor con Ipv4 y un puerto a agregar.
			server_interface(uint16_t port) : m_asioAcceptor(m_asioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {
			}

			virtual ~server_interface() {
				Stop();
			}

			// Inicializa el server.
			bool Start() {
				// Intentaremos hacer los procesos para la conexi�n, si hay falla imprimir� la excepci�n y retornara falso.
				try {
					// Al iniciar esperara a que los clientes se conecten.
					this->WaitForClientConnection();

					// He inicializara un proceso con el contexto.
					this->m_threadContext = std::thread([this]() { m_asioContext.run(); });
				}
				catch (std::exception& e) {
					std::cerr << "[SERVIDOR] Excepci�n encontrada: " << e.what() << "\n";
					return false;
				}

				// Si no hay fallos al intentar establecer el trabajo del contexto, el server se inicio exitosamente.
				printf("[SERVIDOR] Se inicio.\n");
				return true;
			}

			// Detiene al server.
			void Stop() {
				// Pedimos que el contexto finalice su trabajo.
				this->m_asioContext.stop();

				// Verificamos si el proceso del contexto se puede bloquear, si es as�, bloqueamos.
				if (this->m_threadContext.joinable()) {
					this->m_threadContext.join();
				}

				// Y finalmente informamos que el servidor se ha detenido
				printf("[SERVIDOR] Se detuvo.\n");
			}

			// M�todo ASYNC, indica a asio para que espero por una conexi�n.
			void WaitForClientConnection() {
				// Esta funci�n es sincr�nica, y se encargara de aceptar clientes ya verificados.
					// Se usara una funci�n lambda para simplificarlo, la cual tendr� de par�metros un manejador de c�digo y
					// un socket donde estar� el server.
				this->m_asioAcceptor.async_accept([this](std::error_code ec, asio::ip::tcp::socket socket) {
						// Si no hay error verificaremos, si lo hay, informaremos el por que.
						if (!ec) {
							// Informamos de que la conexi�n fue aceptada.
							std::cout << "[SERVIDOR] Se ha generado una nueva conexi�n: " << socket.remote_endpoint() << "\n";
							
							// Crearemos una nueva conexi�n compartida con la funci�n crear compartici�n.
							std::shared_ptr<connection<T>> newConn = std::make_shared<connection<T>>(connection<T>::owner::server, m_asioContext, std::move(socket), m_qMessagesIn);

							// Hecha la conexi�n, el cliente deber� tener la opci�n de cancelar la conexi�n.

							// As� que usaremos un if para darle tal opci�n con el evento al conectarse el cliente.
							if (OnClientConnect(newConn)) {
								// La conexi�n se permiti�, as� que agregaremos un contenedor de nuevas conexiones y la moveremos
								// en caso de que esta cambie de posici�n.
								m_deqConnections.push_back(std::move(newConn));

								// Ya agregado, es momento de asignarle un ID con el m�todo siguiente.
								m_deqConnections.back()->ConnectToClient(nIDCounter++);

								// Y ahora indicamos que el cliente se conecto con la ID asignada.
								std::cout << "[" << m_deqConnections.back()->GetID() << "] Conexi�n aprobada.\n";
							}
							else {
								// Si se ejecuta este apartado, es por que el cliente deneg� la conexi�n.
								printf("[-----] Conexi�n denegada\n");
							}

						}
						else {
							// Si se ejecuta esto es o por que hubo un error, o por que la validaci�n no fue aceptada.
							std::cout << "[SERVIDOR] Se ha generado un nuevo error de conexi�n: " << ec.message() << "\n";
						}

						// Como esto es una funci�n sincr�nica, y el trabajo del servidor
						// Tenemos que darle m�s trabajo, haci�ndole que vuelva a esperar por otra conexi�n.
						WaitForClientConnection();
					}
				);
			}

			// Env�a un mensaje a un cliente en especifico.
				// Se agrega como par�metro una conexi�n compartida (cliente) y el mensaje a enviar.
			void MessageClient(std::shared_ptr<connection<T>> client, const message<T>& msg) {
				// Verificamos si el cliente es valido y este esta conectado.
				if (client && client->IsConnected()) {
					// Y si cumple con lo anterior, podemos mandarle un respectivo mensaje.
					client->Send(msg);
				}
				else {
					// Y si no cumple con lo anterior, se puede asumir que esta desconectado, as� que hay que ejecutar el evento respectivo.
					this->OnClientDisconnect(client);
					// Y eliminarlo
					client.reset();
					this->m_deqConnections.erase(std::remove(this->m_deqConnections.begin(), this->m_deqConnections.end(), client), this->m_deqConnections.end());
				}
			}

			// Env�a un mensaje a todos los clientes.
				// Se agrega como par�metro el mensaje, y una conexi�n compartida (cliente) a ser ignorado.
			void MessageAllClients(const message<T>& msg, std::shared_ptr<connection<T>> pIgnoreClient = nullptr) {

				// Este booleano se utilizara en caso de que haya algun cliente que ya es invalido, para desconectarlo
				bool bInvalidClientExists = false;

				// Utilizaremos un foreach para mandar el mensaje a todos los clientes, tomando el valor de referencia de las conexi�nes.
				for (auto& client : this->m_deqConnections) {
					// Verificamos si el cliente es valido y este esta conectado.
					if (client && client->IsConnected()) {
						// Y tambi�n verificamos si no es un cliente a ignorar.
						if (client != pIgnoreClient) {
							// Y si cumple con lo anterior, podemos mandarle un respectivo mensaje.
							client->Send(msg);
						}
					}
					else {
						// Y si no cumple con lo anterior, se puede asumir que esta desconectado, as� que hay que ejecutar el evento respectivo.
						this->OnClientDisconnect(client);
						// Y eliminarlo
						client.reset();

						// Y como se detecto almenos 1 cliente invalido, se podra hacer que se eliminen todos los desconectados.
						bInvalidClientExists = true;
					}
				}

				// Si se llego a encontrar un cliente invalido, se eliminaran todos los invalidos, ya que
				// esta funci�n se encargara automaticamente de eliminar los ya inexistentes.
				if (bInvalidClientExists) {
					this->m_deqConnections.erase(std::remove(this->m_deqConnections.begin(), this->m_deqConnections.end(), nullptr), this->m_deqConnections.end());
				}
			}
		
			// Esta funci�n ser� llamada por el usuario para poder actualizar los procesos en la cola.
				// Se agrega un par�metro de 32 bytes sin asignar, el cual tiene como predeterminado el valor -1
				// debido a que como es un entero sin asignar, ponerle -1 hace que llegue a su m�ximo valor (2,147,483,648).
				// Y tambi�n un par�metro para indicar si el programa debe de esperarse.
			void Update(size_t nMaxMessages = -1, bool bWait = false) {

				// Si indicamos previamente que el programa se esperara, este esperara.
				if (bWait) {
					this->m_qMessagesIn.wait();
				}

				// Variable que servir� para contar los mensajes le�dos.
				size_t nMessagesCount = 0;

				// Si llega a encontrar mensajes vac�os, ya no leer� nada.
				while (nMessagesCount < nMaxMessages && !this->m_qMessagesIn.empty()) {
					// Obtiene el mensaje frontal.
					auto msg = this->m_qMessagesIn.pop_front();

					// Pasa el mensaje al manejador/evento correspondiente.
					this->OnMessage(msg.remote, msg.msg);

					nMessagesCount++;
				}
			}
		};

	}
}