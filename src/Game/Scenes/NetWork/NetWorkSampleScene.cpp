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

#define SERVER
namespace NetWorkTest {

	std::unique_ptr<NetWorkManagerBase> net_manager;
	NetWork* vchat_network = nullptr;
	UDPNetWork* udp_network = nullptr;

	std::string received_text = "";
	std::string send_text = "";
	Vector3 player_position = Vector3(300.0f, 300.0f, 0.0f);
	std::vector<std::pair<u32, Vector3>> another_player_position;



	// ���M�p�L���[�i�^���R�[���o�b�N����͂����ɐςނ����j
	struct AudioChunk {
		bool isHeader = false;
		std::vector<char> bytes;
	};
	static std::mutex g_sendMutex;
	static std::deque<AudioChunk> g_sendQueue;
	static size_t g_queuedBytes = 0;
	static constexpr size_t MAX_QUEUED_BYTES = 4 * 1024; // 4KB ����i��ꂽ��Â����̂���j���j

	int using_recbuffer_num = 0;

	// �O���[�o��I/O
	static Audio::WinMMRecorder g_recorder(48000U, 16U, 1U, 5U);
	static Audio::WinMMWavePlayer g_player;





	// ���M���[�e�B���e�B: �w�b�_�{�y�C���[�h(���̃o�C�i��)��A�����Ĉꊇ���M
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

	// IP -> 32bit �L�[���i���ꑗ�M�����ʂɎg�p�j
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
	// ���M���L�[�Ńv���C���[���W��Upsert
	static inline void UpsertPlayer(std::vector<std::pair<u32, Vector3>>& list, IPDATA ip, const Vector3& pos) {
		u32 key = MakeIPKey(ip);
		for (auto& kv : list) {
			if (kv.first == key) { kv.second = pos; return; }
		}
		list.emplace_back(key, pos);
	}

	// ��M����: �~�ρ��t���[�~���O����ʖ��ɏ���
	//�����Ŗ��Ȃ̂́ATCP�̏ꍇ�A�����p�P�b�g����x�ɗ���\�������邱��
	//�����}�Y�����āA
	//
	// ��������̃C���[�W
	// +-----------------------------+-----------------------------+
	// |		1�p�P�b�g��			 |			2�p�P�b�g��         |
	// |[�w�b�_][���][���][���][���]|[�w�b�_][���][���][���][���]|
	// 
	// �����������Ƀp�P�b�g������ł���ꍇ�ANetWork�̎�M�o�b�t�@�ɂ�1�p�P�b�g�ڂ̏���2�p�P�b�g�ڂ̃w�b�_���ꏏ�ɓ����Ă���
	// ���ʂɃo�b�t�@�����̂܂܃L���X�g����ƁAPC�N�͂������߂���
	// +-----------------------------+-----------------------------+
	// |[�w�b�_][                    ���                          ]|
	// �Ȃ�قǁA�w�b�_�̌��ɏ�񂪑����Ă���ȁB�悵�A����S������Ă��܂���
	// �����f�[�^�̂悤�ɁA�w�b�_�̌��ɏ�񂪑����Ă���ꍇ�͕ʂɖ��͖���(2�p�P�b�g�ڂ̃w�b�_���m�C�Y�Ƃ��Č���邱�ƈȊO��)
	// �ł́A�����f�[�^�̌�Ƀe�L�X�g�f�[�^��������...?
	// 
	//									   ^^^^^^^^^^^^^^^^^^^^^^^^^^^
	// [�w�b�_][���g][���g][���g][���g][���g]<[�w�b�_][M][E][S][S][A][G][E]>
	//									   ^^^^^^^^^^^^^^^^^^^^^^^^^^^
	//									    ������������!!�e�L�X�g�������v���[���[�ɋz��ꂽ�@�@�@!!
	// 
	// ���ĂȖ�ŁATCP�ŕ����p�P�b�g����x�ɗ���\��������ꍇ�́A��M�o�b�t�@����͂��ăp�P�b�g���ɕ�������K�v������
	// while���񂵂āA��M�o�b�t�@�Ƀp�P�b�g���c���Ă�������͂𑱂���
	// �C���[�W
	// +-----------------------------+-----------------------+---------------------------------------------------+
	// |		1�p�P�b�g��			 |		 2�p�P�b�g�� �@    |			            3�p�P�b�g��     �@             |
	// |[�w�b�_][���][���][���][���]|[�w�b�_][���][���][���]|[�w�b�_][���][���][���][���][���][���][���][���] |
	//   ^^^^^
	//   �޸�A4byte�̏����
	//  �Ȃ�قǁA�w�b�_�̃T�C�Y+4byte���|�b�v!!
	// +-----------------------------+-----------------------+---------------------------------------------------+
	// |		  �؂���			 |		 2�p�P�b�g�� �@    |			            3�p�P�b�g��     �@             |
	// |[          -null-           ]|[�w�b�_][���][���][���]|[�w�b�_][���][���][���][���][���][���][���][���] |
	//								   ^^^^^
	//								   �޸ʁA3byte�̏����
	// �ق񂶂�A�w�b�_�T�C�Y+3byte���|�b�v!!
	// +-----------------------------+-----------------------+----------------------------------------------------+
	// |		  �؂���			 |		   �؂���  �@ �@ |			            3�p�P�b�g��     �@      �@�@ �@ |
	// |[          -null-           ]|[        -null-       ]|[�w�b�_][���][���][���][���][���][���][���][���] |
	//														   ^^^^^
	//															�޸ʁA8byte�̏����
	//	�Ō�ɂ��w�b�_�T�C�Y+8byte���|�b�v!!
	// 
	// +-----------------------------+-----------------------+-----------------------------------------------------+
	// |		  �؂���			 |		   �؂���  �@ �@ |			             �؂���      �@      �@ �@ �@  |
	// |[          -null-           ]|[        -null-       ]|[                      -null-                     ]�@|
	// 
	// ��ɂȂ����ȁB�I���!!


	//���̉ۑ�
	//UDP�̗ǂ����ĉ�������...�H
	//
	// 
	// �����A���A���^�C������
	// �ʐM���s����ł��A�ŐV�̏��𑗂葱���邱�Ƃ��ł���
	// �������A��M���͂����͂����Ȃ�
	// �Ȃ���?
	// 
	// UDP�͑������Ƃ���������A��{�I�Ƀo�b�t�@�̐擪�����񂪗��܂��Ă���
	// �܂�A�Â����قǃo�b�t�@�̐擪�ɋ߂��A�V�������قǃo�b�t�@�̌��ɋ߂�
	// 
	// ���܂ɃC���M�����[�͂���B�͂����Ԃ��ۏ؂���Ă��Ȃ����߁A�Â����ƐV������񂪕����I�ɋt�]���邱�Ƃ�����
	// �������A��{�I�ɂ͌Â����قǃo�b�t�@�̐擪�ɋ߂�
	// 
	// ���Ƀv���C���[�̈ʒu���Ȃǂ́A�ŐV�̏����g������
	// �ނ���A�ŐV�̏�񂳂�����ΌÂ����̂͑S���̂ĂĂ��ǂ�
	// 
	// ������������o�b�t�@�̍Ō���ɍŐV��񂪂��邩������Ȃ�
	// �Ȃ̂ɐ擪�����������������͂��Ă���ƁA�����ƐV������񂪓͂��āA
	// �ŐV���͊��ɍŐV�ł͂Ȃ��Ȃ��Ă��邩������Ȃ�
	//
	// �ǂȂ������...?
	//�{���͂����ƍ��x�ȃo�b�t�@�Ǘ����K�v�Ȃ̂�������Ȃ����A����͌��ߑł���
	//�������������u��{�I�Ɂv�̕����𗘗p����
	//�K���Ƃ͌���Ȃ����A���m���ŐV������񂪃o�b�t�@�̌��ɂ���Ȃ�A
	//�o�b�t�@�̌�납�瑖�����āA�e���M�����ƂɍŐV1�������𔽉f����

	//����ׂ̈����ɁANetWorkManager����������UDP�̎�M���p�P�b�g���Ƃł͂Ȃ��o�b�t�@�ɂ��ߍ��ޕ����ɂ���
	//^(�������������Ȃ��C�����邪�AUDP�̓�����d���Ȃ�)
	//�� (���Y�^) GPT�̓p�P�b�g���ƂɃV�[�P���X�ԍ���U��������Ă��Ă������A����͎������Ȃ�

	static void ProcessReceivedUDPBytes(u8* data, size_t length) {
		if (!data || length == 0) return;

		// �܂��̓t���[�����E�����W�i�擪����j
		std::vector<size_t> offsets;
		offsets.reserve(8);

		size_t iterator = 0;
		while (iterator + sizeof(PacketHeader) <= length) {
			PacketHeader* hdr = reinterpret_cast<PacketHeader*>(data + iterator);
			const size_t packet_size = sizeof(PacketHeader) + static_cast<size_t>(hdr->sizeBytes);
			if (packet_size == 0 || iterator + packet_size > length) {
				// �s��/�r�؂�p�P�b�g�͈ȍ~��j��
				break;
			}
			offsets.push_back(iterator);
			iterator += packet_size;
		}

		// ���M�����ƂɁu�ŐV�����g�������v: ��납�瑖�����Ċe���M���̍ŐV1���������f
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
						UpsertPlayer(another_player_position, hdr->ip, pos);
						updatedKeys.push_back(key);
					}
				}
				break;
			default:
				break;
			}
			// �e���M�����Ƃ�1���m�肷��܂Ōp���i�S���M���̍ŐV���E���j
		}

		// ���������������^�C�v�͐擪���珇�ɏ����i�����ێ��j
		for (size_t off : offsets) {
			PacketHeader* hdr = reinterpret_cast<PacketHeader*>(data + off);
			u8* payload = data + off + sizeof(PacketHeader);

			switch (static_cast<PacketType>(hdr->type)) {
			case PACKET_TYPE_WAVE:
				// ���̂܂܃v���C���[�ɓn���i�w�b�_/PCM �ǂ�����B�v���C���[���Ńw�b�_���o�j
				g_player.Feed(payload, hdr->sizeBytes);
				break;
			case PACKET_TYPE_COMMAND:
				// �K�v�ɉ����ăR�}���h����������
				break;
			default:
				// ���m�^�C�v�͔j�� or ���ɍŐV�̂ݏ����ς�
				break;
			}
		}
	}



	static void ProcessReceivedBytes(u8* data, size_t length) {
		if (!data || length == 0) return;

		//�R�C�c������̃C�e���[�^(�����ɂ̓|�C���^)
		size_t iterator = 0;
		PacketHeader* hdr = reinterpret_cast<PacketHeader*>(data);


		while (true) {

			//���ꂼ��̃p�P�b�g�̃w�b�_�͐擪�A�h���X+�C�e���[�^�̈ʒu����n�܂�
			hdr = reinterpret_cast<PacketHeader*>(data + iterator);

			//�y�C���[�h(�f�[�^�{��)�́A�擪�A�h���X+�C�e���[�^�̈ʒu+�w�b�_�T�C�Y�̈ʒu����n�܂�
			u8* payload = data + iterator + sizeof(PacketHeader);
			switch (static_cast<PacketType>(hdr->type)) {
			case PACKET_TYPE_WAVE:
				// ���̂܂܃v���C���[�ɓn���i�w�b�_/PCM �ǂ�����B�v���C���[���Ńw�b�_���o�j
				g_player.Feed(payload, hdr->sizeBytes);
				break;
			case PACKET_TYPE_TEXT: {
				// �o�C�i���e�L�X�g�𕶎��񉻁i�k���I�[�����z��j
				//�y�C���[�h�̐擪����I�[�܂ł𕶎���Ƃ��ăL���X�g
				received_text.assign(reinterpret_cast<char*>(payload),
					reinterpret_cast<char*>(payload) + hdr->sizeBytes);
				break;
			}
			case PACKET_TYPE_PLAYER_POSITION: {

				if (hdr->sizeBytes >= sizeof(Vector3)) {
					const Vector3 pos = *reinterpret_cast<Vector3*>(payload);
					UpsertPlayer(another_player_position, hdr->ip, pos);
				}
				// �K�v�ɉ����Ĉʒu��񏈗�������
				break;
			}
			case PACKET_TYPE_COMMAND:
				// �K�v�ɉ����ăR�}���h����������
				break;
			default:
				// ���m�^�C�v�͔j��
				break;
			}
			// ���̃p�P�b�g��
			//�p�P�b�g�T�C�Y�ہX(�w�b�_�T�C�Y+�㑱���̃T�C�Y��)�ړ�����Ɗy
			iterator += sizeof(PacketHeader) + hdr->sizeBytes;
			//���ֈړ��������ʁA�w�b�_�T�C�Y�ɖ����Ȃ�(=�ǂ�ł�r���ŃI�[�o�[�t���[����)�ꍇ�A�I��
			if (iterator + sizeof(PacketHeader) > length)
				break;

		}
	}

	// �Ⴆ�΃A�v���N�����ɃZ�b�g�A�b�v
	void SetupRecorderForStreaming(NetWork** networkPtr) {

		g_recorder.SetOnWaveChunk([networkPtr](const u8* data, size_t size, bool /*isHeader*/) {
			// �R�[���o�b�N�ł̓L���[�ς݂̂݁i��u���b�L���O�j
			using_recbuffer_num++;
			if (using_recbuffer_num > 4)
				using_recbuffer_num = 0;
			AudioChunk chunk;
			chunk.isHeader = false; // �w�b�_���͑��M���ł͕s�v�i�v���C���[�����͌��o�j
			chunk.bytes.resize(size);
			if (size > 0) {
				std::memcpy(chunk.bytes.data(), data, size);
			}

			std::lock_guard<std::mutex> lk(g_sendMutex);
			// ������ߎ��͌Â��f�[�^����j��
			while (g_queuedBytes + chunk.bytes.size() > MAX_QUEUED_BYTES && !g_sendQueue.empty()) {
				g_queuedBytes -= g_sendQueue.front().bytes.size();
				g_sendQueue.pop_front();
			}
			g_queuedBytes += chunk.bytes.size();
			g_sendQueue.emplace_back(std::move(chunk));
			});
	}

	// �L���[��f���o���đ��M
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

	int NetWorkSampleScene::Init()
	{
#ifndef SERVER
		net_manager = std::make_unique<NetWorkManagerBase>(NetWorkManagerBase::NETWORK_MANAGER_MODE_CONNECT, 11451);
		udp_network = net_manager->OpenUDPSocket(11452); // �K�v�ɉ����� UDP ���J��
		if (udp_network)
			udp_network->on_receive = [](void* data, size_t length) {
			ProcessReceivedUDPBytes(reinterpret_cast<u8*>(data), length);
			};
#else
		net_manager = std::make_unique<NetWorkManagerBase>(NetWorkManagerBase::NETWORK_MANAGER_MODE_LISTEN, 11451);
		udp_network = net_manager->OpenUDPSocket(11453); // �K�v�ɉ����� UDP ���J��
		if (udp_network)
			udp_network->on_receive = [](void* data, size_t length) {
			ProcessReceivedUDPBytes(reinterpret_cast<u8*>(data), length);
			};
#endif // SERVER

		net_manager->SetOnNewConnectionCallback([this](NetWork* new_connect) {
			is_connected = true;
			vchat_network = new_connect;

			vchat_network->on_receive = [this](void* data, size_t length) {
				ProcessReceivedBytes(reinterpret_cast<u8*>(data), length);
				};

			// �^���J�n�i�l�b�g���[�N�ڑ���j
			g_recorder.Start();
			});

		net_manager->SetOnDisconnectionCallback([this]() {
			is_connected = false;
			vchat_network = nullptr;
			if (g_recorder.IsRunning())
				g_recorder.Stop();
			g_player.Stop();
			});

		SetupRecorderForStreaming(&vchat_network);
		return 0;
	}

	void NetWorkSampleScene::Update()
	{
		net_manager->Update();

		// �^���R�[���o�b�N����W�߂��f�[�^�������ő��M
		DrainAudioChunks();
#ifndef SERVER
		if (Input::GetKeyDown(KeyCode::Space) && !is_connected)
		{
			if (udp_network) {
				std::string test_msg = "UDP Test Message";
				SendPacket(udp_network, { 192,168,0,103 }, 11453, PACKET_TYPE_TEXT, test_msg.data(), static_cast<u32>(test_msg.size()));
			}

			IPDATA other_ip = { 192,168,0,103 };
			auto net = net_manager->Connect(other_ip, 11451);
			if (net) {
				vchat_network = net;
				is_connected = true;

				vchat_network->on_receive = [this](void* data, size_t length) {
					ProcessReceivedBytes(reinterpret_cast<u8*>(data), length);
					};

				// �^���J�n�i�l�b�g���[�N�ڑ���j
				g_recorder.Start();
			}
		}
#endif

		// �e�L�X�g���M�i��: ���t���[������Ƒш���������邽�߁A���^�p�ł̓C�x���g�쓮�����j
		if (is_connected) {
			while (char ch = DxLib::GetInputChar(true)) {


				if (ch == '\b' && !send_text.empty()) {
					send_text.pop_back();
				}
				else if (ch >= 32) { // ���䕶�����O
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

		if (udp_network) {
			if (Input::GetKey(KeyCode::Up))
				player_position.y -= 500.0f * Time::DeltaTime();
			if (Input::GetKey(KeyCode::Down))
				player_position.y += 500.0f * Time::DeltaTime();
			if (Input::GetKey(KeyCode::Left))
				player_position.x -= 500.0f * Time::DeltaTime();
			if (Input::GetKey(KeyCode::Right))
				player_position.x += 500.0f * Time::DeltaTime();
#ifdef SERVER
			// �T�[�o�[�͐ڑ����̑S�N���C�A���g�ɑ��M -> ������u���[�h�L���X�g
			//���ЃT�[�o�����ł́A�N���C�A���g�̈ʒu�����T�[�o�[���W�񂵂āA�S�N���C�A���g�ɔz�M����
			//�����ł����A�T�[�o�ɐڑ����Ă����N���C�A���g��IP�A�h���X���z�M�����ꍇ�A
			//�N���C�A���g�́A���̃N���C�A���g��IP�A�h���X��m�邱�Ƃ��ł���
			//���������̂����̓}�b�`���O�T�[�o�����˂鎖�ɂȂ�̂ŁA���ЃT�[�o�����ɂ��郁���b�g�������

			for (auto& pos : another_player_position) {
				IPDATA sender_ip = MakeIPFromKey(pos.first);

				// �����̈ʒu���͑���Ȃ�
				std::for_each(another_player_position.begin(), another_player_position.end(),
					[&](const auto& p) {
						if (p.first != pos.first)
							SendPacket(udp_network, sender_ip, 11452, PACKET_TYPE_PLAYER_POSITION, &p.second, sizeof(Vector3), MakeIPFromKey(p.first));
					});
				SendPacket(udp_network, sender_ip, 11452, PACKET_TYPE_PLAYER_POSITION, &player_position, sizeof(Vector3));
			}

			//			SendPacket(udp_network, { 192,168,0,104 }, 11452, PACKET_TYPE_PLAYER_POSITION, &player_position, sizeof(Vector3));
#else

			SendPacket(udp_network, { 192,168,0,103 }, 11453, PACKET_TYPE_PLAYER_POSITION, &player_position, sizeof(Vector3));
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

		DxLib::DrawFormatString(100, 200, 0xffffff, "Using recbuffer No.%d", using_recbuffer_num);

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
		DxLib::DrawCircleAA(player_position.x, player_position.y, 20.0f, 32, Color::CYAN, true);
		for (const auto& kv : another_player_position) {
			const Vector3& p = kv.second;
			DxLib::DrawCircleAA(p.x, p.y, 20.0f, 32, Color::MAGENTA, true);
		}
		DxLib::DrawFormatString(0, 0, Color::WHITE, "FPS:%.1f", Time::GetFPS());
	}

	void NetWorkSampleScene::Exit()
	{
		if (g_recorder.IsRunning()) {
			g_recorder.Stop();
		}
		g_player.Stop();

		net_manager.reset();
	}

}