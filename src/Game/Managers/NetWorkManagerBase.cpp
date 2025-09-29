#include "precompile.h"
#include "NetWorkManagerBase.h"


void NetWork::Send(const void* data, size_t data_size)
{
	//�n���h���������Ȃ瑗�M���Ȃ�
	if (handle != -1)
	{
		DxLib::NetWorkSend(handle, data, static_cast<int>(data_size));
	}
}

NetWork::~NetWork()
{
	//�n���h�����L���Ȃ�ؒf����
	CloseNetWork(handle);
	//�ؒf�R�[���o�b�N���Ă�
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
	//���[�h�ɉ����āA���b�X�����J�n����
	if (mode == NETWORK_MANAGER_MODE_LISTEN || mode == NETWORK_MANAGER_MODE_BOTH)
	{

		int result = DxLib::PreparationListenNetWork(port_num);
		//���s�̏ꍇ�A�I��
		if (result == -1)
		{
			return;
		}
	}
	//������IP�A�h���X���擾
	DxLib::GetMyIPAddress(&my_ip);

	//�V�K�ڑ��Ɛؒf���Ď�����X���b�h���N��
	//�V�K�ڑ����o�X���b�h
	auto check_connection_lambda = [this]() {
		CheckForNewConnect(kill_thread_flag);
		};
	check_connection_thread = std::thread(check_connection_lambda);

	//�ؒf���o�X���b�h
	auto check_disconnection_lambda = [this]() {
		CheckForDisConnect(kill_thread_flag);
		};
	check_disconnection_thread = std::thread(check_disconnection_lambda);

}

// �V����UDP�\�P�b�g���J��
UDPNetWork* NetWorkManagerBase::OpenUDPSocket(unsigned short port)
{
	if (udp_network)
		return nullptr; // ���ɊJ���Ă���

	//UDP�\�P�b�g���쐬
	int socket = DxLib::MakeUDPSocket(port);
	//�쐬�ɐ���������AUDPNetWork�𐶐�
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
	//�S�ڑ��ɑ΂��āA��M�f�[�^������Ύ�M����
	//�ʃX���b�h�ŐV�K�ڑ����������ꍇ�Avector�̗v�f�������Ă��܂�
	//���̍ۂ̃C�e���[�^�̔j���h�����߁A���b�N��������
	if (udp_network) {
		//UDP�\�P�b�g�ɑ΂��Ă����l�Ɏ�M���s��
		int data_arrived = DxLib::CheckNetWorkRecvUDP(udp_network->socket);
		bool break_flag = false;
		int data_size = 0;
		while (data_arrived > 0 || !break_flag) {
			IPDATA from_ip;
			int from_port;
			//��M�����f�[�^����M�o�b�t�@�ɓǂݍ���
			//���ʂ̓L���b�V�����Ă���
			int result = DxLib::NetWorkRecvUDP(udp_network->socket, &from_ip, &from_port, udp_network->buffer.data() + data_size, udp_network->buffer.size() - data_size, false);

			switch (result) {
			case -1:
				//�G���[�̏ꍇ�A�������Ȃ�
				//���{���̓G���[���e�ɉ������������K�v
				break_flag = true;
				break;
			case -2:
				//�o�b�t�@������Ȃ��ꍇ�A�������Ƃ��ăo�b�t�@���N���A���Ď�M�f�[�^��j������
				//�����Ⴀ�Ȃ��ł���Ă�̂ŁA�{���͗ǂ��Ȃ�
				DxLib::NetWorkRecvBufferClear(udp_network->socket);
				break_flag = true;
				break;
			case -3:
				//��M�f�[�^���Ȃ��ꍇ�A���[�v�𔲂���
				break_flag = true;
				break;
			default:
				//���������ꍇ�Aresult�Ɏ�M�f�[�^�̃T�C�Y�������Ă���
				data_size += result;
				break;
			}
			data_arrived = DxLib::CheckNetWorkRecvUDP(udp_network->socket);

		}
		//�f�[�^�̓��e�͗l�X�Ȃ̂ŁA�R�[���o�b�N�ɔC����
		if (udp_network->on_receive)
			udp_network->on_receive(udp_network->buffer.data(), static_cast<size_t>(data_size));
	}
	std::scoped_lock lock(mutex_);
	for (auto& net : networks) {
		if (net->handle == -1)
			continue;

		//��M�ς݂����U�蕪�����I����Ă��Ȃ��f�[�^�̗ʂ��擾
		int recv_size = DxLib::GetNetWorkDataLength(net->handle);
		//��M�f�[�^������A���o�b�t�@�̗e�ʂɎ��܂�Ȃ�o�b�t�@�ɐU�蕪��
		if (recv_size > 0 && recv_size < net->buffer.size()) {
			//��M�����f�[�^����M�o�b�t�@�ɓǂݍ���
			DxLib::NetWorkRecv(net->handle, net->buffer.data(), recv_size);

			//�f�[�^�̓��e�͗l�X�Ȃ̂ŁA�R�[���o�b�N�ɔC����
			if (net->on_receive)
				net->on_receive(net->buffer.data(), static_cast<size_t>(recv_size));
		}
		//��M�f�[�^�����邪�o�b�t�@�Ɏ��܂�Ȃ��ꍇ
		else if (recv_size > net->buffer.size()) {
			//�������Ƃ��āA��M�o�b�t�@���N���A���Ď�M�f�[�^��j������
			{
				DxLib::NetWorkRecvBufferClear(net->handle);
				continue;
			}
			//�{���́A�o�b�t�@�𓮓I�Ɋm�ۂ������A�������͎��镪�������Ȃǂ̏������K�v
			//�Ƃ肠�����A����ł͎��镪�������
			//���f�[�^�̓r���Ő؂�Ă��܂��\��������̂ŁA�댯->�v���P


			recv_size = net->buffer.size();
			DxLib::NetWorkRecv(net->handle, net->buffer.data(), recv_size);
			//�f�[�^�̓��e�͗l�X�Ȃ̂ŁA�R�[���o�b�N�ɔC����
			if (net->on_receive)
				net->on_receive(net->buffer.data(), static_cast<size_t>(recv_size));

		}
	}


}

// �V�K�ڑ������o���āA�Ǘ����ɒu��
void NetWorkManagerBase::CheckForNewConnect(const bool& finish_flag)
{
	//�ʃX���b�h�ɂ��āA��ɐV�K�ڑ����Ď�����
	while (!finish_flag) {
		//�V�K�ڑ����擾
		int new_handle = DxLib::GetNewAcceptNetWork();

		//�����V�K�ڑ�������΁A�Ǘ����ɒu��
		if (new_handle >= 0)
		{
			IPDATA other_ip;
			//�ڑ����IP���擾
			DxLib::GetNetWorkIP(new_handle, &other_ip);

			//unique_ptr�ō쐬->�}�l�[�W�����Ǘ�
			auto net = std::make_unique<NetWork>(new_handle, other_ip, SEC2MICRO(Time::GetTimeFromStart()));

			//�ڑ��������̃f�t�H���g�R�[���o�b�N��ݒ�(������͊�{�����������s�\)
			net->on_disconnect = on_disconnection;

			{
				std::scoped_lock lock(mutex_);
				networks.push_back(std::move(net));
				if (on_new_connection)
					on_new_connection(networks.back().get());
			}

		}
		//�����ACPU���ׂ������邽�߂ɏ����x�܂������B�悵�A�x�܂��悤
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

// �ؒf���ꂽ�ڑ������o���āA�Ǘ�������O��
void NetWorkManagerBase::CheckForDisConnect(const bool& finish_flag)
{
	//�ʃX���b�h�ɂ��āA��ɐؒf���Ď�����
	while (!finish_flag) {
		//�ؒf���ꂽ�ڑ����擾
		int lost_handle = DxLib::GetLostNetWork();

		//�����ؒf���ꂽ�ڑ�������΁A�Ǘ�������O��
		if (lost_handle >= 0)
		{
			//�C�e���[�^�̔j���h�����߁A���b�N��������
			std::scoped_lock lock(mutex_);

			//vector����Y������ڑ���T��
			auto it = std::find_if(networks.begin(), networks.end(),
				[lost_handle](const std::unique_ptr<NetWork>& net) {
					return net->handle == lost_handle;
				});

			//����������폜
			if (it != networks.end()) {
				networks.erase(it);
			}
		}
		//�����ACPU���ׂ������邽�߂ɏ����x�܂������B�悵�A�x�܂��悤
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

NetWork* NetWorkManagerBase::Connect(IPDATA other, unsigned short port,
	std::function<void(NetWork*)> on_connect,
	std::function<void()> on_disconnect)
{
	//�V���ɂ����炩��ڑ����s��
	int handle = DxLib::ConnectNetWork(other, port);

	//���s
	if (handle == -1)
		return nullptr;
	//����
	//unique_ptr�ō쐬->�}�l�[�W�����Ǘ�
	auto net = std::make_unique<NetWork>(handle, other, SEC2MICRO(Time::GetTimeFromStart()));

	//�ڑ��������̃f�t�H���g�R�[���o�b�N��ݒ�
	//�����Ŏw�肳��Ă���΂������D��
	net->on_disconnect = on_disconnect ? on_disconnect : this->on_disconnection;

	//move���Ă��܂��Ă͕Ԃ��Ȃ��̂ŁA���|�C���^���L���b�V��
	NetWork* ret = net.get();
	//�ڑ������R�[���o�b�N���Ă�
	//�����Ŏw�肳��Ă���΂������D��
	if (on_connect)
		on_connect(ret);
	else if (on_new_connection)
		on_new_connection(ret);
	//�}�l�[�W���̊Ǘ����ɒu��
	{
		std::scoped_lock lock(mutex_);
		//vector�̃C�e���[�^�����Ă��܂��Ƃ����Ȃ��̂ŁA���b�N��������
		//�������A�R�[���o�b�N���ɑ��̃}�V���ɐڑ�����ȂǂƂ������Ƃ����蓾��̂ŁA
		//�R�[���o�b�N�O�Ƀ��b�N��������͔̂�����

		//vector�ɓo�^
		networks.push_back(std::move(net));
	}
	//�L���b�V�����Ă������|�C���^��Ԃ�
	return ret;
}

