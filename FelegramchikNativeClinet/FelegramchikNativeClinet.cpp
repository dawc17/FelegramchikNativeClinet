#include <iostream>
#include <asio.hpp>

int main()
{
	try {
		asio::io_context io_context;

		asio::ip::address address = asio::ip::make_address("127.0.0.1");
		asio::ip::tcp::endpoint endpoint(address, 1337);
		asio::ip::tcp::socket socket(io_context);

		socket.connect(endpoint);
		std::cout << "Connected to server. \n";

		std::cout << "Enter message: ";
		std::string message;
		std::getline(std::cin, message);
		asio::write(socket, asio::buffer(message));

		std::array<char, 1024> buffer;
		size_t length = socket.read_some(asio::buffer(buffer));
		std::cout << "Reply from server: ";
		std::cout.write(buffer.data(), length);
		std::cout << std::endl;
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
	return 0;
}

