#pragma once
#include "net_common.h"

// Debido a que es un namespace, si se vuelve a llamar en otro pragama,
// lo que sucede es que en sus métodos se agregan haciendo que tenga aún más elementos.
	// En esta librería se usara para poder procesar seguramente los subprocesos hechos por los clientes
	// en el servidor.

namespace cap {
	namespace net {

		// Está clase se encargara de procesar la cola de subprocesos hechos de manera segura.
		template <typename T>
		class tsqueue {
		public:
			tsqueue() = default;
			tsqueue(const tsqueue<T>&) = delete;
			virtual ~tsqueue() { clear(); }

			// Retorna y mantiene el ítem al frente de la cola de subprocesos.
			const T& front() {
				// Protege a la variable para evitar problemas en caso de que se este ejecutando otra cosa.
				std::scoped_lock lock(this->muxQueue);
				return this->deqQueue.front();
			}

			// Retorna y mantiene el ítem en la parte trasera de la cola de subprocesos.
			const T& back() {
				// Protege a la variable para evitar problemas en caso de que se este ejecutando otra cosa.
				std::scoped_lock lock(this->muxQueue);
				return this->deqQueue.back();
			}

			// Agrega un ítem a la parte trasera de la cola de subprocesos.
			void push_back(const T& item) {
				// Protege a la variable para evitar problemas en caso de que se este ejecutando otra cosa.
				std::scoped_lock lock(this->muxQueue);

				// Llamaremos a la lista dinámica y agregaremos el ítem a la parte trasera usando la función move.
				this->deqQueue.emplace_back(std::move(item));
			}

			// Agrega un ítem a la parte trasera de la cola de subprocesos.
			void push_front(const T& item) {
				// Protege a la variable para evitar problemas en caso de que se este ejecutando otra cosa.
				std::scoped_lock lock(this->muxQueue);

				// Llamaremos a la lista dinámica y agregaremos el ítem a la parte frontera usando la función move.
				this->deqQueue.emplace_front(std::move(item));
			}

			// Retorna verdadero si la cola de subprocesos esta vacía.
			bool empty() {
				// Protege a la variable para evitar problemas en caso de que se este ejecutando otra cosa.
				std::scoped_lock lock(this->muxQueue);
				return deqQueue.empty();
			}

			// Retorna el numero de ítems que tiene la cola de subprocesos.
			size_t count() {
				// Protege a la variable para evitar problemas en caso de que se este ejecutando otra cosa.
				std::scoped_lock lock(this->muxQueue);
				return this->deqQueue.size();
			}

			// Limpia la cola de subprocesos.
			void clear() {
				// Protege a la variable para evitar problemas en caso de que se este ejecutando otra cosa.
				std::scoped_lock lock(this->muxQueue);
				this->deqQueue.clear();
			}

			// Remueve y retorna el ítem de la parte frontera de la cola de subprocesos.
			T pop_front() {
				// Protege a la variable para evitar problemas en caso de que se este ejecutando otra cosa.
				std::scoped_lock lock(this->muxQueue);

				// Tomamos la parte frontera de la cola de subprocesos con la función move, para así evitar problemas en caso
				// de que se cambie o se mueva el ítem.
				auto t = std::move(this->deqQueue.front());

				// Hacemos que se elimine el ítem frontal.
				this->deqQueue.pop_front();

				return t;
			}

			// Remueve y retorna el ítem de la parte trasera de la cola de subprocesos.
			T pop_front() {
				// Protege a la variable para evitar problemas en caso de que se este ejecutando otra cosa.
				std::scoped_lock lock(this->muxQueue);

				// Tomamos la parte trasera de la cola de subprocesos con la función move, para así evitar problemas en caso
				// de que se cambie o se mueva el ítem.
				auto t = std::move(this->deqQueue.front());

				// Hacemos que se elimine el ítem trasera.
				this->deqQueue.pop_front();

				return t;
			}

		private:

		protected:
			
			// Variable que nos servirá en caso de que el tsqueue necesito permisos exclusivos cuando llegue a cierto punto del código.
			// Y también nos permite bloquear ciertos objetos.
			std::mutex muxQueue;

			// Variable que nos permitirá tener una lista dinámica que puede crecer hacia delante o hacia atrás que nos ayudara a manipular
			// los subprocesos.
			std::deque<T> deqQueue;

		};
	}
}