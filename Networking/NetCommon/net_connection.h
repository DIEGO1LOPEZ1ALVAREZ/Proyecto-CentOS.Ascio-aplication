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

			// Método sincrónico, comprime el contexto listo para poder leer el encabezado de un mensaje.
			void ReadHeader() {
				// Le indicamos a asio que lea de forma sincrónica.
					// Dándole el socket de conexión, un buffer donde se guardara el mensaje con el tamaño del encabezado del mensaje.
					// Y como en cada método donde hay algo sincrónico, creamos una función lambda para que ejecute directamente.
						// Donde pedirá un manejador de errores y el tamaño del encabezado.
				asio::async_read(this->m_socket, asio::buffer(&this->m_msgTemporaryIn.header, sizeof(message_header<T>)), [this](std::error_code ec, std::size_t length) {
						// Si no hay ningún error podemos continuar con la lectura del encabezado.
						if (!ec) {
							// Verificamos que el mensaje temporal tenga tamaño.
							if (m_msgTemporaryIn.header.size > 0) {
								// Si tiene espacio, significa que hay espacio para copear el mensaje.
								// Así que le asignamos el tamaño al cuerpo el del encabezado.
								m_msgTemporaryIn.body.resize(m_msgTemporaryIn.header.size);

								// Ahora le indicamos que lea el cuerpo.
								ReadBody();
							}
							else {
								// Si se llega a esta parte, significa que el mensaje temporal no tiene espacio.
								// Por ende no tiene cuerpo el mensaje.

								// Así que lo mandamos directamente a la cola de mensajes.
								AddToIncomingMessageQueue();
							}
						}
						else {
							// Si se ejecuta esta sección es debido a que sucedió un fallo al intentar conseguir el encabezado.
							// Y notificamos que fallo.
							printf("[%u] La lectura del encabezado fallo.\n", id);

							// Y cerramos el socket para evitar flujos.
							m_socket.close();
						}
					});
			}

			// Método sincrónico, comprime el contexto listo para poder escribir el encabezado de un mensaje.
			void WriteHeader() {
				// Le indicamos  a asio que escriba de forma sincrónica.
					// Dándole el socket de conexión, un buffer donde se guardaran los mensajes salientes, y el tamaño.
					// Y como en cada método donde hay algo sincrónico, creamos una función lambda para que ejecute directamente.
						// Donde pedirá un manejador de errores y el tamaño del cuerpo.
				asio::async_write(this->m_socket, asio::buffer(&this->m_qMessagesOut.front().header, sizeof(message_header<T>)), [this](std::error_code ec, std::size_t length) {
						// Si no hay ningún error podemos continuar con la escritura del encabezado.
						if (!ec) {
							// Verificamos que haya tamaño para poder escribir.
							if (m_qMessagesOut.front().body.size() > 0) {
								// Si lo hay, escribimos en el cuerpo.
								WriteBody();
							}
							else {
								// Si no lo hay, lo eliminamos.
								m_qMessagesOut.pop_front();

								// Verificamos si no hay mensajes.
								if (!m_qMessagesOut.empty()) {
									// Si es así, volvemos a repetir el proceso.
									WriteHeader();
								}
							}
						}
						else {
							// Si hay algún error, debemos indicar que no se pudo escribir el encabezado, y cerramos la conexión.
							printf("[%u] La escritura del encabezado fallo.", id);
							m_socket.close();
						}
					});
			}

			// Método sincrónico, comprime el contexto listo para poder leer el cuerpo de un mensaje.
			void ReadBody() {
				// Le indicamos a asio que lea de forma sincrónica.
					// Dándole el socket de conexión, un buffer donde se guardara el mensaje con el tamaño del cuerpo del mensaje.
					// Y como en cada método donde hay algo sincrónico, creamos una función lambda para que ejecute directamente.
						// Donde pedirá un manejador de errores y el tamaño del cuerpo.
				asio::async_read(this->m_socket, asio::buffer(this->m_msgTemporaryIn.body.data(), this->m_msgTemporaryIn.body.size()), [this](std::error_code ec, std::size_t length) {
						// Si no hay ningún error podemos continuar con la lectura del cuerpo.
						if(!ec) {
							// Entonces si no hubo ningún error significa que tanto hay cuerpo como encabezado y se pueden procesar ambos.
							// Así que lo agregamos a la cola de mensajes.
							AddToIncomingMessageQueue();
						}
						else {
							// Si llega a esta parte es por que hubo algún error.
							// Así que notificamos de ello.
							printf("[%u] La lectura del cuerpo fallo.\n", id);

							// Y cerramos el socket para evitar flujos.
							m_socket.close();
						}
					});
			}

			// Método sincrónico, comprime el contexto listo para poder escribir el cuerpo de un mensaje.
			void WriteBody() {
				// Le indicamos  a asio que escriba de forma sincrónica.
					// Dándole el socket de conexión, un buffer donde se guardaran los mensajes salientes del cuerpo, y el tamaño.
					// Y como en cada método donde hay algo sincrónico, creamos una función lambda para que ejecute directamente.
						// Donde pedirá un manejador de errores y el tamaño del cuerpo.
				asio::async_write(this->m_socket, asio::buffer(this->m_qMessagesOut.front().body.data(), this->m_qMessagesOut.front().body.size()),
					[this](std::error_code ec, std::size_t length) {
						// Verificamos que no haya ningún error.
						if (!ec) {
							// Entonces si no hubo ningún error significa que tanto el cuerpo y el encabezado fueron escritos correctamente.
							// Así que el mensaje fue escrito y se puede eliminar de la cola de mensajes escritos y procesados.
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

			// Esta función permitira que si el que ejecuta este proceso es el servidor
			// permitirle que transforme los mensajes a mensajes con autor.
			void AddToIncomingMessageQueue() {
				if (this->m_nOwnerType == owner::server) {
					// Agregamos a la lista los mensajes entrantes para que sea compartidos en esta conexión.
					this->m_qMessagesIn.push_back({ this->shared_from_this(), m_msgTemporaryIn });
				}
				else {
					// Si no es owner, entonces agregamos únicamente los mensajes entrantes.
					this->m_qMessagesIn.push_back({ nullptr, m_msgTemporaryIn });
				}

				// Ya terminado de agregar los mensajes, le indicamos que vuelva leer otro mensaje.
				ReadHeader();
			}

		public:

			// Crearemos una numeración de tipo de autor de conexión.
			enum class owner {
				server,
				client
			};

			// Para crear la conexión, necesitaremos el padre de la conexión (quien crea la conexión), el contexto de la conexión,
			// el socket donde se hace el proceso y la cola de subprocesos seguro donde se recibirán los mensajes.
			connection(owner parent, asio::io_context& asioContext, asio::ip::tcp::socket socket, tsqueue<owned_message<T>>& qIn)
				: m_asioContext(asioContext), m_socket(std::move(socket)), m_qMessagesIn(qIn) {
				// Le establecemos quien es el nuevo autor de la conexión.
				this->m_nOwnerType = parent;
			}

			virtual ~connection() {}

			// Método que retorna la ID de la conexión.
			uint32_t GetID() const {
				return id;
			}

			// Método que asigna la ID al cliente siempre y cuando la conexión sea del servidor y también le permita recivir y mandar información.
			void ConnectToClient(uint32_t uid = 0) {
				// Verificamos que el que ejecuta esto, es el servidor, si lo es permitirá asignar la ID.
				if (this->m_nOwnerType == owner::server) {
					// Revisamos si al socket esta encendido.
					if (this->m_socket.is_open()) {
						// Y asignamos la ID.
						this->id = uid;

						// Ahora le indicamos al servidor que este a la espera de lectura de mensajes.
						this->ReadHeader();
					}
				}
			}

			// Método que conecta el cliente al servidor.
				// Nota, solo sirve si eres cliente.
			void ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints) {
				// Verificamos si es cliente el que esta ejecutando esto.
				if (m_nOwnerType == owner::client) {
					// Le pedimos a asio que intente conectarse al punto de la dirección dado.
					// Le damos como parámetro el socket, el punto de la dirección y la función lambda a ejecutar directamente.
						// La cual tendrá un manejador de errores y de igual forma el punto de la dirección.
					asio::async_connect(this->m_socket, endpoints, [this](std::error_code ec, asio::ip::tcp::endpoint endpoint) {
							// Verificamos que no haya errores.
							if (!ec) {
								// Si no hay errores, indicamos que empiece a leer mensajes.
								ReadHeader();
							}
						});
				}
			}
			
			// Método que cierra la conexión.
			void Disconnect() {
				// Verificamos que estemos conectados.
				if (this->IsConnected()) {
					// Para poder desconectarnos, debemos darle el contexto de la conexión e usar una
					// función lambda para cerrar directamente. Todo esto con el método post.
					asio::post(this->m_asioContext, [this]() { m_socket.close(); });
				}
			}

			// Método que retorna un valor booleano en caso de que hay una conexión establecida al servidor.
			bool IsConnected() const {
				return this->m_socket.is_open();
			};

			// Método envía el mensaje dado.
			void Send(const message<T>& msg) {
				// Le indicamos a asio que mande los datos con el método post, dandole así
				// El contexto donde se está trabajando y ejecutamos directamente el resultado con una
				// función lambda para hacer que el servidor este en el estado de escribir mensajes.
				asio::post(this->m_asioContext, [this, msg]() {
						
						// Verificamos si esta escribiendo más mensajes.
						bool bWritingMessage = !m_qMessagesOut.empty();

						// Primero agregamos el mensaje a la cola de mensajes de salida.
						m_qMessagesOut.push_back(msg);

						// Verificamos que no este escribiendo más mensajes.
						if (!bWritingMessage) {
							// Y finalmente empezamos el proceso de escribir.
							WriteHeader();
						}
					});
			}

		protected:
			// Cada conexión tiene un socket único para el control remoto.
			asio::ip::tcp::socket m_socket;

			// Este contexto es compartido con toda la instancia asio, ya que se debe usar el mismo contexto y no múltiples.
			asio::io_context& m_asioContext;

			// Esta cola de subprocesos sostiene todos los mensajes a ser enviado hacia el control remoto de esta conexión.
			tsqueue<message<T>> m_qMessagesOut;

			// Esta cola de sobprocesos sostiene todos los mensajes a ser recividos del control remoto de esta conexión.
			// Notar que es una referencía como el "propietario" de esta conexión, se espera que se provea una cola de subprocesos.
			tsqueue<owned_message<T>>& m_qMessagesIn;

			// Al igual que la variable del mismo tipo, esta variable se encargara de guardar los mensajes de forma temporal.
			message<T> m_msgTemporaryIn;

			// Creamos el tipo de autor para poder especificar que tipo de conexión es y haya un tipo de evento.
			// Notar que el "autor" decide como algunas conexiones se comportan.
			owner m_nOwnerType = owner::server;

			// Variable que guardara la ID de la conexión.
			uint32_t id = 0;

		};

	}
}