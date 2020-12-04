#pragma once

#include "net_common.h"
#include "net_tsqueue.h"
#include "net_message.h"
#include "net_connection.h"

namespace cap {
	namespace net {

		// Esta clase se encargara de hacer los procesos guardados en la cosa de subprocesos.
		// También es la clase principal que nos permitira crear el servidor el cual también
		// manipulara quienes pueden conectarse y como seran tratados.
		template <typename T>
		class server_interface {
		private:

		protected:

		public:
			server_interface(uint16_t port) {
			}

			virtual ~server_interface() {

			}

			bool Start() {

			}

			void Stop() {

			}
		};

	}
}