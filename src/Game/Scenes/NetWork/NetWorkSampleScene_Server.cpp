#include "precompile.h"
#include "NetWorkSampleScene_Server.h"
#include "Game/Managers/NetWorkManagerBase.h"
#include <filesystem>
#include <fstream>
#include <mutex>
#include <deque>
#include <vector>
#include <cstdint>
#include <cstring>
#include "Game/Objects/SampleAnimation/SampleAnimationObject.h"
#include "System/Objects/CameraObject.h"

namespace NetWorkTest_Server {

	std::unique_ptr<NetWorkManagerBase> net_manager;
	std::vector<NetWork*> vchat_network;
	UDPNetWork* udp_network = nullptr;

	std::vector<std::pair<u32, std::string>> received_text;
	std::string send_text = "";
	SafeWeakPtr<SampleAnimation::SampleAnimationObject> player = nullptr;
	std::vector<std::pair<u32, SafeWeakPtr<SampleAnimation::SampleAnimationObject>>> another_players;
	IPDATA another_ip = { 127,0,0,0 };
	unsigned short another_port = 11453;
	struct PlayerData {
		Vector3 position;
		unsigned int anim_state = 0;
	};




	// 送信ユーティリティ: ヘッダ＋ペイロード(生のバイナリ)を連結して一括送信
	static void SendPacket(NetWork* net, PacketType type, const void* payload, u32 sizeBytes, IPDATA overrided_ip = { 0,0,0,0 }) {
		if (!net) return;
		const auto buf = net_manager->CreatePacket(type, payload, sizeBytes, overrided_ip);
		net->Send(buf.data(), buf.size());
	}
	static void SendPacket(UDPNetWork* net, IPDATA ip, unsigned short port, PacketType type, const void* payload, u32 sizeBytes, IPDATA overrided_ip = { 0,0,0,0 }) {
		if (!net) return;
		const auto buf = net_manager->CreatePacket(type, payload, sizeBytes, overrided_ip);
		net->Send(ip, port, buf.data(), buf.size());
	}


	static inline IPDATA MakeIPFromKey(u32 key) {
		IPDATA ip;
		ip.d1 = (key >> 24) & 0xFF;
		ip.d2 = (key >> 16) & 0xFF;
		ip.d3 = (key >> 8) & 0xFF;
		ip.d4 = key & 0xFF;
		return ip;
	}
	// 送信元キーでプレイヤー座標をUpsert
	static inline void UpsertPlayer(std::vector<std::pair<u32, SafeWeakPtr<SampleAnimation::SampleAnimationObject>>>& list, IPDATA ip, const PlayerData& data) {
		u32 key = MakeIPKey(ip);
		for (auto& kv : list) {
			if (kv.first == key) {
				kv.second->transform->position = data.position;
				kv.second->ManipulateAsAnotherPlayer(data.anim_state);
				return;
			}
		}
		auto obj = SceneManager::Object::Create<SampleAnimation::SampleAnimationObject>("Player" + std::to_string(key));
		obj->manipulate_mode = true;
		list.emplace_back(key, obj);
	}

	// 受信処理: 蓄積→フレーミング→種別毎に処理
	//ここで問題なのは、TCPの場合、複数パケットが一度に来る可能性があること
	//何がマズいって、
	//
	// メモリ上のイメージ
	// +-----------------------------+-----------------------------+
	// |		1パケット目			 |			2パケット目         |
	// |[ヘッダ][情報][情報][情報][情報]|[ヘッダ][情報][情報][情報][情報]|
	// 
	// こういう風にパケットが並んでいる場合、NetWorkの受信バッファには1パケット目の情報と2パケット目のヘッダが一緒に入っている
	// 普通にバッファをそのままキャストすると、PC君はこう解釈する
	// +-----------------------------+-----------------------------+
	// |[ヘッダ][                    情報                          ]|
	// なるほど、ヘッダの後ろに情報が続いているな。よし、情報を全部取ってしまえと
	// 音声データのように、ヘッダの後ろに情報が続いている場合は別に問題は無い(2パケット目のヘッダがノイズとして現れること以外は)
	// では、音声データの後にテキストデータが来たら...?
	// 
	//									   ^^^^^^^^^^^^^^^^^^^^^^^^^^^
	// [ヘッダ][音波][音波][音波][音波][音波]<[ヘッダ][M][E][S][S][A][G][E]>
	//									   ^^^^^^^^^^^^^^^^^^^^^^^^^^^
	//									    ｱｱｱｱｱｱｱｱｯｯｯｯ!!テキストが音声プレーヤーに吸われたァァァ!!
	// 
	// ってな訳で、TCPで複数パケットが一度に来る可能性がある場合は、受信バッファを解析してパケット毎に分割する必要がある
	// whileを回して、受信バッファにパケットが残っている限り解析を続ける
	// イメージ
	// +-----------------------------+-----------------------+---------------------------------------------------+
	// |		1パケット目			 |		 2パケット目 　    |			            3パケット目     　             |
	// |[ヘッダ][情報][情報][情報][情報]|[ヘッダ][情報][情報][情報]|[ヘッダ][情報][情報][情報][情報][情報][情報][情報][情報] |
	//   ^^^^^
	//   ﾎﾞｸ、4byteの情報ﾅﾉ
	//  なるほど、ヘッダのサイズ+4byteをポップ!!
	// +-----------------------------+-----------------------+---------------------------------------------------+
	// |		  切り取り			 |		 2パケット目 　    |			            3パケット目     　             |
	// |[          -null-           ]|[ヘッダ][情報][情報][情報]|[ヘッダ][情報][情報][情報][情報][情報][情報][情報][情報] |
	//								   ^^^^^
	//								   ﾎﾞｸﾊ、3byteの情報ﾅﾉ
	// ほんじゃ、ヘッダサイズ+3byteをポップ!!
	// +-----------------------------+-----------------------+----------------------------------------------------+
	// |		  切り取り			 |		   切り取り  　 　 |			            3パケット目     　      　　 　 |
	// |[          -null-           ]|[        -null-       ]|[ヘッダ][情報][情報][情報][情報][情報][情報][情報][情報] |
	//														   ^^^^^
	//															ﾎﾞｸﾊ、8byteの情報ﾅﾉ
	//	最後にもヘッダサイズ+8byteをポップ!!
	// 
	// +-----------------------------+-----------------------+-----------------------------------------------------+
	// |		  切り取り			 |		   切り取り  　 　 |			             切り取り      　      　 　 　  |
	// |[          -null-           ]|[        -null-       ]|[                      -null-                     ]　|
	// 
	// 空になったな。終わり!!


	//次の課題
	//UDPの良さって何だっけ...？
	//
	// 
	// そう、リアルタイム性だ
	// 通信が不安定でも、最新の情報を送り続けることができる
	// しかし、受信側はそうはいかない
	// なぜか?
	// 
	// UDPは送りつけるという性質上、基本的にバッファの先頭から情報が溜まっていく
	// つまり、古い情報ほどバッファの先頭に近く、新しい情報ほどバッファの後ろに近い
	// 
	// たまにイレギュラーはいる。届く順番が保証されていないため、古い情報と新しい情報が部分的に逆転することもある
	// しかし、基本的には古い情報ほどバッファの先頭に近い
	// 
	// 特にプレイヤーの位置情報などは、最新の情報を使いたい
	// むしろ、最新の情報さえ取れれば古いものは全部捨てても良い
	// 
	// もしかしたらバッファの最後尾に最新情報があるかもしれない
	// なのに先頭からゆっくりゆっくり解析していると、もっと新しい情報が届いて、
	// 最新情報は既に最新ではなくなっているかもしれない
	//
	// どないするん...?
	//本来はもっと高度なバッファ管理が必要なのかもしれないが、今回は決め打ちで
	//さっき言った「基本的に」の部分を利用する
	//必ずとは限らないが、高確率で新しい情報がバッファの後ろにあるなら、
	//バッファの後ろから走査して、各送信元ごとに最新1件だけを反映する

	//これの為だけに、NetWorkManagerを改装してUDPの受信をパケットごとではなくバッファにため込む方式にした
	//^(少しもったいない気もするが、UDPの特性上仕方ない)
	//※ (備忘録) GPTはパケットごとにシーケンス番号を振る方式を提案してきたが、今回は実装しない

	static void ProcessReceivedUDPBytes(u8* data, size_t length) {
		if (!data || length == 0) return;

		// まずはフレーム境界を収集（先頭から）
		std::vector<size_t> offsets;
		offsets.reserve(8);

		size_t iterator = 0;
		while (iterator + sizeof(PacketHeader) <= length) {
			PacketHeader* hdr = reinterpret_cast<PacketHeader*>(data + iterator);
			const size_t packet_size = sizeof(PacketHeader) + static_cast<size_t>(hdr->sizeBytes);
			if (packet_size == 0 || iterator + packet_size > length) {
				// 不正/途切れパケットは以降を破棄
				break;
			}
			offsets.push_back(iterator);
			iterator += packet_size;
		}

		// 送信元ごとに「最新だけ使いたい」: 後ろから走査して各送信元の最新1件だけ反映
		std::vector<u32> updatedKeys;
		updatedKeys.reserve(8);

		for (size_t i = offsets.size(); i > 0; --i) {
			const size_t off = offsets[i - 1];
			PacketHeader* hdr = reinterpret_cast<PacketHeader*>(data + off);
			u8* payload = data + off + sizeof(PacketHeader);

			switch (static_cast<PacketType>(hdr->type)) {
			case PACKET_TYPE_PLAYER_POSITION:
				if (hdr->sizeBytes >= sizeof(PlayerData)) {
					const u32 key = MakeIPKey(hdr->ip);
					bool seen = false;
					for (u32 k : updatedKeys) { if (k == key) { seen = true; break; } }
					if (!seen) {
						const PlayerData data_ = *reinterpret_cast<PlayerData*>(payload);
						UpsertPlayer(another_players, hdr->ip, data_);
						updatedKeys.push_back(key);
					}
				}
				break;
			default:
				break;
			}
			// 各送信元ごとに1件確定するまで継続（全送信元の最新を拾う）
		}

		// 逐次処理したいタイプは先頭から順に処理（順序維持）
		for (size_t off : offsets) {
			PacketHeader* hdr = reinterpret_cast<PacketHeader*>(data + off);
			u8* payload = data + off + sizeof(PacketHeader);

			switch (static_cast<PacketType>(hdr->type)) {
			case PACKET_TYPE_WAVE:

				break;
			case PACKET_TYPE_COMMAND:
				// 必要に応じてコマンド処理を実装
				break;
			default:
				// 未知タイプは破棄 or 既に最新のみ処理済み
				break;
			}
		}
	}



	static void ProcessReceivedBytes(u8* data, size_t length) {
		if (!data || length == 0) return;

		//コイツが今回のイテレータ(厳密にはポインタ)
		size_t iterator = 0;
		PacketHeader* hdr = reinterpret_cast<PacketHeader*>(data);


		while (true) {

			//それぞれのパケットのヘッダは先頭アドレス+イテレータの位置から始まる
			hdr = reinterpret_cast<PacketHeader*>(data + iterator);

			//ペイロード(データ本体)は、先頭アドレス+イテレータの位置+ヘッダサイズの位置から始まる
			u8* payload = data + iterator + sizeof(PacketHeader);
			switch (static_cast<PacketType>(hdr->type)) {
			case PACKET_TYPE_WAVE:

				break;
			case PACKET_TYPE_TEXT: {
				// バイナリテキストを文字列化（ヌル終端無し想定）
				//ペイロードの先頭から終端までを文字列としてキャスト
				auto txt = std::find_if(received_text.begin(), received_text.end(), [&hdr](const auto& p) {return p.first == MakeIPKey(hdr->ip); });
				if (txt == received_text.end()) {
					received_text.push_back({ MakeIPKey(hdr->ip), {reinterpret_cast<char*>(payload),
					reinterpret_cast<char*>(payload) + hdr->sizeBytes} });
				}
				else
					txt->second.assign(reinterpret_cast<char*>(payload),
						reinterpret_cast<char*>(payload) + hdr->sizeBytes);
				for (int i = 0; i < vchat_network.size(); i++) {
					auto net = vchat_network[i];
					if (hdr->ip != net->ip)
						SendPacket(net, PACKET_TYPE_TEXT, payload,
							hdr->sizeBytes, hdr->ip);
				}

				break;
			}
			case PACKET_TYPE_PLAYER_POSITION: {

				if (hdr->sizeBytes >= sizeof(PlayerData)) {
					const PlayerData data_ = *reinterpret_cast<PlayerData*>(payload);
					UpsertPlayer(another_players, hdr->ip, data_);
				}
				// 必要に応じて位置情報処理を実装
				break;
			}
			case PACKET_TYPE_COMMAND:
				// 必要に応じてコマンド処理を実装
				break;
			default:
				// 未知タイプは破棄
				break;
			}
			// 次のパケットへ
			//パケットサイズ丸々(ヘッダサイズ+後続情報のサイズ分)移動すると楽
			iterator += sizeof(PacketHeader) + hdr->sizeBytes;
			//次へ移動した結果、ヘッダサイズに満たない(=読んでる途中でオーバーフローする)場合、終了
			if (iterator + sizeof(PacketHeader) > length)
				break;

		}
	}



	int NetWorkSampleScene_Server::Init()
	{

		ModelManager::LoadAsModel("data/player/model.mv1", "player_model");
		ModelManager::LoadAsAnimation("data/player/anim_stand.mv1", "idle");
		ModelManager::LoadAsAnimation("data/player/anim_walk.mv1", "walk");
		ModelManager::LoadAsAnimation("data/player/anim_run.mv1", "run");
		ModelManager::LoadAsAnimation("data/player/anim_jump.mv1", "jump");
		ModelManager::LoadAsAnimation("data/player/anim_salute.mv1", "salute");
		ModelManager::LoadAsAnimation("data/player/anim_aim.mv1", "aim");
		ModelManager::LoadAsAnimation("data/player/anim_reload.mv1", "reload");
		AudioManager::Load("data/Sound/reload.mp3", "reload_se", false);

		net_manager = std::make_unique<NetWorkManagerBase>(NetWorkManagerBase::NETWORK_MANAGER_MODE_LISTEN, 11451);
		udp_network = net_manager->OpenUDPSocket(11453); // 必要に応じて UDP も開く
		if (udp_network)
			udp_network->on_receive = [](void* data, size_t length) {
			ProcessReceivedUDPBytes(reinterpret_cast<u8*>(data), length);
			};
		player = SceneManager::Object::Create<SampleAnimation::SampleAnimationObject>("You");
		SceneManager::Object::Create<CameraObject>()->transform->position = { 0,10,-10 };

		net_manager->SetOnNewConnectionCallback([this](NetWork* new_connect) {
			vchat_network.push_back(new_connect);

			new_connect->on_receive = [this](void* data, size_t length) {
				ProcessReceivedBytes(reinterpret_cast<u8*>(data), length);
				};

			});

		net_manager->SetOnDisconnectionCallback([this](NetWork* net) {
			auto it = std::find(vchat_network.begin(), vchat_network.end(), net);
			vchat_network.erase(it);

			});

		return 0;
	}

	void NetWorkSampleScene_Server::Update()
	{
		net_manager->Update();




		// テキスト送信（例: 毎フレーム送ると帯域を圧迫するため、実運用ではイベント駆動推奨）
		if (!vchat_network.empty()) {
			while (char ch = DxLib::GetInputChar(true)) {


				if (ch == '\b' && !send_text.empty()) {
					send_text.pop_back();
				}
				else if (ch >= 32) { // 制御文字除外
					send_text.push_back(ch);
				}
				else
					break;
			}
			if (Input::GetKeyDown(KeyCode::Return)) {
				for (const auto& net : vchat_network)
					SendPacket(net, PACKET_TYPE_TEXT, send_text.data(), static_cast<u32>(send_text.size()));
				send_text.clear();
			}
		}

		if (udp_network) {
			if (Input::GetKey(KeyCode::Up))
				player->transform->position.z += 10.0f * Time::DeltaTime();
			if (Input::GetKey(KeyCode::Down))
				player->transform->position.z -= 10.0f * Time::DeltaTime();
			if (Input::GetKey(KeyCode::Left))
				player->transform->position.x -= 10.0f * Time::DeltaTime();
			if (Input::GetKey(KeyCode::Right))
				player->transform->position.x += 10.0f * Time::DeltaTime();

			// サーバーは接続中の全クライアントに送信 -> いわゆるブロードキャスト
			//権威サーバ方式では、クライアントの位置情報をサーバーが集約して、全クライアントに配信する
			//ここでもし、サーバに接続してきたクライアントのIPアドレスも配信した場合、
			//クライアントは、他のクライアントのIPアドレスを知ることができる
			//ただしそのやり方はマッチングサーバも兼ねる事になるので、権威サーバ方式にするメリットが薄れる

			for (auto& pos : another_players) {
				IPDATA sender_ip = MakeIPFromKey(pos.first);

				// 自分の位置情報は送らない
				std::for_each(another_players.begin(), another_players.end(),
					[&](const auto& p) {
						if (p.first != pos.first) {
							PlayerData pd;
							pd.position = p.second->transform->position;
							pd.anim_state = p.second->GetCurrentAnimState();
							SendPacket(udp_network, sender_ip, another_port, PACKET_TYPE_PLAYER_POSITION, &pd, sizeof(PlayerData), MakeIPFromKey(p.first));
						}
					});
				PlayerData pd;
				pd.position = player->transform->position;
				pd.anim_state = player->GetCurrentAnimState();
				SendPacket(udp_network, sender_ip, another_port, PACKET_TYPE_PLAYER_POSITION, &pd, sizeof(PlayerData));
			}



			//			SendPacket(udp_network, { 192,168,0,104 }, 11452, PACKET_TYPE_PLAYER_POSITION, &player_position, sizeof(Vector3));


		}
	}

	void NetWorkSampleScene_Server::LateDraw()
	{
		DxLib::DrawString(100, 50, "Server Mode", 0xffff00);





		SetFontSize(72);
		if (!vchat_network.empty()) {
			DxLib::DrawString(100, 100, "Connected!", 0xff0000);
		}
		else {
			DxLib::DrawString(100, 100, "Not Connected", 0x0000ff);
		}
		SetFontSize(21);

		for (int i = 0; i < vchat_network.size() && i < received_text.size(); i++) {
			auto received = received_text[i];
			auto net = vchat_network[i];
			if (!received.second.empty() && net) {
				DxLib::DrawFormatString(100, 180 + i * 21, 0xffffff, "ID:%d< %s", received.first, received.second.c_str());
			}
		}
		DrawFormatString(100, 400, 0xffffff, "You> %s", send_text.c_str());
		DxLib::DrawBox(150 + send_text.size() * 12, 421, 160 + send_text.size() * 12, 430, 0xffffff, true);
		SetFontSize(DEFAULT_FONT_SIZE);

		DxLib::DrawFormatString(0, 0, Color::WHITE, "FPS:%.1f", Time::GetFPS());
		IPDATA my_ip = net_manager->GetMyIP();
		DrawFormatString(600, 0, Color::WHITE, "Your IP Address is %d.%d.%d.%d", my_ip.d1, my_ip.d2, my_ip.d3, my_ip.d4);
		DrawFormatString(600, 30, Color::WHITE, "Open Ports  TCP:%d,UDP:%d", net_manager->GetPort(), net_manager->GetUDPPort());
	}

	void NetWorkSampleScene_Server::Exit()
	{

		another_players.clear();
		net_manager.reset();
	}

}