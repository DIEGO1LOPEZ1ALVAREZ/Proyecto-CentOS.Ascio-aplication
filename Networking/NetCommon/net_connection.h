#pragma once

#include "net_common.h"
#include "net_tsqueue.h"
#include "net_message.h"

namespace cap {
	namespace net {

		// Declaraci�n primitiva.
		template<typename T>
		class server_interface;

		// Clase que nos permitir� crear un pointer compartido dentro del todo el objeto.
		// Esto actuara como la conexi�n.
		template <typename T>
		class connection : public std::enable_shared_from_this<connection<T>> {
		private:

			// M�todo sincr�nico, comprime el contexto listo para poder leer el encabezado de un mensaje.
			void ReadHeader() {
				// Le indicamos a asio que lea de forma sincr�nica.
					// D�ndole el socket de conexi�n, un buffer donde se guardara el mensaje con el tama�o del encabezado del mensaje.
					// Y como en cada m�todo donde hay algo sincr�nico, creamos una funci�n lambda para que ejecute directamente.
						// Donde pedir� un manejador de errores y el tama�o del encabezado.
				asio::async_read(this->m_socket, asio::buffer(&this->m_msgTemporaryIn.header, sizeof(message_header<T>)), [this](std::error_code ec, std::size_t length) {
						// Si no hay ning�n error podemos continuar con la lectura del encabezado.
						if (!ec) {
							// Verificamos que el mensaje temporal tenga tama�o.
							if (m_msgTemporaryIn.header.size > 0) {
								// Si tiene espacio, significa que hay espacio para copear el mensaje.
								// As� que le asignamos el tama�o al cuerpo el del encabezado.
								m_msgTemporaryIn.body.resize(m_msgTemporaryIn.header.size);

								// Ahora le indicamos que lea el cuerpo.
								ReadBody();
							}
							else {
								// Si se llega a esta parte, significa que el mensaje temporal no tiene espacio.
								// Por ende no tiene cuerpo el mensaje.

								// As� que lo mandamos directamente a la cola de mensajes.
								AddToIncomingMessageQueue();
							}
						}
						else {
							// Si se ejecuta esta secci�n es debido a que sucedi� un fallo al intentar conseguir el encabezado.
							// Y notificamos que fallo.
							printf("[%u] La lectura del encabezado fallo.\n", id);

							// Y cerramos el socket para evitar flujos.
							m_socket.close();
						}
					});
			}

			// M�todo sincr�nico, comprime el contexto listo para poder escribir el encabezado de un mensaje.
			void WriteHeader() {
				// Le indicamos a asio que escriba de forma sincr�nica.
					// D�ndole el socket de conexi�n, un buffer donde se guardaran los mensajes salientes, y el tama�o.
					// Y como en cada m�todo donde hay algo sincr�nico, creamos una funci�n lambda para que ejecute directamente.
						// Donde pedir� un manejador de errores y el tama�o del cuerpo.
				asio::async_write(this->m_socket, asio::buffer(&this->m_qMessagesOut.front().header, sizeof(message_header<T>)), [this](std::error_code ec, std::size_t length) {
						// Si no hay ning�n error podemos continuar con la escritura del encabezado.
						if (!ec) {
							// Verificamos que haya tama�o para poder escribir.
							if (m_qMessagesOut.front().body.size() > 0) {
								// Si lo hay, escribimos en el cuerpo.
								WriteBody();
							}
							else {
								// Si no lo hay, lo eliminamos.
								m_qMessagesOut.pop_front();

								// Verificamos si no hay mensajes.
								if (!m_qMessagesOut.empty()) {
									// Si es as�, volvemos a repetir el proceso.
									WriteHeader();
								}
							}
						}
						else {
							// Si hay alg�n error, debemos indicar que no se pudo escribir el encabezado, y cerramos la conexi�n.
							printf("[%u] La escritura del encabezado fallo.", id);
							m_socket.close();
						}
					});
			}

			// M�todo sincr�nico, comprime el contexto listo para poder leer el cuerpo de un mensaje.
			void ReadBody() {
				// Le indicamos a asio que lea de forma sincr�nica.
					// D�ndole el socket de conexi�n, un buffer donde se guardara el mensaje con el tama�o del cuerpo del mensaje.
					// Y como en cada m�todo donde hay algo sincr�nico, creamos una funci�n lambda para que ejecute directamente.
						// Donde pedir� un manejador de errores y el tama�o del cuerpo.
				asio::async_read(this->m_socket, asio::buffer(this->m_msgTemporaryIn.body.data(), this->m_msgTemporaryIn.body.size()), [this](std::error_code ec, std::size_t length) {
						// Si no hay ning�n error podemos continuar con la lectura del cuerpo.
						if(!ec) {
							// Entonces si no hubo ning�n error significa que tanto hay cuerpo como encabezado y se pueden procesar ambos.
							// As� que lo agregamos a la cola de mensajes.
							AddToIncomingMessageQueue();
						}
						else {
							// Si llega a esta parte es por que hubo alg�n error.
							// As� que notificamos de ello.
							printf("[%u] La lectura del cuerpo fallo.\n", id);

							// Y cerramos el socket para evitar flujos.
							m_socket.close();
						}
					});
			}

			// M�todo sincr�nico, comprime el contexto listo para poder escribir el cuerpo de un mensaje.
			void WriteBody() {
				// Le indicamos a asio que escriba de forma sincr�nica.
					// D�ndole el socket de conexi�n, un buffer donde se guardaran los mensajes salientes del cuerpo, y el tama�o.
					// Y como en cada m�todo donde hay algo sincr�nico, creamos una funci�n lambda para que ejecute directamente.
						// Donde pedir� un manejador de errores y el tama�o del cuerpo.
				asio::async_write(this->m_socket, asio::buffer(this->m_qMessagesOut.front().body.data(), this->m_qMessagesOut.front().body.size()),
					[this](std::error_code ec, std::size_t length) {
						// Verificamos que no haya ning�n error.
						if (!ec) {
							// Entonces si no hubo ning�n error significa que tanto el cuerpo y el encabezado fueron escritos correctamente.
							// As� que el mensaje fue escrito y se puede eliminar de la cola de mensajes escritos y procesados.
							m_qMessagesOut.pop_front();

							// Verificamos que ya no haya mensajes a escribir, si no los hay, entonces podemos repetir el proceso.
							if (!m_qMessagesOut.empty()) {
								WriteHeader();
							}
						}
						else {
							// Si lo hay notificamos que fallo la escritura del cuerpo y cerramos para evitar flujos.
							printf("[%u] La escritura del cuerpo fallo.", id);
							m_socket.close();
						}
					});
			}

			// Esta funci�n permitira que si el que ejecuta este proceso es el servidor
			// permitirle que transforme los mensajes a mensajes con autor.
			void AddToIncomingMessageQueue() {
				if (this->m_nOwnerType == owner::server) {
					// Agregamos a la lista los mensajes entrantes para que sea compartidos en esta conexi�n.
					this->m_qMessagesIn.push_back({ this->shared_from_this(), m_msgTemporaryIn });
				}
				else {
					// Si no es owner, entonces agregamos �nicamente los mensajes entrantes.
					this->m_qMessagesIn.push_back({ nullptr, m_msgTemporaryIn });
				}

				// Ya terminado de agregar los mensajes, le indicamos que vuelva leer otro mensaje.
				ReadHeader();
			}

			// Funci�n de encriptar datos.
			uint64_t scramble(uint64_t nInput) {
				uint64_t out = nInput ^ 0xDEADBEEFC0DECAFE;
				out = (out & 0xF0F0F0F0F0F0F0) >> 4 | (out & 0x0F0F0F0F0F0F0F) << 4;
				return out ^ 0xC0DEFACE12345678;
			}

			// M�todo sincr�nico, funci�n utilizada por ambos, cliente y servidor para escribir datos de validaci�n.
			void WriteValidation() {
				// Le indicamos a asio que escriba de forma sincr�nica.
					// Esto sera utilizado para que en el socket dado, el buffer de asio pueda escribir una validaci�n.
				asio::async_write(this->m_socket, asio::buffer(&this->m_nADVOut, sizeof(uint64_t)), [this](std::error_code ec, std::size_t length) {
						// Verificamos que no haya errores.
						if (!ec) {
							// Si no hay errores, la valadici�n fue enviada y los clientes lo unico que deben
							// de hacer es sentarse a esperar.
							if (m_nOwnerType == owner::client) {
								ReadHeader();
							}
						}
						else {
							// Si hubo errores, cerramos para evitar flujos.
							m_socket.close();
						}
					});
			}

			// M�todo sincr�nico, funci�n que nos servir� para leer validaciones.
				// Se agrega un par�metro el cual es la direcci�n del servidor al que esta procesando todo esto.
			void ReadValidation(cap::net::server_interface<T>* server = nullptr) {
				// Le indicamos a asio que lea de forma sincr�nica.
					// Esta funci�n leera lo que se haya agregado de validaci�n y lo fijara y validara con la direcci�n dada.
				asio::async_read(this->m_socket, asio::buffer(&this->m_nADVIn, sizeof(uint64_t)), [this, server](std::error_code ec, std::size_t length) {
						// Verificamos que no haya errores.
						if (!ec) {
							// Verificamos quien es el que ejecuta esta funci�n.
							if (m_nOwnerType == owner::server) {
								// Si la conexi�n es la de un servidor, nos encargaremos de verificar que la validaci�n sea correcta.
								if (m_nADVIn == m_nADVCheck) {
									// Si el cliente valido bien, le permitiremos al cliente conectarse,
									// pero antes ejecutaremos el evento cuando un cliente es verificado.
									// Tambi�n notificamos de paso.
									printf("Cliente validado\n");
									server->OnClientValidated(this->shared_from_this());

									// Y como dicho previamente, el cliente fue valido ahora podremos sentarnos a escucharlo.
									ReadHeader();
								}
								else {
									// Si el cliente no valio bien, lo desconectamos y lo agregamos a la lista negra >:(
									printf("Cliente desconectado (Fall� la validaci�n)\n");
									// Y obviamente cerramos para evitar flujos.
									m_socket.close();
								}
							}
							else {
								// Si la conexi�n es de un cliente, lo unico que debe de ser es resolver la verificaci�n.
								m_nADVOut = scramble(m_nADVIn);

								// Y escribimos el resultado.
								WriteValidation();
							}
						}
						else {
							// Si hay un error significa que hubo un problema mayor que la validaci�n.
							printf("Cliente desconectado (Lectura de Validaci�n)\n");
							m_socket.close();
						}
					});
			}

		public:

			// Crearemos una numeraci�n de tipo de autor de conexi�n.
			enum class owner {
				server,
				client
			};

			// Para crear la conexi�n, necesitaremos el padre de la conexi�n (quien crea la conexi�n), el contexto de la conexi�n,
			// el socket donde se hace el proceso y la cola de subprocesos seguro donde se recibir�n los mensajes.
			connection(owner parent, asio::io_context& asioContext, asio::ip::tcp::socket socket, tsqueue<owned_message<T>>& qIn)
				: m_asioContext(asioContext), m_socket(std::move(socket)), m_qMessagesIn(qIn) {
				// Le establecemos quien es el nuevo autor de la conexi�n.
				this->m_nOwnerType = parent;

				// Construcci�n de la validaci�n.
				if (this->m_nOwnerType == owner::server) {

					// Le daremos un numero al azar a la validaci�n saliente para el cliente, para que el lo resulva,
					// y sea aceptado.
					this->m_nADVOut = (uint64_t(std::chrono::system_clock::now().time_since_epoch().count()) * uint64_t(rand() % 150)) % sizeof(uint64_t);

					// Pre calculando el resultando para revisar cuando el cliente responda.
					this->m_nADVCheck = this->scramble(this->m_nADVOut);
				}
				else {
					// Si no es servidor, pues lo asignamos el valor default ya que el cliente
					// solo debe de mandar su validaci�n a la hora de conectarse.
					this->m_nADVIn = 0;
					this->m_nADVOut = 0;
				}
			}

			virtual ~connection() {}

			// M�todo que retorna la ID de la conexi�n.
			uint32_t GetID() const {
				return id;
			}

			// M�todo que asigna la ID al cliente siempre y cuando la conexi�n sea del servidor y tambi�n le permita recivir y mandar informaci�n.
			// Tambi�n pide saber que servidor ejecuta esto para poder validar al cliente.
			void ConnectToClient(cap::net::server_interface<T>* server, uint32_t uid = 0) {
				// Verificamos que el que ejecuta esto, es el servidor, si lo es permitir� asignar la ID.
				if (this->m_nOwnerType == owner::server) {
					// Revisamos si al socket esta encendido.
					if (this->m_socket.is_open()) {
						// Y asignamos la ID.
						this->id = uid;

						// Escribimos la validaci�n para que el cliente pueda validarse as� y demostrar que es parte del sistema.
						this->WriteValidation();

						// Y ahora esperamos sincr�nicamente a que el cliente mande su validaci�n para leerla.
						this->ReadValidation(server);
					}
				}
			}

			// M�todo que conecta el cliente al servidor.
				// Nota, solo sirve si eres cliente.
			void ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints) {
				// Verificamos si es cliente el que esta ejecutando esto.
				if (m_nOwnerType == owner::client) {
					// Le pedimos a asio que intente conectarse al punto de la direcci�n dado.
					// Le damos como par�metro el socket, el punto de la direcci�n y la funci�n lambda a ejecutar directamente.
						// La cual tendr� un manejador de errores y de igual forma el punto de la direcci�n.
					asio::async_connect(this->m_socket, endpoints, [this](std::error_code ec, asio::ip::tcp::endpoint endpoint) {
							// Verificamos que no haya errores.
							if (!ec) {
								// Si no hay errores, indicamos que vaya a leer la validaci�n.
								ReadValidation();
							}
						});
				}
			}
			
			// M�todo que cierra la conexi�n.
			void Disconnect() {
				// Verificamos que estemos conectados.
				if (this->IsConnected()) {
					// Para poder desconectarnos, debemos darle el contexto de la conexi�n e usar una
					// funci�n lambda para cerrar directamente. Todo esto con el m�todo post.
					asio::post(this->m_asioContext, [this]() { m_socket.close(); });
				}
			}

			// M�todo que retorna un valor booleano en caso de que hay una conexi�n establecida al servidor.
			bool IsConnected() const {
				return this->m_socket.is_open();
			};

			// M�todo env�a el mensaje dado.
			void Send(const message<T>& msg) {
				// Le indicamos a asio que mande los datos con el m�todo post, dandole as�
				// El contexto donde se est� trabajando y ejecutamos directamente el resultado con una
				// funci�n lambda para hacer que el servidor este en el estado de escribir mensajes.
				asio::post(this->m_asioContext, [this, msg]() {
						
						// Verificamos si esta escribiendo m�s mensajes.
						bool bWritingMessage = !m_qMessagesOut.empty();

						// Primero agregamos el mensaje a la cola de mensajes de salida.
						m_qMessagesOut.push_back(msg);

						// Verificamos que no este escribiendo m�s mensajes.
						if (!bWritingMessage) {
							// Y finalmente empezamos el proceso de escribir.
							WriteHeader();
						}
					});
			}

		protected:
			// Cada conexi�n tiene un socket �nico para el control remoto.
			asio::ip::tcp::socket m_socket;

			// Este contexto es compartido con toda la instancia asio, ya que se debe usar el mismo contexto y no m�ltiples.
			asio::io_context& m_asioContext;

			// Esta cola de subprocesos sostiene todos los mensajes a ser enviado hacia el control remoto de esta conexi�n.
			tsqueue<message<T>> m_qMessagesOut;

			// Esta cola de sobprocesos sostiene todos los mensajes a ser recividos del control remoto de esta conexi�n.
			// Notar que es una referenc�a como el "propietario" de esta conexi�n, se espera que se provea una cola de subprocesos.
			tsqueue<owned_message<T>>& m_qMessagesIn;

			// Al igual que la variable del mismo tipo, esta variable se encargara de guardar los mensajes de forma temporal.
			message<T> m_msgTemporaryIn;

			// Creamos el tipo de autor para poder especificar que tipo de conexi�n es y haya un tipo de evento.
			// Notar que el "autor" decide como algunas conexiones se comportan.
			owner m_nOwnerType = owner::server;

			// Variable que guardara la ID de la conexi�n.
			uint32_t id = 0;

			// Valores para validaci�n.
			uint64_t m_nADVOut = 0;
			uint64_t m_nADVIn = 0;
			uint64_t m_nADVCheck = 0;

		};

	}
}