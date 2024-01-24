import socket
import threading

def handle_client(client_socket, address):
	print(f"Connection from {address} has been established.")
	while True:
		try:
			msg = client_socket.recv(1024)  # Receive data from the client
			if not msg:
				break  # No message means the client has disconnected
			print(f"Message from {address}: {msg.decode('utf-8')}")
		except ConnectionResetError:
			print(f"Connection lost with {address}")
			break
	client_socket.close()
	print(f"Connection closed with {address}")

def main():
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	s.bind(('0.0.0.0', 13200))  # Bind to IP and port
	s.listen(5)  # Listen for incoming connections
	print("Server is waiting for a connection...")

	while True:
		client_socket, address = s.accept()  # Accept a connection
		client_thread = threading.Thread(target=handle_client, args=(client_socket, address))
		client_thread.start()

if __name__ == "__main__":
	main()
