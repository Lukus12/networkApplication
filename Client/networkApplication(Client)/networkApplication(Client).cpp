#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <inaddr.h>
#include <stdio.h>
#include <vector>
#pragma comment(lib, "Ws2_32.lib")

using namespace std;

const char SERVER_IP[] = "127.0.0.1";		// IPv4-адрес сервера
const short SERVER_PORT_NUM = 5678;	//  порт прослушивания на стороне сервера
const short BUFF_SIZE = 1024;		// Максимальный размер буфера для обмена информацией между сервером и клиентом

int erStat;
in_addr ip_to_num;
SOCKET ClientSock;
vector <char> servBuff(BUFF_SIZE), clientBuff(BUFF_SIZE);	// Создание буферов для отправки и получения данных
short packet_size;

void ClientHandler() {
	while (true) {
		packet_size = recv(ClientSock, servBuff.data(), servBuff.size(), 0);
		if (packet_size == SOCKET_ERROR) {
			cout << "Can't receive message from Server. Error # " << WSAGetLastError() << endl;
			closesocket(ClientSock);
			WSACleanup();
			return;
		}
		else {
			cout << endl << "Incoming message:" << servBuff.data() << endl;
			cout << "Send a message: ";
		}

	}
}

int main()
{
	//1. --проверка на возможность использования сокетов--//
	inet_pton(AF_INET, SERVER_IP, &ip_to_num);

	WSADATA wsData;
	int erStat = WSAStartup(MAKEWORD(2, 2), &wsData);
	//int WSAStartup (WORD <запрашиваемая версия сокетов>, WSADATA* <указатель на структуру, хранящую текущую версию реализации сокетов>)

	if (erStat != 0) {
		cout << "Error WinSock version initializaion #";
		cout << WSAGetLastError();
		return 1;
	}
	else
		cout << "WinSock initialization is OK" << endl;
	//2.--создание сокета и его инициализация--//

	ClientSock = socket(AF_INET, SOCK_STREAM, 0);
	//SOCKET socket(int <семейство используемых адресов>, int <тип сокета>, int <тип протокола>)
	// IPv4 указывается как AF_INET; TCP (SOCK_STREAM);  если тип сокета указан как TCP или UDP – можно передать значение 0

	//Функция socket() возвращает дескриптор с номером сокета, под которым он зарегистрирован в ОС.
	//Если же инициализировать сокет по каким-то причинам не удалось – возвращается значение INVALID_SOCKET
	if (ClientSock == INVALID_SOCKET) {
		cout << "Error initialization socket # " << WSAGetLastError() << endl;
		closesocket(ClientSock);
		WSACleanup();
		return 1;
	}
	else
		cout << "Server socket initialization is OK" << endl;
	//--Клиент--Организация подключения к серверу---//

	/*необходимо исполнение Этапов 1, 2 . Привязка сокета к конкретному процессу (bind()) не требуется,
	т.к. сокет будет привязан к серверному Адресу и Порту через вызов функции connect()(по сути аналог bind() для Клиента).*/
	/*
		Процедура по добавлению данных в структуру sockaddr аналогична тому, как это делалось на Этапе 3
		для Сервера при вызове функции bind(). Принципиально важный момент – в эту структуру для клиента
		должна заноситься информация о сервере, т.е. IPv4-адрес сервера и номер «слушающего» порта на сервере.
	*/

	sockaddr_in servInfo;

	ZeroMemory(&servInfo, sizeof(servInfo));

	servInfo.sin_family = AF_INET;
	servInfo.sin_addr = ip_to_num;	  // получение IPv4 сервера с помощью функции inet_pton()
	servInfo.sin_port = htons(SERVER_PORT_NUM);

	erStat = connect(ClientSock, (sockaddr*)&servInfo, sizeof(servInfo));

	if (erStat != 0) {
		cout << "Connection to Server is FAILED. Error # " << WSAGetLastError() << endl;
		closesocket(ClientSock);
		WSACleanup();
		return 1;
	}
	else
		cout << "Connection established SUCCESSFULLY. Ready to send a message to Server" << endl;


	//-- Чат между сервером и Клиентом. Отключение, если клиент отправляет "EXIT" --//

	//packet_size = 0;				// Размер отправляемого/принимаемого пакета в байтах

	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ClientHandler, NULL, NULL, NULL);

	while (true) {

		cout << "Send a message: ";
		cin.getline(clientBuff.data(), clientBuff.size());
		packet_size = send(ClientSock, clientBuff.data(), clientBuff.size(), 0);
		if (packet_size == SOCKET_ERROR) {
			cout << "Can't send message to Server. Error # " << WSAGetLastError() << endl;
			closesocket(ClientSock);
			WSACleanup();
			return 1;
		}

		Sleep(20);

		// Проверка на закрытие чата 
		if (clientBuff[0] == 'E' && clientBuff[1] == 'X' && clientBuff[2] == 'I' && clientBuff[3] == 'T') {
			shutdown(ClientSock, SD_BOTH);
			closesocket(ClientSock);
			WSACleanup();
			return 0;
		}

	}

	closesocket(ClientSock);
	WSACleanup();
}