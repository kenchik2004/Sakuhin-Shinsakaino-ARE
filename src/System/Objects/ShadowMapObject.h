#pragma once
#include "System/Object.h"



struct ShadowInfo {
	//       [4]+------ --+[5]
	//        �^|  �@  �^ |
	//      �^  |6  �^    |
	// [0] +-----+ [1]   +[7]
	//     |     |   �^
	// [2] +-----+ [3]
	Vector3 frustum_vertices_[8];

	float  near_z_;
	float  far_z_;
	float  aspect_ratio_;
	float  fov_y_;
	mat4x4 mat_camera_world_;    // �V�[���̃J�����̃��[���h�s��

	Vector4 bounding_sphere_;    // �o�E���f�B���O��  [xyz_]:���S���W [___w]:���a

public:
	void update();

	void draw();
};

class ModelRenderer;
class ShadowMapObject :
	public Object
{
public:
	USING_SUPER(ShadowMapObject);
	int Init() override;
	void PreDraw() override;
	void LateDraw() override;
	void Exit() override;

	inline void SetShadowMapSize(u32 size) {
		//�������O�ɂ����ݒ�ł��Ȃ�
		if (is_initialized)
			return;
		shadow_map_size = size;
	}

	inline void SetCascadeCount(u32 count) {
		//�������O�ɂ����ݒ�ł��Ȃ�
		if (is_initialized)
			return;
		cascade_count = count;
	}

	inline u32 ShadowMapSize() const { return shadow_map_size; }
	inline u32 CascadeCount() const { return cascade_count; }
	void RegisterModelRenderer(ModelRenderer* renderer);
	void UnregisterModelRenderer(ModelRenderer* renderer);

	std::vector<mat4x4> GetLightViewProjs() const { return shadowmap_view_projs; }

private:
	bool InitializeShadowMap();
	void ShadowMapDrawBegin();
	void ShadowMapDrawEnd();
	bool is_initialized = false;
	SafeSharedPtr<Texture> shadow_map = nullptr;
	u32 shadow_map_size = 1024;
	u32 cascade_count = 4;
	std::vector<ModelRenderer*> model_renderers;

	Vector3 shadowmap_center = { 0,0,0 };
	Vector3 light_dir = { 1,-0.5,0 };
	mat4x4 shadowmap_view;		//���C�g�r���[�s��
	mat4x4 shadowmap_proj;		//���C�g�v���W�F�N�V�����s��
	std::vector<mat4x4> shadowmap_view_projs; //���C�g�r���[�~�v���W�F�N�V�����s��
	std::vector<ShadowInfo> shadow_infos;	//�J�X�P�[�h�V���h�E�}�b�v�p���

	//�V���h�E�}�b�v�p�T���v���[
	ID3D11SamplerState* shadow_sampler = nullptr;
};

