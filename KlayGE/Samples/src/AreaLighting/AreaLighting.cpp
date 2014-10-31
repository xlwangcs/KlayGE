#include <KlayGE/KlayGE.hpp>
#include <KFL/ThrowErr.hpp>
#include <KFL/Util.hpp>
#include <KFL/Math.hpp>
#include <KlayGE/Font.hpp>
#include <KlayGE/RenderFactory.hpp>
#include <KlayGE/InputFactory.hpp>
#include <KlayGE/RenderEngine.hpp>
#include <KlayGE/RenderEffect.hpp>
#include <KlayGE/FrameBuffer.hpp>
#include <KlayGE/SceneManager.hpp>
#include <KlayGE/Context.hpp>
#include <KlayGE/ResLoader.hpp>
#include <KlayGE/RenderSettings.hpp>
#include <KlayGE/Mesh.hpp>
#include <KlayGE/SceneObjectHelper.hpp>
#include <KlayGE/PostProcess.hpp>
#include <KlayGE/Camera.hpp>
#include <KlayGE/DeferredRenderingLayer.hpp>
#include <KlayGE/ParticleSystem.hpp>
#include <KlayGE/PerfProfiler.hpp>

#include <sstream>

#include "SampleCommon.hpp"
#include "AreaLighting.hpp"

using namespace std;
using namespace KlayGE;

namespace
{
	class PointLightSourceUpdate
	{
	public:
		PointLightSourceUpdate(float move_speed, float3 const & pos)
			: move_speed_(move_speed), pos_(pos)
		{
		}

		void operator()(LightSource& light, float app_time, float /*elapsed_time*/)
		{
			light.ModelMatrix(MathLib::translation(sin(app_time * 1000 * move_speed_), 0.0f, 0.0f)
				* MathLib::translation(pos_));
		}

	private:
		float move_speed_;
		float3 pos_;
	};


	enum
	{
		Exit,
		Profile
	};

	InputActionDefine actions[] =
	{
		InputActionDefine(Exit, KS_Escape),
		InputActionDefine(Profile, KS_P)
	};
}

int SampleMain()
{
	ContextCfg cfg = Context::Instance().Config();
	cfg.deferred_rendering = true;
	Context::Instance().Config(cfg);

	AreaLightingApp app;
	app.Create();
	app.Run();

	return 0;
}

AreaLightingApp::AreaLightingApp()
			: App3DFramework("AreaLighting")
{
	ResLoader::Instance().AddPath("../../Samples/media/AreaLighting");
}

bool AreaLightingApp::ConfirmDevice() const
{
	RenderDeviceCaps const & caps = Context::Instance().RenderFactoryInstance().RenderEngineInstance().DeviceCaps();
	if (caps.max_shader_model < 2)
	{
		return false;
	}
	return true;
}

void AreaLightingApp::OnCreate()
{
	this->LookAt(float3(-12.2f, 15.8f, -2.4f), float3(-11.5f, 15.1f, -2.2f));
	this->Proj(0.1f, 500.0f);

	KlayGE::function<TexturePtr()> c_cube_tl = ASyncLoadTexture("Lake_CraterLake03_filtered_c.dds", EAH_GPU_Read | EAH_Immutable);
	KlayGE::function<TexturePtr()> y_cube_tl = ASyncLoadTexture("Lake_CraterLake03_filtered_y.dds", EAH_GPU_Read | EAH_Immutable);
	KlayGE::function<RenderablePtr()> model_ml = ASyncLoadModel("sponza_crytek.7z//sponza_crytek.meshml", EAH_GPU_Read | EAH_Immutable);

	font_ = SyncLoadFont("gkai00mp.kfont");

	deferred_rendering_ = Context::Instance().DeferredRenderingLayerInstance();

	AmbientLightSourcePtr ambient_light = MakeSharedPtr<AmbientLightSource>();
	ambient_light->SkylightTex(y_cube_tl, c_cube_tl);
	ambient_light->Color(float3(0.1f, 0.1f, 0.1f));
	ambient_light->AddToSceneManager();

	point_light_ = MakeSharedPtr<PointLightSource>();
	point_light_->Attrib(0);
	point_light_->Color(float3(0.8f, 0.96f, 1.0f) * 40.0f);
	point_light_->Position(float3(0, 0, 0));
	point_light_->Falloff(float3(1, 0, 1));
	point_light_->BindUpdateFunc(PointLightSourceUpdate(1 / 1000.0f, float3(0, 10, 0)));
	point_light_->AddToSceneManager();
	point_light_->Enabled(false);

	point_light_src_ = MakeSharedPtr<SceneObjectLightSourceProxy>(point_light_);
	checked_pointer_cast<SceneObjectLightSourceProxy>(point_light_src_)->Scaling(0.1f, 0.1f, 0.1f);
	point_light_src_->AddToSceneManager();
	point_light_src_->Visible(false);

	sphere_area_light_ = MakeSharedPtr<SphereAreaLightSource>();
	sphere_area_light_->Attrib(0);
	sphere_area_light_->Color(point_light_->Color());
	sphere_area_light_->Position(point_light_->Position());
	sphere_area_light_->Falloff(point_light_->Falloff());
	sphere_area_light_->BindUpdateFunc(PointLightSourceUpdate(1 / 1000.0f, float3(0, 10, 0)));
	sphere_area_light_->AddToSceneManager();
	sphere_area_light_->Enabled(false);

	sphere_area_light_src_ = MakeSharedPtr<SceneObjectLightSourceProxy>(sphere_area_light_);
	checked_pointer_cast<SceneObjectLightSourceProxy>(sphere_area_light_src_)->Scaling(0.1f, 0.1f, 0.1f);
	sphere_area_light_src_->AddToSceneManager();
	sphere_area_light_src_->Visible(false);

	tube_area_light_ = MakeSharedPtr<TubeAreaLightSource>();
	tube_area_light_->Attrib(0);
	tube_area_light_->Color(point_light_->Color());
	tube_area_light_->Position(point_light_->Position());
	tube_area_light_->Falloff(point_light_->Falloff());
	tube_area_light_->BindUpdateFunc(PointLightSourceUpdate(1 / 1000.0f, float3(0, 10, 0)));
	tube_area_light_->AddToSceneManager();
	tube_area_light_->Enabled(false);

	tube_area_light_src_ = MakeSharedPtr<SceneObjectLightSourceProxy>(tube_area_light_);
	checked_pointer_cast<SceneObjectLightSourceProxy>(tube_area_light_src_)->Scaling(0.1f, 0.1f, 0.1f);
	tube_area_light_src_->AddToSceneManager();
	tube_area_light_src_->Visible(false);

	SceneObjectPtr scene_obj = MakeSharedPtr<SceneObjectHelper>(model_ml, SceneObject::SOA_Cullable, 0);
	scene_obj->AddToSceneManager();

	for (int i = -5; i < 5; ++ i)
	{
		RenderModelPtr sphere_mesh = SyncLoadModel("sphere_high.7z//sphere_high.meshml", EAH_GPU_Read | EAH_Immutable);
		sphere_mesh->GetMaterial(0)->specular = float3(0.4f, 0.4f, 0.4f);
		sphere_mesh->GetMaterial(0)->shininess = (i + 6) * 10.0f;
		SceneObjectPtr sphere_obj = MakeSharedPtr<SceneObjectHelper>(sphere_mesh, SceneObject::SOA_Cullable);
		sphere_obj->ModelMatrix(MathLib::scaling(10.0f, 10.0f, 10.0f) * MathLib::translation(i + 0.5f, 5.0f, 0.0f));
		sphere_obj->AddToSceneManager();
	}

	fpcController_.Scalers(0.05f, 0.5f);

	InputEngine& inputEngine(Context::Instance().InputFactoryInstance().InputEngineInstance());
	InputActionMap actionMap;
	actionMap.AddActions(actions, actions + sizeof(actions) / sizeof(actions[0]));

	action_handler_t input_handler = MakeSharedPtr<input_signal>();
	input_handler->connect(KlayGE::bind(&AreaLightingApp::InputHandler, this, KlayGE::placeholders::_1, KlayGE::placeholders::_2));
	inputEngine.ActionMap(actionMap, input_handler);

	UIManager::Instance().Load(ResLoader::Instance().Open("AreaLighting.uiml"));
	dialog_ = UIManager::Instance().GetDialogs()[0];

	id_light_type_combo_ = dialog_->IDFromName("LightTypeCombo");
	id_radius_static_ = dialog_->IDFromName("RadiusStatic");
	id_radius_slider_ = dialog_->IDFromName("RadiusSlider");
	id_ctrl_camera_ = dialog_->IDFromName("CtrlCamera");

	dialog_->Control<UIComboBox>(id_light_type_combo_)->OnSelectionChangedEvent().connect(KlayGE::bind(&AreaLightingApp::LightTypeChangedHandler, this, KlayGE::placeholders::_1));
	this->LightTypeChangedHandler(*dialog_->Control<UIComboBox>(id_light_type_combo_));

	dialog_->Control<UISlider>(id_radius_slider_)->OnValueChangedEvent().connect(KlayGE::bind(&AreaLightingApp::RadiusChangedHandler, this, KlayGE::placeholders::_1));
	this->RadiusChangedHandler(*dialog_->Control<UISlider>(id_radius_slider_));

	dialog_->Control<UICheckBox>(id_ctrl_camera_)->OnChangedEvent().connect(KlayGE::bind(&AreaLightingApp::CtrlCameraHandler, this, KlayGE::placeholders::_1));
	this->CtrlCameraHandler(*dialog_->Control<UICheckBox>(id_ctrl_camera_));

	sky_box_ = MakeSharedPtr<SceneObjectSkyBox>();
	checked_pointer_cast<SceneObjectSkyBox>(sky_box_)->CompressedCubeMap(y_cube_tl, c_cube_tl);
	sky_box_->AddToSceneManager();
}

void AreaLightingApp::OnResize(uint32_t width, uint32_t height)
{
	App3DFramework::OnResize(width, height);

	RenderEngine& re = Context::Instance().RenderFactoryInstance().RenderEngineInstance();
	deferred_rendering_->SetupViewport(0, re.CurFrameBuffer(), 0);

	UIManager::Instance().SettleCtrls();
}

void AreaLightingApp::InputHandler(InputEngine const & /*sender*/, InputAction const & action)
{
	switch (action.first)
	{
	case Exit:
		this->Quit();
		break;

	case Profile:
#ifndef KLAYGE_SHIP
		PerfProfiler::Instance().ExportToCSV("profile.csv");
#endif
		break;
	}
}

void AreaLightingApp::LightTypeChangedHandler(UIComboBox const & sender)
{
	switch (sender.GetSelectedIndex())
	{
	case 0:
		point_light_->Enabled(true);
		point_light_src_->Visible(true);
		sphere_area_light_->Enabled(false);
		sphere_area_light_src_->Visible(false);
		tube_area_light_->Enabled(false);
		tube_area_light_src_->Visible(false);
		dialog_->Control<UIStatic>(id_radius_static_)->SetEnabled(false);
		dialog_->Control<UISlider>(id_radius_slider_)->SetEnabled(false);
		break;

	case 1:
		point_light_->Enabled(false);
		point_light_src_->Visible(false);
		sphere_area_light_->Enabled(true);
		sphere_area_light_src_->Visible(true);
		tube_area_light_->Enabled(false);
		tube_area_light_src_->Visible(false);
		dialog_->Control<UIStatic>(id_radius_static_)->SetEnabled(true);
		dialog_->Control<UISlider>(id_radius_slider_)->SetEnabled(true);
		break;

	case 2:
		point_light_->Enabled(false);
		point_light_src_->Visible(false);
		sphere_area_light_->Enabled(false);
		sphere_area_light_src_->Visible(false);
		tube_area_light_->Enabled(true);
		tube_area_light_src_->Visible(true);
		dialog_->Control<UIStatic>(id_radius_static_)->SetEnabled(true);
		dialog_->Control<UISlider>(id_radius_slider_)->SetEnabled(true);
		break;

	default:
		break;
	}
}

void AreaLightingApp::RadiusChangedHandler(UISlider const & sender)
{
	float radius = sender.GetValue() / 100.0f;

	checked_pointer_cast<SphereAreaLightSource>(sphere_area_light_)->Radius(radius);
	checked_pointer_cast<SceneObjectLightSourceProxy>(sphere_area_light_src_)->Scaling(radius, radius, radius);

	checked_pointer_cast<TubeAreaLightSource>(tube_area_light_)->Extend(float3(0.1f, 0.1f, radius));
	checked_pointer_cast<SceneObjectLightSourceProxy>(tube_area_light_src_)->Scaling(radius, 0.1f, 0.1f);

	std::wostringstream stream;
	stream << L"Radius: " << radius;
	dialog_->Control<UIStatic>(id_radius_static_)->SetText(stream.str());
}

void AreaLightingApp::CtrlCameraHandler(UICheckBox const & sender)
{
	if (sender.GetChecked())
	{
		fpcController_.AttachCamera(this->ActiveCamera());
	}
	else
	{
		fpcController_.DetachCamera();
	}
}

void AreaLightingApp::DoUpdateOverlay()
{
	RenderEngine& renderEngine(Context::Instance().RenderFactoryInstance().RenderEngineInstance());
	SceneManager& scene_mgr = Context::Instance().SceneManagerInstance();

	UIManager::Instance().Render();

	font_->RenderText(0, 0, Color(1, 1, 0, 1), L"Deferred Rendering", 16);
	font_->RenderText(0, 18, Color(1, 1, 0, 1), renderEngine.ScreenFrameBuffer()->Description(), 16);

	std::wostringstream stream;
	stream.precision(2);
	stream << std::fixed << this->FPS() << " FPS";
	font_->RenderText(0, 36, Color(1, 1, 0, 1), stream.str(), 16);

	stream.str(L"");
	stream << deferred_rendering_->NumObjectsRendered() << " Scene objects "
		<< deferred_rendering_->NumRenderablesRendered() << " Renderables "
		<< deferred_rendering_->NumPrimitivesRendered() << " Primitives "
		<< deferred_rendering_->NumVerticesRendered() << " Vertices";
	font_->RenderText(0, 54, Color(1, 1, 1, 1), stream.str(), 16);

	stream.str(L"");
	stream << scene_mgr.NumDrawCalls() << " Draws/frame "
		<< scene_mgr.NumDispatchCalls() << " Dispatches/frame";
	font_->RenderText(0, 72, Color(1, 1, 1, 1), stream.str(), 16);
}

uint32_t AreaLightingApp::DoUpdate(uint32_t pass)
{
	return deferred_rendering_->Update(pass);
}
