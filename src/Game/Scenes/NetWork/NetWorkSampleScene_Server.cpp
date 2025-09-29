#include "precompile.h"
#include "NetWorkSampleScene_Server.h"
#include "Game/Managers/NetWorkManagerBase.h"
namespace NetWorkTest{
	int NetWorkSampleScene_Server::Init()
	{
		net_manager = std::make_unique<NetWorkManagerBase>(NETWORK_MANAGER_MODE_LISTEN, 11451);
		g_recvAccum.clear();
		g_recorder.SetOnWaveChunk([](const uint8_t* data, size_t size, bool isHeader) {
			if (size == 0 || !data) return;
			std::lock_guard<std::mutex> lk(g_sendMutex);
			if (g_queuedBytes + size > MAX_QUEUED_BYTES) {
				// ��ꂽ��Â����̂���j��
				while (!g_sendQueue.empty() && g_queuedBytes + size > MAX_QUEUED_BYTES) {
					g_queuedBytes -= g_sendQueue.front().bytes.size();
					g_sendQueue.pop_front();
				}
			}
			g_sendQueue.push_back({ isHeader, std::vector<uint8_t>(data, data + size) });
			g_queuedBytes += size;
			});
		g_recorder.Stop();
		g_player.Stop();
		net_manager->on_connect = [this](NetWork* net) {
			vchat_network = net;
			is_connected = true;
			vchat_network->on_receive = [this](void* data, size_t length) {
				ProcessReceivedBytes(reinterpret_cast<const uint8_t*>(data), length);
				};
			// �^���J�n�i�l�b�g���[�N�ڑ���j
			g_recorder.Start();
			};
		net_manager->on_disconnect = [this](NetWork* net) {
			if (net == vchat_network) {
				vchat_network = nullptr;
				is_connected = false;
				// �^����~�i�l�b�g���[�N�ؒf���j
				if (g_recorder.IsRunning())
					g_recorder.Stop();
				g_player.Stop();
				g_recvAccum.clear();
			}
			};
		
		return 0;
	}
}