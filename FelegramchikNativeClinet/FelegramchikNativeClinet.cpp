#include <iostream>
#include <asio.hpp>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <array>

std::mutex cout_mutex;

void clear_previous_line() {
    // \033[A moves cursor up 1 line
    // \r moves cursor to beginning of line
    // \033[K clears from cursor to end of line
    std::cout << "\033[A\r\033[K";
}

int main()
{
    try {
        asio::io_context io_context;

        asio::ip::address address = asio::ip::make_address("127.0.0.1");
        asio::ip::tcp::endpoint endpoint(address, 1337);
        asio::ip::tcp::socket socket(io_context);

        socket.connect(endpoint);

        std::cout << "Connected to server at " << endpoint << "\n";

        // Prompt for and send nickname
        std::cout << "Please enter your nickname: ";
        std::string nickname;
        std::getline(std::cin, nickname);
        asio::write(socket, asio::buffer(nickname + "\n"));

        std::cout << "\nWelcome to the chat! Type 'quit' to exit.\n\n";

        std::atomic<bool> running(true);

        // thread for receiving messages from server
        std::thread receive_thread([&socket, &running]() {
            try {
                std::array<char, 1024> buffer;
                while (running) {
                    asio::error_code ec;
                    size_t length = socket.read_some(asio::buffer(buffer), ec);

                    if (ec == asio::error::eof || !running) {
                        if (running) {
                            std::lock_guard<std::mutex> lock(cout_mutex);
                            std::cout << "\rServer disconnected.           " << std::endl;
                        }
                        running = false;
                        break;
                    }
                    else if (ec) {
                        std::lock_guard<std::mutex> lock(cout_mutex);
                        std::cout << "\rRead error: " << ec.message() << std::endl;
                        running = false;
                        break;
                    }

                    if (length > 0) {
                        std::lock_guard<std::mutex> lock(cout_mutex);

                        // clear the prompt line, print the message, then reprint the prompt
                        std::cout << "\r" << std::string(80, ' ') << "\r";

                        std::string message(buffer.data(), length);
                        std::cout << message;

                        std::cout << "Enter message: " << std::flush;
                    }
                }
            }
            catch (const std::exception&) {
                running = false;
            }
            });

        // main thread for sending messages
        while (running) {
            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "Enter message: " << std::flush;
            }

            std::string message;
            if (!std::getline(std::cin, message) || message == "quit") {
                running = false;
                break;
            }

            if (!message.empty()) {
                // move cursor up and clear the "Enter message: ..." line
                clear_previous_line();

                asio::error_code ec;
                asio::write(socket, asio::buffer(message + "\n"), ec);

                if (ec) {
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cout << "\rWrite error: " << ec.message() << std::endl;
                    running = false;
                    break;
                }
            }
            else {
                // clear the prompt and reprint it
                clear_previous_line();
            }
        }

        // cleanup
        running = false;
        socket.close();
        if (receive_thread.joinable()) {
            receive_thread.join();
        }

        std::cout << "Disconnected from server." << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}