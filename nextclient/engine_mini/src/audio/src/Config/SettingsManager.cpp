#include "Config/SettingsManager.hpp"
#include "../../engine.h"

namespace MetaAudio
{
    //engine cvars
    static cvar_t* nosound = nullptr;
    static cvar_t* snd_show = nullptr;
    static cvar_t* sxroom_off = nullptr;
    static cvar_t* sxroom_type = nullptr;
    static cvar_t* sxroomwater_type = nullptr;
    static cvar_t* volume = nullptr;

    //active control
    static cvar_t* al_doppler = nullptr;
    static cvar_t* al_occlusion = nullptr;
    static cvar_t* al_occlusion_fade = nullptr;
    static cvar_t* al_xfi_workaround = nullptr;

    static constexpr char DEFAULT_XFI_WORKAROUND[] = "0"; // Disabled
    static constexpr char DEFAULT_OCCLUDER[] = "0"; // GoldSrc
    static constexpr char DEFAULT_OCCLUSION[] = "1";
    static constexpr char DEFAULT_OCCLUSION_FADE[] = "1";
    static constexpr char DEFAULT_DOPPLER_FACTOR[] = "0.3";

    void SettingsManager::Init(const cl_enginefunc_t& engFuncs)
    {
        if (al_xfi_workaround == nullptr) al_xfi_workaround = engFuncs.pfnRegisterVariable("al_xfi_workaround", DEFAULT_XFI_WORKAROUND, FCVAR_EXTDLL);
        if (al_doppler == nullptr) al_doppler = engFuncs.pfnRegisterVariable("al_doppler", DEFAULT_DOPPLER_FACTOR, FCVAR_EXTDLL);
        if (al_occlusion == nullptr) al_occlusion = engFuncs.pfnRegisterVariable("al_occlusion", DEFAULT_OCCLUSION, FCVAR_EXTDLL);
        if (al_occlusion_fade == nullptr) al_occlusion_fade = engFuncs.pfnRegisterVariable("al_occlusion_fade", DEFAULT_OCCLUSION_FADE, FCVAR_EXTDLL);

        if (!COM_CheckParm("-nosound"))
        {
            if (nosound == nullptr) nosound = engFuncs.pfnGetCvarPointer("nosound");
            if (volume == nullptr) volume = engFuncs.pfnGetCvarPointer("volume");
            if (sxroomwater_type == nullptr) sxroomwater_type = engFuncs.pfnGetCvarPointer("waterroom_type");
            if (sxroom_type == nullptr) sxroom_type = engFuncs.pfnGetCvarPointer("room_type");
            if (sxroom_off == nullptr) sxroom_off = engFuncs.pfnGetCvarPointer("room_off");
            if (snd_show == nullptr) snd_show = engFuncs.pfnGetCvarPointer("snd_show");
        }
    }

    bool SettingsManager::NoSound()
    {
        return nosound == nullptr || static_cast<bool>(nosound->value);
    }

    bool SettingsManager::SoundShow()
    {
        return static_cast<bool>(snd_show->value);
    }

    float SettingsManager::DopplerFactor()
    {
        return al_doppler->value;
    }

    float SettingsManager::Volume()
    {
        return volume->value;
    }

    bool SettingsManager::ReverbEnabled()
    {
        return !static_cast<bool>(sxroom_off->value);
    }

    size_t SettingsManager::ReverbType()
    {
        return static_cast<size_t>(sxroom_type->value);
    }

    size_t SettingsManager::ReverbUnderwaterType()
    {
        return static_cast<size_t>(sxroomwater_type->value);
    }

    XFiWorkaround SettingsManager::XfiWorkaround()
    {
        return static_cast<XFiWorkaround>(al_xfi_workaround->value);
    }

    OccluderType SettingsManager::Occluder()
    {
        return OccluderType::GoldSrc;
    }

    bool SettingsManager::OcclusionEnabled()
    {
        return static_cast<bool>(al_occlusion->value);
    }

    bool SettingsManager::OcclusionFade()
    {
        return static_cast<bool>(al_occlusion_fade->value);
    }
}
