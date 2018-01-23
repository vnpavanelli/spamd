#include <ctime>
#include <future>
#include <iostream>
#include <string>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/regex.hpp>

using namespace std;
using boost::asio::ip::tcp;

const bool DEBUG = false;

std::string make_daytime_string()
{
  using namespace std; // For time_t, time and ctime;
  time_t now = time(0);
  return ctime(&now);
}

class tcp_connection
  : public boost::enable_shared_from_this<tcp_connection>
{
	public:
		typedef boost::shared_ptr<tcp_connection> pointer;

		static pointer create(boost::asio::io_service& io_service)
		{
			return pointer(new tcp_connection(io_service));
		}

		tcp::socket& socket()
		{
			return socket_;
		}

		void empata_string(const char *ptr) {
			if (ptr == nullptr) return;
			while (*ptr != '\0' && socket_.is_open()) {
				try {
					socket_.send(boost::asio::buffer(ptr, 1));
				} catch (std::exception &e) {
					cout << "Excecao: " << e.what() << endl;
					socket_.close();
					return;
				}
				// usleep(1e5);
				sleep(13);
				ptr++;
			}
		}

		string le(void) {
			boost::asio::streambuf b;
			try {
				boost::asio::read_until(socket_, b, "\r\n");
			} catch (std::exception &e) {
				cout << "le: excecao: " << e.what() << endl;
				socket_.close();
				return("");
			}
			std::istream is(&b);
			std::string line;
			std::getline(is, line, '\r'); 
			return line;
		}

		bool captura_regex(string const &in, string const &r_str, string &out) {
			boost::regex r(r_str);
			boost::smatch sm;
			if (boost::regex_match(in, sm, r)) {
				out = sm[2].str();
				return true;
			} else {
				return false;
			}
		}

		void start()
		{
			// FAZ A EMPATA FODA
			int estado = 0;
			string vitima, vitima_email, vitima_destino, vitima_ip;
			time_t inicio = time(0);
			vitima_ip =  socket_.remote_endpoint().address().to_string();
			cout << "Empatando IP " << vitima_ip << endl;
			while (socket_.is_open()) switch (estado) {
				case 0:
					empata_string("220 mail.wexperts.com.br WSPAMD\r\n");
					if(DEBUG)            cout << "cabecalho enviado" << endl;
					if (socket_.is_open()) estado = 1;
					else estado = 7;
					break;
				case 1:
					{
						string comando = le();
						if (!socket_.is_open()) {
							estado = 7;
							break;
						}
						if(DEBUG)            cout << "Linha lida1: " << comando << endl;
						if (captura_regex(comando, "^(HELO|EHLO) (.*)$", vitima)) {
							if(DEBUG)	    	cout << "Vitima host: " << vitima << endl;
							if (vitima.empty()) {
								empata_string("501 helo requires domain name\r\n");
								if (!socket_.is_open()) { estado = 7; break; }
								estado = 1;
							} else {
								estado = 3;
							}
						} else {
							estado = 2;
						}
					}
					break;
				case 2:
					empata_string("250 You are about to try to deliver spam.\r\n");
					if (!socket_.is_open()) { estado = 7; break; }
					estado = 1;
					break;
				case 3:
					empata_string("250 Helo spam sender, nice to waste your time\r\n");
					if (!socket_.is_open()) { estado = 7; break; }
					estado = 4;
					break;
				case 4:
					{
						string comando = le();
						if (!socket_.is_open()) { estado = 7; break; }
						if(DEBUG)            cout << "Linha lida4: " << comando << endl;
						if (captura_regex(comando, "^(MAIL FROM:)(.*)$", vitima_email)) {
							if(DEBUG)                cout << "Vitima email: " << vitima_email << endl;
							empata_string("250 Your time will be spent for nothing, dont spam me\r\n");
							if (!socket_.is_open()) { estado = 7; break; }
							estado = 5;
						} else {
							empata_string("500 5.5.1 Command unrecognized\r\n");
							if (!socket_.is_open()) { estado = 7; break; }
						}
					}
					break;
				case 5:
					{
						string comando = le();
						if (!socket_.is_open()) { estado = 7; break; }
						if(DEBUG)            cout << "Linha lida5: " << comando << endl;
						if (captura_regex(comando, "^(RCPT TO:)(.*)$", vitima_destino)) {
							if(DEBUG)                cout << "Vitima destino: " << vitima_destino << endl;
							empata_string("250 Your time will be spent for nothing, dont spam me\r\n");
							if (!socket_.is_open()) { estado = 7; break; }
							estado = 6;
						} else {
							empata_string("500 5.5.1 Command unrecognized\r\n");
							if (!socket_.is_open()) { estado = 7; break; }
						}
					}
					break;
				case 6:
					{
						string comando = le();
						if (!socket_.is_open()) { estado = 7; break; }
						if(DEBUG)            cout << "Linha lida6: " << comando << endl;
						string nada;
						if (captura_regex(comando, "^(DATA)(.*)$", nada)) {
							//                cout << "DATA recebido " << endl;
							empata_string("354 Enter spam, end with \".\" on a line by itself\r\n");
							estado = 7;
						} else {
							empata_string("500 5.5.1 Command unrecognized\r\n");
							if (!socket_.is_open()) { estado = 7; break; }
						}
					}
					break;
				case 7:
					boost::asio::streambuf b;
					try {
						boost::asio::read_until(socket_, b, "\r\n.\r\n");
					} catch (std::exception &e) {
						//                cout << "Excecao em DATA: " << e.what() << endl;
					}
					if (socket_.is_open()) empata_string("451 Temporary failure, please try again later.\r\n");
					time_t fim = time(0);
					cout << "Vitima " << vitima << "[" << vitima_ip << "] empatada por " << fim-inicio << " segundos, enviando email de " << vitima_email << " para " << vitima_destino << endl;
					socket_.close();
			}
		}

	private:
		tcp_connection(boost::asio::io_service& io_service)
			: socket_(io_service)
		{
		}

		void handle_write(const boost::system::error_code& /*error*/,
				size_t /*bytes_transferred*/)
		{
		}

		tcp::socket socket_;
		std::string message_;
};

class tcp_server
{
public:
  tcp_server(boost::asio::io_service& io_service)
    : acceptor_(io_service, tcp::endpoint(tcp::v4(), 1025))
  {
    start_accept();
  }

private:
  void start_accept()
  {
    tcp_connection::pointer new_connection =
      tcp_connection::create(acceptor_.get_io_service());

    acceptor_.async_accept(new_connection->socket(),
        boost::bind(&tcp_server::handle_accept, this, new_connection,
          boost::asio::placeholders::error));
  }

  void handle_accept(tcp_connection::pointer new_connection,
      const boost::system::error_code& error)
  {
    if (!error)
    {
    	std::thread([new_connection](){ new_connection->start(); }).detach();
    }
    start_accept();
  }

  tcp::acceptor acceptor_;
};

int main()
{
    cout << "Iniciando WSPAMD" << endl;
  try
  {
    boost::asio::io_service io_service;
    tcp_server server(io_service);
    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }
  return 0;
}
