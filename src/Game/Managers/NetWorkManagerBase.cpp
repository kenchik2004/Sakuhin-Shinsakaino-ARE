#include "precompile.h"
#include "NetWorkManagerBase.h"


void NetWork::Send(const void* data, size_t data_size)
{
	//ハンドルが無効なら送信しない
	if (handle != -1)
	{
		DxLib::NetWorkSend(handle, data, static_cast<int>(data_size));
	}
}

NetWork::~NetWork()
{
	//ハンドルが有効なら切断する
	CloseNetWork(handle);
	//切断コールバックを呼ぶ
	if (on_disconnect)
		on_disconnect();
}

void UDPNetWork::Send(IPDATA ip, unsigned short port, const void* data, size_t data_size) const {
	if (socket != -1) {
		DxLib::NetWorkSendUDP(socket, ip, port, data, static_cast<int>(data_size));

	}
}


NetWorkManagerBase::NetWorkManagerBase(int mode, unsigned short port) :port_num(port)
{
	//モードに応じて、リッスンを開始する
	if (mode == NETWORK_MANAGER_MODE_LISTEN || mode == NETWORK_MANAGER_MODE_BOTH)
	{

		int result = DxLib::PreparationListenNetWork(port_num);
		//失敗の場合、終了
		if (result == -1)
		{
			return;
		}
	}
	//自分のIPアドレスを取得
	DxLib::GetMyIPAddress(&my_ip);

	//新規接続と切断を監視するスレッドを起動
	//新規接続検出スレッド
	auto check_connection_lambda = [this]() {
		CheckForNewConnect(kill_thread_flag);
		};
	check_connection_thread = std::thread(check_connection_lambda);

	//切断検出スレッド
	auto check_disconnection_lambda = [this]() {
		CheckForDisConnect(kill_thread_flag);
		};
	check_disconnection_thread = std::thread(check_disconnection_lambda);

}

// 新しくUDPソケットを開く
UDPNetWork* NetWorkManagerBase::OpenUDPSocket(unsigned short port)
{
	if (udp_network)
		return nullptr; // 既に開いている

	//UDPソケットを作成
	int socket = DxLib::MakeUDPSocket(port);
	//作成に成功したら、UDPNetWorkを生成
	if (socket != -1) {
		udp_network = std::make_unique<UDPNetWork>(socket, port);
	}
	return udp_network.get();
}

std::vector<char> NetWorkManagerBase::CreatePacket(PacketType type, const void* payload, u32 sizeBytes, IPDATA overrided_ip)
{
	constexpr size_t HEADER_SIZE = sizeof(PacketHeader);

	if (overrided_ip == IPDATA{ 0,0,0,0 })
		overrided_ip = my_ip;
	PacketHeader hdr{ overrided_ip, static_cast<u32>(type), sizeBytes };
	std::vector<char> buf(HEADER_SIZE + sizeBytes);
	std::memcpy(buf.data(), &hdr, HEADER_SIZE);
	if (sizeBytes > 0 && payload) {
		std::memcpy(buf.data() + HEADER_SIZE, payload, sizeBytes);
	}
	return buf;

}


void NetWorkManagerBase::Update()
{
	//全接続に対して、受信データがあれば受信する
	//別スレッドで新規接続があった場合、vectorの要素が増えてしまう
	//その際のイテレータの破壊を防ぐため、ロックをかける
	if (udp_network) {
		//UDPソケットに対しても同様に受信を行う
		int data_arrived = DxLib::CheckNetWorkRecvUDP(udp_network->socket);
		bool break_flag = false;
		int data_size = 0;
		while (data_arrived > 0 || !break_flag) {
			IPDATA from_ip;
			int from_port;
			//受信したデータを受信バッファに読み込む
			//結果はキャッシュしておく
			int result = DxLib::NetWorkRecvUDP(udp_network->socket, &from_ip, &from_port, udp_network->buffer.data() + data_size, udp_network->buffer.size() - data_size, false);

			switch (result) {
			case -1:
				//エラーの場合、何もしない
				//※本来はエラー内容に応じた処理が必要
				break_flag = true;
				break;
			case -2:
				//バッファが足りない場合、仮実装としてバッファをクリアして受信データを破棄する
				//※しゃあなしでやってるので、本来は良くない
				DxLib::NetWorkRecvBufferClear(udp_network->socket);
				break_flag = true;
				break;
			case -3:
				//受信データがない場合、ループを抜ける
				break_flag = true;
				break;
			default:
				//成功した場合、resultに受信データのサイズが入っている
				data_size += result;
				break;
			}
			data_arrived = DxLib::CheckNetWorkRecvUDP(udp_network->socket);

		}
		//データの内容は様々なので、コールバックに任せる
		if (udp_network->on_receive)
			udp_network->on_receive(udp_network->buffer.data(), static_cast<size_t>(data_size));
	}
	std::scoped_lock lock(mutex_);
	for (auto& net : networks) {
		if (net->handle == -1)
			continue;

		//受信済みだが振り分けが終わっていないデータの量を取得
		int recv_size = DxLib::GetNetWorkDataLength(net->handle);
		//受信データがある、かつバッファの容量に収まるならバッファに振り分け
		if (recv_size > 0 && recv_size < net->buffer.size()) {
			//受信したデータを受信バッファに読み込む
			DxLib::NetWorkRecv(net->handle, net->buffer.data(), recv_size);

			//データの内容は様々なので、コールバックに任せる
			if (net->on_receive)
				net->on_receive(net->buffer.data(), static_cast<size_t>(recv_size));
		}
		//受信データがあるがバッファに収まらない場合
		else if (recv_size > net->buffer.size()) {
			//仮実装として、受信バッファをクリアして受信データを破棄する
			{
				DxLib::NetWorkRecvBufferClear(net->handle);
				continue;
			}
			//本来は、バッファを動的に確保し直す、もしくは取れる分だけ取るなどの処理が必要
			//とりあえず、今回では取れる分だけ取る
			//※データの途中で切れてしまう可能性があるので、危険->要改善


			recv_size = net->buffer.size();
			DxLib::NetWorkRecv(net->handle, net->buffer.data(), recv_size);
			//データの内容は様々なので、コールバックに任せる
			if (net->on_receive)
				net->on_receive(net->buffer.data(), static_cast<size_t>(recv_size));

		}
	}


}

// 新規接続を検出して、管理下に置く
void NetWorkManagerBase::CheckForNewConnect(const bool& finish_flag)
{
	//別スレッドにして、常に新規接続を監視する
	while (!finish_flag) {
		//新規接続を取得
		int new_handle = DxLib::GetNewAcceptNetWork();

		//もし新規接続があれば、管理下に置く
		if (new_handle >= 0)
		{
			IPDATA other_ip;
			//接続先のIPを取得
			DxLib::GetNetWorkIP(new_handle, &other_ip);

			//unique_ptrで作成->マネージャが管理
			auto net = std::make_unique<NetWork>(new_handle, other_ip, SEC2MICRO(Time::GetTimeFromStart()));

			//接続解除時のデフォルトコールバックを設定(こちらは基本書き換えが不可能)
			net->on_disconnect = on_disconnection;

			{
				std::scoped_lock lock(mutex_);
				networks.push_back(std::move(net));
				if (on_new_connection)
					on_new_connection(networks.back().get());
			}

		}
		//正直、CPU負荷を下げるために少し休ませたい。よし、休ませよう
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

// 切断された接続を検出して、管理下から外す
void NetWorkManagerBase::CheckForDisConnect(const bool& finish_flag)
{
	//別スレッドにして、常に切断を監視する
	while (!finish_flag) {
		//切断された接続を取得
		int lost_handle = DxLib::GetLostNetWork();

		//もし切断された接続があれば、管理下から外す
		if (lost_handle >= 0)
		{
			//イテレータの破壊を防ぐため、ロックをかける
			std::scoped_lock lock(mutex_);

			//vectorから該当する接続を探す
			auto it = std::find_if(networks.begin(), networks.end(),
				[lost_handle](const std::unique_ptr<NetWork>& net) {
					return net->handle == lost_handle;
				});

			//見つかったら削除
			if (it != networks.end()) {
				networks.erase(it);
			}
		}
		//正直、CPU負荷を下げるために少し休ませたい。よし、休ませよう
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

NetWork* NetWorkManagerBase::Connect(IPDATA other, unsigned short port,
	std::function<void(NetWork*)> on_connect,
	std::function<void()> on_disconnect)
{
	//新たにこちらから接続を行う
	int handle = DxLib::ConnectNetWork(other, port);

	//失敗
	if (handle == -1)
		return nullptr;
	//成功
	//unique_ptrで作成->マネージャが管理
	auto net = std::make_unique<NetWork>(handle, other, SEC2MICRO(Time::GetTimeFromStart()));

	//接続解除時のデフォルトコールバックを設定
	//引数で指定されていればそちらを優先
	net->on_disconnect = on_disconnect ? on_disconnect : this->on_disconnection;

	//moveしてしまっては返せないので、生ポインタをキャッシュ
	NetWork* ret = net.get();
	//接続成功コールバックを呼ぶ
	//引数で指定されていればそちらを優先
	if (on_connect)
		on_connect(ret);
	else if (on_new_connection)
		on_new_connection(ret);
	//マネージャの管理下に置く
	{
		std::scoped_lock lock(mutex_);
		//vectorのイテレータが壊れてしまうといけないので、ロックをかける
		//ただし、コールバック中に他のマシンに接続するなどということもあり得るので、
		//コールバック前にロックをかけるのは避ける

		//vectorに登録
		networks.push_back(std::move(net));
	}
	//キャッシュしておいたポインタを返す
	return ret;
}

