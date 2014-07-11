#ifndef _ENVLIGHTING_HPP
#define _ENVLIGHTING_HPP

#include <KlayGE/App3D.hpp>
#include <KlayGE/Font.hpp>
#include <KlayGE/CameraController.hpp>

class EnvLightingApp : public KlayGE::App3DFramework
{
public:
	EnvLightingApp();

	bool ConfirmDevice() const;

private:
	void InitObjects();
	void DoUpdateOverlay();
	KlayGE::uint32_t DoUpdate(KlayGE::uint32_t pass);
	void OnResize(KlayGE::uint32_t width, KlayGE::uint32_t height);

	void InputHandler(KlayGE::InputEngine const & sender, KlayGE::InputAction const & action);

	KlayGE::FontPtr font_;
	std::vector<KlayGE::SceneObjectPtr> spheres_;
	KlayGE::SceneObjectPtr sky_box_;

	KlayGE::TrackballCameraController obj_controller_;

	KlayGE::TexturePtr y_cube_map_;
	KlayGE::TexturePtr c_cube_map_;
};

#endif		// _ENVLIGHTING_HPP
