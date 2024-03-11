#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <vector>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

const char IP_SERV[] = "127.0.0.1";		// IPv4-адрес сервера
const int PORT_NUM = 5678;				// порт прослушивания на стороне сервера
const short BUFF_SIZE = 1024;			// Максимальный размер буфера для обмена информацией между сервером и клиентом

SOCKET connections[100];
SOCKET ClientConn, ServSock;
int index = 0;


//-- Чат между сервером и Клиентом. Отключение, если клиент отправляет "EXIT" --//
void CustomerChat(int id) {
	vector <char> servBuff(BUFF_SIZE), clientBuff(BUFF_SIZE);
	while (true) {
		recv(connections[id], servBuff.data(), servBuff.size(), 0); // принимаем сообщение клиента
		for (int i = 0; i < index; i++) { // отправялем полученное сообшение всем остальным клиентам
			if (i == id) { // исключаем клиента, который отправил нам это сообщение
				continue;
			}
			short packet_size = send(connections[i], servBuff.data(), servBuff.size(), 0);
			if (packet_size == SOCKET_ERROR) {
				cout << "Can't send message to Client. Error # " << WSAGetLastError() << endl;
				closesocket(ServSock);
				closesocket(ClientConn);
				WSACleanup();
				return;
			}
		}
	}

	closesocket(ServSock);
	closesocket(ClientConn);
	WSACleanup();
}

int main() {

	int erStat;	// код ошибки
	in_addr ip_to_num; //Результат перевода IP-адреса будет содержится в структуре ip_to_num
	erStat = inet_pton(AF_INET, IP_SERV, &ip_to_num); //inet_pton- функция, которая переводит обычную строку типа char[],
	//содержащую IPv4 адрес в привычном виде с точками - разделителями в структуру типа in_addr

	if (erStat <= 0) {
		cout << "Error in IP translation to special numeric format" << endl;
		return 1;
	}
	//1. --проверка на возможность использования сокетов--//
	WSADATA wsData;

	erStat = WSAStartup(MAKEWORD(2, 2), &wsData);
	//int WSAStartup (WORD <запрашиваемая версия сокетов>, WSADATA* <указатель на структуру, хранящую текущую версию реализации сокетов>)

	if (erStat != 0) {
		cout << "Error WinSock version initializaion #";
		cout << WSAGetLastError();
		return 1;
	}
	else
		cout << "WinSock initialization is OK" << endl;

	//2.--создание сокета и его инициализация--//

	ServSock = socket(AF_INET, SOCK_STREAM, 0);
	//SOCKET socket(int <семейство используемых адресов>, int <тип сокета>, int <тип протокола>)
	// IPv4 указывается как AF_INET; TCP (SOCK_STREAM);  если тип сокета указан как TCP или UDP – можно передать значение 0

	//Функция socket() возвращает дескриптор с номером сокета, под которым он зарегистрирован в ОС.
	//Если же инициализировать сокет по каким-то причинам не удалось – возвращается значение INVALID_SOCKET
	if (ServSock == INVALID_SOCKET) {
		cout << "Error initialization socket # " << WSAGetLastError() << endl;
		closesocket(ServSock);
		WSACleanup();
		return 1;
	}
	else
		cout << "Server socket initialization is OK" << endl;

	//3. --Привязка сокета к паре IP-адрес/Порт-- //


	//нужно явно указать адрес и порт для привязки сокета. Для этого можно воспользоваться другой структурой,
	//родственной sockaddr, которая легко приводится к этому типу - структурой типа sockaddr_in.

	//------Преобразование IPv4 адреса для подключения сокета к локальному сервера------//


	//ввод данных для структуры типа sockaddr_in
	sockaddr_in servInfo; //тут будет хранится вся нужная информация для привязки сокета
	ZeroMemory(&servInfo, sizeof(servInfo));

	servInfo.sin_family = AF_INET; //TCP
	servInfo.sin_addr = ip_to_num; //преобразованный IP
	servInfo.sin_port = htons(PORT_NUM); //порт. Всегда указывается через вызов функции htons()

	// int bind(SOCKET <имя сокета, к которому необходимо привязать адрес и порт>, sockaddr * < указатель на структуру,
	//содержащую детальную информацию по адресу и порту, к которому надо привязать сокет>, int <размер структуры, содержащей адрес и порт>)
	//Функция bind() возвращает 0, если удалось успешно привязать сокет к адресу и порту, 
	// и код ошибки в ином случае, который можно расшифровать вызовом WSAGetLastError()

	erStat = bind(ServSock, (sockaddr*)&servInfo, sizeof(servInfo));
	if (erStat != 0) {
		cout << "Error Socket binding to server info. Error # " << WSAGetLastError() << endl;
		closesocket(ServSock);
		WSACleanup();
		return 1;
	}
	else
		cout << "Binding socket to Server info is OK" << endl;

	//--Сервер--«Прослушивание» привязанного порта для идентификации подключений---//

	/* int listen(SOCKET <«слушающий» сокет, который мы создавали на предыдущих этапах>,
	int <максимальное количество процессов, разрешенных к подключению>) */
	/* После вызова данной функции исполнение программы приостанавливается до тех пор,
	пока не будет соединения с Клиентом, либо пока не будет возвращена ошибка прослушивания порта */
	erStat = listen(ServSock, SOMAXCONN);

	if (erStat != 0) {
		cout << "Can't start to listen to. Error # " << WSAGetLastError() << endl;
		closesocket(ServSock);
		WSACleanup();
		return 1;
	}
	else {
		cout << "Listening..." << endl;
	}

	//-- Подключение клиента --//


	//-- Подтверждение подключения --//
	/*
		После начала прослушивания (вызов функции listen()) следующей функцией должна идти функция accept(),
		которую будет искать программа после того, как установится соединение с Клиентом
	*/
	sockaddr_in clientInfo; //данные подключившегося Клиента после вызова accept()

	ZeroMemory(&clientInfo, sizeof(clientInfo));

	int clientInfo_size = sizeof(clientInfo); //преобразования для функиции accept

	/*
		SOCKET accept(SOCKET <"слушающий" сокет на стороне Сервера>, sockaddr* <указатель на пустую структуру sockaddr,
		в которую будет записана информация по подключившемуся Клиенту>, int* <указатель на размер структуры типа sockaddr>)
	*/
	/*
		Если подключение подтверждено, то вся информация по текущему соединению передаётся на новый сокет,
		который будет отвечать со стороны Сервера за конкретное соединение с конкретным Клиентом.
	*/
	for (int i = 0; i < 100; i++) {

		ClientConn = accept(ServSock, (sockaddr*)&clientInfo, &clientInfo_size);

		if (ClientConn == INVALID_SOCKET) {
			cout << "Client detected, but can't connect to a client. Error # " << WSAGetLastError() << endl;
			closesocket(ServSock);
			closesocket(ClientConn);
			WSACleanup();
			return 1;
		}
		else {
			cout << "Client Connection!" << endl;
			char clientIP[22];

			inet_ntop(AF_INET, &clientInfo.sin_addr, clientIP, INET_ADDRSTRLEN);	// Преобразуем IP-адрес подключенных 
			//клиентов в стандартный строковый формат

			cout << "Client connected with IP address " << clientIP << endl;

		}

		connections[i] = ClientConn;
		index++;
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)CustomerChat, (LPVOID)(i), NULL, NULL); //создание новых потоков для работы с несколькими клиентами
	}


	return 0;
}

