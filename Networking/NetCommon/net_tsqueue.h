#pragma once
#include "net_common.h"

// Debido a que es un namespace, si se vuelve a llamar en otro pragama,
// lo que sucede es que en sus m�todos se agregan haciendo que tenga a�n m�s elementos.
	// En esta librer�a se usara para poder procesar seguramente los subprocesos hechos por los clientes
	// en el servidor.

namespace cap {
	namespace net {

		// Est� clase se encargara de procesar la cola de subprocesos hechos de manera segura.
		template<typename T>
		class tsqueue {
		public:
			tsqueue() = default;
			tsqueue(const tsqueue<T>&) = delete;
			virtual ~tsqueue() { this->clear(); }

			// Retorna y mantiene el �tem al frente de la cola de subprocesos.
			const T& front() {
				// Protege a la variable para evitar problemas en caso de que se este ejecutando otra cosa.
				std::scoped_lock lock(muxQueue);
				return this->deqQueue.front();
			}

			// Retorna y mantiene el �tem en la parte trasera de la cola de subprocesos.
			const T& back() {
				// Protege a la variable para evitar problemas en caso de que se este ejecutando otra cosa.
				std::scoped_lock lock(muxQueue);
				return this->deqQueue.back();
			}

			// Agrega un �tem a la parte trasera de la cola de subprocesos.
			void push_back(const T& item) {
				// Protege a la variable para evitar problemas en caso de que se este ejecutando otra cosa.
				std::scoped_lock lock(muxQueue);

				// Llamaremos a la lista din�mica y agregaremos el �tem a la parte trasera usando la funci�n move.
				this->deqQueue.emplace_back(std::move(item));

				// Bloqueamos en caso de que haya subprocesos o procesos para evitar errores de supercarga.
				std::unique_lock<std::mutex> ul(muxBlocking);

				// Notificamos a la variable que se encarga de bloquear que lo haga para salir del modo de espera.
				this->cvBlocking.notify_one();
			}

			// Agrega un �tem a la parte trasera de la cola de subprocesos.
			void push_front(const T& item) {
				// Protege a la variable para evitar problemas en caso de que se este ejecutando otra cosa.
				std::scoped_lock lock(muxQueue);

				// Llamaremos a la lista din�mica y agregaremos el �tem a la parte frontera usando la funci�n move.
				this->deqQueue.emplace_front(std::move(item));

				// Bloqueamos en caso de que haya subprocesos o procesos para evitar errores de supercarga.
				std::unique_lock<std::mutex> ul(muxBlocking);

				// Notificamos a la variable que se encarga de bloquear que lo haga para salir del modo de espera.
				this->cvBlocking.notify_one();
			}

			// Retorna verdadero si la cola de subprocesos esta vac�a.
			bool empty() {
				// Protege a la variable para evitar problemas en caso de que se este ejecutando otra cosa.
				std::scoped_lock lock(muxQueue);
				return deqQueue.empty();
			}

			// Retorna el numero de �tems que tiene la cola de subprocesos.
			size_t count() {
				// Protege a la variable para evitar problemas en caso de que se este ejecutando otra cosa.
				std::scoped_lock lock(muxQueue);
				return this->deqQueue.size();
			}

			// Limpia la cola de subprocesos.
			void clear() {
				// Protege a la variable para evitar problemas en caso de que se este ejecutando otra cosa.
				std::scoped_lock lock(muxQueue);
				this->deqQueue.clear();
			}

			// Remueve y retorna el �tem de la parte frontera de la cola de subprocesos.
			T pop_front() {
				// Protege a la variable para evitar problemas en caso de que se este ejecutando otra cosa.
				std::scoped_lock lock(muxQueue);

				// Tomamos la parte frontera de la cola de subprocesos con la funci�n move, para as� evitar problemas en caso
				// de que se cambie o se mueva el �tem.
				auto t = std::move(deqQueue.front());

				// Hacemos que se elimine el �tem frontal.
				this->deqQueue.pop_front();

				return t;
			}

			// Remueve y retorna el �tem de la parte trasera de la cola de subprocesos.
			T pop_back() {
				// Protege a la variable para evitar problemas en caso de que se este ejecutando otra cosa.
				std::scoped_lock lock(muxQueue);

				// Tomamos la parte trasera de la cola de subprocesos con la funci�n move, para as� evitar problemas en caso
				// de que se cambie o se mueva el �tem.
				auto t = std::move(deqQueue.back());

				// Hacemos que se elimine el �tem trasera.
				this->deqQueue.pop_back();

				return t;
			}

			// Hace que los procesos sean protegidos hasta que deje de estar vacio la cola de subprocesos.
			void wait() {
				// Mientras la cola de subprocesos este vac�a, el server descansara hasta que tenga nuevos procesos.
				while (this->empty()) {
					// Bloqueamos para evitar problemas.
					std::unique_lock<std::mutex> ul(muxBlocking);

					// Y indicamos a la variable que se quede a la espera.
					this->cvBlocking.wait(ul);
				}
			}

		private:

		protected:
			
			// Variable que nos servir� en caso de que el tsqueue necesito permisos exclusivos cuando llegue a cierto punto del c�digo.
			// Y tambi�n nos permite bloquear ciertos objetos.
			std::mutex muxQueue;

			// Variable que nos permitir� tener una lista din�mica que puede crecer hacia delante o hacia atr�s que nos ayudara a manipular
			// los subprocesos.
			std::deque<T> deqQueue;

			// Variable que bloqueara cualquier tipo de proceso o subproceso que este ejecutandose en el momento, esto servirara para evitar fallas.
			std::condition_variable cvBlocking;

			// Variable que nos servira en caso de que la variable cvBlocking necesite permisos.
			std::mutex muxBlocking;

		};
	}
}