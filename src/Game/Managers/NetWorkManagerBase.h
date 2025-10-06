#pragma once

#include <mutex>

// パケット種別
//パケット種数が増えたら適宜追加してください
enum PacketType : u32 {
	PACKET_TYPE_WAVE = 0,
	PACKET_TYPE_TEXT = 1,
	PACKET_TYPE_PLAYER_POSITION = 2,
	PACKET_TYPE_COMMAND = 3,
};

struct PacketHeader {
	IPDATA ip;
	u32  type;      // PacketType
	u32 sizeBytes; // 後続ペイロードサイズ
};

// TCP通信を行うためのクラス
class NetWork {
public:
	int handle = -1;
	std::array<u8, 65536> buffer;
	IPDATA ip;
	const unsigned long long unique_id;
	std::function<void(void* data, size_t length)> on_receive;
	std::function<void()> on_disconnect;
	void Send(const void* data, size_t data_size);

	NetWork(int handle, IPDATA other_ip, unsigned long long id) :unique_id(id) {
		this->handle = handle;
		this->ip = other_ip;
		buffer.fill(0);
	}
	~NetWork();
};

// UDP通信を行うためのクラス
class UDPNetWork {

	//UDP通信は、TCPと違い、接続という概念がない
	//そのため、on_disconnectのようなコールバックは存在しない
	//また、handleも存在しない(ソケットは1つだけ)
public:
	int socket = -1;
	unsigned short port = 0;
	std::array<u8, 65536> buffer;

	std::function<void(void* data, size_t length)> on_receive;
	void Send(IPDATA ip, unsigned short port, const void* data, size_t data_size) const;
	UDPNetWork(int socket, unsigned short port) {
		this->socket = socket;
		this->port = port;
		buffer.fill(0);
	}
	~UDPNetWork() {
		if (socket != -1) {
			DxLib::DeleteUDPSocket(socket);
		}
	}

};

class NetWorkManagerBase
{
protected:
	std::mutex mutex_;
	std::vector<std::unique_ptr<NetWork>> networks;
	std::unique_ptr<UDPNetWork> udp_network;
	std::function<void(NetWork*)> on_new_connection;
	std::function<void()> on_disconnection;
	std::thread check_connection_thread;
	std::thread check_disconnection_thread;
	IPDATA my_ip = { 127,0,0,1 };
	unsigned short port_num = 0;
	unsigned short udp_port_num = 0;


private:
	bool kill_thread_flag = false;
	void CheckForNewConnect(const bool& finish_flag);
	void CheckForDisConnect(const bool& finish_flag);
public:
	void SetOnNewConnectionCallback(std::function<void(NetWork*)> func) { on_new_connection = func; }
	void SetOnDisconnectionCallback(std::function<void()> func) { on_disconnection = func; }
	void Update();
	NetWork* Connect(IPDATA other, unsigned short port,
		std::function<void(NetWork*)> on_connect = nullptr,
		std::function<void()> on_disconnect = nullptr);
	NetWorkManagerBase(int mode = 0, unsigned short port = 35000);
	virtual ~NetWorkManagerBase() {
		kill_thread_flag = true;	// スレッド終了フラグを立てる
		// スレッド終了まで待機
		{
			if (check_connection_thread.joinable())
				check_connection_thread.join();
			if (check_disconnection_thread.joinable())
				check_disconnection_thread.join();
		}
	};
	UDPNetWork* OpenUDPSocket(unsigned short port = 35001);
	UDPNetWork* GetUDPSocket() const { return udp_network.get(); }
	const IPDATA& GetMyIP() const { return my_ip; }
	const unsigned short& GetPort() const { return port_num; }
	const unsigned short& GetUDPPort() const { return udp_port_num; }
	std::vector<char> CreatePacket(PacketType type, const void* payload, u32 sizeBytes, IPDATA overrided_ip = { 0,0,0,0 });
public:
	static constexpr int NETWORK_MANAGER_MODE_LISTEN = 0;
	static constexpr int NETWORK_MANAGER_MODE_CONNECT = 1;
	static constexpr int NETWORK_MANAGER_MODE_BOTH = 2;

};
// IPDATA同士の比較(DxLibにはなぜか存在しない)
constexpr bool operator== (const IPDATA& lhs, const IPDATA& rhs) {
	return (lhs.d1 == rhs.d1) && (lhs.d2 == rhs.d2) && (lhs.d3 == rhs.d3) && (lhs.d4 == rhs.d4);
}
