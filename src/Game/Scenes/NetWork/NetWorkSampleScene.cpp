#include "precompile.h"
#include "NetWorkSampleScene.h"
#include "Game/Managers/NetWorkManagerBase.h"
#include "Audio/WinMMRecorder.h"
#include "Audio/WinMMWavePlayer.h"
#include <filesystem>
#include <fstream>
#include <mutex>
#include <deque>
#include <vector>
#include <cstdint>
#include <cstring>
#include "Game/Objects/SampleAnimation/SampleAnimationObject.h"
#include "System/Objects/CameraObject.h"

#define SERVER
//#define USE_VOICE_CHAT
namespace NetWorkTest {

	std::unique_ptr<NetWorkManagerBase> net_manager;
	NetWork* vchat_network = nullptr;
	UDPNetWork* udp_network = nullptr;

	std::string received_text = "";
	std::string send_text = "";
	SafeWeakPtr<SampleAnimation::SampleAnimationObject> player = nullptr;
	std::vector<std::pair<u32, SafeWeakPtr<SampleAnimation::SampleAnimationObject>>> another_players;
	IPDATA another_ip = { 127,0,0,0 };
	unsigned short another_port = 0;
	struct PlayerData {
		Vector3 position;
		unsigned int anim_state;
	};


#ifdef USE_VOICE_CHAT

	// 送信用キュー（録音コールバックからはここに積むだけ）
	struct AudioChunk {
		bool isHeader = false;
		std::vector<char> bytes;
	};
	static std::mutex g_sendMutex;
	static std::deque<AudioChunk> g_sendQueue;
	static size_t g_queuedBytes = 0;
	static constexpr size_t MAX_QUEUED_BYTES = 4 * 1024; // 4KB 上限（溢れたら古いものから破棄）

	int using_recbuffer_num = 0;

	// グローバルI/O
	static Audio::WinMMRecorder g_recorder(48000U, 16U, 1U, 5U);
	static Audio::WinMMWavePlayer g_player;
#endif





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

	// IP -> 32bit キー化（同一送信元識別に使用）
	static inline u32 MakeIPKey(const IPDATA& ip) {
		return (static_cast<u32>(ip.d1) << 24) | (static_cast<u32>(ip.d2) << 16) |
			(static_cast<u32>(ip.d3) << 8) | static_cast<u32>(ip.d4);
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
		auto obj = SceneManager::Object::Create<SampleAnimation::SampleAnimationObject>("AnotherPlayer" + std::to_string(key));
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
				if (hdr->sizeBytes >= sizeof(Vector3)) {
					const u32 key = MakeIPKey(hdr->ip);
					bool seen = false;
					for (u32 k : updatedKeys) { if (k == key) { seen = true; break; } }
					if (!seen) {
						const Vector3 pos = *reinterpret_cast<Vector3*>(payload);
						UpsertPlayer(another_players, hdr->ip, pos);
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
#ifdef USE_VOICE_CHAT
				// そのままプレイヤーに渡す（ヘッダ/PCM どちらも可。プレイヤー内でヘッダ検出）
				g_player.Feed(payload, hdr->sizeBytes);
#endif
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
#ifdef USE_VOICE_CHAT
				// そのままプレイヤーに渡す（ヘッダ/PCM どちらも可。プレイヤー内でヘッダ検出）
				g_player.Feed(payload, hdr->sizeBytes);
#endif
				break;
			case PACKET_TYPE_TEXT: {
				// バイナリテキストを文字列化（ヌル終端無し想定）
				//ペイロードの先頭から終端までを文字列としてキャスト
				received_text.assign(reinterpret_cast<char*>(payload),
					reinterpret_cast<char*>(payload) + hdr->sizeBytes);
				break;
			}
			case PACKET_TYPE_PLAYER_POSITION: {

				if (hdr->sizeBytes >= sizeof(Vector3)) {
					const Vector3 pos = *reinterpret_cast<Vector3*>(payload);
					UpsertPlayer(another_players, hdr->ip, pos);
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

#ifdef USE_VOICE_CHAT
	// 例えばアプリ起動時にセットアップ
	void SetupRecorderForStreaming(NetWork** networkPtr) {

		g_recorder.SetOnWaveChunk([networkPtr](const u8* data, size_t size, bool /*isHeader*/) {
			// コールバックではキュー積みのみ（非ブロッキング）
			using_recbuffer_num++;
			if (using_recbuffer_num > 4)
				using_recbuffer_num = 0;
			AudioChunk chunk;
			chunk.isHeader = false; // ヘッダ情報は送信側では不要（プレイヤーが自力検出）
			chunk.bytes.resize(size);
			if (size > 0) {
				std::memcpy(chunk.bytes.data(), data, size);
			}

			std::lock_guard<std::mutex> lk(g_sendMutex);
			// 上限超過時は古いデータから破棄
			while (g_queuedBytes + chunk.bytes.size() > MAX_QUEUED_BYTES && !g_sendQueue.empty()) {
				g_queuedBytes -= g_sendQueue.front().bytes.size();
				g_sendQueue.pop_front();
			}
			g_queuedBytes += chunk.bytes.size();
			g_sendQueue.emplace_back(std::move(chunk));
			});
	}

	// キューを吐き出して送信
	static void DrainAudioChunks() {
		std::deque<AudioChunk> local;
		{
			std::lock_guard<std::mutex> lk(g_sendMutex);
			if (g_sendQueue.empty()) return;
			local.swap(g_sendQueue);
			g_queuedBytes = 0;
		}

		for (auto& ch : local) {
			if (vchat_network && !ch.bytes.empty()) {
				SendPacket(vchat_network, PACKET_TYPE_WAVE, ch.bytes.data(), static_cast<u32>(ch.bytes.size()));
			}
		}
	}
#endif

	int NetWorkSampleScene::Init()
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

#ifndef SERVER
		net_manager = std::make_unique<NetWorkManagerBase>(NetWorkManagerBase::NETWORK_MANAGER_MODE_CONNECT, 11451);
		udp_network = net_manager->OpenUDPSocket(11452); // 必要に応じて UDP も開く
		if (udp_network)
			udp_network->on_receive = [](void* data, size_t length) {
			ProcessReceivedUDPBytes(reinterpret_cast<u8*>(data), length);
			};
#else
		net_manager = std::make_unique<NetWorkManagerBase>(NetWorkManagerBase::NETWORK_MANAGER_MODE_LISTEN, 11451);
		udp_network = net_manager->OpenUDPSocket(11453); // 必要に応じて UDP も開く
		if (udp_network)
			udp_network->on_receive = [](void* data, size_t length) {
			ProcessReceivedUDPBytes(reinterpret_cast<u8*>(data), length);
			};
#endif // SERVER
		player = SceneManager::Object::Create<SampleAnimation::SampleAnimationObject>("Player");
		SceneManager::Object::Create<CameraObject>()->transform->position = { 0,10,-10 };

		net_manager->SetOnNewConnectionCallback([this](NetWork* new_connect) {
			is_connected = true;
			vchat_network = new_connect;

			vchat_network->on_receive = [this](void* data, size_t length) {
				ProcessReceivedBytes(reinterpret_cast<u8*>(data), length);
				};
#ifdef USE_VOICE_CHAT
			// 録音開始（ネットワーク接続後）
			g_recorder.Start();
#endif
			});

		net_manager->SetOnDisconnectionCallback([this]() {
			is_connected = false;
			vchat_network = nullptr;
#ifdef USE_VOICE_CHAT
			if (g_recorder.IsRunning())
				g_recorder.Stop();
			g_player.Stop();
#endif
			});
#ifdef USE_VOICE_CHAT
		SetupRecorderForStreaming(&vchat_network);
#endif
		return 0;
	}

	void NetWorkSampleScene::Update()
	{
		net_manager->Update();

		// 録音コールバックから集めたデータをここで送信
#ifdef USE_VOICE_CHAT
		DrainAudioChunks();
#endif
#ifndef SERVER
		if (Input::GetKeyDown(KeyCode::Space) && !is_connected)
		{
			if (udp_network) {
				std::string test_msg = "UDP Test Message";
				SendPacket(udp_network, another_ip, 11453, PACKET_TYPE_TEXT, test_msg.data(), static_cast<u32>(test_msg.size()));
			}

			auto net = net_manager->Connect(another_ip, another_port);
			if (net) {
				vchat_network = net;
				is_connected = true;

				vchat_network->on_receive = [this](void* data, size_t length) {
					ProcessReceivedBytes(reinterpret_cast<u8*>(data), length);
					};

#ifdef USE_VOICE_CHAT
				// 録音開始（ネットワーク接続後）
				g_recorder.Start();
#endif
			}
		}
#endif

		// テキスト送信（例: 毎フレーム送ると帯域を圧迫するため、実運用ではイベント駆動推奨）
		if (is_connected) {
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
				SendPacket(vchat_network, PACKET_TYPE_TEXT, send_text.data(), static_cast<u32>(send_text.size()));
				send_text.clear();
			}
		}
#ifndef SERVER 
		else {
			while (char ch = DxLib::GetInputChar(true)) {


				if (ch == '\b' && !send_text.empty()) {
					send_text.pop_back();
				}
				else if (ch == 46 || (ch >= 48 && ch <= 58)) { // IPで使うのは0~9とドットのみ
					send_text.push_back(ch);
				}
				else
					break;
			}
			if (Input::GetKeyDown(KeyCode::Return)) {
				std::vector<std::string> octets;
				std::string segment;
				std::istringstream tokenStream(send_text);
				while (std::getline(tokenStream, segment, '.')) {
					octets.push_back(segment);
				}
				if (octets.size() == 5) {
					another_ip.d1 = static_cast<u8>(std::stoi(octets[0]));
					another_ip.d2 = static_cast<u8>(std::stoi(octets[1]));
					another_ip.d3 = static_cast<u8>(std::stoi(octets[2]));
					another_ip.d4 = static_cast<u8>(std::stoi(octets[3]));
					another_port = static_cast<unsigned short>(std::stoul(octets[4]));
				}
				send_text.clear();
			}
		}
#endif // !SERVER

		if (udp_network) {
			if (Input::GetKey(KeyCode::Up))
				player->transform->position.z += 10.0f * Time::DeltaTime();
			if (Input::GetKey(KeyCode::Down))
				player->transform->position.z -= 10.0f * Time::DeltaTime();
			if (Input::GetKey(KeyCode::Left))
				player->transform->position.x -= 10.0f * Time::DeltaTime();
			if (Input::GetKey(KeyCode::Right))
				player->transform->position.x += 10.0f * Time::DeltaTime();
#ifdef SERVER
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

							SendPacket(udp_network, sender_ip, 11452, PACKET_TYPE_PLAYER_POSITION, &p.second->transform->position, sizeof(Vector3), MakeIPFromKey(p.first));
						}
					});
				SendPacket(udp_network, sender_ip, 11452, PACKET_TYPE_PLAYER_POSITION, &player->transform->position, sizeof(Vector3));
			}

			//			SendPacket(udp_network, { 192,168,0,104 }, 11452, PACKET_TYPE_PLAYER_POSITION, &player_position, sizeof(Vector3));
#else

			SendPacket(udp_network, another_ip, another_port, PACKET_TYPE_PLAYER_POSITION, &player->transform->position, sizeof(Vector3));
#endif // SERVER

		}
	}

	void NetWorkSampleScene::Draw()
	{
#ifdef SERVER
		DxLib::DrawString(100, 50, "Server Mode", 0xffff00);
#else
		DxLib::DrawString(100, 50, "Client Mode", 0xffff00);
#endif // SERVER



#ifdef USE_VOICE_CHAT
		DxLib::DrawFormatString(100, 200, 0xffffff, "Using recbuffer No.%d", using_recbuffer_num);
#endif // USE_VOICE_CHAT

		SetFontSize(72);
		if (is_connected) {
			DxLib::DrawString(100, 100, "Connected!", 0xff0000);
		}
		else {
			DxLib::DrawString(100, 100, "Not Connected", 0x0000ff);
		}
		SetFontSize(21);

		if (!received_text.empty() && vchat_network) {
			DxLib::DrawFormatString(100, 300, 0xffffff, "ID:%d< %s", vchat_network->unique_id, received_text.c_str());
		}
		DrawFormatString(100, 400, 0xffffff, "You> %s", send_text.c_str());
		DxLib::DrawBox(150 + send_text.size() * 12, 421, 160 + send_text.size() * 12, 430, 0xffffff, true);
		SetFontSize(DEFAULT_FONT_SIZE);
		/*
		DxLib::DrawCircleAA(player_position.x, player_position.y, 20.0f, 32, Color::CYAN, true);
		for (const auto& kv : another_players) {
			const Vector3& p = kv.second->transform->position;
			DxLib::DrawCircleAA(p.x, p.y, 20.0f, 32, Color::MAGENTA, true);
		}
		*/
		DxLib::DrawFormatString(0, 0, Color::WHITE, "FPS:%.1f", Time::GetFPS());
		IPDATA my_ip = net_manager->GetMyIP();
		DrawFormatString(600, 0, Color::WHITE, "Your IP Address is %d.%d.%d.%d", my_ip.d1, my_ip.d2, my_ip.d3, my_ip.d4);
		DrawFormatString(600, 30, Color::WHITE, "Open Ports  TCP:%d,UDP:%d", net_manager->GetPort(), net_manager->GetUDPPort());
	}

	void NetWorkSampleScene::Exit()
	{
#ifdef USE_VOICE_CHAT
		if (g_recorder.IsRunning()) {
			g_recorder.Stop();
		}
		g_player.Stop();
#endif
		another_players.clear();
		net_manager.reset();
	}

}